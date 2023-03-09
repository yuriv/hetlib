//
// Created by yuri on 10/1/21.
//

#ifndef HETLIB_HET_H
#define HETLIB_HET_H

#include "uncommon_definitions.h"
#include "match.h"

#include <deque>
#include <vector>
#include <functional>

#include <experimental/type_traits>

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
  { c.insert_or_assign(std::declval<K>(), std::declval<V>()) } -> std::convertible_to<std::pair<typename C<K, V>::iterator, bool>>;
};

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

template <template <typename...> class InnerC, template <typename, typename, typename...> class OuterC> requires is_assoc_container<OuterC>
class hetero_container {
  template<typename T> static OuterC<hetero_container const *, InnerC<T>> _items;
  template <typename T> static consteval OuterC<hetero_container const *, InnerC<T>> & items()
    requires is_suitable_container<hetero_container const *, InnerC<T>, OuterC> {
    return _items<T>;
  }

public:
//  template <typename T> using iterator = typename OuterC<hetero_container const *, InnerC<T>>::iterator;
//  template <typename T> using const_iterator = typename OuterC<hetero_container const *, InnerC<T>>::const_iterator;
//  template <typename T> using reverse_iterator = typename OuterC<hetero_container const *, InnerC<T>>::reverse_iterator;
//  template <typename T> using const_reverse_iterator = typename OuterC<hetero_container const *, InnerC<T>>::const_reverse_iterator;

  template <typename T> using inner_iterator = typename InnerC<T>::iterator;
//  template <typename T> using inner_const_iterator = typename InnerC<T>::const_iterator;
//  template <typename T> using inner_reverse_iterator = typename InnerC<T>::reverse_iterator;
//  template <typename T> using inner_const_reverse_iterator = typename InnerC<T>::const_reverse_iterator;

  hetero_container() = default;
  hetero_container(hetero_container const & other) {
    assign(other);
  }

  hetero_container(hetero_container && other)  noexcept {
    assign(std::move(other));
  }

  template <typename... Ts> explicit hetero_container(Ts const &... ts) {
    (..., push_back(ts));
  }

  template <typename... Ts> explicit hetero_container(Ts &&... ts) {
    (..., push_back(std::move(ts)));
  }

  virtual ~hetero_container() {
    clear();
  }

  hetero_container & operator=(hetero_container const & other) {
    assign(other);
    return *this;
  }

  hetero_container & operator=(hetero_container && other) noexcept {
    assign(std::move(other));
    return *this;
  }

  void assign(hetero_container const & other) {
    clear();
    _clear_functions = other._clear_functions;
    _copy_functions = other._copy_functions;
    _size_functions = other._size_functions;
    _empty_functions = other._empty_functions;
    for (auto && copy_function : _copy_functions) {
      copy_function(other, *this);
    }
  }

  void assign(hetero_container && other) noexcept {
    assign(static_cast<hetero_container const &>(other));
  }
  /**
   * @brief Inserts new element t before position pos
   * @tparam T -- type bucket
   * @param pos -- position to insert
   * @param t -- element to insert
   * @return iterator of the element
   */
  template<typename T> inner_iterator<T> insert(inner_iterator<T> pos, const T & t) {
    // inserts new key or access existing
    return insert(pos, std::move(t));
  }

  template<typename T> inner_iterator<T> insert(inner_iterator<T> pos, T && t) {
    // inserts new key or access existing
    register_operations<T>();
    auto & c = hetero_container::items<T>()[this];
    return c.insert(pos, std::forward<T>(t));
  }

