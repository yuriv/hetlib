//
// Created by yuri on 3/9/23.
//

#ifndef HETLIB_HET_VALUE_H
#define HETLIB_HET_VALUE_H

#include "details/domains.h"

#include <tuple>
#include <functional>
#include <unordered_map>

#include "details/typesafe.h"

namespace het {

using namespace metaf::util;

template <template <typename, typename, typename...> class C>
class hetero_value {
  template <typename T> static C<hetero_value const *, T> _values;
  template <typename T> static consteval C<hetero_value const *, T> & values() requires is_suitable_container<hetero_value const *, T, C> {
    return _values<T>;
  }

public:
  hetero_value() = default;
  hetero_value(hetero_value const & value) {
    assign(value);
  }
  hetero_value(hetero_value && value) noexcept {
    assign(std::move(value));
  }
  template <typename... Ts> explicit hetero_value(Ts &&... ts) {
    (add_value(std::forward<Ts>(ts)), ...);
  }

  virtual ~hetero_value() {
    clear();
  }

  hetero_value & operator=(hetero_value const & value) {
    assign(value);
    return *this;
  }

  hetero_value & operator=(hetero_value && value) noexcept {
    assign(std::move(value));
    return *this;
  }

  void assign(hetero_value const & value) {
    clear();
    _clear_functions = value._clear_functions;
    _copy_functions = value._copy_functions;
    _arity = value.arity();
    for (auto && copy : _copy_functions) {
      copy(value, *this);
    }
  }

  void assign(hetero_value && value) noexcept {
    assign(static_cast<hetero_value const *>(value));
  }

  template <typename... Ts> void assign_values(Ts const &... values) {
    clear();
    (add_value(values), ...);
  }

  template <typename... Ts> void assign_values(Ts &&... values) {
    clear();
    (add_value(std::forward<Ts>(values)), ...);
  }

  template <typename T> [[nodiscard]] bool contains() const {
    auto & vs = hetero_value::values<T>();
    return vs.find(this) != std::end(vs);
  }

  /**
   * \brief Returns const value of type T
   * \tparam T requested value type
   * \return value
   * \throw std::range_error if requested value not presented
   */
  template <typename T> [[nodiscard]] const T & value() const {
    auto & vs = hetero_value::values<T>();
    if(auto it = vs.find(this); it != std::end(vs)) {
      return it->second;
    }
    throw std::range_error(AT "Out of range");
  }

  /**
   * \brief Returns mutable value of type T
   * \tparam T requested value type
   * \return value
   * \throw std::range_error if requested value not presented
   */
  template <typename T> [[nodiscard]] T & value() {
    auto & vs = hetero_value::values<T>();
    if(auto it = vs.find(this); it != std::end(vs)) {
      return it->second;
    }
    throw std::range_error(AT "Out of range");
  }

  /**
   * \brief Returns or adds const ref to the value of type T
   * \tparam T requested value type
   * \return value value to add
   */
  template <typename T> [[nodiscard]] const T & value_or_add(T const & value = T{}) {
    return value_or_add(std::move(value));
  }

  template <typename T> [[nodiscard]] const T & value_or_add(T && value = T{}) {
    auto & vs = hetero_value::values<T>();
    if(auto it = vs.find(this); it == std::end(vs)) {
      return add_value(std::forward<T>(value))->second;
    } else {
      return it->second;
    }
  }

  template <typename T, typename... Ts> void add_values(T && value, Ts &&... values) {
    add_value(std::forward<T>(value)), (add_value(std::forward<Ts>(values)), ...);
  }

  template <typename T, typename... Ts> void modify_values(T && value, Ts &&... values) {
    add_value(std::forward<T>(value)), (..., add_value(std::forward<Ts>(values)));
  }

  template <typename T, typename... Ts>
  void erase_values() {
    erase_value<T>(), (..., erase_value<Ts>());
  }

  void clear() {
    for (auto && clear : _clear_functions) {
      clear(*this);
    }
  }

  [[nodiscard]] bool empty() const {
    return _arity == 0;
  }

  [[nodiscard]] std::size_t arity() const {
    return _arity;
  }

  template <typename T, typename... Ts>
  [[nodiscard]] constexpr auto visit() const {
    return [this]<typename F>(F && f) -> VisitorReturn {
        return (visit_single<F, T>(std::forward<F>(f)) == VisitorReturn::Break) ||
        (... || (visit_single<F, Ts>(std::forward<F>(f)) == VisitorReturn::Break)) ?
        VisitorReturn::Break : VisitorReturn::Continue;
    };
  }

  template <typename T, typename... Ts>
  [[nodiscard]] constexpr auto equal(hetero_value const & rhs) const {
    return [this, &rhs]<typename F>(F && f) -> bool {
      return equal_single(std::forward<F>(f), rhs.template value<T>()) &&
             (... && equal_single(std::forward<F>(f), rhs.template value<Ts>()));
    };
  }

  template <typename T, typename... Ts>
  [[nodiscard]] constexpr auto equal(hetero_value && rhs) const {
    return [this, &rhs]<typename F>(F && f) -> bool {
      return equal_single(std::forward<F>(f), rhs.template value<T>()) &&
             (... && equal_single(std::forward<F>(f), rhs.template value<Ts>()));
    };
  }

  template <typename T, typename... Ts>
  [[nodiscard]] constexpr auto match() const {
    return [this]<typename F, typename... Fs>(F && f, Fs &&... fs) {
      return match_single<T>(f, fs...) && (... && match_single<Ts>(f, fs...));
    };
  }

