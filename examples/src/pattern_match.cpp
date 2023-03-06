//
// Created by yuri on 2/12/23.
//

#include "het/het.h"

#include <iostream>

using namespace std::string_view_literals;
using namespace std::string_literals;

template <typename K, typename V> struct property_root {
  property_root() {
    _properties.template push_back(std::make_tuple(K{}, V{}));
  }
  property_root(K && k, V && v) {
    _properties.template push_back(std::make_tuple(std::forward<K>(k), std::forward<V>(v)));
  }
  property_root(property_root const & p) { *this = p; }
  property_root(property_root && p) noexcept { *this = std::move(p); }

  property_root & operator=(property_root const & p) { *this = std::move(p); return *this; }
  property_root & operator=(property_root && p) noexcept {
    _name = p._name;
    _properties = p._properties;
    return *this;
  }

  [[nodiscard]] het::hvalue const & name() const { return _name; }
  [[nodiscard]] het::hvalue & name() { return _name; }

  [[nodiscard]] het::hvalue value() const {
    if(auto b = het::query_first<std::pair<K, V>>(_properties, std::pair{&std::pair<K, V>::first, _name.template value<K>()}); b.first) {
      return het::hvalue{b.second};
    }
    return {};
  }

  template <typename... Ps> requires(sizeof...(Ps) > 0) void add(Ps &&... ps) {
    (_properties.push_back(std::forward<Ps>(ps)), ...);
  }

private:
  het::hvalue _name;
  het::hvector _properties;
};

template <typename V> using property = property_root<std::string, V>;

int main() {
  property<std::string_view> psv{"prop1"s, "1"sv};
  property<int> pi{"prop2"s, 1};
  property<double> pd{"prop3"s, 3.};

  psv.add(pi, pd);
  het::hvalue n = psv.value();

  het::hvector{""sv, std::make_tuple(1, 2.f), ""s, 1.}
    .match<std::string_view/*exactly*/, std::tuple<int, float>/*exactly*/, double/*default*/, std::string/*string_view cast*/, int*/*absent*/>()(
      [](std::tuple<int, float>){std::cout << 1;},
      [](std::string_view){std::cout << 2;},
      [](auto){std::cout << "default";} // should be last
    );
  return 0;
}
