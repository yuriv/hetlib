//
// Created by yuri on 3/9/23.
//

#ifndef HETLIB_DOMAINS_H
#define HETLIB_DOMAINS_H

#include "uncommon_definitions.h"
#include "match.h"

namespace het {

enum class VisitorReturn { Break, Continue };

template <typename T> concept projection_clause = requires(T t) {
  t.first;
  t.second;
  std::convertible_to<std::decay_t<decltype(t.first)>, std::decay_t<decltype(t.second)>>;
  std::invocable<std::decay_t<decltype(t.first)>>;
  std::equality_comparable<std::decay_t<decltype(t.second)>>;
};

template <template <typename, typename, typename...> class C>
concept is_assoc_container = requires {
  typename C<int, int>::key_type;
};

template <typename K, typename V, template <typename, typename, typename...> class C>
concept is_suitable_container = requires(C<K, V> c) {
  typename C<K, V>::key_type;
  typename C<K, V>::iterator;
  { std::begin(c) } -> std::convertible_to<typename C<K, V>::iterator>;
  { std::end(c) } -> std::convertible_to<typename C<K, V>::iterator>;
  { c.find(std::declval<K>()) } -> std::convertible_to<typename C<K, V>::iterator>;
  { c.contains(std::declval<K>()) } -> std::convertible_to<bool>;
  { c.insert_or_assign(std::declval<K>(), std::declval<V>()) } -> std::convertible_to<std::pair<typename C<K, V>::iterator, bool>>;
};

} // namespace het

#endif //HETLIB_DOMAINS_H
