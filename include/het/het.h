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

namespace het {

enum class VisitorReturn { Break, Continue };

template <template <typename...> class C>
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

  template <typename T> const T & value() const {
    auto it = hetero_value::_values<T>.find(this);
    if(it == std::end(hetero_value::_values<T>)) {
      throw std::range_error(AT "Out of range");
    }
    return it->second;
  }

  template <typename T> [[nodiscard]] const T & value_or_add(T const & value = T{}) {
    auto it = hetero_value::_values<T>.find(this);
    if(it == std::end(hetero_value::_values<T>)) {
      it = add_value(value);
    }
    return it->second;
  }

  template <typename T> [[nodiscard]] const T & value_or_add(T && value = T{}) {
    return value_or_add(static_cast<T const &>(value));
  }

  template <typename T, typename... Ts> void add_values(T const & value, Ts const &... values) {
    add_value(value), (..., add_value(values));
  }

  template <typename T, typename... Ts> void add_values(T && value, Ts &&... values) {
    add_value(value), (..., add_value(std::forward<Ts>(values)));
  }

  template <typename T, typename... Ts> void modify_values(T const & value, Ts const &... values) {
    add_value(value), (..., add_value(values));
  }

  template <typename T, typename... Ts> void modify_values(T && value, Ts &&... values) {
    add_value(value), (..., add_value(std::forward<Ts>(values)));
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
  auto visit() const {
    return [this]<typename F>(F && f) -> VisitorReturn {
      return (visit_single<std::decay_t<F>, T>(std::forward<F>(f)) == VisitorReturn::Break) ||
        (... || (visit_single<std::decay_t<F>, Ts>(std::forward<F>(f)) == VisitorReturn::Break)) ?
          VisitorReturn::Break : VisitorReturn::Continue;
    };
  }

  template <typename T, typename... Ts>
  auto equal(hetero_value const & rhs) const {
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
    auto it = register_operations<T>();
    if(it != std::end(hetero_value::_values<T>)) {
      it->second = value;
    } else {
      hetero_value::_values<T>[this] = value;
      it = hetero_value::_values<T>.find(this);
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

template <template <typename...> typename C>
template <typename T> C<hetero_value<C> const *, T> hetero_value<C>::_values;

template <template <typename...> class InnerC, template <typename...> class OuterC>
class hetero_container {
  template<typename T> static OuterC<hetero_container const *, InnerC<T>> _items;

public:
  template <typename T> using return_value_type = typename InnerC<T>::iterator;

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
   */
  template<typename T> void insert(typename InnerC<T>::iterator pos, const T & t) {
    register_operations<T>();
    hetero_container::_items<T>[this].insert(pos, t); // inserts new key or access existing
  }

  template<typename T> void insert(typename InnerC<T>::iterator pos, T && t) {
    register_operations<std::decay_t<T>>();
    hetero_container::_items<std::decay_t<T>>[this].insert(pos, std::forward<T>(t)); // inserts new key or access existing
  }

  template <typename T, typename... Args> T & emplace(typename InnerC<T>::iterator pos, Args &&... args) {
    register_operations<T>();
    return hetero_container::_items<T>[this].emplace(pos, std::forward<Args>(args)...); // inserts new key or access existing
  }

  template<typename T> void push_front(const T & t) {
    register_operations<T>();
    hetero_container::_items<T>[this].insert(hetero_container::_items<T>[this].begin(), t); // inserts new key or access existing
  }

  template<typename T> void push_front(T && t) {
    register_operations<std::decay_t<T>>();
    hetero_container::_items<std::decay_t<T>>[this].insert(hetero_container::_items<std::decay_t<T>>[this].begin(), std::forward<T>(t)); // inserts new key or access existing
  }

  template <typename T, typename... Args> T & emplace_front(Args &&... args) {
    register_operations<T>();
    return hetero_container::_items<T>[this].emplace(hetero_container::_items<T>[this].begin(), std::forward<Args>(args)...); // inserts new key or access existing
  }

  template<typename T> void push_back(const T & t) {
    register_operations<T>();
    hetero_container::_items<T>[this].push_back(t); // inserts new key or access existing
  }

  template<typename T> void push_back(T && t) {
    register_operations<std::decay_t<T>>();
    hetero_container::_items<std::decay_t<T>>[this].push_back(std::forward<T>(t)); // inserts new key or access existing
  }

  template <typename T, typename... Args> T & emplace_back(Args &&... args) {
    register_operations<T>();
    return hetero_container::_items<T>[this].emplace_back(std::forward<Args>(args)...); // inserts new key or access existing
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
    size_t sum = 0;
    for (auto && size_function : _size_functions) {
      sum += size_function(*this);
    }
    // gotta be careful about this overflowing
    return sum;
  }

  template <typename T> auto & fraction() {
    return hetero_container::_items<T>.at(this);
  }

  template <typename T> auto const & fraction() const {
    return hetero_container::_items<T>.at(this);
  }

  template <typename T, typename... Clues> bool has(Clues &&... clues) const {
    auto const it = hetero_container::_items<T>.find(this);
    if (it != std::cend(hetero_container::_items<T>)) {
      return (... || (std::find_if(std::cbegin(it->second), std::cend(it->second),
        [&clues...](T && value) {
          return std::invoke(clues, value) == clues.second;
      }) != std::cend(it->second)));
    }
    return false;
  }

  template <typename T, std::invocable<T> Projection, typename Arg> return_value_type<T> find(Projection const & projection, Arg const & arg) const {
    return find<T>(projection, std::move(arg));
  }

  template <typename T, std::invocable<T> Projection, typename Arg> return_value_type<T> find(Projection const & projection, Arg && arg) const {
    auto it = hetero_container::_items<T>.find(this);
    if(it != std::end(hetero_container::_items<T>)) {
      auto found = std::find_if(std::begin((*it).second), std::end((*it).second),
        [&projection, &arg](auto & that) {
          return std::invoke(projection, that) == arg;
      });
      if(found != std::end((*it).second)) {
        return return_value_type<T>{true, found};
      }
    }
    return return_value_type<T>{false, typename InnerC<T>::iterator{}};
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
    std::pair<bool, typename hetero_container<IC, OC>::template return_value_type<T>>;
  template<typename T, template <typename...> class IC, template <typename...> class OC, std::invocable<T> F>
  friend constexpr auto find_last(hetero_container<IC, OC> const & hc, F && f) ->
    std::pair<bool, typename IC<T>::reverse_iterator>;
  template<typename T, template <typename...> class IC, template <typename...> class OC, std::output_iterator<T> O, std::invocable<T> F>
  friend constexpr bool find_all(hetero_container<IC, OC> const & hc, O output, F && f);

  template<typename T, template <typename...> class IC, template <typename...> class OC, typename Clue, typename... Clues>
  friend constexpr auto query_first(hetero_container<IC, OC> const & hc, Clue && clue, Clues &&... clues) ->
    std::pair<bool, typename hetero_container<IC, OC>::template return_value_type<T>>;
  template<typename T, template <typename...> class IC, template <typename...> class OC, typename F, typename Clue, typename... Clues>
  friend constexpr auto query_first_if(hetero_container<IC, OC> const & hc, F && f, Clue && clue, Clues &&... clues) ->
    std::pair<bool, typename hetero_container<IC, OC>::template return_value_type<T>>;

  template<typename T, template <typename...> class IC, template <typename...> class OC, typename Clue, typename... Clues>
  friend constexpr auto query_last(hetero_container<IC, OC> const & hc, Clue && clue, Clues &&... clues) ->
    std::pair<bool, typename IC<T>::reverse_iterator>;
  template<typename T, template <typename...> class IC, template <typename...> class OC, typename F, typename Clue, typename... Clues>
  friend constexpr auto query_last_if(hetero_container<IC, OC> const & hc, F && f, Clue && clue, Clues &&... clues) ->
    std::pair<bool, typename IC<T>::reverse_iterator>;

private:
  template<typename T, typename U> using visit_function = decltype(std::declval<T>().operator()(std::declval<U&>()));

  template<typename T, typename U> static constexpr bool has_visit_v = std::experimental::is_detected<visit_function, T, U>::value;

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

template <template <typename...> class InnerC, template <typename...> class OuterC>
template<typename T>
OuterC<hetero_container<InnerC, OuterC> const *, InnerC<T>> hetero_container<InnerC, OuterC>::_items;

/**
 * \brief Finds first occurrence of the element of the type T matched to predicate F
 * \tparam T type of the element to search
 * \tparam F type of the match predicate
 * \param hc container to search
 * \param f match predicate
 * \return pair of presence flag and iterator (true and
 *          valid iterator points to the element and false and invalid iterator otherwise)
 */
template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, std::invocable<T> F>
constexpr auto find_first(hetero_container<InnerC, OuterC> const & hc, F && f) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template return_value_type<T>> {
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
constexpr auto find_last(hetero_container<InnerC, OuterC> const & hc, F && f) -> std::pair<bool, typename InnerC<T>::reverse_iterator> {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if (iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    auto found = std::find_if((*iter).second.rbegin(), (*iter).second.rend(), std::forward<F>(f));
    if(found != (*iter).second.rend()) {
      return std::pair{true, found};
    }
  }
  return std::pair{false, (*iter).second.rend()};
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
  bool retval = false;
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if(iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    std::copy_if(iter->second.cbegin(), iter->second.cend(), output, std::forward<F>(f));
    retval = true;
  }
  return retval;
}

/**
 * \brief
 * \tparam T
 * \tparam Clue
 * \tparam Clues
 * \param hc
 * \param clue
 * \param clues
 * \return
 */
template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, typename Clue, typename... Clues>
constexpr auto query_first(hetero_container<InnerC, OuterC> const & hc, Clue && clue, Clues &&... clues) ->
      std::pair<bool, typename hetero_container<InnerC, OuterC>::template return_value_type<T>> {
  return query_first_if<T>(hc, []<typename Prj, typename Obj, typename Arg>(Prj const & prj, Obj const & obj, Arg const & arg) {
    return std::invoke(prj, obj) == arg; //  simplest case is obj.prj == arg
  }, std::forward<Clue>(clue), std::forward<Clues>(clues)...);
}

/**
 * \brief
 * \tparam T
 * \tparam F
 * \tparam Clue
 * \tparam Clues
 * \param hc
 * \param f
 * \param clue
 * \param clues
 * \return
 */
template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, typename F, typename Clue, typename... Clues>
constexpr auto query_first_if(hetero_container<InnerC, OuterC> const & hc, F && f, Clue && clue, Clues &&... clues) ->
    std::pair<bool, typename hetero_container<InnerC, OuterC>::template return_value_type<T>> {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if (iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    for(auto it = iter->second.begin(); it != iter->second.end(); ++it) {
      if(std::forward<F>(f)(std::forward<Clue>(clue).first, *it, std::forward<Clue>(clue).second) &&
          (... && std::forward<F>(f)(std::forward<Clues>(clues).first, *it, std::forward<Clues>(clues).second))) {
        return std::pair{true, it};
      }
    }
  }
  return std::pair{false, iter->second.end()};
}

template<typename T, template <typename...> class InnerC, template <typename...> class OuterC, typename Clue, typename... Clues>
constexpr auto query_last(hetero_container<InnerC, OuterC> const & hc, Clue && clue, Clues &&... clues) ->
    std::pair<bool, typename InnerC<T>::reverse_iterator> {
  return query_last_if(hc, []<typename Prj, typename Obj, typename Arg>(Prj const & proj, Obj const & obj, Arg const & arg) {
                     return std::invoke(proj, obj) == arg;
                   }, std::forward<Clue>(clue), std::forward<Clues>(clues)...);
}

template<typename T, typename F, template <typename...> class InnerC, template <typename...> class OuterC, typename Clue, typename... Clues>
constexpr auto query_last_if(hetero_container<InnerC, OuterC> const & hc, F && f, Clue && clue, Clues &&... clues) ->
    std::pair<bool, typename InnerC<T>::reverse_iterator> {
  auto iter = hetero_container<InnerC, OuterC>::template _items<T>.find(&hc);
  if (iter != hetero_container<InnerC, OuterC>::template _items<T>.cend()) {
    for(auto it = (*iter).second.rbegin(); it != (*iter).second.rend(); ++it) {
      if(std::forward<F>(f)(std::forward<Clue>(clue).first, *it, std::forward<Clue>(clue).second) &&
         (... && std::forward<F>(f)(std::forward<Clues>(clues).first, *it, std::forward<Clues>(clues).second))) {
        return std::pair{true, it};
      }
    }
  }
  return std::pair{false, (*iter).second.rend()};
}

// ------------------------------------------ Specializations
template <template <typename...> class InnerC>
struct hetero_container<InnerC, std::list> {
public:
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

  hetero_container & operator=(hetero_container && other)  noexcept {
    *this = other;
    return *this;
  }

  template<typename T>
  void insert(typename InnerC<T>::iterator pos, const T & t) {
    register_operations<T>()->values.insert(pos, t); // inserts new key or access existing
  }

  template <typename T, typename... Args>
  T & emplace(typename InnerC<T>::iterator pos, Args &&... args) {
    return register_operations<T>()->values.emplace(pos, std::forward<Args>(args)...); // inserts new key or access existing
  }

  template<typename T>
  void push_front(const T & t) {
    typename std::list<T>::const_iterator it = register_operations<T>();
    it->values.insert(it->values.begin(), t); // inserts new key or access existing
  }

  template <typename T, typename... Args>
  T & emplace_front(Args &&... args) {
    typename std::list<T>::const_iterator it = register_operations<T>();
    return it->values.emplace(it->values.begin(), std::forward<Args>(args)...); // inserts new key or access existing
  }

  template<typename T>
  void push_back(const T & t) {
    register_operations<T>()->values.push_back(t); // inserts new key or access existing
  }

  template <typename T, typename... Args>
  T & emplace_back(Args &&... args) {
    register_operations<T>()->values.emplace_back(std::forward<Args>(args)...); // inserts new key or access existing
  }

  template<typename T>
  void pop_front() {
    auto [has, found] = find_this_key<T>();
    if(has) {
      found->values.erase(found->values.begin());
    }
  }

  template<typename T>
  void pop_back() {
    auto [has, found] = find_this_key<T>();
    if(has) {
      found->values.pop_back();
    }
  }

  template<typename T>
  void erase(std::size_t index) {
    auto [has, found] = find_this_key<T>();
    if(has) {
      auto &c = found->values;
      c.erase(std::begin(c) + index);
      if (c.empty()) {
        // TODO: remove dependent *_functions
      }
    }
  }

  template<typename T>
  bool erase(T const & e) requires std::equality_comparable<T> {
    auto [has, found] = find_this_key<T>();
    bool erased = false;
    if(has) {
      auto &c = found->vaue;
      for (auto it = c.begin(); it != c.end(); ++it) {
        if (*it == e) {
          c.erase(it);
          erased = true;
          break;
        }
      }
      if (c.empty()) {
        // TODO: remove dependent *_functions
      }
    }
    return erased;
  }

  template<typename T>
  T & at(std::size_t index) {
    auto [has, found] = find_this_key<T>();
    if(!has) {
      throw std::range_error{AT "Out of range"};
    }
    return found->values.at(index);
  }

  template<typename T>
  T const & at(std::size_t index) const {
    auto [has, found] = find_this_key<T>();
    if(!has) {
      throw std::range_error{AT "Out of range"};
    }
    return found->values.at(index);
  }

  void clear() {
    for (auto && clear_function : _clear_functions) {
      clear_function(*this);
    }
  }

  [[nodiscard]] bool empty() const {
    bool retval = true;
    for (auto && empty_function : _empty_functions) {
      if(!empty_function(*this)) {
        retval = false;
        break;
      }
    }
    return retval;
  }

  template<typename T>
  [[nodiscard]] size_t count_of() const {
    auto [has, found] = find_this_key<T>();
    if(has) {
      return found->values.size();
    }
    return 0;
  }

  [[nodiscard]] size_t size() const {
    size_t sum = 0;
    for (auto && size_function : _size_functions) {
      sum += size_function(*this);
    }
    // gotta be careful about this overflowing
    return sum;
  }

  template <typename T>
  auto & fraction() {
    auto [has, found] = find_this_key<T>();
    if(!has) {
      throw std::range_error{AT "Out of range"};
    }
    return found->values;
  }

  template <typename T>
  auto const & fraction() const {
    auto [has, found] = find_this_key<T>();
    if(!has) {
      throw std::range_error{AT "Out of range"};
    }
    return found->values;
  }

  template <typename Projection, typename Arg, typename T>
  bool find(Projection const & projection, Arg const & arg, T & result) {
    return find(projection, std::move(arg), result);
  }

  template <typename Projection, typename Arg, typename T>
  bool find(Projection const & projection, Arg && arg, T & result) {
    auto iter = hetero_container::_items<T>.find(this);
    if (iter != hetero_container::_items<T>.cend()) {
      for(auto it = (*iter).second.begin(); it != (*iter).second.end(); ++it) {
        if(std::invoke(projection, *it) == arg) {
          result = *it;
          return true;
        }
      }
    }
    return false;
  }

//  template<typename T>
//  void visit(T && visitor) {
//    visit_impl(visitor, typename std::decay_t<T>::types{});
//  }

  template<typename... Ts>
  auto visit() {
    return [this]<typename F>(F && f) {
      return (..., visit_single<std::decay_t<F>, Ts>(std::forward<F>(f)));
    };
  }

  template<typename T, std::invocable<T> F>
  friend std::pair<bool, typename InnerC<T>::iterator> find_first(F && f) {
    auto it = std::find_if(std::cbegin(hetero_container::_items<T>),
                           std::cend(hetero_container::_items<T>), std::forward<F>(f));
    if (it != std::cend(hetero_container::_items<T>)) {
      auto found = std::find_if(std::begin(it->values), std::end(it->values), std::forward<F>(f));
      if(found != std::end(it->values)) {
        return std::pair{true, found};
      }
    }
    return std::pair{false, std::end(it->values)};
  }

  template<typename T, typename Clue, typename... Clues>
  friend std::pair<bool, typename InnerC<T>::iterator> query_first(hetero_container const & hc, Clue && clue, Clues &&... clues) {
    return query_first_if<T>(hc,
                             [] <typename Prj, typename Obj, typename Arg>(Prj const & prj, Obj const & obj, Arg const & arg) {
                               return std::invoke(prj, obj) == arg;
                             }, std::forward<Clue>(clue), std::forward<Clues>(clues)...);
  }

  template<typename T, typename F, typename Clue, typename... Clues>
  friend std::pair<bool, typename InnerC<T>::iterator> query_first_if(hetero_container const & hc, F && f,
                                                                      Clue && clue, Clues &&... clues) {
    auto iter = hetero_container::_items<T>.find(&hc);
    if (iter != hetero_container::_items<T>.cend()) {
      for(auto it = (*iter).second.begin(); it != (*iter).second.end(); ++it) {
        if(std::forward<F>(f)(std::forward<Clue>(clue).first, *it, std::forward<Clue>(clue).second) &&
            (... && std::forward<F>(f)(std::forward<Clues>(clues).first, *it, std::forward<Clues>(clues).second))) {
          return std::pair{true, it};
        }
      }
    }
    return std::pair{false, (*iter).second.end()};
  }

  template<typename T, std::invocable<T> F>
  friend std::pair<bool, typename InnerC<T>::reverse_iterator> find_last(hetero_container const & hc, F && f) {
    auto iter = hetero_container::_items<T>.find(&hc);
    if (iter != hetero_container::_items<T>.cend()) {
      auto found = std::find_if((*iter).second.rbegin(), (*iter).second.rend(), std::forward<F>(f));
      if(found != (*iter).second.rend()) {
        return std::pair{true, found};
      }
    }
    return std::pair{false, (*iter).second.rend()};
  }

  template<typename T, std::invocable<T> Prj, std::invocable<T>... Prjs, typename Arg, typename... Args>
  friend std::pair<bool, typename InnerC<T>::reverse_iterator> find_last(hetero_container const & hc,
                                                                         Prj const & prj, Prjs const &... prjs, Arg && arg, Args &&... args) {
    static_assert(sizeof...(Prjs) == sizeof...(Args),
                  "Quantities of the projections and arguments should be equal");
    return find_last(hc,
                     []<typename Proj, typename Obj, typename Argm>(Proj const & proj, Obj const & obj, Argm const & arg) {
                       return std::invoke(proj, obj) == arg;
                     }, prj, prjs..., std::move(arg), std::move(args)...);
  }

  template<typename T, typename F, std::invocable<T> Prj, std::invocable<T>... Prjs, typename Arg, typename... Args>
  friend std::pair<bool, typename InnerC<T>::reverse_iterator> find_last(hetero_container const & hc, F && f,
                                                                         Prj const & prj, Prjs const &... prjs, Arg && arg, Args &&... args) {
    static_assert(sizeof...(Prjs) == sizeof...(Args),
                  "Quantities of the projections and arguments should be equal");
    auto iter = hetero_container::_items<T>.find(&hc);
    if (iter != hetero_container::_items<T>.cend()) {
      for(auto it = (*iter).second.rbegin(); it != (*iter).second.rend(); ++it) {
        if(std::forward<F>(f)(std::forward<Prj>(prj), *it, std::forward<Arg>(arg)) &&
           (... && std::forward<F>(f)(prjs, *it, args))) {
          return std::pair{true, it};
        }
      }
    }
    return std::pair{false, (*iter).second.rend()};
  }

  template<typename T, std::output_iterator<T> O, std::invocable<T> F>
  friend void find_all(hetero_container const & hc, O output, F && f) {
    auto iter = hetero_container::_items<T>.find(&hc);
    if (iter != hetero_container::_items<T>.cend()) {
      std::copy_if((*iter).second.cbegin(), (*iter).second.cend(), output, std::forward<F>(f));
    }
  }

private:
  template <typename T> struct kv {
    const hetero_container * key;
    InnerC<T> values;
  };

  template<typename T> static std::list<kv<T>> _items;

  template<typename T>
  static std::pair<bool, typename std::list<kv<T>>::iterator> find_key(const hetero_container * key) {
    auto it = std::find_if(std::begin(hetero_container::_items<T>),
      std::end(hetero_container::_items<T>), [key](auto const & e) { return e.key == key; });
    return std::pair{it != std::end(hetero_container::_items<T>), it};
  }

  template<typename T>
  std::pair<bool, typename std::list<kv<T>>::iterator> find_this_key() {
    return hetero_container::find_key<T>(this);
  }

  template<typename T, typename U>
  using visit_function = decltype(std::declval<T>().operator()(std::declval<U&>()));

  template<typename T, typename U>
  static constexpr bool has_visit_v = std::experimental::is_detected<visit_function, T, U>::value;

  template<typename T, typename U>
  void visit_single(T & visitor) {
    visit_single<std::decay_t<T>, U>(std::move(visitor));
  }

  template<typename T, typename U>
  void visit_single(T && visitor) {
    static_assert(has_visit_v<T, U>, "Visitor shouldn't accept provided type");
    auto iter = hetero_container::_items<U>.find(this);
    if (iter != hetero_container::_items<U>.cend()) {
      visitor(iter->second);
    }
  }

//  template<typename T, template<typename...> class TList, typename... Types>
//  void visit_impl(T && visitor, TList<Types...>) {
//    (..., visit_impl_help<std::decay_t<T>, Types>(visitor));
//  }
//
//  template<typename T, typename U>
//  void visit_impl_help(T& visitor) {
//    static_assert(has_visit_v<T, U>, "Visitor should accept a reference for each queried type");
//    auto [has, found] = find_this_key();
//    if (has) {
//      for (auto && element : found->values) {
//        visitor(element);
//      }
//    }
//  }

  template<typename T>
  typename std::list<kv<T>>::iterator register_operations() {
    auto [has, found] = find_this_key<T>();
    if (!has) { // don't have it yet, so create functions for printing, copying, moving, and destroying
      _clear_functions.emplace_back([](hetero_container& c) {
        auto [has, found] = hetero_container::find_key<T>(&c);
        if(has) {
          hetero_container::_items<T>.erase(found);
        }
      });
      // if someone copies me, they need to call each copy_function and pass themself
      _copy_functions.emplace_back([](const hetero_container& from, hetero_container& to) {
        auto [has1, found1] = hetero_container::find_key<T>(&to);
        if(!has1) {
          throw std::range_error{AT "Out of range"};
        }
        auto [has2, found2] = hetero_container::find_key<T>(&from);
        if(!has2) {
          throw std::range_error{AT "Out of range"};
        }
        found1->values = found2->values;
      });
      _size_functions.emplace_back([](const hetero_container& c) {
        auto [has, found] = hetero_container::find_key<T>(&c);
        if(!has) {
          throw std::range_error{AT "Out of range"};
        }
        return found->values.size();
      });
      _empty_functions.emplace_back([](const hetero_container& c) {
        auto [has, found] = hetero_container::find_key<T>(&c);
        if(!has) {
          throw std::range_error{AT "Out of range"};
        }
        return found->values.empty();
      });

      hetero_container::_items<T>.push_back(kv<T>{this, InnerC<T>{}});
      found = std::prev(std::end(hetero_container::_items<T>));
    }

    if(found == std::end(hetero_container::_items<T>)) {
      throw std::range_error{AT "Out of range"};
    }
    return found;
  }

  std::vector<std::function<void(hetero_container&)>> _clear_functions;
  std::vector<std::function<void(const hetero_container&, hetero_container&)>> _copy_functions;
  std::vector<std::function<size_t(const hetero_container&)>> _size_functions;
  std::vector<std::function<bool(const hetero_container&)>> _empty_functions;
};

template <template <typename...> class InnerC>
template<typename T>
std::list<typename hetero_container<InnerC, std::list>::template kv<T>> hetero_container<InnerC, std::list>::_items;

using hvalue = hetero_value<std::unordered_map>;

template <template <typename...> class InnerC> using hash_key_container = hetero_container<InnerC, std::unordered_map>;
template <template <typename...> class InnerC> using list_key_container = hetero_container<InnerC, std::list>;

using hvector = hash_key_container<std::vector>;
using hdeque = hash_key_container<std::deque>;

using lvector = list_key_container<std::vector>;
using ldeque = list_key_container<std::vector>;

} // namespace het

#endif // HETLIB_HET_H
