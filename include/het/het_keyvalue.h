//
// Created by yuri on 3/9/23.
//

#ifndef HETLIB_HET_KEYVALUE_H
#define HETLIB_HET_KEYVALUE_H

#include "details/domains.h"

#include <tuple>
#include <functional>
#include <unordered_map>

#include "details/typesafe.h"

namespace het {

using namespace metaf::util;

template <template <typename, typename, typename...> class C>
class hetero_key_value {
  template <typename K, typename T> static C<hetero_key_value const *, C<K, T>> _values;
  template <typename K, typename T> requires is_suitable_container<hetero_key_value const *, T, C> && is_suitable_container<K, T, C>
  static consteval C<hetero_key_value const *, C<K, T>> & values() {
    return _values<K, T>;
  }

public:
  hetero_key_value() = default;
  hetero_key_value(hetero_key_value const & value) {
    assign(value);
  }
  hetero_key_value(hetero_key_value && value) noexcept {
    assign(std::move(value));
  }
  template <typename... Ts> explicit hetero_key_value(Ts &&... ts) {
    (add_value(std::forward<Ts>(ts)), ...);
  }

  virtual ~hetero_key_value() {
    clear();
  }

  hetero_key_value & operator=(hetero_key_value const & value) {
    assign(value);
    return *this;
  }

  hetero_key_value & operator=(hetero_key_value && value) noexcept {
    assign(std::move(value));
    return *this;
  }

  void assign(hetero_key_value const & value) {
    clear();
    _clear_functions = value._clear_functions;
    _copy_functions = value._copy_functions;
    for (auto && copy : _copy_functions) {
      copy(value, *this);
    }
  }

  void assign(hetero_key_value && value) noexcept {
    assign(static_cast<hetero_key_value const *>(value));
  }

  template <typename... Ts> void assign_values(Ts const &... values) {
    clear();
    (add_value(values), ...);
  }

  template <typename... Ts> void assign_values(Ts &&... values) {
    clear();
    (add_value(std::forward<Ts>(values)), ...);
  }

  template <typename T, typename K> [[nodiscard]] constexpr bool contains() const {
    return hetero_key_value::values<K, T>().contains(this);
  }

  template <typename T, typename K> [[nodiscard]] bool contains(K && key) const {
    auto & vs = hetero_key_value::values<K, T>();
    if(auto it = vs.find(this); it != std::cend(vs)) {
      return it->second.contains(std::forward<K>(key));
    }
    return false;
  }

  /**
   * \brief Returns const value of type T
   * \tparam T requested value type
   * \return value
   * \throw std::range_error if requested value not presented
   */
  template <typename T, typename K> [[nodiscard]] const T & value(K && key) const {
    auto & vs = hetero_key_value::values<K, T>();
    if(auto it = vs.find(this); it != std::end(vs)) {
      if(auto it2 = it->second.find(std::forward<K>(key)); it2 != std::end(it->second)) {
        return it2->second;
      }
    }
    throw std::range_error(AT "Out of range");
  }

  /**
   * \brief Returns mutable value of type T
   * \tparam T requested value type
   * \return value
   * \throw std::range_error if requested value not presented
   */
  template <typename T, typename K> [[nodiscard]] T & value(K && key) {
    auto & vs = hetero_key_value::values<K, T>();
    if(auto it = vs.find(this); it != std::end(vs)) {
      if(auto it2 = it->second.find(std::forward<K>(key)); it2 != std::end(it->second)) {
        return it2->second;
      }
    }
    throw std::range_error(AT "Out of range");
  }

  /**
   * \brief Returns or adds const ref to the value of type T
   * \tparam T requested value type
   * \return value value to add
   */
  template <typename K, typename T> [[nodiscard]] const T & value_or_add(K && key, T const & value = T{}) {
    return value_or_add(std::forward<K>(key), std::move(value));
  }

  template <typename K, typename T> [[nodiscard]] const T & value_or_add(K && key, T && value = T{}) {
    auto & vs = hetero_key_value::values<K, T>();
    if(auto it = vs.find(this); it == std::end(vs)) {
      return add_value(std::forward<K>(key), std::forward<T>(value))->second;
    } else if(auto it2 = it->second.find(std::forward<K>(key)); it2 == std::end(it->second)) {
      return add_value(std::forward<K>(key), std::forward<T>(value))->second;
    } else {
      return it2->second;
    }
  }