  template <typename... Ts> auto to_tuple() -> std::tuple<safe_ref<Ts>...> {
    if(!(contains<safe_ref<Ts>>() && ...)) {
      throw std::out_of_range(AT "try to access unbounded value");
    }
    return {value<safe_ref<Ts>>() ...};
  }

private:
  template <typename T, typename... Fs> constexpr bool match_single(Fs &&... fs) {
    auto f = stg::util::fn_select_applicable<T>::check(std::forward<Fs>(fs)...);
    auto & vs = hetero_value::values<T>();
    if(auto it = vs.find(this); it != std::end(vs)) {
      f(it->second);
      return true;
    }
    return false;
  }

  template<typename T, typename U> constexpr VisitorReturn visit_single(T && visitor) const {
    static_assert(std::is_invocable_v<T, U>, "predicate should accept provided type");
    auto & vs = hetero_value::values<U>();
    if(auto it = vs.find(this); it != std::end(vs)) {
      return visitor(it->second);
    }
    return VisitorReturn::Continue;
  }

  template<typename T, typename U> constexpr bool equal_single(T && cmp, U const & arg) const {
    static_assert(std::is_invocable_v<T, U, U>, "predicate should accept provided type");
    auto & vs = hetero_value::values<U>();
    if(auto it = vs.find(this); it != std::end(vs)) {
      return cmp(it->second, arg);
    }
    return false;
  }

  template <typename T> auto add_value(T const & value) -> typename C<hetero_value const *, T>::iterator {
    return add_value(std::move(value));
  }

  template <typename T> auto add_value(T & value) -> typename C<hetero_value const *, T>::iterator {
    return add_value(std::move(value));
  }

  template <typename T> auto add_value(T && value) -> typename C<hetero_value const *, T>::iterator {
    register_operations<T>();
    auto & vs = hetero_value::values<T>();
    return vs.insert_or_assign(this, std::forward<T>(value)).first;
  }

  template <typename T> void erase_value() {
    if(contains<T>()) {
      hetero_value::values<T>().erase(this);
      _arity--;
    }
  }

  template<typename T> auto register_operations() -> typename C<hetero_value const *, T>::iterator {
    auto & vs = hetero_value::values<T>();
    auto it = vs.find(this);
    // don't have it yet, so create functions for copying and destroying
    if (it == std::end(vs)) {
      _clear_functions.template emplace_back([](hetero_value & c) { hetero_value::values<T>().erase(&c); });
      // if someone copies me, they need to call each copy_function and pass themself
      _copy_functions.template emplace_back([](const hetero_value & from, hetero_value & to) {
        if(&from != &to) {
          auto & vs = hetero_value::values<T>();
          if constexpr(std::is_copy_constructible_v<T>) {
            vs.insert_or_assign(&to, from.template value<T>());
          } else {
            vs.insert_or_assign(&to, std::move(const_cast<hetero_value&>(from).template value<T>()));
          }
        }
      });
      _arity++;
    }
    return it;
  }

  std::size_t _arity{0};
  std::vector<std::function<void(hetero_value &)>> _clear_functions;
  std::vector<std::function<void(const hetero_value &, hetero_value &)>> _copy_functions;
};

template <template <typename, typename, typename...> typename C>
template <typename T> C<hetero_value<C> const *, T> hetero_value<C>::_values;

template <typename T, template <typename...> typename C>
constexpr T & get(het::hetero_value<C> & hv) requires (!std::is_void_v<T>) {
  return hv.template value<T>();
}

template <typename T, template <typename...> typename C>
constexpr T && get(het::hetero_value<C> && hv) requires (!std::is_void_v<T>) {
  return std::move(hv.template value<T>());
}

template <typename T, template <typename...> typename C>
constexpr T const & get(het::hetero_value<C> const & hv) requires (!std::is_void_v<T>) {
  return hv.template value<T>();
}

template <typename T, template <typename...> typename C>
constexpr T const && get(het::hetero_value<C> const && hv) requires (!std::is_void_v<T>) {
  return std::move(hv.template value<T>());
}

template <typename T, template <typename...> typename C>
constexpr T const * get_if(het::hetero_value<C> const & hv) noexcept requires (!std::is_void_v<T>) {
  static_assert(!std::is_void_v<T>, "T must not be void");
  return hv.template contains<T>() ? std::addressof(get<T>(hv)) : nullptr;
}

template <typename T, template <typename...> typename C>
constexpr T * get_if(het::hetero_value<C> & hv) noexcept requires (!std::is_void_v<T>) {
  return hv.template contains<T>() ? std::addressof(get<T>(hv)) : nullptr;
}

template <typename T, template <typename...> typename C>
constexpr T * get_if(het::hetero_value<C> && hv) noexcept requires (!std::is_void_v<T>) {
  return hv.template contains<T>() ? std::addressof(get<T>(hv)) : nullptr;
}

template <typename... Ts, template <typename...> class C>
auto to_tuple(hetero_value<C> const & hv) -> std::tuple<safe_ref<Ts>...> {
  return to_tuple<Ts...>(std::move(hv));
}

template <typename... Ts, template <typename...> class C>
auto to_tuple(hetero_value<C> & hv) -> std::tuple<safe_ref<Ts>...> {
  return to_tuple<Ts...>(std::move(hv));
}

template <typename... Ts, template <typename...> class C>
auto to_tuple(hetero_value<C> && hv) -> std::tuple<safe_ref<Ts>...> {
  return hv.template to_tuple<Ts...>();
}

using hvalue = hetero_value<std::unordered_map>;

} // namespace het

#endif //HETLIB_HET_VALUE_H
