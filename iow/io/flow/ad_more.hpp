#pragma once

#include <iow/io/flow/tags.hpp>

namespace iow{ namespace io{ namespace flow{
  
struct ad_more
{
  template<typename T>
  void operator()(T& t)
  {
    std::cout << "flow::more" << std::endl;
    auto d = t.get_aspect().template get< _factory_ >()(t);
    if ( d!=nullptr )
    {
      std::cout << "flow::more ready" << std::endl;
      t.get_aspect().template get< _some_ >()(t, std::move(d) );
    }
    else
    {
      std::cout << "flow::more done" << std::endl;
      t.get_aspect().template gete< _on_done_ >()(t);
    }
  }
};

}}}