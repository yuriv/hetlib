//
// Created by yuri on 8/25/21.
//

#include <doctest.h>

#include "het/het.h"

#include <string>
#include <iostream>
#include <tuple>

#include "include/hetero_test.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

template <auto E> struct enum_wrapper {
  static constexpr decltype(E) value = E;
};

template<std::size_t N> struct string_literal {
  constexpr string_literal(const char (&str)[N]) {
    std::copy_n(str, N, value);
  }
  char value[N];
};

template <string_literal S> struct string_wrapper {
  char const * value = S.value;
};

template <auto E, string_literal S> struct pair {
  decltype(E) first = E;
  string_literal<sizeof(decltype(S)::value)> second = S;
};

template <string_literal S, auto E> struct pair_opposite {
  string_literal<sizeof(decltype(S)::value)> first = S;
  decltype(E) second = E;
};

template <auto E, string_literal S>
constexpr auto make_dictionary_pair(std::false_type) {
  return pair<E, S>{};
}

template <string_literal S, auto E>
constexpr auto make_dictionary_pair(std::true_type) {
  return pair_opposite<S, E>{};
}

auto print_container = []<template <typename...> class IC, template <typename...> class OC>(het::hetero_container<IC, OC> & hc)
  { hc.template visit<int, double, char, std::string, nvo<float>>()(print_visitor{}); std::cout << std::endl; };

TEST_CASE("heterogeneous list based container test") {
  het::hetero_container<std::vector, std::list> hc_lv;
  CHECK(hc_lv.empty());

  hc_lv.push_back('a');
  hc_lv.push_back(1);
  hc_lv.push_back(2.0);
  hc_lv.push_back(3);
  hc_lv.push_back(std::string{"foo"});
  hetero_rec hr;
  hr._h.push_back(12);
  hr._p.first = "name";
  hr._p.second = "value";
  hc_lv.push_back(hr);

  CHECK(!hc_lv.empty());
  CHECK(hc_lv.size() == 6);

  CHECK(hc_lv.fraction<char>().size() == 1);
  CHECK(hc_lv.fraction<char>()[0] == 'a');

  CHECK(hc_lv.fraction<int>().size() == 2);
  CHECK(hc_lv.fraction<int>()[0] == 1);
  CHECK(hc_lv.fraction<int>()[1] == 3);

  CHECK(hc_lv.fraction<std::string>().size() == 1);
  CHECK(hc_lv.fraction<std::string>()[0] == "foo");

  CHECK(hc_lv.fraction<hetero_rec>().size() == 1);
  CHECK(hc_lv.fraction<hetero_rec>()[0]._h.fraction<int>().size() == 1);
  CHECK(hc_lv.fraction<hetero_rec>()[0]._h.fraction<int>()[0] == 12);
  CHECK(hc_lv.at<hetero_rec>(0)._p.first == "name");
  CHECK(hc_lv.at<hetero_rec>(0)._p.second == "value");
}

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

  {
    auto retval = het::find_first<int>(hc, [](auto const & value) { return value == 3; });
//    std::cout << "+++++++++++++++++++++ " << (retval.first ? *retval.second : -1) << "\n";
    CHECK(retval.first);
    CHECK((*retval.second == 3));
  }
  {
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

    auto retval = het::query_first<std::pair<std::string_view, std::string_view>>(hb,
                  std::pair{&std::pair<std::string_view, std::string_view>::first, std::string_view{"R0"}},
                  std::pair{&std::pair<std::string_view, std::string_view>::second, std::string_view{"orange"}});

//    std::cout << "+++++++++++++++++++++ " << (retval.first ? retval.second->first : std::make_pair(""sv, ""sv).first) << "\n";
    CHECK(retval.first);
    CHECK((*retval.second == std::make_pair("R0"sv, "orange"sv)));
  }
  {
    auto retval = het::find_last<int>(hc, [](auto const & value) { return value == 1; });
//    std::cout << "+++++++++++++++++++++ " << (retval.first ? *retval.second : -1) << "\n";
    CHECK(retval.first);
    CHECK((*retval.second == 1));
  }
  {
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
    het::find_all<int>(hb, std::back_inserter(o), [](auto const & value) { return value == 1; });
//    for(auto && i : o) {
//      std::cout << "+++++++++++++++++++++ " << i << "\n";
//    }
    CHECK(o.size() == 6);
    CHECK(*o.cbegin() == 1);
  }

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
