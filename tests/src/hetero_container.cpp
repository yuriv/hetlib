//
// Created by yuri on 8/25/21.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include "include/het/hetero_container.h"
#include "het/het.h"

#include "property_impl.h"

#include <string>
#include <iostream>
#include <tuple>

template <typename T> struct nvo {
  void print(std::ostream& os) const { os << _value << "\n"; }
  T _value;
  friend std::ostream& operator<<(std::ostream& os, nvo const& n) {
    n.print(os);
    return os;
  }
};

struct hetero_rec {
  umf::hetero_container _h;
  umf::property_impl<std::string> _p;
};

struct print_visitor : umf::visitor_base<int, double, char, std::string, nvo<float>> {
  template<typename T> umf::VisitorReturn operator()(T& in) {
    std::cout << in << " ";
    return umf::VisitorReturn::vrContinue;
  }
};


struct het_print_visitor : het::visitor_base<int, std::string_view, std::string, nvo<float>> {
  template<typename T> het::VisitorReturn operator()(T& in) {
    std::cout << in << " ";
    return het::VisitorReturn::vrContinue;
  }
};

struct het_print_visitor1 : het::visitor_base<char, double, int, std::string, hetero_rec> {
  template<typename T> het::VisitorReturn operator()(T& in) {
    std::cout << in << " ";
    return het::VisitorReturn::vrContinue;
  }
};

std::ostream & operator<<(std::ostream & os, hetero_rec const & hr) {
  const_cast<hetero_rec &>(hr)._h.visit(print_visitor{});
  os << "\n" << hr._p << "\n";
  return os;
}

enum class PropertyKind {
  pkRegular = 0x0,/**
   * Regular kind of property, mutable, non mandatory,  not required
   */

  pkMandatory = 0x1,/**
   * Mandatory property, permanent
   */

  pkReadonly = 0x2,/**
   * Readonly property
   */

  pkRequired = 0x4,/**
   * Required value property
   */

  pkPropertySet = 0x8/**
   * Value is property set
   */
};

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

