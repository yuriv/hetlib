//
// Created by yuri on 8/25/21.
//

#include <doctest.h>

#include "het/het.h"

#include <string>
//#include <iostream>
#include <tuple>
#include <sstream>
#include <memory>

#include "include/hetero_test.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

//auto print_container = []<template <typename...> class IC, template <typename...> class OC>(het::hetero_container<IC, OC> & hc)
//  { hc.template visit<int, double, char, std::string, nvo<float>>()(print_visitor{}); std::cout << std::endl; };

TEST_CASE("heterogeneous deque based container test") {
  het::hdeque hc_dum;
  CHECK(hc_dum.empty());

  hc_dum.push_back('a');
  hc_dum.push_back(1);
  hc_dum.push_back(2.0);
  hc_dum.push_back(3);
  hc_dum.push_back("foo"s);
  hetero_rec hr;
  hr._h.push_back(12);
  hr._p.first = "name";
  hr._p.second = "value";
  hc_dum.push_back(hr);

  CHECK(!hc_dum.empty());
  CHECK(hc_dum.size() == 6);

  CHECK(hc_dum.fraction<char>().size() == 1);
  CHECK(hc_dum.fraction<char>()[0] == 'a');

  CHECK(hc_dum.fraction<int>().size() == 2);
  CHECK(hc_dum.fraction<int>()[0] == 1);
  CHECK(hc_dum.fraction<int>()[1] == 3);

  CHECK(hc_dum.fraction<std::string>().size() == 1);
  CHECK(hc_dum.fraction<std::string>()[0] == "foo");

  CHECK(hc_dum.fraction<hetero_rec>().size() == 1);
  CHECK(hc_dum.fraction<hetero_rec>()[0]._h.fraction<int>().size() == 1);
  CHECK(hc_dum.fraction<hetero_rec>()[0]._h.fraction<int>()[0] == 12);
  CHECK(hc_dum.at<hetero_rec>(0)._p.first == "name");
  CHECK(hc_dum.at<hetero_rec>(0)._p.second == "value");

//  std::cout << "Visit container -------------------------\n";
//  hc_dum.visit<char, int, double, std::string, hetero_rec>()(
//      [](auto const & value) {
//        if constexpr (std::same_as<decltype(value), hetero_rec>) {
//          std::cout << value._p << " ";
//        } else {
//          std::cout << value << " ";
//        }
//        return het::VisitorReturn::Continue;
//      });
//  std::cout << " \n-------------------------\n";
}

TEST_CASE("heterogeneous vector unbounded value access") {
  het::hvector hc;
  CHECK_THROWS_AS(het::to_tuple<int>(hc), std::out_of_range);
}

TEST_CASE("heterogeneous vector to tuple transform") {
  het::hvector hc;
  CHECK(hc.empty());

  hc.push_back('a');
  hc.push_back(1);
  hc.push_back(2.0);
  hc.push_back(3);
  hc.push_back(std::string{"foo"});

  auto [i1, i2, d, s, c] = het::to_tuple<int, int, double, std::string, char>(hc);

  CHECK(i1 == 1);
  CHECK(i2 == 3);
  CHECK(d == 2.0);
  CHECK(s == "foo"s);
  CHECK(c == 'a');
}

TEST_CASE("heterogeneous vector move ctor test") {
  het::hvector hc(std::make_unique<int>(123));
  CHECK(*hc.template fraction<std::unique_ptr<int>>()[0].get() == *std::make_unique<int>(123).get());
}

TEST_CASE("heterogeneous vector based container test") {
  het::hvector hc;
  CHECK(hc.empty());

  hc.push_back('a');
  hc.push_back(1);
  hc.push_back(2.0);
  hc.push_back(3);
  hc.push_back(std::string{"foo"});
  hetero_rec hr;
  hr._h.push_back(12);
  hc.push_back(hr);

  CHECK(!hc.empty());
  CHECK(hc.size() == 6);

//  print_container(hc);

  CHECK(hc.count_of<double>() == 1);
  CHECK(hc.at<double>(0) == 2.0);
  hc.erase<double>(0);
  CHECK(hc.count_of<double>() == 0);
  CHECK(hc.size() == 5);

//  print_container(hc);

  nvo<float> n{100.f };
  hc.push_back(n);
  hc.push_back(120);

  CHECK(hc.count_of<char>() == 1);
  CHECK(hc.count_of<int>() == 3);
  CHECK(hc.count_of<double>() == 0);
  CHECK(hc.count_of<std::string>() == 1);
  CHECK(hc.count_of<nvo<float>>() == 1);
  CHECK(hc.size() == 7);

//  print_container(hc);

  hc.fraction<int>()[0] = 2021;
  CHECK(hc.at<int>(0) == 2021);

//  std::cout << "'int' fraction:\n";
//  for(auto&& e : hc.fraction<int>()) {
//    std::cout << e << "\n";
//  }
}

