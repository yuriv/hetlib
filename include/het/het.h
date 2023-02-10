//
// Created by yuri on 10/1/21.
//

#ifndef HETLIB_HET_H
#define HETLIB_HET_H

#include "uncommon_definitions.h"

#include <deque>
#include <map>
#include <vector>
#include <list>
#include <functional>

#include <experimental/type_traits>
#include <ranges>

namespace het {

enum class VisitorReturn { Break, Continue };

template <typename T> concept projection_clause = requires(T t) {
  t.first;
  t.second;
  std::invocable<std::decay_t<decltype(t.first)>>;
};

template <template <typename, typename, typename...> class C> concept is_assoc_container = requires {
  typename C<int, int>::key_type;
};

template <template <typename, typename, typename...> class C> requires is_assoc_container<C>
class hetero_value {
  template <typename T> static C<hetero_value const *, T> _values;

public:

  hetero_value() = default;
  hetero_value(hetero_value const & value) {
    assign(value);
  }
  hetero_value(hetero_value && value) noexcept {
    *this = std::move(value);
  }
  template <typename... Ts> explicit hetero_value(Ts &&... ts) {
    (..., add_value(std::forward<Ts>(ts)));
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

  void assign(hetero_value && value) {
    assign(static_cast<hetero_value const &>(value));
  }

  template <typename... Ts> void assign_values(Ts const &... values) {
    clear();
    (..., add_value(values));
  }

  template <typename... Ts> void assign_values(Ts &&... values) {
    clear();
    (..., add_value(std::forward<Ts>(values)));
  }

  template <typename T> [[nodiscard]] bool contains() const {
    return hetero_value::_values<T>.find(this) != std::end(hetero_value::_values<T>);
  }

  /**
   * \brief Returns const value of type T
   * \tparam T requested value type
   * \return value
   * \throw std::range_error if requested value not presented
   */
  template <typename T> [[nodiscard]] const T & value() const {
    auto it = hetero_value::_values<T>.find(this);
    if(it == std::end(hetero_value::_values<T>)) {
      throw std::range_error(AT "Out of range");
    }
    return it->second;
  }

  /**
   * \brief Returns mutable value of type T
   * \tparam T requested value type
   * \return value
   * \throw std::range_error if requested value not presented
   */
  template <typename T> [[nodiscard]] T & value() {
    auto it = hetero_value::_values<T>.find(this);
    if(it == std::end(hetero_value::_values<T>)) {
      throw std::range_error(AT "Out of range");
    }
    return it->second;
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
    auto it = hetero_value::_values<T>.find(this);
    if(it == std::end(hetero_value::_values<T>)) {
      add_value(std::forward<T>(value));
    }
    return it->second;
  }

//  template <typename T, typename... Ts> void add_values(T const & value, Ts &&... values) {
//    add_value(std::move(value)), (..., add_value(std::forward<Ts>(values)));
//  }

  template <typename T, typename... Ts> void add_values(T && value, Ts &&... values) {
    add_value(std::forward<T>(value)), (..., add_value(std::forward<Ts>(values)));
  }

//  template <typename T, typename... Ts> void modify_values(T const & value, Ts &&... values) {
//    add_value(value), (..., add_value(values));
//  }

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
  [[nodiscard]] auto visit() const {
    return [this]<typename F>(F && f) -> VisitorReturn {
      return (visit_single<std::decay_t<F>, T>(std::forward<F>(f)) == VisitorReturn::Break) ||
        (... || (visit_single<std::decay_t<F>, Ts>(std::forward<F>(f)) == VisitorReturn::Break)) ?
          VisitorReturn::Break : VisitorReturn::Continue;
    };
  }

  template <typename T, typename... Ts>
  [[nodiscard]] auto equal(hetero_value const & rhs) const {
    return [this, &rhs]<typename F>(F && f) -> bool {
        return equal_single<std::decay_t<F>, T>(std::forward<F>(f), rhs.template value<T>()) &&
            (... && equal_single<std::decay_t<F>, Ts>(std::forward<F>(f), rhs.template value<Ts>()));
    };
  }

  template <typename T, typename... Ts>
  auto equal(hetero_value && rhs) const {
    return [this, &rhs]<typename F>(F && f) -> bool {
      return equal_single<std::decay_t<F>, T>(std::forward<F>(f), rhs.template value<T>()) &&
          (... && equal_single<std::decay_t<F>, Ts>(std::forward<F>(f), rhs.template value<Ts>()));
    };
  }

private:
  template<typename T, typename U> using visit_function = decltype(std::declval<T>().operator()(std::declval<U&>()));
  template<typename T, typename U> using equal_function = decltype(std::declval<T>().operator()(std::declval<U&>(), std::declval<U&>()));

  template<typename T, typename U> static constexpr bool has_visit_v = std::experimental::is_detected<visit_function, T, U>::value;
  template<typename T, typename U> static constexpr bool has_equal_v = std::experimental::is_detected<equal_function, T, U>::value;

  template<typename T, typename U> void visit_single(T const & visitor) {
    return visit_single<std::decay_t<T>, U>(std::move(visitor));
  }

  template<typename T, typename U> VisitorReturn visit_single(T && visitor) const {
    static_assert(has_visit_v<T, U>, "Visitor should accept provided type");
    auto iter = hetero_value::_values<U>.find(this);
    if (iter != hetero_value::_values<U>.cend()) {
      return visitor(iter->second);
    }
    return VisitorReturn::Continue;
  }

  template<typename T, typename U> bool equal_single(T && visitor, U const & arg) const {
    static_assert(has_equal_v<T, U>, "Comparator should accept provided type");
    auto iter = hetero_value::_values<U>.find(this);
    if (iter != hetero_value::_values<U>.cend()) {
      return visitor(iter->second, arg);
    }
    return false;
  }

  template <typename T> auto add_value(T const & value) {
    return add_value(std::move(value));
  }

  template <typename T> auto add_value(T && value) {
    //int dd = value;
    using To = std::decay_t<T>;
    auto it = register_operations<To>();
    if(it != std::end(hetero_value::_values<To>)) {
      it->second = std::forward<To>(value);
    } else {
      hetero_value::_values<To>[this] = std::forward<To>(value);
      it = hetero_value::_values<To>.find(this);
    }
    return it;
  }

  template <typename T> void erase_value() {
    if(contains<T>()) {
      hetero_value::_values<T>.erase(this);
      _arity--;
    }
  }

  template<typename T> typename C<hetero_value const *, T>::iterator register_operations() {
    auto it = _values<T>.find(this);
    // don't have it yet, so create functions for printing, copying, moving, and destroying
    if (it == std::end(_values<T>)) {
      _clear_functions.template emplace_back([](hetero_value & c) {
        hetero_value::_values<T>.erase(&c);
      });
      // if someone copies me, they need to call each copy_function and pass themself
      _copy_functions.template emplace_back([](const hetero_value & from, hetero_value & to) {
        if(&from != &to) {
          hetero_value::_values<T>[&to] = hetero_value::_values<T>[&from];
        }
      });
      _arity++;
    }
    return it;
  }

  std::size_t _arity{0};
  std::vector<std::function<void(hetero_value&)>> _clear_functions;
  std::vector<std::function<void(const hetero_value&, hetero_value&)>> _copy_functions;
};

template <template <typename, typename, typename...> typename C> requires is_assoc_container<C>
template <typename T> C<hetero_value<C> const *, T> hetero_value<C>::_values;

template <template <typename...> class InnerC, template <typename, typename, typename...> class OuterC> requires is_assoc_container<OuterC>
class hetero_container {
  template<typename T> static OuterC<hetero_container const *, InnerC<T>> _items;

public:
  template <typename T> using iterator = typename OuterC<hetero_container const *, InnerC<T>>::iterator;
  template <typename T> using const_iterator = typename OuterC<hetero_container const *, InnerC<T>>::const_iterator;
  template <typename T> using reverse_iterator = typename OuterC<hetero_container const *, InnerC<T>>::reverse_iterator;
  template <typename T> using const_reverse_iterator = typename OuterC<hetero_container const *, InnerC<T>>::const_reverse_iterator;

