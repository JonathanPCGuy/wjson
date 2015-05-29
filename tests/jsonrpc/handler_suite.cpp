#include <iow/jsonrpc/handler.hpp>
#include <iow/jsonrpc/method.hpp>

#include <fas/testing.hpp>
#include "req.hpp"
#include <algorithm>

struct nomethod:
  ::iow::jsonrpc::method_list<>
{};

typedef iow::jsonrpc::handler<nomethod> nohandler;

int test_count = 0;

UNIT(nohandler_unit, "")
{
  using namespace fas::testing;
  using namespace ::iow::jsonrpc;
  
  nohandler h;
  for (auto r: bad_request)
  {
    auto req = std::make_unique< iow::jsonrpc::data_type>( r[0].begin(), r[0].end() );
    incoming_holder hold( std::move(req) );
    hold.parse();
    h.invoke(std::move(hold), [&t, r](iow::jsonrpc::data_ptr res)
    {
      t << message("request: ") << r[0];
      t << message("response: ") << std::string(res->begin(), res->end());
      if ( std::string(res->begin(), res->end()) != r[1] )
      {
        t << ::fas::testing::error("should be:") << r[1];
      }
      ++test_count;
    });
  }

  t << equal<expect>(test_count, 6) << FAS_TESTING_FILE_LINE;
  
  /*if ( test_count!=6 )
  {
    std::cout << "ERROR. invalid test count " << test_count << std::endl;
    exit(-1);
  }
  */
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////

typedef std::vector<int> test1_params;
typedef iow::json::array< std::vector< iow::json::value<int> > > test1_json;


struct itest1
{
  virtual ~itest1() {}
  virtual void method1(std::unique_ptr<test1_params> req, std::function< void(std::unique_ptr<test1_params>) > callback) = 0;
  virtual void method2(std::unique_ptr<test1_params> req, std::function< void(std::unique_ptr<test1_params>) > callback) = 0;
};

class test1: public itest1
{
public:
  
  virtual void method1(std::unique_ptr<test1_params> req, std::function< void(std::unique_ptr<test1_params>) > callback)
  {
    std::reverse(req->begin(), req->end());
    if ( callback )
      callback(std::move(req));
  }

  virtual void method2(std::unique_ptr<test1_params> req, std::function< void(std::unique_ptr<test1_params>) > callback)
  {
    if ( callback )
      callback(nullptr);
  }
};


JSONRPC_TAG(method1)
JSONRPC_TAG(method2)


struct method_list: iow::jsonrpc::method_list
<
  iow::jsonrpc::target<itest1>,
  iow::jsonrpc::invoke_method< _method1_, test1_json,      test1_json,      itest1, &itest1::method1>,
  iow::jsonrpc::invoke_method< _method2_, test1_json,      test1_json,      itest1, &itest1::method2>
>
{};

typedef iow::jsonrpc::handler<method_list> handler1;


UNIT(handler1_unit, "")
{
  using namespace fas::testing;
  using namespace ::iow::jsonrpc;

   std::string unavailable = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32003,\"message\":\"Service Unavailable.\"},\"id\":1}";
  
  test_count = 0;
  handler1 h;
  for (auto r: good_request)
  {
    auto req = std::make_unique< iow::jsonrpc::data_type>( r[0].begin(), r[0].end() );
    incoming_holder hold( std::move(req) );
    hold.parse();
    
    h.invoke(std::move(hold), [&t, unavailable, r](iow::jsonrpc::data_ptr res)
    {
      t << message("request: ") << r[0];
      t << message("response: ") << std::string(res->begin(), res->end());
      if ( std::string(res->begin(), res->end()) != unavailable )
      {
        t << ::fas::testing::error("should be:") << unavailable;
      }
      ++test_count;
    });
  }
}

UNIT(handler2_unit, "")
{
  using namespace fas::testing;
  using namespace ::iow::jsonrpc;

 //  std::string unavailable = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32003,\"message\":\"Service Unavailable.\"},\"id\":1}";
  auto t1 = std::make_shared<test1>();
  test_count = 0;
  handler1 h = handler1(t1);
  h.target() = t1;
  for (auto r: good_request)
  {
    auto req = std::make_unique< iow::jsonrpc::data_type>( r[0].begin(), r[0].end() );
    incoming_holder hold( std::move(req) );
    hold.parse();
    
    h.invoke( std::move(hold), [&t,r](iow::jsonrpc::data_ptr res)
    {
      t << message("request: ") << r[0];
      t << message("response: ") << std::string(res->begin(), res->end());
      if ( std::string(res->begin(), res->end()) != r[1] )
      {
        t << ::fas::testing::error("should be:") << r[1];
      }
      ++test_count;
    });
  }
}

BEGIN_SUITE(handler_suite, "")
  ADD_UNIT(nohandler_unit)
  ADD_UNIT(handler2_unit)
END_SUITE(handler_suite)