  template <typename... Ts> requires (sizeof...(Ts) > 0) void add_values(Ts &&... values) {
    (add_value(std::forward<Ts>(values)), ...);
  }

  template <typename... Ts> requires (sizeof...(Ts) > 0) void modify_values(Ts &&... values) {
    (add_value(std::forward<Ts>(values)), ...);
  }

  template <typename T, typename K> bool erase_value(K && key) {
    auto & vs = hetero_key_value::values<K, T>();
    if(auto c = vs.find(this); c != std::end(vs)) {
      return c->second.erase(std::forward<K>(key));
    }
    return false;
  }

  void clear() {
    for (auto && clear_function : _clear_functions) {
      clear_function(*this);
    }
  }

  [[nodiscard]] bool empty() const {
    return size() == 0;
  }

  [[nodiscard]] std::size_t size() const {
    std::size_t sz = 0;
    for (auto && size_function : _size_functions) {
      sz += size_function(*this);
    }
    return sz;
  }

  template <typename K, typename... Ts> requires (sizeof...(Ts) > 0)
  [[nodiscard]] constexpr auto visit() const {
    return [this]<typename F>(F && f) -> VisitorReturn {
      return ((visit_single<F, K, Ts>(std::forward<F>(f)) == VisitorReturn::Break) || ...) ?
        VisitorReturn::Break : VisitorReturn::Continue;
    };
  }

  template <typename K, typename... Ts> requires (sizeof...(Ts) > 0)
  [[nodiscard]] constexpr auto match() const {
    return [this]<typename F, typename... Fs>(F && f, Fs &&... fs) {
      return (match_single<K, Ts>(f, fs...) && ...);
    };
  }

  template <typename... Ts, typename K> requires (sizeof...(Ts) > 0)
  auto to_tuple(K && key) const -> std::tuple<safe_ref<Ts>...> {
    if(!(contains<safe_ref<Ts>>(std::forward<K>(key)) && ...)) {
      throw std::out_of_range(AT "try to access unbounded value");
    }
    return {value<safe_ref<Ts>>(std::forward<K>(key)) ...};
  }

  template <typename T, typename... Ks> requires (sizeof...(Ks) > 0)
  auto to_vector(Ks &&... keys) const -> std::vector<safe_ref<T>> {
    if(!(contains<safe_ref<T>>(std::forward<Ks>(keys)) && ...)) {
      throw std::out_of_range(AT "try to access unbounded value");
    }
    return {value<safe_ref<T>>(std::forward<Ks>(keys)) ...};
  }

private:
  template <typename K, typename T, typename... Fs> constexpr bool match_single(Fs &&... fs) const {
    auto f = stg::util::fn_select_applicable<K, T>::check(std::forward<Fs>(fs)...);
    auto & vs = hetero_key_value::values<K, T>();
    if(auto it = vs.find(this); it != std::end(vs)) {
      for(auto && kv : it->second) {
        f(kv.first, kv.second);
      }
      return true;
    }
    return false;
  }

  template<typename T, typename K, typename U> constexpr VisitorReturn visit_single(T && visitor) const {
    static_assert(std::is_invocable_v<T, K, U>, "predicate should accept provided type");
    auto & vs = hetero_key_value::values<K, U>();
    if(auto it = vs.find(this); it != std::end(vs)) {
      for(auto && kv : it->second) {
        if(std::forward<T>(visitor)(kv.first, kv.second) == VisitorReturn::Break) {
          return VisitorReturn::Break;
        }
      }
    }
    return VisitorReturn::Continue;
  }

  template <typename T> auto add_value(T && value) -> typename C<typename T::first_type, typename T::second_type>::iterator {
    return add_value(std::forward<T>(value).first, std::move(std::forward<T>(value).second)); // typename C<K, T>::iterator
  }

  template <typename K, typename T> auto add_value(K && key, T const & value) -> typename C<K, T>::iterator {
    return add_value(std::forward<K>(key), std::move(value));
  }

