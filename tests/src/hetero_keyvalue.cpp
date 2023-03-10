//
// Created by yuri on 1/28/23.
//

#include <doctest.h>
#include <sstream>
#include <cstring>

#include "het/het_keyvalue.h"

#include "include/hetero_test.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

TEST_CASE("heterogeneous key-value add-or-default ctor test") {
  het::hkeyvalue hkv(std::make_pair(1, "string"s));
  CHECK(hkv.value_or_add(1, "string"s) == "string"s);
  het::hkeyvalue hkv1 = hkv;
  CHECK(hkv1.value_or_add(2, "string2"s) == "string2"s);
}

TEST_CASE("heterogeneous key-value one-by-one ctor test") {
  het::hkeyvalue hkv;
  hkv.add_values(std::make_pair(1, 1));
  hkv.add_values(std::make_pair(2l, 2));
  hkv.add_values(std::make_pair(std::string_view("string"), 3));
  hkv.add_values(std::make_pair(3.f, "stringview"sv));
  hkv.add_values(std::make_pair('c', "stringview"sv));
  hkv.add_values(std::make_pair(4., "stringview"sv));
  CHECK_EQ(hkv.value<int>(1), 1);
  CHECK_EQ(hkv.value<int>(2l), 2);
  CHECK_EQ(hkv.value<int>(std::string_view("string")), 3);
  CHECK_EQ(hkv.value<std::string_view>(3.f), "stringview"sv);
  CHECK_EQ(hkv.value<std::string_view>('c'), "stringview"sv);
  CHECK_EQ(hkv.value<std::string_view>(4.), "stringview"sv);
}

TEST_CASE("heterogeneous key-value unbounded value access") {
//  het::hkeyvalue hkv;
//  auto t1 = het::to_tuple<int, double>(1, hkv);
//  auto t2 = het::to_vector<int>("key1"sv, hkv);
//  CHECK_THROWS_AS(het::to_tuple<int, double>(1, hkv), std::out_of_range); // {int(1)->int+, int(1)->double+}
//  CHECK_THROWS_AS(het::to_vector<int>("key1"sv, hkv), std::out_of_range); // {string_view("key")->int+}
}

TEST_CASE("heterogeneous key-value to tuple transform") {
  het::hkeyvalue hkv(
      std::make_pair('a', 1),
      std::make_pair(1, 2),
      std::make_pair(2.0, 3),
      std::make_pair("foo"s, 4),
      std::make_pair(1, "stringview"sv));

  auto [i1, sv] = het::to_tuple<int, std::string_view>(hkv, 1);

  CHECK(i1 == 2);
  CHECK(sv == "stringview"sv);
}

TEST_CASE("heterogeneous key-value to vector transform") {
  het::hkeyvalue hkv(
      std::make_pair('a', 1),
      std::make_pair(1, 2),
      std::make_pair(2.0, 3),
      std::make_pair("foo"s, 4),
      std::make_pair(1, "stringview"sv));

  auto v = het::to_vector<int>(hkv, 'a', 1, 2.0, "foo"s);

  CHECK(v == std::vector<int>{1, 2, 3, 4});
}

TEST_CASE("heterogeneous key-value move ctor test") {
//  het::hkeyvalue hkv(std::make_pair(1, std::make_unique<int>(123)));
//  CHECK(*hkv.template value<std::unique_ptr<int>>(1, hkv).get() == *std::make_unique<int>(123).get());
}

TEST_CASE("heterogeneous key-value bulk ctor test") {
  het::hkeyvalue hkv;
  hkv.add_values(std::make_pair(1, 1), std::make_pair(2l, 2l), std::make_pair(std::string_view("string"), 3),
                 std::make_pair(3.f, 4.f), std::make_pair('c', 5), std::make_pair(4., 6.));
  CHECK_EQ(hkv.value<int>(1), 1);
  CHECK_EQ(hkv.value<long>(2l), 2l);
  CHECK_EQ(hkv.value<int>("string"sv), 3);
  CHECK_EQ(hkv.value<float>(3.f), 4.f);
  CHECK_EQ(hkv.value<int>('c'), 5);
  CHECK_EQ(hkv.value<double>(4.), 6.);
}

TEST_CASE("heterogeneous value add/modify/erase/replace test") {
  het::hkeyvalue hkv;
  hkv.add_values(std::make_pair(1, 1), std::make_pair(2l, 2l), std::make_pair(std::string_view("string"), 3),
                 std::make_pair(3.f, 4.f), std::make_pair('c', 5), std::make_pair(4., 6.));

//  auto sz = sizeof(hkv);
//  CHECK_EQ(hkv.value<int>(), 1);
//
//  hkv.erase_values<int>();
//  CHECK_EQ(hv.arity(), 5);
//
//  hkv.add_values(42);
//  CHECK_EQ(hv.arity(), 6);
//  CHECK_EQ(hkv.value<int>(), 42);
//
//  hkv.modify_values("new string"sv, 769);
//  CHECK_EQ(hv.arity(), 6);
//  CHECK_EQ(hkv.value<std::string_view>(), "new string"sv);
//  CHECK_EQ(hkv.value<int>(), 769);
//
//  hkv.add_values("string"sv);
//  CHECK_EQ(hv.arity(), 6);
//  CHECK_EQ(hkv.value<std::string_view>(), "string"sv);
//
//  CHECK_EQ(sz, sizeof(hkv));
}