TEST_CASE("heterogeneous container test") {
  auto print_container = [](het::hetero_container& hc) { hc.visit(print_visitor{}); std::cout << std::endl; };
  //auto print_value = [](het::hetero_value<std::unordered_map> & hv) { hv.visit(het_print_visitor{}); std::cout << std::endl; };
  //auto print_value1 = [](het::hetero_value<std::unordered_map> & hv) { hv.visit(het_print_visitor1{}); std::cout << std::endl; };

  {
    het::hvalue hv;
    CHECK(hv.value_or_add(1) == 1);
    het::hvalue hv1 = hv;
    CHECK(hv1.value_or_add(2) == 1);
//    print_value(hv);
//    print_value(hv1);

//    static_assert(std::is_same_v<enum_wrapper<PropertyKind::pkRegular>,
//        enum_wrapper<PropertyKind::pkMandatory>>, "");
//    static_assert(std::is_same_v<string_wrapper<"pkReadonly">,
//        string_wrapper<"pkRequired">>, "");

    umf::adaptive_property dict;
    dict.add_names(
      enum_wrapper<PropertyKind::pkRegular>{},
      enum_wrapper<PropertyKind::pkMandatory>{},
      enum_wrapper<PropertyKind::pkReadonly>{},
      enum_wrapper<PropertyKind::pkRequired>{},
      enum_wrapper<PropertyKind::pkPropertySet>{},
      string_wrapper<"pkRegular">{},
      string_wrapper<"pkMandatory">{},
      string_wrapper<"pkReadonly">{},
      string_wrapper<"pkRequired">{},
      string_wrapper<"pkPropertySet">{}
    );
    dict.add_values(
      std::map<PropertyKind, char const *>{
        {PropertyKind::pkRegular, "pkRegular"},
        {PropertyKind::pkMandatory, "pkMandatory"},
        {PropertyKind::pkReadonly, "pkReadonly"},
        {PropertyKind::pkRequired, "pkRequired"},
        {PropertyKind::pkPropertySet, "pkPropertySet"}
      },
      std::map<char const *, PropertyKind>{
        {"pkRegular", PropertyKind::pkRegular},
        {"pkMandatory", PropertyKind::pkMandatory},
        {"pkReadonly", PropertyKind::pkReadonly},
        {"pkRequired", PropertyKind::pkRequired},
        {"pkPropertySet", PropertyKind::pkPropertySet}
      }
    );
  }
  {
    het::hvalue hv;
    CHECK(hv.empty());

    hv.add_values(123);
    hv.add_values(std::string_view{"sed"}, std::string{"foo"}, nvo<float>{3.14f});
    CHECK(hv.arity() == 4);

//    std::cout << "hv(" << &hv <<"): ";
//    print_value(hv);

    het::hvalue hv1;
    CHECK(hv1.empty());

    hv1.add_values('a', 1, 2.0, 3);
    hv1.add_values(std::string{"foo"});
    hetero_rec hr;
    hr._h.push_back(12);
    hr._p.set_name("name").set_category(2).
      set_kind(umf::PropertyKind::pkReadonly).set_value("initial");
    hv1.add_values(hr);

    CHECK(!hv1.empty());
    CHECK(hv1.arity() == 5);

    CHECK(hv1.value<int>() == 3);

    CHECK(hv1.value<hetero_rec>()._h.fraction<int>().size() == 1);
    CHECK(hv1.value<hetero_rec>()._h.fraction<int>()[0] == 12);
    CHECK(hv1.value<hetero_rec>()._p.name() == "name");
    CHECK(hv1.value<hetero_rec>()._p.category() == 2);
    CHECK(hv1.value<hetero_rec>()._p.kind() == umf::PropertyKind::pkReadonly);
    CHECK(hv1.value<hetero_rec>()._p.value() == "initial");

//    std::cout << "hv1(" << &hv1 <<"): ";
//    print_value1(hv1);

    het::hvalue hv2 = hv1;
    CHECK(!hv2.empty());
    CHECK(hv2.arity() == 5);

    CHECK(hv2.contains<hetero_rec>());

    CHECK(hv2.value<int>() == 3);

    hv2.erase_values<int, char>();
    CHECK(!hv2.empty());
    CHECK(hv2.arity() == 3);

    std::cout << "Visit value -------------------------\n";
    hv1.template visit<char, double, int, std::string, hetero_rec>()([](auto const & value) {
      std::cout << value << ", ";
      return het::VisitorReturn::vrContinue;
    });
    std::cout << "\n---------------------------\n";

    hv1 = hv;
    CHECK(!hv1.empty());
    CHECK(hv1.arity() == 4);

    hv2.add_values('a', 3);

    hv = hv2;
    CHECK(!hv.empty());
    CHECK(hv.arity() == 5);

    std::cout << "Visit value -------------------------\n";
    hv2.template visit<char, double, int, std::string, hetero_rec>()([](auto const & value) {
      std::cout << value << ", ";
      return het::VisitorReturn::vrContinue;
    });
    std::cout << "\n---------------------------\n";

    CHECK(hv.value<int>() == 3);

    CHECK(hv.value<hetero_rec>()._h.fraction<int>().size() == 1);
    CHECK(hv.value<hetero_rec>()._h.fraction<int>()[0] == 12);
    CHECK(hv.value<hetero_rec>()._p.name() == "name");
    CHECK(hv.value<hetero_rec>()._p.category() == 2);
    CHECK(hv.value<hetero_rec>()._p.kind() == umf::PropertyKind::pkReadonly);
    CHECK(hv.value<hetero_rec>()._p.value() == "initial");

    CHECK(!hv2.empty());
    CHECK(hv2.arity() == 5);

    CHECK(hv2.value<int>() == 3);

    CHECK(hv2.value<hetero_rec>()._h.fraction<int>().size() == 1);
    CHECK(hv2.value<hetero_rec>()._h.fraction<int>()[0] == 12);
    CHECK(hv2.value<hetero_rec>()._p.name() == "name");
    CHECK(hv2.value<hetero_rec>()._p.category() == 2);
    CHECK(hv2.value<hetero_rec>()._p.kind() == umf::PropertyKind::pkReadonly);
    CHECK(hv2.value<hetero_rec>()._p.value() == "initial");
  }
  {
    het::hetero_container<std::vector, std::list> hc_lv;
    CHECK(hc_lv.empty());

    hc_lv.push_back('a');
    hc_lv.push_back(1);
    hc_lv.push_back(2.0);
    hc_lv.push_back(3);
    hc_lv.push_back(std::string{"foo"});
    hetero_rec hr;
    hr._h.push_back(12);
    hr._p.set_name("name");
    hr._p.set_category(2);
    hr._p.set_kind(umf::PropertyKind::pkReadonly);
    hr._p.set_value("initial");
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
    CHECK(hc_lv.at<hetero_rec>(0)._p.name() == "name");
    CHECK(hc_lv.at<hetero_rec>(0)._p.category() == 2);
    CHECK(hc_lv.at<hetero_rec>(0)._p.kind() == umf::PropertyKind::pkReadonly);
    CHECK(hc_lv.at<hetero_rec>(0)._p.value() == "initial");
  }

  {
    het::hdeque hc_dum;
    CHECK(hc_dum.empty());

    hc_dum.push_back('a');
    hc_dum.push_back(1);
    hc_dum.push_back(2.0);
    hc_dum.push_back(3);
    hc_dum.push_back(std::string{"foo"});
    hetero_rec hr;
    hr._h.push_back(12);
    hr._p.set_name("name");
    hr._p.set_category(2);
    hr._p.set_kind(umf::PropertyKind::pkReadonly);
    hr._p.set_value("initial");
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
    CHECK(hc_dum.at<hetero_rec>(0)._p.name() == "name");
    CHECK(hc_dum.at<hetero_rec>(0)._p.category() == 2);
    CHECK(hc_dum.at<hetero_rec>(0)._p.kind() == umf::PropertyKind::pkReadonly);
    CHECK(hc_dum.at<hetero_rec>(0)._p.value() == "initial");

    std::cout << "Visit container -------------------------\n";
    hc_dum.visit<char, int, double, std::string, hetero_rec>()([](auto const & value) {
      if constexpr (std::same_as<decltype(value), hetero_rec>) {
        std::cout << value._p << " ";
      } else {
        std::cout << value << " ";
      }
      return het::VisitorReturn::vrContinue;
    });
    std::cout << " \n-------------------------\n";
  }

  umf::hetero_container hc;
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
    auto retval = umf::find_first<int>(hc, [](auto const &value) { return value == 3; });
    std::cout << "+++++++++++++++++++++ " << (retval.first ? *retval.second : -1) << "\n";
    CHECK(retval.first);
    CHECK((*retval.second == 3));
  }
  {
    umf::hetero_container hb{
        umf::property_impl<std::string>{"R0", "apple"},
        umf::property_impl<std::string>{"R1", "orange"},
        umf::property_impl<std::string>{"R2", "melon"},
        umf::property_impl<std::string>{"R3", "thistle"},
        umf::property_impl<std::string>{"R4", "trefoil"},
        umf::property_impl<std::string>{"R0", "apple"},
        umf::property_impl<std::string>{"R1", "orange"},
        umf::property_impl<std::string>{"R2", "melon"},
        umf::property_impl<std::string>{"R3", "thistle"},
        umf::property_impl<std::string>{"R4", "trefoil"},
        umf::property_impl<std::string>{"R0", "orange"},
        umf::property_impl<std::string>{"R1", "apple"},
        umf::property_impl<std::string>{"R2", "thistle"},
        umf::property_impl<std::string>{"R3", "melon"},
        umf::property_impl<std::string>{"R4", "trefoil"}
    };

//    auto retval = umf::query_first<umf::property_impl<std::string>>(hb,
//                                                                    std::pair{&umf::property_impl<std::string>::name,
//                                                                              std::string{"R0"}},
//                                                                    std::pair{&umf::property_impl<std::string>::value,
//                                                                              std::string{"orange"}}
//    );
//
//    std::cout << "+++++++++++++++++++++ " << (retval.first ? *retval.second : umf::property_impl<std::string>{}) << "\n";
//    CHECK(retval.first);
//    CHECK((*retval.second == umf::property_impl<std::string>{"R0", "orange"}));
  }
  {
    auto retval = umf::find_last<int>(hc,[](auto const& value) { return value == 1; });
    std::cout << "+++++++++++++++++++++ " << (retval.first ? *retval.second : -1) << "\n";
    CHECK(retval.first);
    CHECK((*retval.second == 1));
  }
  {
    umf::hetero_container hb;

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
    umf::find_all<int>(hb, std::back_inserter(o), [](auto const& value) { return value == 1; });
    for(auto && i : o) {
      std::cout << "+++++++++++++++++++++ " << i << "\n";
    }
    CHECK(o.size() == 6);
    CHECK(*o.cbegin() == 1);
  }