  template <typename K, typename T> auto add_value(K && key, T & value) -> typename C<K, T>::iterator {
    return add_value(std::forward<K>(key), std::move(value));
  }

  template <typename K, typename T> auto add_value(K && key, T && value) -> typename C<K, T>::iterator {
    register_operations<K, T>(); // ensures new values<K,T> functions added
    auto & vs = hetero_key_value::values<K, T>();
    return vs[this].insert_or_assign(std::forward<K>(key), std::forward<T>(value)).first; // ensures new values<K,T> entry added
  }

  template<typename K, typename T> auto register_operations() -> typename C<hetero_key_value const *, C<K, T>>::iterator {
    auto & vs = hetero_key_value::values<K, T>();
    // don't have it yet, so create functions for copying and destroying
    auto it = vs.find(this);
    if (it == std::end(vs)) {
      _clear_functions.template emplace_back([](hetero_key_value & c) { hetero_key_value::values<K, T>().erase(&c); });
      // if someone copies me, they need to call each copy_function and pass themself
      _copy_functions.template emplace_back([](const hetero_key_value & from, hetero_key_value & to) {
        if(&from != &to) {
          auto & vs = hetero_key_value::values<K, T>();
          if constexpr(std::is_copy_constructible_v<T>) {
            vs.insert_or_assign(&to, hetero_key_value::template values<K, T>().at(&from));
          } else {
            vs.insert_or_assign(&to, std::move(hetero_key_value::template values<K, T>().at(&const_cast<hetero_key_value&>(from))));
          }
        }
      });
      _size_functions.emplace_back([](const hetero_key_value & c) { return hetero_key_value::values<K, T>().at(&c).size(); });
    }
    return it;
  }

  std::vector<std::function<void(hetero_key_value &)>> _clear_functions;
  std::vector<std::function<void(const hetero_key_value &, hetero_key_value &)>> _copy_functions;
  std::vector<std::function<size_t(const hetero_key_value &)>> _size_functions;
};

template <template <typename, typename, typename...> typename C>
template <typename K, typename T> C<hetero_key_value<C> const *, C<K, T>> hetero_key_value<C>::_values;

template <typename... Ts, typename K, template <typename...> class C> requires (sizeof...(Ts) > 0)
auto to_tuple(hetero_key_value<C> const & hkv, K && key) -> std::tuple<safe_ref<Ts>...> {
  return to_tuple<Ts...>(move(hkv), std::forward<K>(key));
}

template <typename... Ts, typename K, template <typename...> class C> requires (sizeof...(Ts) > 0)
auto to_tuple(hetero_key_value<C> & hkv, K && key) -> std::tuple<safe_ref<Ts>...> {
  return to_tuple<Ts...>(std::move(hkv), std::forward<K>(key));
}

template <typename... Ts, typename K, template <typename...> class C> requires (sizeof...(Ts) > 0)
auto to_tuple(hetero_key_value<C> && hkv, K && key) -> std::tuple<safe_ref<Ts>...> {
  return hkv.template to_tuple<Ts...>(std::forward<K>(key));
}

template <typename T, template <typename...> class C, typename... Ks> requires (sizeof...(Ks) > 0)
auto to_vector(hetero_key_value<C> const & hkv, Ks &&... keys) -> std::vector<safe_ref<T>> {
  return to_vector<T>(std::move(hkv), std::forward<Ks>(keys)...);
}

template <typename T, template <typename...> class C, typename... Ks> requires (sizeof...(Ks) > 0)
auto to_vector(hetero_key_value<C> & hkv, Ks &&... keys) -> std::vector<safe_ref<T>> {
  return to_vector<T>(std::move(hkv), std::forward<Ks>(keys)...);
}

template <typename T, template <typename...> class C, typename... Ks> requires (sizeof...(Ks) > 0)
auto to_vector(hetero_key_value<C> && hkv, Ks &&... keys) -> std::vector<safe_ref<T>> {
  return hkv.template to_vector<T>(std::forward<Ks>(keys)...);
}

using hkeyvalue = hetero_key_value<std::unordered_map>;

} // namespace het

#endif //HETLIB_HET_KEYVALUE_H