  template <typename T> using inner_iterator = typename InnerC<T>::iterator;
  template <typename T> using inner_const_iterator = typename InnerC<T>::const_iterator;
  template <typename T> using inner_reverse_iterator = typename InnerC<T>::reverse_iterator;
  template <typename T> using inner_const_reverse_iterator = typename InnerC<T>::const_reverse_iterator;

  hetero_container() = default;
  hetero_container(hetero_container const & other) {
    *this = other;
  }

  hetero_container(hetero_container && other)  noexcept {
    *this = std::move(other);
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
    clear();
    _clear_functions = other._clear_functions;
    _copy_functions = other._copy_functions;
    _size_functions = other._size_functions;
    _empty_functions = other._empty_functions;
    for (auto && copy_function : _copy_functions) {
      copy_function(other, *this);
    }
    return *this;
  }

  hetero_container & operator=(hetero_container && other) noexcept {
    *this = other;
    return *this;
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
    register_operations<T>();
    return hetero_container::_items<T>[this].insert(pos, t);
  }

  template<typename T> inner_iterator<T> insert(inner_iterator<T> pos, T && t) {
    // inserts new key or access existing
    register_operations<std::decay_t<T>>();
    return hetero_container::_items<std::decay_t<T>>[this].insert(pos, std::forward<T>(t));
  }