//  hc.push_back(umf::property_set{
//    umf::property_impl{"n1", 12, umf::PropertyKind::pkRegular, 0},
////    umf::property_impl{"n2", 13.f, umf::PropertyKind::pkRegular, 0},
////    umf::property_impl{"n3", std::string{"str"}, umf::PropertyKind::pkRegular, 0},
////    umf::property_impl{"n4", 14., umf::PropertyKind::pkRegular, 0}
//  });

  CHECK(!hc.empty());
  CHECK(hc.size() == 6);

  print_container(hc);

  CHECK(hc.count_of<double>() == 1);
  //std::cout << hc.at<double>(0) << "\n";
  CHECK(hc.at<double>(0) == 2.0);
  hc.erase<double>(0);
  CHECK(hc.count_of<double>() == 0);
  CHECK(hc.size() == 5);

  print_container(hc);

  nvo<float> n{100.f };
  hc.push_back(n);
  hc.push_back(120);

//  std::cout << hc.count_of<char>() << "\n";
//  std::cout << hc.count_of<int>() << "\n";
//  std::cout << hc.count_of<double>() << "\n";
//  std::cout << hc.count_of<std::string>() << "\n";
//  std::cout << hc.count_of<nvo<float>>() << "\n";
//  std::cout << hc.size() << "\n";

  CHECK(hc.count_of<char>() == 1);
  CHECK(hc.count_of<int>() == 3);
  CHECK(hc.count_of<double>() == 0);
  CHECK(hc.count_of<std::string>() == 1);
  CHECK(hc.count_of<nvo<float>>() == 1);
  CHECK(hc.size() == 7);

  print_container(hc);

  hc.fraction<int>()[0] = 2021;
  CHECK(hc.at<int>(0) == 2021);

//  CHECK_THROWS_WITH_AS((hc.get_fraction<std::pair<int, int>>()),
//                       "There are no fraction like this.", std::out_of_range);
//  WARN_THROWS_WITH_AS((hc.get_fraction<std::pair<int, int>>()), "exception: there are no fraction like this.", std::out_of_range);

  std::cout << "'int' fraction:\n";
  for(auto&& e : hc.fraction<int>()) {
    std::cout << e << "\n";
  }
}