TEST_CASE("heterogeneous value visitor test") {
  het::hkeyvalue hkv;
  hkv.add_values(std::make_pair(1, 1), std::make_pair(2, 2l), std::make_pair(3, std::string_view("string")),
                 std::make_pair(3.f, 4.f), std::make_pair('c', 5), std::make_pair(4., 6.));

  std::stringstream ss;
  auto f = hkv.visit</*key type*/int, /*value_types*/int, long, std::string_view>()(
      [&ss](auto && k, auto && t) {
        ss << k << ", " << t << "\n";
        return het::VisitorReturn::Continue;
      });
  CHECK_EQ(f, het::VisitorReturn::Continue);
  CHECK_EQ(std::strcmp(ss.str().c_str(), "1, 1\n2, 2\n3, string\n"), 0);
}

TEST_CASE("heterogeneous non-trivial value test") {
//  het::hkeyvalue hkv;
//  CHECK(hkv.empty());
//
//  hkv.add_values(std::make_pair(123, 1));
//  hkv.add_values(std::make_pair("sed"sv, 2), std::make_pair("foo"s, 3.f), std::make_pair(nvo<float>{3.14f}, 4.));
//  CHECK(hkv.size() == 4);
//
//  het::hkeyvalue hkv1;
//  CHECK(hkv1.empty());
//
//  hv1.add_values('a', 1, 2.0, 3);
//  hv1.add_values("foo"s);
//  hetero_rec hr;
//  hr._h.push_back(12);
//  hr._p.first = "name";
//  hr._p.second = "value";
//  hv1.add_values(hr);
//
//  CHECK(!hkv1.empty());
//
//  CHECK(hkv1.value<int>() == 3);
//
//  CHECK(hkv1.value<hetero_rec>()._h.fraction<int>().size() == 1);
//  CHECK(hkv1.value<hetero_rec>()._h.fraction<int>()[0] == 12);
//  CHECK(hkv1.value<hetero_rec>()._p.first == "name");
//  CHECK(hkv1.value<hetero_rec>()._p.second == "value");
//
//  het::hkeyvalue hkv2 = hkv1;
//  CHECK(!hkv2.empty());
//
//  CHECK(hkv2.contains<hetero_rec>());
//
//  CHECK(hkv2.value<int>() == 3);
//
//  hkv2.erase_values<int, char>();
//  CHECK(!hkv2.empty());
//
////  std::cout << "Visit value -------------------------\n";
////  hv1.visit<char, double, int, std::string, hetero_rec>()([](auto const & value) {
////    std::cout << value << ", ";
////    return het::VisitorReturn::Continue;
////  });
////  std::cout << "\n---------------------------\n";
//
//  hkv1 = hkv;
//  CHECK(!hkv1.empty());
//  CHECK(hkv1.size() == 4);
//
//  hkv2.add_values('a', 3);
//
//  hkv = hkv2;
//  CHECK(!hkv.empty());
//  CHECK(hkv.arity() == 5);
//
////  std::cout << "Visit value -------------------------\n";
////  hv2.template visit<char, double, int, std::string, hetero_rec>()([](auto const & value) {
////    std::cout << value << ", ";
////    return het::VisitorReturn::Continue;
////  });
////  std::cout << "\n---------------------------\n";
//
//  CHECK(hkv.value<int>() == 3);
//
//  CHECK(hkv.value<hetero_rec>()._h.fraction<int>().size() == 1);
//  CHECK(hkv.value<hetero_rec>()._h.fraction<int>()[0] == 12);
//  CHECK(hkv.value<hetero_rec>()._p.first == "name");
//  CHECK(hkv.value<hetero_rec>()._p.second == "value");
//
//  CHECK(!hkv2.empty());
//  CHECK(hkv2.arity() == 5);
//
//  CHECK(hkv2.value<int>() == 3);
//
//  CHECK(hkv2.value<hetero_rec>()._h.fraction<int>().size() == 1);
//  CHECK(hkv2.value<hetero_rec>()._h.fraction<int>()[0] == 12);
//  CHECK(hkv2.value<hetero_rec>()._p.first == "name");
//  CHECK(hkv2.value<hetero_rec>()._p.second == "value");
}