  template <typename T, typename... Args> T & emplace(inner_iterator<T> pos, Args &&... args) {
    // inserts new key or access existing
    register_operations<T>();
    return hetero_container::_items<T>[this].emplace(pos, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args> T & emplace_front(Args &&... args) {
    // inserts new key or access existing
    register_operations<T>();
    return hetero_container::_items<T>[this].emplace(hetero_container::_items<T>[this].begin(), std::forward<Args>(args)...);
  }

  template <typename T, typename... Args> auto & emplace_back(Args &&... args) {
    register_operations<T>();
    return hetero_container::_items<T>[this].emplace_back(std::forward<Args>(args)...); // inserts new key or access existing
  }

  template<typename T, typename... Ts> void push_front(T const & t, Ts &&... ts) {
    push_front_single(t), (..., push_front_single(std::forward<Ts>(ts)));
  }

  template<typename T, typename... Ts> void push_front(T && t, Ts &&... ts) {
    push_front_single(std::forward<T>(t)), (..., push_front_single(std::forward<Ts>(ts)));
  }

  template<typename T, typename... Ts> void push_back(T const & t, Ts &&... ts) {
    push_back_single(t), (..., push_back_single(std::forward<Ts>(ts)));
  }

  template<typename T, typename... Ts> void push_back(T && t, Ts &&... ts) {
    push_back_single(std::forward<T>(t)), (..., push_back_single(std::forward<Ts>(ts)));
  }

  template<typename T> void pop_front() {
    hetero_container::_items<T>.at(this).erase(hetero_container::_items<T>.at(this).begin());
  }

  template<typename T> void pop_back() {
    hetero_container::_items<T>.at(this).pop_back();
  }

  template<typename T> void erase(std::size_t index) {
    auto & c = hetero_container::_items<T>.at(this);
    c.erase(std::begin(c) + index);
    if(c.empty()) {
      // TODO: remove dependent *_functions
    }
  }

  template<typename T> bool erase(T const & e) requires std::equality_comparable<T> {
    auto & c = hetero_container::_items<T>.at(this);
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
    return hetero_container::_items<T>.at(this).at(index);
  }

  template<typename T> T const & at(std::size_t index) const {
    return hetero_container::_items<T>.at(this).at(index);
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
    auto iter = hetero_container::_items<T>.find(this);
    if (iter != hetero_container::_items<T>.cend()) {
      return hetero_container::_items<T>.at(this).size();
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

  template <typename T> auto & fraction() {
    return hetero_container::_items<T>.at(this);
  }

  template <typename T> auto const & fraction() const {
    return hetero_container::_items<T>.at(this);
  }

  template <typename T> [[nodiscard]] constexpr bool contains() const {
    return hetero_container::_items<T>.find(this) != std::cend(hetero_container::_items<T>);
  }

  template <typename T, typename Clause, typename... Clauses>
  [[nodiscard]] constexpr bool contains(Clause && clause, Clauses &&... clauses) const
  requires std::equality_comparable<T> && projection_clause<Clause> && (... && projection_clause<Clauses>) {
    if (auto const it = hetero_container::_items<T>.find(this); it != std::cend(hetero_container::_items<T>)) {
      auto cmp = [&f = it->second]<typename C>(C && c) -> bool {
        return std::find_if(std::cbegin(f), std::cend(f), [&c](T && value) {
          return std::invoke(c.first, value) == c.second;
        }) != std::cend(f);
      };
      return cmp(std::forward<Clause>(clause)) || (... || cmp(std::forward<Clauses>(clauses)));
    }
    return false;
  }

  template <typename T, typename Clause> auto find(Clause const & clause) const -> std::pair<bool, inner_iterator<T>>
  requires projection_clause<Clause> {
    return find<T>(std::move(clause));
  }

  template <typename T, typename Clause> auto find(Clause && clause) const -> std::pair<bool, inner_iterator<T>>
  requires projection_clause<Clause> {
    if(auto it = hetero_container::_items<T>.find(this); it != std::end(hetero_container::_items<T>)) {
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
   * \brief Generates function to visit elements of the types T, Ts by predicate F
   * \tparam T type to proceed
   * \tparam Ts types to proceed
   * \return function which apply predicate F on elements of specified types
   */
  template <typename T, typename... Ts> auto visit() const {
    return [this]<typename F>(F && f) -> VisitorReturn {
      return (visit_single<F, T>(std::forward<F>(f)) == VisitorReturn::Break) ||
        (... || (visit_single<F, Ts>(std::forward<F>(f)) == VisitorReturn::Break)) ?
          VisitorReturn::Break : VisitorReturn::Continue;
    };
  }

  // Find/Query accessors
  template<typename T, template <typename...> class IC, template <typename...> class OC, std::invocable<T> F>
  friend constexpr auto find_first(hetero_container<IC, OC> const & hc, F && f) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;
  template<typename T, template <typename...> class IC, template <typename...> class OC, std::invocable<T> F>
  friend constexpr auto find_last(hetero_container<IC, OC> const & hc, F && f) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>;
  template<typename T, template <typename...> class IC, template <typename...> class OC, std::output_iterator<T> O, std::invocable<T> F>
  friend constexpr bool find_all(hetero_container<IC, OC> const & hc, O output, F && f);

  template<typename T, template <typename...> class IC, template <typename...> class OC, typename Clause, typename... Clauses>
  friend constexpr auto query_first(hetero_container<IC, OC> const & hc, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>
  requires std::equality_comparable<T> && projection_clause<Clause> && (... && projection_clause<Clauses>);

  template<typename T, template <typename...> class IC, template <typename...> class OC, typename F, typename Clause, typename... Clauses>
  friend constexpr auto query_first_if(hetero_container<IC, OC> const & hc, F && f, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>
  requires std::equality_comparable<T> && projection_clause<Clause> && (... && projection_clause<Clauses>);

  template<typename T, template <typename...> class IC, template <typename...> class OC, typename Clause, typename... Clauses>
  friend constexpr auto query_last(hetero_container<IC, OC> const & hc, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>
  requires std::equality_comparable<T> && projection_clause<Clause> && (... && projection_clause<Clauses>);

  template<typename T, template <typename...> class IC, template <typename...> class OC, typename F, typename Clause, typename... Clauses>
  friend constexpr auto query_last_if(hetero_container<IC, OC> const & hc, F && f, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<IC, OC>::template inner_iterator<T>>
  requires std::equality_comparable<T> && projection_clause<Clause> && (... && projection_clause<Clauses>);

private:
  template<typename T, typename U> using visit_function = decltype(std::declval<T>().operator()(std::declval<U&>()));

  template<typename T, typename U> static constexpr bool has_visit_v = std::experimental::is_detected<visit_function, T, U>::value;

  template<typename T> void push_front_single(const T & t) {
    // inserts new key or access existing
    register_operations<T>();
    hetero_container::_items<T>[this].insert(hetero_container::_items<T>[this].begin(), t);
  }

  template<typename T> void push_front_single(T && t) {
    // inserts new key or access existing
    register_operations<std::decay_t<T>>();
    hetero_container::_items<std::decay_t<T>>[this].insert(hetero_container::_items<std::decay_t<T>>[this].begin(), std::forward<T>(t));
  }

  template<typename T> void push_back_single(const T & t) {
    register_operations<T>();
    hetero_container::_items<T>[this].push_back(t); // inserts new key or access existing
  }

  template<typename T> void push_back_single(T && t) {
    register_operations<std::decay_t<T>>();
    hetero_container::_items<std::decay_t<T>>[this].push_back(std::forward<T>(t)); // inserts new key or access existing
  }

  template<typename T, typename U> VisitorReturn visit_single(T const & visitor) const {
    return visit_single<std::decay_t<T>, U>(std::move(visitor));
  }

  template<typename T, typename U> VisitorReturn visit_single(T && visitor) const {
    static_assert(has_visit_v<T, U>, "Visitor should accept provided type");
    auto iter = hetero_container::_items<U>.find(this);
    if(iter != std::cend(hetero_container::_items<U>)) {
      for(auto & c : iter->second) {
        if(visitor(c) == VisitorReturn::Break) {
          return VisitorReturn::Break;
        }
      }
    }
    return VisitorReturn::Continue;
  }

  template<typename T> void register_operations() {
    // don't have it yet, so create functions for printing, copying, moving, and destroying
    if (_items<T>.find(this) == std::end(_items<T>)) {
      _clear_functions.emplace_back([](hetero_container& c) { _items<T>.erase(&c); });
      // if someone copies me, they need to call each copy_function and pass themself
      _copy_functions.emplace_back([](const hetero_container& from, hetero_container& to) {
        _items<T>[&to] = _items<T>[&from];
      });
      _size_functions.emplace_back([](const hetero_container& c) { return _items<T>.at(&c).size(); });
      _empty_functions.emplace_back([](const hetero_container& c) { return _items<T>.at(&c).empty(); });
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
    auto found = std::find_if(iter->second.begin(), iter->second.end(), std::forward<F>(f));
    if(found != iter->second.end()) {
      return std::pair{true, found};
    }
  }
  return std::pair{false, iter->second.end()};
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
    if(iter->second.end() != iter->second.begin()) {
      for(auto e = std::prev(iter->second.end()); e != iter->second.begin(); --e) {
        if(std::forward<F>(f)(*e)) {
          return std::pair{true, e};
        }
      }
    }
  }
  return std::pair{false, iter->second.end()};
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
template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, typename Clause, typename... Clauses>
constexpr auto query_first(hetero_container<InnerC, OuterC> const & hc, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>>
  requires std::equality_comparable<T> && projection_clause<Clause> && (... && projection_clause<Clauses>) {
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
template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, typename F, typename Clause, typename... Clauses>
constexpr auto query_first_if(hetero_container<InnerC, OuterC> const & hc, F && f, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>>
  requires std::equality_comparable<T> && projection_clause<Clause> && (... && projection_clause<Clauses>) {
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

template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, typename Clause, typename... Clauses>
constexpr auto query_last(hetero_container<InnerC, OuterC> const & hc, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>>
  requires std::equality_comparable<T> && projection_clause<Clause> && (... && projection_clause<Clauses>) {
  return query_last_if<T>(hc, []<typename C, typename Obj>(C && c, Obj const & obj) {
                     return std::invoke(c.first, obj) == c.second;
                   }, std::forward<Clause>(clause), std::forward<Clauses>(clauses)...);
}

template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, typename F, typename Clause, typename... Clauses>
constexpr auto query_last_if(hetero_container<InnerC, OuterC> const & hc, F && f, Clause && clause, Clauses &&... clauses) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template inner_iterator<T>>
  requires std::equality_comparable<T> && projection_clause<Clause> && (... && projection_clause<Clauses>) {
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
constexpr T & get(het::hetero_value<C> & hv) {
  static_assert(!std::is_void_v<T>, "T must not be void");
  return hv.template value<T>();
}

template <typename T, template <typename...> typename C>
constexpr T && get(het::hetero_value<C> && hv) {
  static_assert(!std::is_void_v<T>, "T must not be void");
  return std::move(hv.template value<T>());
}

template <typename T, template <typename...> typename C>
constexpr T const & get(het::hetero_value<C> const & hv) {
  static_assert(!std::is_void_v<T>, "T must not be void");
  return hv.template value<T>();
}

template <typename T, template <typename...> typename C>
constexpr T const && get(het::hetero_value<C> const && hv) {
  static_assert(!std::is_void_v<T>, "T must not be void");
  return std::move(hv.template value<T>());
}

template <typename T, template <typename...> typename C>
constexpr T const * get_if(het::hetero_value<C> const & hv) noexcept {
  static_assert(!std::is_void_v<T>, "T must not be void");
  return hv.template contains<T>() ? std::addressof(get<T>(hv)) : nullptr;
}

template <typename T, template <typename...> typename C>
constexpr T * get_if(het::hetero_value<C> & hv) noexcept {
  static_assert(!std::is_void_v<T>, "T must not be void");
  return hv.template contains<T>() ? std::addressof(get<T>(hv)) : nullptr;
}

template <typename T, template <typename...> typename C>
constexpr T * get_if(het::hetero_value<C> && hv) noexcept {
  static_assert(!std::is_void_v<T>, "T must not be void");
  return hv.template contains<T>() ? std::addressof(get<T>(hv)) : nullptr;
}

} // namespace het

#endif // HETLIB_HET_H
