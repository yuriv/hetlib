//
// Created by yuri on 1/28/23.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include "het/het.h"

#include "include/hetero_test.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

TEST_CASE("heterogeneous value add-or-default ctor test") {
  het::hvalue hv(1);
  CHECK(hv.value_or_add(1) == 1);
  het::hvalue hv1 = hv;
  CHECK(hv1.value_or_add(2) == 1);
}

TEST_CASE("heterogeneous value one-by-one ctor test") {
  het::hvalue hv;
  hv.add_values(1);
  hv.add_values(2l);
  hv.add_values(std::string_view("string"));
  hv.add_values(3.f);
  hv.add_values('c');
  hv.add_values(4.);
  CHECK_EQ(hv.arity(), 6);
  CHECK_EQ(hv.value<int>(), 1);
  CHECK_EQ(hv.value<long>(), 2l);
  CHECK_EQ(hv.value<std::string_view>(), std::string_view("string"));
  CHECK_EQ(hv.value<float>(), 3.f);
  CHECK_EQ(hv.value<char>(), 'c');
  CHECK_EQ(hv.value<double>(), 4.);
}

TEST_CASE("heterogeneous value unbounded value access") {
  het::hvalue hv;
  CHECK_THROWS_AS(het::to_tuple<int>(hv), std::out_of_range);
}

TEST_CASE("heterogeneous value to tuple transform") {
  het::hvalue hv('a', 1, 2.0, "foo"s);

  auto [i1, d, s, c] = het::to_tuple<int, double, std::string, char>(hv);

  CHECK(i1 == 1);
  CHECK(d == 2.0);
  CHECK(s == "foo"s);
  CHECK(c == 'a');
}

TEST_CASE("heterogeneous value move ctor test") {
  het::hvalue hv(std::make_unique<int>(123));
  CHECK(*hv.template value<std::unique_ptr<int>>().get() == *std::make_unique<int>(123).get());
}

TEST_CASE("heterogeneous value bulk ctor test") {
  het::hvalue hv;
  hv.add_values(1, 2l, std::string_view("string"), 3.f, 'c', 4.);
  CHECK_EQ(hv.arity(), 6);
  CHECK_EQ(hv.value<int>(), 1);
  CHECK_EQ(hv.value<long>(), 2l);
  CHECK_EQ(hv.value<std::string_view>(), std::string_view("string"));
  CHECK_EQ(hv.value<float>(), 3.f);
  CHECK_EQ(hv.value<char>(), 'c');
  CHECK_EQ(hv.value<double>(), 4.);
}

TEST_CASE("heterogeneous value compare test") {
  het::hvalue hv;
  hv.add_values(1, 2l, std::string_view("string"), 3.f, 'c', 4.);

  het::hvalue hv1;
  hv1.add_values(1);
  hv1.add_values(2l);
  hv1.add_values(std::string_view("string"));
  hv1.add_values(3.f);
  hv1.add_values('c');
  hv1.add_values(4.);

  CHECK(hv.equal<int, long, std::string_view, float, char, double>(hv1)([]<typename U>(U const & l, U const & h) { return l == h; }));
}

TEST_CASE("heterogeneous value add/modify/erase/replace test") {
  het::hvalue hv;

  auto sz = sizeof(hv);
  hv.add_values(1, 2l, std::string_view("string"), 3.f, 'c', 4.);
  CHECK_EQ(hv.arity(), 6);
  CHECK_EQ(hv.value<int>(), 1);

  hv.erase_values<int>();
  CHECK_EQ(hv.arity(), 5);

  hv.add_values(42);
  CHECK_EQ(hv.arity(), 6);
  CHECK_EQ(hv.value<int>(), 42);

  hv.modify_values("new string"sv, 769);
  CHECK_EQ(hv.arity(), 6);
  CHECK_EQ(hv.value<std::string_view>(), "new string"sv);
  CHECK_EQ(hv.value<int>(), 769);

  hv.add_values("string"sv);
  CHECK_EQ(hv.arity(), 6);
  CHECK_EQ(hv.value<std::string_view>(), "string"sv);

  CHECK_EQ(sz, sizeof(hv));
}

