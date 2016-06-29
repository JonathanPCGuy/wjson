#include <fas/testing.hpp>
#include <iow/json/json.hpp>


UNIT(array1, "Одномерный массив")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  int nums[5]={1,2,3,4,5};
  array< value<int>[5] >::serializer ser;
  ser(nums, std::back_inserter(json) );
  t << equal<expect>( json, "[1,2,3,4,5]") << FAS_ENDL;

  std::fill_n(nums, 5, 0);
  json_error e;
  ser(nums, json.begin(), json.end(), &e);
  t << is_false<expect>(e) << e.where( json.begin(), json.end() ) << ": " << FAS_ENDL;

  json.clear();
  ser(nums, std::back_inserter(json) );
  t << equal<expect>( json, "[1,2,3,4,5]") << FAS_ENDL;

  // Проверки ошибок
  json = "1,2,3,4,5]";
  e.reset();
  ser(nums, json.begin(), json.end(), &e);
  std::string error = e.trace( json.begin(), json.end() );
  t << message("Test JSON error: ") << e.message() << error;
  t << is_true<expect>(e) << FAS_ENDL;
  t << equal<expect>(error, "Expected of '[': >>>1,2,3,4,5]") << FAS_ENDL;
  
  json = "[1,2,3,4,5";
  e.reset();
  ser(nums, json.begin(), json.end(), &e);
  error = e.trace( json.begin(), json.end() );
  t << message("Test JSON error: ") << e.message() << error;
  t << is_true<expect>(e) << FAS_ENDL;
  t << equal<expect>(error, "Unexpected end of ragment: [1,2,3,4,5<<<") << FAS_ENDL;

  json = "[1,2,3,[4,5]";
  e.reset();
  ser(nums, json.begin(), json.end(), &e);
  error = e.trace( json.begin(), json.end() );
  t << message("Test JSON error: ") << e.message() <<error;
  t << is_true<expect>(e) << FAS_ENDL;
  t << equal<expect>(error, "Invalid json number: [1,2,3,>>>[4,5]") << FAS_ENDL;

  json = "[1,2,3 4,5]";
  e.reset();
  ser(nums, json.begin(), json.end(), &e);
  error = e.trace( json.begin(), json.end() );
  t << message("Test JSON error: ") << e.message() << error;
  t << is_true<expect>(e) << FAS_ENDL;
  t << equal<expect>(error, "Expected of ',': [1,2,3 >>>4,5]") << FAS_ENDL;

  json = "[1,2,'3 4',5]";
  e.reset();
  ser(nums, json.begin(), json.end(), &e);
  error = e.trace( json.begin(), json.end() );
  t << message("Test JSON error: ") << e.message() << error;
  t << is_true<expect>(e) << FAS_ENDL;
  t << equal<expect>(error, "Invalid json number: [1,2,>>>'3 4',5]") << FAS_ENDL;
}

UNIT(array2, "Двумерный массив")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  int nums[2][2]={{1,2},{3,4}};
  typedef array< value<int>[2] > array1_json;
  typedef array< array1_json[2] > array2_json;
  array2_json::serializer ser;
  ser(nums, std::back_inserter(json) );
  t << equal<expect>( json, "[[1,2],[3,4]]") << FAS_ENDL;

  json="[[4,3],[2,1]]";
  json_error e;
  ser(nums, json.begin(), json.end(), &e);
  t << is_false<expect>(e) << e.message() <<  e.trace( json.begin(), json.end() ) << ": " << FAS_ENDL;
  json.clear();
  ser(nums, std::back_inserter(json) );
  t << equal<expect>( json, "[[4,3],[2,1]]") << FAS_ENDL;
}

UNIT(array3, "Трехмерный массив")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  int nums[2][2][1]={{{1},{2}},{{3},{4}}};
  typedef array< value<int>[1] > array1_json;
  typedef array< array1_json[2] > array2_json;
  typedef array< array2_json[2] > array3_json;
  array3_json::serializer ser;
  ser(nums, std::back_inserter(json) );
  t << equal<expect>( json, "[[[1],[2]],[[3],[4]]]") << FAS_ENDL;

  json="[[[4],[3]],[[2],[1]]]";
  json_error e;
  ser(nums, json.begin(), json.end(), &e);
  t << is_false<expect>(e) << e.message() << e.trace( json.begin(), json.end() ) << ": " << FAS_ENDL;
  json.clear();
  ser(nums, std::back_inserter(json) );
  t << equal<expect>( json, "[[[4],[3]],[[2],[1]]]") << FAS_ENDL;

  // при десериализации перезапишем только последнюю [1] на [5]
  json="[[[],[]],[[],[5]]]";
  ser(nums, json.begin(), json.end(), &e);
  t << is_false<expect>(e) << e.message() << e.trace( json.begin(), json.end() ) << ": " << FAS_ENDL;
  json.clear();
  ser(nums, std::back_inserter(json) );
  t << equal<expect>( json, "[[[4],[3]],[[2],[5]]]") << FAS_ENDL;

  // Обнулим все элементы кроме последнего
  json="[[[null],[null]],[[null],[5]]]";
  ser(nums, json.begin(), json.end(), &e);
  t << is_false<expect>(e) << e.message() << e.trace( json.begin(), json.end() ) << ": " << FAS_ENDL;
  json.clear();
  ser(nums, std::back_inserter(json) );
  t << equal<expect>( json, "[[[0],[0]],[[0],[5]]]") << FAS_ENDL;

  // сбросим все
  json="null";
  ser(nums, json.begin(), json.end(), &e);
  t << is_false<expect>(e) << e.message() << e.trace( json.begin(), json.end() ) << ": " << FAS_ENDL;
  json.clear();
  ser(nums, std::back_inserter(json) );
  t << equal<expect>( json, "[[[0],[0]],[[0],[0]]]") << FAS_ENDL;
}