  template <typename T, typename... Args> T & emplace(inner_iterator<T> pos, Args &&... args) {
    // inserts new key or access existing
    register_operations<T>();
    auto & c = hetero_container::items<T>()[this];
    return c.emplace(pos, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args> T & emplace_front(Args &&... args) {
    // inserts new key or access existing
    register_operations<T>();
    auto & c = hetero_container::items<T>()[this];
    return c.emplace(c.begin(), std::forward<Args>(args)...);
  }

  template <typename T, typename... Args> T & emplace_back(Args &&... args) {
    // inserts new key or access existing
    register_operations<T>();
    auto & c = hetero_container::items<T>()[this];
    return c.emplace_back(std::forward<Args>(args)...);
  }

  template<typename... Ts> requires (sizeof...(Ts) > 0) void push_front(Ts const &... ts) {
    push_front(std::move(ts)...);
  }

  template<typename... Ts> requires (sizeof...(Ts) > 0) void push_front(Ts &... ts) {
    push_front(std::move(ts)...);
  }

  template<typename... Ts> requires (sizeof...(Ts) > 0) void push_front(Ts &&... ts) {
    (push_front_single(std::forward<Ts>(ts)), ...);
  }

  template<typename... Ts> requires (sizeof...(Ts) > 0) void push_back(Ts const &... ts) {
    push_back(std::move(ts)...);
  }

  template<typename... Ts> requires (sizeof...(Ts) > 0) void push_back(Ts &... ts) {
    push_back(std::move(ts)...);
  }

  template<typename... Ts> requires (sizeof...(Ts) > 0) void push_back(Ts &&... ts) {
    (push_back_single(std::forward<Ts>(ts)), ...);
  }

  template<typename T> void pop_front() {
    auto & c = fraction<T>();
    c.erase(c.begin());
  }

  template<typename T> void pop_back() {
    auto & c = fraction<T>();
    c.erase(std::prev(c.end()));
  }

  template<std::equality_comparable T> void erase(std::size_t index) {
    auto & c = fraction<T>();
    c.erase(std::begin(c) + index);
    if(c.empty()) {
      // TODO: remove dependent *_functions
    }
  }

  template<std::equality_comparable T> bool erase(T const & e) {
    auto & c = fraction<T>();
    bool erased = false;
    for(auto it = c.begin(); it != c.end(); ++it) {
      if(*it == e) {
        c.erase(it);
        erased = true;
        break;
      }
    }
    if(c.empty()) {
      // TODO: remove dependent *_functions
    }
    return erased;
  }

  template<typename T> T & at(std::size_t index) {
    return fraction<T>().at(index);
  }

  template<typename T> T const & at(std::size_t index) const {
    return fraction<T>().at(index);
  }

  void clear() {
    for (auto && clear_function : _clear_functions) {
      clear_function(*this);
    }
  }

  [[nodiscard]] bool empty() const {
    for (auto && empty_function : _empty_functions) {
      if(!empty_function(*this)) {
        return false;
      }
    }
    return true;
  }

  template<typename T> [[nodiscard]] size_t count_of() const {
    if (contains<T>()) {
      return fraction<T>().size();
    }
    return 0;
  }

  [[nodiscard]] size_t size() const {
    auto safe_int_add = [](std::size_t op1, std::size_t op2) -> std::size_t {
      if (op2 > std::numeric_limits<std::size_t>::max() - op1) {
        throw std::overflow_error("Adding overflows");
      }
      return op1 + op2;
    };
    size_t sum = 0;
    for (auto && size_function : _size_functions) {
      sum = safe_int_add(sum, size_function(*this));
    }
    return sum;
  }

  template <typename T> auto fraction() -> InnerC<T> & {
    return hetero_container::items<T>().at(this);
  }

  template <typename T> auto fraction() const -> InnerC<T> const & {
    return hetero_container::items<T>().at(this);
  }

  template <typename T> [[nodiscard]] constexpr bool contains() const {
    auto & items = hetero_container::items<T>();
    return items.find(this) != std::cend(items);
  }

  template <std::equality_comparable T, projection_clause... Clauses> requires(sizeof...(Clauses) > 0)
  [[nodiscard]] constexpr bool contains(Clauses &&... clauses) const {
    auto & items = hetero_container::items<T>();
    if(auto it = items.find(this); it != std::cend(items)) {
      auto cmp = [&f = it->second]<typename C>(C && c) -> bool {
        return std::find_if(std::cbegin(f), std::cend(f), [&c](T && value) {
          return std::invoke(c.first, value) == c.second;
        }) != std::cend(f);
      };
      return (... || cmp(std::forward<Clauses>(clauses)));
    }
    return false;
  }

  template <typename T, projection_clause Clause> auto find(Clause const & clause) const -> std::pair<bool, inner_iterator<T>> {
    return find<T>(std::move(clause));
  }

  template <typename T, projection_clause Clause> auto find(Clause && clause) const -> std::pair<bool, inner_iterator<T>> {
    auto & items = hetero_container::items<T>();
    if(auto it = items.find(this); it != std::cend(items)) {
      auto found = std::find_if(std::begin(it->second), std::end(it->second),
        [clause](T const & that) {
          return std::invoke(clause.first, that) == clause.second;
      });
      if(found != std::end(it->second)) {
        return std::make_pair(true, found);
      }
    }
    return std::make_pair(false, typename InnerC<T>::iterator{});
  }

  /**
   * \brief Generates elements accessor for T, Ts... types by predicate F
   * \tparam T type to proceed
   * \tparam Ts types to proceed
   * \return function which applies predicate F on elements of specified types
   * \attention it captures `this` pointer, watch for the object lifetime
   */
  template <typename T, typename... Ts>
  [[nodiscard]] auto visit() const {
    return [this]<typename F>(F && f) -> VisitorReturn {
      return (visit_single<F, T>(std::forward<F>(f)) == VisitorReturn::Break) ||
        (... || (visit_single<F, Ts>(std::forward<F>(f)) == VisitorReturn::Break)) ?
          VisitorReturn::Break : VisitorReturn::Continue;
    };
  }

  /**
   * \brief Generates elements accessor for T, Ts... types by predicate F
   * \tparam T type to proceed
   * \tparam Ts types to proceed
   * \return function which applies predicate F on elements of specified types
   * \attention it captures `this` pointer, watch for the object lifetime
   */
  template <typename T, typename... Ts>
  [[nodiscard]] auto match() const {
    return [this]<typename F, typename... Fs>(F && f, Fs &&... fs) -> bool {
      return match_single<T>(f, fs...) && (... && match_single<Ts>(f, fs...));
    };
  }

  // Find/Query accessors
  template<typename T, template <typename...> class IC, template <typename...> class OC, std::invocable<T> F>
  friend constexpr auto find_first(hetero_container<IC, OC> const & hc, F && f) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;
  template<typename T, template <typename...> class IC, template <typename...> class OC, std::invocable<T> F>
  friend constexpr auto find_next(hetero_container<IC, OC> const & hc, typename hetero_container<IC, OC>::template inner_iterator<T> pos, F && f) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;

  template<typename... Ts, template <typename...> class IC, template <typename...> class OC, std::invocable<Ts...> F>
  friend constexpr auto find_first(hetero_container<IC, OC> const & hc, F && f) ->
    std::pair<bool, std::tuple<typename hetero_container<IC, OC>::template inner_iterator<Ts>...>>;

  template<typename T, template <typename...> class IC, template <typename...> class OC, std::invocable<T> F>
  friend constexpr auto find_next(hetero_container<IC, OC> const & hc, typename hetero_container<IC, OC>::template inner_iterator<T> pos, F && f) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;

  template<typename T, template <typename...> class IC, template <typename...> class OC, std::invocable<T> F>
  friend constexpr auto find_last(hetero_container<IC, OC> const & hc, F && f) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;
  template<typename T, template <typename...> class IC, template <typename...> class OC, std::invocable<T> F>
  friend constexpr auto find_prev(hetero_container<IC, OC> const & hc, typename hetero_container<IC, OC>::template inner_iterator<T> pos, F && f) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;

  template<typename T, template <typename...> class IC, template <typename...> class OC, std::output_iterator<T> O, std::invocable<T> F>
  friend constexpr bool find_all(hetero_container<IC, OC> const & hc, O output, F && f);

  template<typename T, template <typename...> class IC, template <typename...> class OC, projection_clause Clause, projection_clause... Clauses>
  friend constexpr auto query_first(hetero_container<IC, OC> const & hc, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;

  template<typename T, template <typename...> class IC, template <typename...> class OC, typename F, projection_clause Clause, projection_clause... Clauses>
    requires std::conjunction_v<std::is_invocable<F, Clause, T>, std::is_invocable<F, Clauses, T>...>
  friend constexpr auto query_first_if(hetero_container<IC, OC> const & hc, F && f, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;

  template<typename T, template <typename...> class IC, template <typename...> class OC, projection_clause Clause, projection_clause... Clauses>
  friend constexpr auto query_last(hetero_container<IC, OC> const & hc, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;

  template<typename T, template <typename...> class IC, template <typename...> class OC, typename F, projection_clause Clause, projection_clause... Clauses>
    requires std::conjunction_v<std::is_invocable<F, Clause, T>, std::is_invocable<F, Clauses, T>...>
  friend constexpr auto query_last_if(hetero_container<IC, OC> const & hc, F && f, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;

private:
  // inserts new key or access existing
  template<typename T> auto push_front_single(const T & t) -> hetero_container::inner_iterator<T> {
    return push_front_single(std::move(t));
  }

  // inserts new key or access existing
  template<typename T> auto push_front_single(T & t) -> hetero_container::inner_iterator<T> {
    return push_front_single(std::move(t));
  }

  // inserts new key or access existing
  template<typename T> auto push_front_single(T && t) -> hetero_container::inner_iterator<T> {
    register_operations<T>();
    auto & c = hetero_container::items<T>()[this];
    return c.insert(std::begin(c), std::forward<T>(t));
  }

  template<typename T> auto push_back_single(const T & t) -> hetero_container::inner_iterator<T> {
    return push_back_single(std::move(t));
  }

  template<typename T> auto push_back_single(T & t) -> hetero_container::inner_iterator<T> {
    return push_back_single(std::move(t));
  }

  // inserts new key or access existing
  template<typename T> auto push_back_single(T && t) -> hetero_container::inner_iterator<T> {
    register_operations<T>();
    auto & c = hetero_container::items<T>()[this];
    return c.insert(std::end(c), std::forward<T>(t));
  }

  template<typename T, typename U> auto visit_single(T const & visitor) const -> VisitorReturn {
    return visit_single<T, U>(std::move(visitor));
  }

  template<typename T, typename U> auto visit_single(T && visitor) const -> VisitorReturn {
    static_assert(std::is_invocable_v<T, U>, "predicate should accept provided type");
    auto & items = hetero_container::items<U>();
    if(auto iter = items.find(this); iter != std::cend(items)) {
      for(auto & c : iter->second) {
        if(visitor(c) == VisitorReturn::Break) {
          return VisitorReturn::Break;
        }
      }
    }
    return VisitorReturn::Continue;
  }

  template <typename T, typename... Fs> constexpr bool match_single(Fs &&... fs) const {
    auto f = stg::util::fn_select_applicable<T>::check(std::forward<Fs>(fs)...);
    auto & items = hetero_container::items<T>();
    if(auto iter = items.find(this); iter != std::cend(items)) {
      for(auto & c : iter->second) {
        f(c);
      }
      return true;
    }
    return false;
  }

  template<typename T> void register_operations() {
    // don't have it yet, so create functions for copying, moving, destroying, etc
    if(!contains<T>()) {
      _clear_functions.emplace_back([](hetero_container & c) { hetero_container::items<T>().erase(&c); });
      // if someone copies me, they need to call each copy_function and pass themself
      _copy_functions.emplace_back([](const hetero_container & from, hetero_container & to) {
        auto & items = hetero_container::items<T>();
        if(&to != &from) {
          if constexpr(std::is_copy_constructible_v<T>) {
            items[&to] = items[&from];
          } else {
            items[&to] = std::move(items[&from]);
          }
        }
      });
      _size_functions.emplace_back([](const hetero_container & c) { return hetero_container::items<T>().at(&c).size(); });
      _empty_functions.emplace_back([](const hetero_container & c) { return hetero_container::items<T>().at(&c).empty(); });
    }
  }

  std::vector<std::function<void(hetero_container&)>> _clear_functions;
  std::vector<std::function<void(const hetero_container&, hetero_container&)>> _copy_functions;
  std::vector<std::function<size_t(const hetero_container&)>> _size_functions;
  std::vector<std::function<bool(const hetero_container&)>> _empty_functions;
};

template <template <typename...> class InnerC, template <typename, typename, typename...> class OuterC> requires is_assoc_container<OuterC>
template<typename T>
OuterC<hetero_container<InnerC, OuterC> const *, InnerC<T>> hetero_container<InnerC, OuterC>::_items;

/**
 * \brief Finds first occurrence of the element of the type T matched against predicate F
 * \tparam T type of the element to search
 * \tparam F type of the match predicate
 * \param hc container to search
 * \param f match predicate
 * \return pair of presence flag and iterator (true and
 *          valid iterator points to the element and false and invalid iterator otherwise)
 */
template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, std::invocable<T> F>
constexpr auto find_first(hetero_container<InnerC, OuterC> const & hc, F && f) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>> {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if(iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    return find_next<T>(hc, iter->second.begin(), f);
  }
  return std::pair{false, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>{}};
}

template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, std::invocable<T> F>
constexpr auto find_next(hetero_container<InnerC, OuterC> const & hc, typename hetero_container<InnerC, OuterC>::template inner_iterator<T> pos, F && f) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>> {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if(iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    auto found = std::find_if(std::next(pos), iter->second.end(), std::forward<F>(f));
    if(found != iter->second.end()) {
      return std::pair{true, found};
    }
  }
  return std::pair{false, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>{}};
}

/**
 * \brief
 * \tparam T
 * \tparam InnerC
 * \tparam OuterC
 * \tparam F
 * \param hc
 * \param f
 * \return
 */
template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, std::invocable<T> F>
constexpr auto find_last(hetero_container<InnerC, OuterC> const & hc, F && f) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>> {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if (iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    return find_prev<T>(hc, iter->second.end(), f);
  }
  return std::pair{false, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>{}};
}

template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, std::invocable<T> F>
constexpr auto find_prev(hetero_container<InnerC, OuterC> const & hc, typename hetero_container<InnerC, OuterC>::template inner_iterator<T> pos, F && f) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>> {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if (iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    if(iter->second.end() != iter->second.begin()) {
      for(auto e = std::prev(pos); e != iter->second.begin(); --e) {
        if(std::forward<F>(f)(*e)) {
          return std::pair{true, e};
        }
      }
    }
  }
  return std::pair{false, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>{}};
}

/**
 * \brief Finds all elements of the type T matched to predicate F
 * \tparam T type of the element to search
 * \tparam F type of the match predicate
 * \param hc container to search
 * \param output container to place result set
 * \param f match predicate
 * \return false if result set is empty, true otherwise
 */
template <typename T, template <typename...> class InnerC, template <typename...> class OuterC, std::output_iterator<T> O, std::invocable<T> F>
constexpr bool find_all(hetero_container<InnerC, OuterC> const & hc, O output, F && f) {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if(iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    auto ce = iter->second.cend();
    std::size_t copied = 0;
    for(auto cb = iter->second.cbegin(); cb != ce; ++cb) {
      if(std::forward<F>(f)(*cb)) {
        output = *cb;
        output++;
        copied++;
      }
    }
    return copied > 0;
  }
  return false;
}

/**
 * \brief
 * \tparam T
 * \tparam Clause
 * \tparam Clauses
 * \param hc
 * \param clause
 * \param clauses
 * \return
 */
template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, projection_clause Clause, projection_clause... Clauses>
constexpr auto query_first(hetero_container<InnerC, OuterC> const & hc, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>> {
  return query_first_if<T>(hc, []<typename C, typename Obj>(C && c, Obj const & obj) {
          return std::invoke(c.first, obj) == c.second; //  simplest case is obj.prj == arg
        }, std::forward<Clause>(clause), std::forward<Clauses>(clauses)...);
}

/**
 * \brief
 * \tparam T
 * \tparam F
 * \tparam Clause
 * \tparam Clauses
 * \param hc
 * \param f
 * \param clause
 * \param clauses
 * \return
 */
template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, typename F, projection_clause Clause, projection_clause... Clauses>
  requires std::conjunction_v<std::is_invocable<F, Clause, T>, std::is_invocable<F, Clauses, T>...>
constexpr auto query_first_if(hetero_container<InnerC, OuterC> const & hc, F && f, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>> {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if (iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    for(auto it = iter->second.begin(); it != iter->second.end(); ++it) {
      if(std::forward<F>(f)(std::forward<Clause>(clause), *it) &&
          (... && std::forward<F>(f)(std::forward<Clauses>(clauses), *it))) {
        return std::pair{true, it};
      }
    }
  }
  return std::pair{false, iter->second.end()};
}

template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, projection_clause Clause, projection_clause... Clauses>
constexpr auto query_last(hetero_container<InnerC, OuterC> const & hc, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>> {
  return query_last_if<T>(hc, []<typename C, typename Obj>(C && c, Obj const & obj) {
                     return std::invoke(c.first, obj) == c.second;
                   }, std::forward<Clause>(clause), std::forward<Clauses>(clauses)...);
}

template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, typename F, projection_clause Clause, projection_clause... Clauses>
  requires std::conjunction_v<std::is_invocable<F, Clause, T>, std::is_invocable<F, Clauses, T>...>
constexpr auto query_last_if(hetero_container<InnerC, OuterC> const & hc, F && f, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>> {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if (iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    if(iter->second.end() != iter->second.begin()) {
      for(auto e = std::prev(iter->second.end()); e != iter->second.begin(); --e) {
        if(std::forward<F>(f)(std::forward<Clause>(clause), *e) &&
           (... && std::forward<F>(f)(std::forward<Clauses>(clauses), *e))) {
          return std::pair{true, e};
        }
      }
    }
  }
  return std::pair{false, iter->second.end()};
}

using hvalue = hetero_value<std::unordered_map>;

template <template <typename...> class InnerC> using hash_key_container = hetero_container<InnerC, std::unordered_map>;

using hvector = hash_key_container<std::vector>;
using hdeque = hash_key_container<std::deque>;

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
constexpr T const * get_if(het::hetero_value<C> const & hv) noexcept {
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

namespace {

template<typename T>
struct type_offset {
  static std::size_t offset;

  explicit type_offset(T &&) noexcept { type_offset::offset++; }

  static constexpr void reset() { type_offset::offset = 0; }
};

template<typename T> std::size_t type_offset<T>::offset = 0;

} // namespace anonymous

template <typename... Ts, template <typename...> class C>
auto to_tuple(hetero_value<C> const & hv) -> std::tuple<Ts...> {
  return to_tuple<Ts...>(std::move(hv));
}

template <typename... Ts, template <typename...> class C>
auto to_tuple(hetero_value<C> & hv) -> std::tuple<Ts...> {
  return to_tuple<Ts...>(std::move(hv));
}

template <typename... Ts, template <typename...> class C>
auto to_tuple(hetero_value<C> && hv) -> std::tuple<Ts...> {
  if(!(hv.template contains<Ts>() && ...)) {
    throw std::out_of_range("try to access unbounded value");
  }
  (type_offset<Ts>::reset(), ...);
  return {hv.template value<Ts>() ...};
}

template <typename... Ts, template <typename...> class InnerC, template <typename, typename, typename...> class OuterC>
auto to_tuple(hetero_container<InnerC, OuterC> const & hv) -> std::tuple<Ts...> {
  return to_tuple<Ts...>(std::move(hv));
}

template <typename... Ts, template <typename...> class InnerC, template <typename, typename, typename...> class OuterC>
auto to_tuple(hetero_container<InnerC, OuterC> & hv) -> std::tuple<Ts...> {
  return to_tuple<Ts...>(std::move(hv));
}

template <typename... Ts, template <typename...> class InnerC, template <typename, typename, typename...> class OuterC>
auto to_tuple(hetero_container<InnerC, OuterC> && hv) -> std::tuple<Ts...> {
  if(!(hv.template contains<Ts>() && ...)) {
    throw std::out_of_range("try to access unbounded value");
  }
  (type_offset<Ts>::reset(), ...);
  return {hv.template fraction<Ts>().at(type_offset(Ts{}).offset - 1) ...};
}

} // namespace het

#endif // HETLIB_HET_H