TEST_CASE("vector based container het::find test") {
  het::hvector hc;
  CHECK(hc.empty());

  hc.push_back('a');
  hc.push_back(1);
  hc.push_back(2.0);
  hc.push_back(3);
  hc.push_back("foo"s);
  hetero_rec hr;
  hr._h.push_back(12);
  hc.push_back(hr);

  auto retval = hc.find<hetero_rec>(std::make_pair([](hetero_rec const & value) { return value._h.fraction<int>()[0]; }, 12));
//    std::cout << "+++++++++++++++++++++ " << (retval.first ? *retval.second : -1) << "\n";
  CHECK(retval.first);
  CHECK(retval.second->_h.fraction<int>()[0] == 12);
}

TEST_CASE("vector based container het::find_first/het::find_last test") {
  het::hvector hc;
  CHECK(hc.empty());

  hc.push_back('a');
  hc.push_back(1);
  hc.push_back(2.0);
  hc.push_back(3);
  hc.push_back(3);
  hc.push_back(0);
  hc.push_back("foo"s);
  hetero_rec hr;
  hr._h.push_back(12);
  hc.push_back(hr);

  auto retval1 = het::find_first<int>(hc, [](auto const & value) { return value == 3; });
  //    std::cout << "+++++++++++++++++++++ " << (retval1.first ? *retval1.second : -1) << "\n";
  CHECK(retval1.first);
  CHECK((*retval1.second == 3));

  auto retval2 = het::find_last<int>(hc, [](auto const & value) { return value == 3; });
//    std::cout << "+++++++++++++++++++++ " << (retval.2first ? *retval2.second : -1) << "\n";
  CHECK(retval2.first);
  CHECK((*retval2.second == 3));

  CHECK((*retval1.second == *retval2.second));
  CHECK((retval1.second + 1 == retval2.second));
}

TEST_CASE("vector based container het::find_all test") {
  het::hvector hb;

  hb.push_back(2);
  hb.push_back(1);
  hb.push_back(1);
  hb.push_back(3);
  hb.push_back(1);
  hb.push_back(1);
  hb.push_back(1);
  hb.push_back(1);
  hb.push_back(5);
  std::vector<int> o;
  het::find_all<int>(hb, std::back_inserter(o), [](auto const & value) { return value == 1 || value == 2; });
  //    for(auto && i : o) {
  //      std::cout << "+++++++++++++++++++++ " << i << "\n";
  //    }
  CHECK(o.size() == 7);
  CHECK(*o.cbegin() == 2);
  CHECK(*(o.cbegin() + 1) == 1);
}

TEST_CASE("vector based container het::query_first/het::query_last test") {
  het::hvector hb{
      std::make_pair("R0"sv, "apple"sv),
      std::make_pair("R1"sv, "orange"sv),
      std::make_pair("R2"sv, "melon"sv),
      std::make_pair("R3"sv, "thistle"sv),
      std::make_pair("R4"sv, "trefoil"sv),
      std::make_pair("R0"sv, "apple"sv),
      std::make_pair("R1"sv, "orange"sv),
      std::make_pair("R2"sv, "melon"sv),
      std::make_pair("R3"sv, "thistle"sv),
      std::make_pair("R4"sv, "trefoil"sv),
      std::make_pair("R0"sv, "orange"sv),
      std::make_pair("R1"sv, "apple"sv),
      std::make_pair("R2"sv, "thistle"sv),
      std::make_pair("R3"sv, "melon"sv),
      std::make_pair("R4"sv, "trefoil"sv)
  };

  auto retval1 = het::query_first<std::pair<std::string_view, std::string_view>>(hb,
        std::pair{&std::pair<std::string_view, std::string_view>::first, "R0"sv},
        std::pair{&std::pair<std::string_view, std::string_view>::second, "orange"sv});
  //    std::cout << "+++++++++++++++++++++ " << (retval1.first ? retval1.second->first : std::make_pair(""sv, ""sv).first) << "\n";
  CHECK(retval1.first);
  CHECK((*retval1.second == std::make_pair("R0"sv, "orange"sv)));

  auto retval2 = het::query_last<std::pair<std::string_view, std::string_view>>(hb,
        std::pair{&std::pair<std::string_view, std::string_view>::first, "R2"sv},
        std::pair{&std::pair<std::string_view, std::string_view>::second, "melon"sv});
  //    std::cout << "+++++++++++++++++++++ " << (retval2.first ? retval2.second->first : std::make_pair(""sv, ""sv).first) << "\n";
  CHECK(retval2.first);
  CHECK((*retval2.second == std::make_pair("R2"sv, "melon"sv)));

  CHECK((retval2.second + 3 == retval1.second));
}

TEST_CASE("vector based container het::match test") {
  std::stringstream ss;
  het::hvector{""sv, std::make_tuple(1, 2.f), ""s, 1.}
      .match<
          std::string_view/*exactly*/,
          std::tuple<int, float>/*exactly*/,
          double/*default*/,
          std::string/*string_view cast*/,
          int */*absent*/>()(
            [&ss](std::tuple<int, float>) { ss << 1; },
            [&ss](std::string_view) { ss << 2; },
            [&ss](auto) { ss << "default"; } // should be last
          );
  CHECK(ss.str() == "21default2");
}
