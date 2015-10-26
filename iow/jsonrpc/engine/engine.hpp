#pragma once
#include <iow/logger/logger.hpp>
#include <iow/jsonrpc/engine/call_map.hpp>
#include <iow/jsonrpc/engine/handler_map.hpp>
#include <iow/jsonrpc/engine/engine_options.hpp>
#include <iow/jsonrpc/incoming/incoming_holder.hpp>
#include <iow/jsonrpc/outgoing/outgoing_holder.hpp>
#include <iow/jsonrpc/types.hpp>
#include <iow/jsonrpc/incoming/aux.hpp>
#include <iow/io/io_id.hpp>

#include <type_traits>

namespace iow{ namespace jsonrpc{

//#error
  // TODO: Сделать чеклист по всем вариантам 
  // Отработать
  // для handler_map (сервис)
  //    все handler опции игнорируються (оборачиваются)
  //    reg_io - новый коннект io (для авто-ответа нового коннекта)
  //    reg_jsonrpc - новый коннект jsonrpc (для авто-ответа нового коннекта)
  //    perform_io - обновить хандеры для io
  //    perform_incoming - обновить хандеры для jsonrpc
  // для _common_handler (шлюз)
  //    инициализация в зависимти от опций

/*
 * Входы:
 *      perform_io
 *      perform_jsonrpc
 *      reg_io
 *      reg_jsonrpc
 *      user_interface
 *        ссылку на user_interface можно получить при запросе или при новом "коннекте"(reg_*)
*/

/**
 * Все режимы взаимодействия.
 * Участики:
 *      iinterface - сервре и клиент
 *      ijsonrpc - промежуточные обработчики
 *      iinterface1 и iinterface2 пользовательские объекты
 * iinterface -> iinterface1
 * ijsonrpc   -> iinterface1
 * iinterface1 -> iinterface
 * iinterface1 -> ijsonrpc
 * 
 */
template<typename JsonrpcHandler>
class engine
  : public std::enable_shared_from_this< engine<JsonrpcHandler> >
{
public:
  typedef engine<JsonrpcHandler> self;
  typedef JsonrpcHandler handler_type;
  typedef typename handler_type::target_type target_type;
  typedef std::shared_ptr<handler_type> handler_ptr;
  typedef typename handler_type::options_type handler_options_type;
  typedef typename handler_type::result_handler_t result_handler_t;
  typedef typename handler_type::request_serializer_t request_serializer_t;
  typedef typename handler_type::notify_serializer_t notify_serializer_t;
  
  typedef engine_options<handler_options_type> options_type;
  
  typedef typename handler_type::io_id_t io_id_t;
  typedef typename handler_type::outgoing_handler_t outgoing_handler_t;
  typedef typename handler_type::data_ptr data_ptr;
  typedef typename outgoing_holder::call_id_t call_id_t;

  ~engine()
  {
    _handler_map.stop();
    _call_map.clear();
    
    //_common_handler = nullptr;
  }
  
  engine()
    : _call_counter(1)
  {
  }
  /***************************************************************/
  /* Управляющие методы                                          */
  /***************************************************************/

  template<typename O>
  void start(O&& opt)
  {
    _io_id = ::iow::io::create_id<size_t>();
    this->reconfigure( std::forward<O>(opt) );
  }

  template<typename O>
  void reconfigure(O&& opt1)
  {
    typename std::decay<O>::type opt = opt1;

    /*
    if ( _common_handler == nullptr )
    {
      auto h = std::make_shared<handler_type>();
      _common_handler = std::make_shared<handler_type>();
    }
    */

    _outgoing_rpc_factory = [opt, this](io_id_t io_id, outgoing_handler_t handler, bool reg_io) -> handler_ptr
    {
      return this->create_handler_(io_id, opt, std::move(handler), reg_io );
    };

    _incoming_io_factory = [opt, this](io_id_t io_id, ::iow::io::incoming_handler_t handler, bool reg_io) -> handler_ptr
    {
      return this->create_handler_(io_id, opt, std::move(handler), reg_io );
    };

    
    _outgoing_io_factory = [opt, this](io_id_t io_id, ::iow::io::outgoing_handler_t handler, bool reg_io) -> handler_ptr
    {
      return this->create_handler_(io_id, opt, std::move(handler), reg_io );
    };
    
  }

  void stop()
  {
    /*
    if ( _common_handler!=nullptr )
    {
      _common_handler->stop();
    }
    */
    _handler_map.stop();
    _call_map.clear();
  }

// iterface implementation

  template<typename Tg, typename Req, typename ...Args>
  void call(Req req, io_id_t io_id, Args... args)
  {
    if ( auto h = _handler_map.find(io_id) )
    {
      h->template call<Tg>(std::move(req), std::forward<Args>(args)...);
    }
  }

  /***************************************************************/
  /* jsonrpc                                                     */
  /***************************************************************/

  void reg_jsonrpc( io_id_t io_id, outgoing_handler_t handler )
  {
    _outgoing_rpc_factory(io_id, handler, true);
  }

  void perform_jsonrpc(incoming_holder holder, io_id_t io_id, outgoing_handler_t handler) 
  {
    this->perform_incoming_( std::move(holder), io_id, std::move(handler) );
  }

  /***************************************************************/
  /* data_ptr                                                    */
  /***************************************************************/
  
  
  outgoing_handler_t io2rpc( ::iow::io::outgoing_handler_t handler )
  {
    if ( handler == nullptr )
      return nullptr;

    return [handler](outgoing_holder holder)
    {
      auto d = holder.detach();
      handler( std::move( d ) );
    };
  }

  void reg_io( io_id_t io_id, ::iow::io::incoming_handler_t handler )
  {
    this->_incoming_io_factory(io_id, handler, true);
  }

  void unreg_io( io_id_t io_id )
  {
    _handler_map.erase(io_id);
  }

  void perform_io(data_ptr d, io_id_t io_id, ::iow::io::outgoing_handler_t handler) 
  {
    using namespace std::placeholders;
    aux::perform( std::move(d), io_id, handler, std::bind(&engine::perform_io_once_, this, _1, _2, _3 ));
    /*auto h = this->_incoming_io_factory(io_id, handler, false);
    aux::perform( 
      std::move(d), io_id, std::move( handler ), 
      [](data_ptr d, io_id_t io_id, ::iow::io::outgoing_handler_t handler)
      {
      }
    );*/
    //incoming_holder h(d);
    /*
    using namespace std::placeholders;
    auto rpc = io2rpc( std::move(handler));
    aux::perform( std::move(d), io_id, std::move( rpc ), std::bind(&engine::perform_incoming_, this, _1, _2, _3 ));
    */
  }

  io_id_t get_id() const
  {
    return _io_id;
  }

private:
  
  void perform_io_once_(incoming_holder holder, io_id_t io_id, ::iow::io::outgoing_handler_t handler)
  {
    this->perform_incoming_( std::move(holder), io_id, [handler](outgoing_holder holder)
    {
      handler( std::move(holder.detach()) );
    });
    /*
    auto h = this->_outgoing_io_factory(io_id, handler, false);
    h->invoke( std::move(holder), [](outgoing_holder holder)
    {
      
    });
    */
  }


  void perform_incoming_(incoming_holder holder, io_id_t io_id, outgoing_handler_t handler) 
  {
    if ( holder.is_notify() || holder.is_request() )
    {
      this->perform_request_( std::move(holder), io_id, std::move(handler) );
    }
    else if (holder.is_response() || holder.is_error() )
    {
      this->perform_response_( std::move(holder), io_id, std::move(handler) );
    }
    else
    {
      JSONRPC_LOG_ERROR( "jsonrpc::engine: Invalid Request: " << holder.str() )
      aux::send_error( std::move(holder), std::make_unique<invalid_request>(), std::move(handler) );
    }
  }

  void perform_request_(incoming_holder holder, io_id_t io_id, outgoing_handler_t handler) 
  {
    auto h = _outgoing_rpc_factory(io_id, handler, false);
    h->invoke( std::move(holder), std::move(handler) );
  }

  void perform_response_(incoming_holder holder, io_id_t /*io_id*/, outgoing_handler_t handler) 
  {
    call_id_t call_id = holder.template get_id<call_id_t>();
    if ( auto result_handler = _call_map.detach(call_id) )
    {
      result_handler(std::move(holder));
    }
    else if ( handler!=nullptr )
    {
      JSONRPC_LOG_WARNING("jsonrpc::engind incoming response with call_id=" << call_id << " not found")
      handler( outgoing_holder() );
    }
  }

  /**
   * upgrate_options_ - инициализация send_request и send_notify 
   */

  
  template<typename O>
  void upgrate_options_(O& opt, ::iow::io::outgoing_handler_t handler)
  {
    std::weak_ptr<self> wthis = this->shared_from_this();
    opt.sender_handler = [handler, wthis](const char* name, notify_serializer_t ns1, request_serializer_t rs1, result_handler_t rh1)
    {
      auto pthis = wthis.lock();
      if ( pthis==nullptr )
        return;
      
      if ( rs1!=nullptr )
      {
        auto call_id = pthis->_call_counter.fetch_add(1);
        pthis->_call_map.set(call_id, std::move(rh1));
        auto d = std::move(rs1(name, call_id));
        handler( std::move(d));
      }
      else if (ns1!=nullptr)
      {
        auto d = std::move(ns1(name));
        handler( std::move(d));
      }
      else
      {
        handler(nullptr);
      }
    };
    // Не нужен! здесь wthis
    /*
    std::weak_ptr<self> wthis = this->shared_from_this();
    opt.send_request = [handler, wthis](const char* name, result_handler_t result_handler, request_serializer_t ser)
    {
      if ( auto pthis = wthis.lock() )
      {
        auto call_id = pthis->_call_counter.fetch_add(1);
        pthis->_call_map.set(call_id, std::move(result_handler));
        auto d = std::move(ser(name, call_id));
        // TODO: wrap
        // нужен здесь wthis
        handler( std::move(d));
      }
    };

    opt.send_notify = [handler, wthis](const char* name, notify_serializer_t ser)
    {
      if ( auto pthis = wthis.lock() )
      {
        auto d = std::move(ser(name));
        // TODO: wrap
        handler( std::move(d));
      }
    };
    */
  }
  

  template<typename O>
  void upgrate_options_(O& opt, ::iow::io::incoming_handler_t handler)
  {
    io_id_t io_id = this->_io_id;
    std::weak_ptr<self> wthis = this->shared_from_this();
    opt.sender_handler = [handler, wthis, io_id](const char* name, notify_serializer_t ns1, request_serializer_t rs1, result_handler_t rh1)
    {
      auto pthis = wthis.lock();
      if ( pthis == nullptr )
        return;
      
      if ( rs1!=nullptr && rh1!=nullptr )
      {
        auto call_id = pthis->_call_counter.fetch_add(1);
        pthis->_call_map.set(call_id, std::move(rh1));
        auto d = std::move( rs1(name, call_id) );
        
        handler( std::move(d), io_id, [wthis, handler, io_id](data_ptr d)
        {
          auto pthis = wthis.lock();
          if ( pthis == nullptr )
            return;

          pthis->perform_io( std::move(d), io_id, [io_id, handler](data_ptr d)
          {
            handler( std::move(d), io_id, nullptr );
          });
        });
      }
      else if ( ns1!=nullptr)
      {
        auto d = std::move(ns1(name));
        handler( std::move(d), io_id, nullptr);
      }
    };
    /*
    // Не нужен! здесь wthis
    std::weak_ptr<self> wthis = this->shared_from_this();
    opt.send_request = [handler, wthis](const char* name, result_handler_t result_handler, request_serializer_t ser)
    {
      if ( auto pthis = wthis.lock() )
      {
        auto call_id = pthis->_call_counter.fetch_add(1);
        pthis->_call_map.set(call_id, std::move(result_handler));
          
        auto d = std::move(ser(name, call_id));
        // TODO: wrap
        // нужен здесь wthis
        handler( std::move(d), pthis->_io_id, [wthis, handler](data_ptr d)
        {
          if ( auto pthis = wthis.lock() )
          {
            io_id_t io_id = pthis->_io_id;
            pthis->perform_io( std::move(d), io_id, [io_id, handler](data_ptr d)
            {
              handler( std::move(d), io_id, nullptr );
            });
          }
        });
      }
    };

    opt.send_notify = [handler, wthis](const char* name, notify_serializer_t ser)
    {
      if ( auto pthis = wthis.lock() )
      {
        auto d = std::move(ser(name));
        // TODO: wrap
        handler( std::move(d), pthis->_io_id, nullptr);
      }
    };
    */
  }

  template<typename O>
  void upgrate_options_(O& opt, outgoing_handler_t handler)
  {
    if ( handler == nullptr )
    {
      opt.sender_handler = nullptr;
      return;
    }
    
    std::weak_ptr<self> wthis = this->shared_from_this();
    opt.sender_handler = [handler, wthis](const char* name, notify_serializer_t ns1, request_serializer_t rs1, result_handler_t rh1)
    {
      auto pthis = wthis.lock();
      if ( pthis==nullptr )
        return;
      
      call_id_t call_id = 0;
      outgoing_holder::result_handler_t rhw = nullptr;
      
      if ( rs1!=nullptr && rh1!=nullptr )
      {
        io_id_t io_id = pthis->_io_id;
        outgoing_holder::result_handler_t rhw = [wthis,  handler, io_id](incoming_holder holder)
        {
          auto pthis = wthis.lock();
          if ( pthis==nullptr )
            return;

          if ( holder.is_response() || holder.is_error() )
          {
            if ( auto h = pthis->_call_map.detach( holder.get_id<call_id_t>() ) )
            {
              h(std::move(holder) );
            }
          }
          else
          {
            pthis->perform_jsonrpc( std::move(holder), io_id, [handler](outgoing_holder holder)
            {
              handler( std::move(holder) ) ;
            });
          }
        }; // rh=[](){}
        
        call_id = pthis->_call_counter.fetch_add(1);
        pthis->_call_map.set(call_id, std::move(rh1) );
      }
      
      if ( ns1!=nullptr )
      {
        outgoing_holder holder(name, std::move(ns1), std::move(rs1), std::move(rhw), call_id );
        handler( std::move(holder) );
      }
      else
      {
        handler( std::move(outgoing_holder()) );
      }
    };

    
    /*
    if ( handler == nullptr )
    {
      opt.send_request = nullptr;
      opt.send_notify = nullptr;
      return;
    }

    // TODO: сделать один общий отправитель 
    std::weak_ptr<self> wthis = this->shared_from_this();
    opt.send_request = [handler, wthis](const char* name, result_handler_t rh1, request_serializer_t rs1)
    {
      if ( auto pthis = wthis.lock() )
      {
        io_id_t io_id = pthis->_io_id;
        outgoing_holder::result_handler_t rh = [wthis,  handler, io_id](incoming_holder holder)
        {
          if ( auto pthis = wthis.lock() )
          {
            if ( holder.is_response() || holder.is_error() )
            {
              if ( auto h = pthis->_call_map.detach( holder.get_id<call_id_t>() ) )
              {
                h(std::move(holder) );
              }
            }
            else
            {
              pthis->perform_jsonrpc( std::move(holder), io_id, [handler](outgoing_holder holder)
              {
                handler( std::move(holder) ) ;
              });
            }
          };
        };
        outgoing_holder::request_serializer_t rs = std::move(rs1);

        auto call_id = pthis->_call_counter.fetch_add(1);
        pthis->_call_map.set(call_id, rh1 );
        
        static_assert( std::is_same<outgoing_holder::request_serializer_t, request_serializer_t>::value, "TATAM" );
        outgoing_holder holder(name, std::move(rs), std::move(rh), call_id );
        handler(  std::move(holder) );
      }
    };

    opt.send_notify = [handler, wthis](const char* name, notify_serializer_t ns1)
    {
      if ( auto pthis = wthis.lock() )
      {
        outgoing_holder::notify_serializer_t ns = std::move(ns1);
        // auto d = std::move(ser(name));
        // TODO: wrap
        outgoing_holder holder(name, std::move(ns) );
        handler( std::move(holder)  );
      }
    };
    */
  }

  /*
  template<typename O>
  void upgrate_options_(O& opt, incoming_handler_t handler)
  {
    if ( handler == nullptr )
    {
      opt.send_request = nullptr;
      opt.send_notify = nullptr;
      return;
    }

    std::weak_ptr<self> wthis = this->shared_from_this();
    opt.send_request = [handler, wthis](const char* name, result_handler_t rh1, request_serializer_t rs1)
    {
      if ( auto pthis = wthis.lock() )
      {
        outgoing_holder::result_handler_t rh = std::move(rh1);
        outgoing_holder::request_serializer_t rs = std::move(rs1);

        // TODO: убрать call_id ??? 
        auto call_id = pthis->_call_counter.fetch_add(1);
        pthis->_call_map.set(call_id, rh );
        
        static_assert( std::is_same<outgoing_holder::request_serializer_t, request_serializer_t>::value, "TATAM" );
        outgoing_holder holder(name, std::move(rs), std::move(rh) );
        handler(  std::move(holder) );
      }
    };

    opt.send_notify = [handler, wthis](const char* name, notify_serializer_t ns1)
    {
      if ( auto pthis = wthis.lock() )
      {
        outgoing_holder::notify_serializer_t ns = std::move(ns1);
        // auto d = std::move(ser(name));
        // TODO: wrap
        outgoing_holder holder(name, std::move(ns) );
        handler( std::move(holder) );
      }
    };
  }
  */


  // OutgoingHandler io::outgoing_handler_t или jsonrpc::outgoing_handler_t
  template<typename Opt, typename OutgoingHandler>
  handler_ptr create_handler_(io_id_t io_id, Opt opt, OutgoingHandler handler, bool reg_io)
  {
    bool reinit;
    auto ph = _handler_map.findocre(io_id, reg_io, reinit);
    if ( reinit )
    {
      // специализация в зависимости от типа OutgoingHandler
      this->upgrate_options_(opt, std::move(handler));
      if ( !ph->status() )
      {
        ph->start(opt);
      }
      else if ( reinit )
      {
        ph->reconfigure(opt);
      }
    }
    return ph;
  }

private:

  std::function<handler_ptr(io_id_t, outgoing_handler_t, bool)> _outgoing_rpc_factory;
  std::function<handler_ptr(io_id_t, ::iow::io::outgoing_handler_t, bool)> _outgoing_io_factory;
  //std::function<handler_ptr(io_id_t, incoming_handler_t, bool)> _incoming_rpc_factory;
  std::function<handler_ptr(io_id_t, ::iow::io::incoming_handler_t, bool)> _incoming_io_factory;

  typedef handler_map<handler_type> handler_map_t;
  handler_map_t _handler_map;
  //handler_ptr _common_handler;
  call_map _call_map;
  std::atomic<int> _call_counter;
  //std::atomic<bool> _direct_mode;
  std::atomic<io_id_t> _io_id;
};

}}
