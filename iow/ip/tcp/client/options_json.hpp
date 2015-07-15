#pragma once

#include <iow/ip/tcp/client/options.hpp>
#include <iow/io/client/options_json.hpp>
#include <iow/json/json.hpp>

namespace iow{ namespace ip{ namespace tcp{ namespace client{

struct options_json
{
  JSON_NAME(port)
  JSON_NAME(backlog)
  
  typedef json::object<
    options,
    json::member_list<
      json::base< ::iow::io::client::options_json< ::iow::ip::tcp::connection::options > >,
      json::member< n_port, options, std::string, &options::port >,
      json::member< n_backlog, options, int, &options::backlog >
    >
  > type;
  typedef type::target target;
  typedef type::serializer serializer;
  typedef type::member_list member_list;
};

}}}}