UNIT(array4, "std::vector< std::string >")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  std::vector< std::string > vs;
  vector_of_strings< 10 >::serializer ser;
  vs.push_back("Привет");
  vs.push_back("Мир");
  vs.push_back("!");
  vs.push_back("世界你好!");
  ser(vs, std::back_inserter(json));
  t << equal<expect>( json, "[\"Привет\",\"Мир\",\"!\",\"世界你好!\"]" ) << FAS_ENDL;
  json = "[\"Привет\",\"Мир\",\"!\",\"\u4e16\u754c\u4f60\u597d!\"]";
  t << message( "re-serialization" ) << json << " -> " << "[\"Привет\",\"Мир\",\"!\",\"世界你好!\"]" ;
  json_error e;
  vs.clear();
  ser(vs, json.begin(), json.end(), &e);
  t << equal<expect>( vs[0], "Привет" ) << FAS_ENDL;
  t << equal<expect>( vs[1], "Мир" ) << FAS_ENDL;
  t << equal<expect>( vs[2], "!" ) << FAS_ENDL;
  t << equal<expect>( vs[3], "世界你好!" ) << FAS_ENDL;
}

UNIT(array5, "std::deque< int >")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  std::deque< int > dq = {1,2,3,4,5};
  array< std::deque< value<int> > >::serializer ser;
  ser(dq, std::back_inserter(json));
  t << equal<expect>(json, "[1,2,3,4,5]") << FAS_ENDL;
  std::deque< int > dq2;
  json_error e;
  ser(dq2, json.begin(), json.end(), &e );
  t << is_false<expect>(e) << FAS_ENDL;
  t << equal<expect>(dq, dq2) << FAS_ENDL;
}

UNIT(array6, "std::list< int >")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  std::list< int > dq = {1,2,3,4,5};
  array< std::list< value<int> > >::serializer ser;
  ser(dq, std::back_inserter(json));
  t << equal<expect>(json, "[1,2,3,4,5]") << FAS_ENDL;
  std::list< int > dq2;
  json_error e;
  ser(dq2, json.begin(), json.end(), &e );
  t << is_false<expect>(e) << FAS_ENDL;
  t << equal<expect>(dq, dq2) << FAS_ENDL;
}

UNIT(array7, "std::set< int >")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  std::set< int > dq = {1,2,3,4,5};
  array< std::set< value<int> > >::serializer ser;
  ser(dq, std::back_inserter(json));
  t << equal<expect>(json, "[1,2,3,4,5]") << FAS_ENDL;
  std::set< int > dq2;
  json_error e;
  ser(dq2, json.begin(), json.end(), &e );
  t << is_false<expect>(e) << FAS_ENDL;
  t << equal<expect>(dq, dq2) << FAS_ENDL;
}

UNIT(array8, "std::multiset< int >")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  std::multiset< int > dq = {1,1,1,2,3,4,5};
  array< std::multiset< value<int> > >::serializer ser;
  ser(dq, std::back_inserter(json));
  t << equal<expect>(json, "[1,1,1,2,3,4,5]") << FAS_ENDL;
  std::multiset< int > dq2;
  json_error e;
  ser(dq2, json.begin(), json.end(), &e );
  t << is_false<expect>(e) << FAS_ENDL;
  t << equal<expect>(dq, dq2) << FAS_ENDL;
}

UNIT(array9, "std::unordered_set< int >")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  std::unordered_set< int > dq = {1,2,3,4,5};
  array< std::unordered_set< value<int> > >::serializer ser;
  ser(dq, std::back_inserter(json));
  t << message("unordered_set: ") << json;
  // t << equal<expect>(json, "[1,2,3,4,5]") << FAS_ENDL;
  std::unordered_set< int > dq2;
  json_error e;
  ser(dq2, json.begin(), json.end(), &e );
  t << is_false<expect>(e) << FAS_ENDL;
  t << equal<expect>(dq, dq2) << FAS_ENDL;
}

UNIT(array10, "std::unordered_multiset< int >")
{
  using namespace fas::testing;
  using namespace iow::json;

  std::string json;
  std::unordered_multiset< int > dq = {1,2,3,4,5};
  array< std::unordered_multiset< value<int> > >::serializer ser;
  ser(dq, std::back_inserter(json));
  // t << equal<expect>(json, "[1,2,3,4,5]") << FAS_ENDL;
  t << message("unordered_multiset: ") << json;
  std::unordered_multiset< int > dq2;
  json_error e;
  ser(dq2, json.begin(), json.end(), &e );
  t << is_false<expect>(e) << FAS_ENDL;
  t << equal<expect>(dq, dq2) << FAS_ENDL;
}


BEGIN_SUITE(array, "")
  ADD_UNIT(array1)
  ADD_UNIT(array2)
  ADD_UNIT(array3)
  ADD_UNIT(array4)
  ADD_UNIT(array5)
  ADD_UNIT(array6)
  ADD_UNIT(array7)
  ADD_UNIT(array8)
  ADD_UNIT(array9)
  ADD_UNIT(array10)
END_SUITE(array)