TEST_CASE("heterogeneous value visitor test") {
  het::hvalue hv;
  hv.add_values(1, 2l, std::string_view("string"), 3.f, 'c', 4.);

  std::stringstream ss;
  auto f = hv.visit<int, long, std::string_view, float, char, double>()(
      [&ss](auto && t) {
        ss << t << "\n";
        return het::VisitorReturn::Continue;
      });
  CHECK_EQ(f, het::VisitorReturn::Continue);
  CHECK_EQ(std::strcmp(ss.str().c_str(), "1\n2\nstring\n3\nc\n4\n"), 0);
}

TEST_CASE("heterogeneous non-trivial value test") {
  het::hvalue hv;
  CHECK(hv.empty());

  hv.add_values(123);
  hv.add_values("sed"sv, "foo"s, nvo<float>{3.14f});
  CHECK(hv.arity() == 4);

  het::hvalue hv1;
  CHECK(hv1.empty());

  hv1.add_values('a', 1, 2.0, 3);
  hv1.add_values("foo"s);
  hetero_rec hr;
  hr._h.push_back(12);
  hr._p.first = "name";
  hr._p.second = "value";
  hv1.add_values(hr);

  CHECK(!hv1.empty());
  CHECK(hv1.arity() == 5);

  CHECK(hv1.value<int>() == 3);

  CHECK(hv1.value<hetero_rec>()._h.fraction<int>().size() == 1);
  CHECK(hv1.value<hetero_rec>()._h.fraction<int>()[0] == 12);
  CHECK(hv1.value<hetero_rec>()._p.first == "name");
  CHECK(hv1.value<hetero_rec>()._p.second == "value");

  het::hvalue hv2 = hv1;
  CHECK(!hv2.empty());
  CHECK(hv2.arity() == 5);

  CHECK(hv2.contains<hetero_rec>());

  CHECK(hv2.value<int>() == 3);

  hv2.erase_values<int, char>();
  CHECK(!hv2.empty());
  CHECK(hv2.arity() == 3);

//  std::cout << "Visit value -------------------------\n";
//  hv1.visit<char, double, int, std::string, hetero_rec>()([](auto const & value) {
//    std::cout << value << ", ";
//    return het::VisitorReturn::Continue;
//  });
//  std::cout << "\n---------------------------\n";

  hv1 = hv;
  CHECK(!hv1.empty());
  CHECK(hv1.arity() == 4);

  hv2.add_values('a', 3);

  hv = hv2;
  CHECK(!hv.empty());
  CHECK(hv.arity() == 5);

//  std::cout << "Visit value -------------------------\n";
//  hv2.template visit<char, double, int, std::string, hetero_rec>()([](auto const & value) {
//    std::cout << value << ", ";
//    return het::VisitorReturn::Continue;
//  });
//  std::cout << "\n---------------------------\n";

  CHECK(hv.value<int>() == 3);

  CHECK(hv.value<hetero_rec>()._h.fraction<int>().size() == 1);
  CHECK(hv.value<hetero_rec>()._h.fraction<int>()[0] == 12);
  CHECK(hv.value<hetero_rec>()._p.first == "name");
  CHECK(hv.value<hetero_rec>()._p.second == "value");

  CHECK(!hv2.empty());
  CHECK(hv2.arity() == 5);

  CHECK(hv2.value<int>() == 3);

  CHECK(hv2.value<hetero_rec>()._h.fraction<int>().size() == 1);
  CHECK(hv2.value<hetero_rec>()._h.fraction<int>()[0] == 12);
  CHECK(hv2.value<hetero_rec>()._p.first == "name");
  CHECK(hv2.value<hetero_rec>()._p.second == "value");
}
