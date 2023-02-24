/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   util.h
 * Author: yuri
 *
 * Created on March 13, 2018, 6:19 PM
 */

#ifndef TYPE_CONSTRS_UTIL_H
#define TYPE_CONSTRS_UTIL_H

#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <type_traits>
#include <functional>

namespace metaf {
namespace util {

/**
 * \brief Currying F over argument Args...
 * 
 * @param f - functor to curry
 * @param args - arguments for currying
 * @return curried value
 */
template<typename F, typename... Args>
constexpr auto curry(F&& f, Args&&... args) {
  return [=](auto&&... rest) {
    return f(std::forward<Args>(args)..., std::forward<decltype(rest)>(rest)...);
  };
}

/// \brief Helper to pretty print type T into error log.
template <typename T> struct Debug { private: Debug() {} };

/// \brief Helper to hold boolean arguments sequence.
template <bool... T> struct RestrictResultSet {};

/// \brief Helper to select either is type argument or not.
template <typename... Ts> struct JustType : std::true_type {};

namespace bools {

/// \brief Compile-time boolean constant.
template <bool B> using bool_const = std::integral_constant<bool, B>;

/// \brief Joins all bool constants using operator &&.
template <bool... Bs> struct conjunction;

template <> struct conjunction<> {
  static constexpr bool value = true;
};

template <bool X, bool... Xs> struct conjunction<X, Xs...> {
  static constexpr bool value = X && conjunction<Xs...>::value;
};

/**
 * \brief Conjugates Xs... by &&.
 * 
 * @return conjunction<Xs...>
 */
template <bool... Xs> constexpr auto intersect() -> conjunction<Xs...> {
  return {};
}

/// \brief Joins all bool constants using operator ||.
template <bool... BoolConstants> struct disjunction;

template <> struct disjunction<> {
  static constexpr bool value = false;
};

template <bool X, bool... Xs> struct disjunction<X, Xs...> {
  static constexpr bool value = X || disjunction<Xs...>::value;
};

/**
 * \brief Conjugates Xs... by ||.
 * 
 * @return disjunction<Xs...>
 */
template <bool... Xs> constexpr auto unite() -> disjunction<Xs...> {
  return {};
}

/// \brief Joins all types \bXs... with binary predicate \bP. Binary predicate returns some
/// result type by evaluate of two input types \bP<X, Y>::type, X,Y, type are types.
template <template<typename, typename> class P, typename... Xs> struct join_with_p_t;

template <template<typename, typename> class P, typename X> struct join_with_p_t<P, X> {
  using type = typename P<X, X>::type;
};

template <template<typename, typename> class P, typename X0, typename... Xs> struct join_with_p_t<P, X0, Xs...> {
  using type = typename P<X0, typename join_with_p_t<P, Xs...>::type>::type;
};

/// \brief Joins all constants \bXs... of the type \bX with binary predicate \bP. 
/// Binary predicate returns some value of the type \bX by evaluate of two input 
/// values of type \bX. \bP<x, x>::value, \bx,x are values of the type \bX, \bvalue
/// is value of the type returns by predicate \bP.
template <typename X,  template<X, X> class P, X... Xs> struct join_with_p;

template <typename X, template<X, X> class P, X X0> struct join_with_p<X, P, X0> {
  static constexpr X value = P<X0, X0>::value;
};

template <typename X, template<X, X> class P, X X0, X... Xs> struct join_with_p<X, P, X0, Xs...> {
  static constexpr auto value = P<X0, join_with_p<X, P, Xs...>::value>::value;
};

template <typename X, template<X, X> class P, X... Xs> 
constexpr auto join_with() -> join_with_p<X, P, Xs...> { return {}; }

} // namespace bools

namespace tuples {

namespace detail {
template <typename F, size_t... Is>
constexpr auto index_apply(F&& f, std::index_sequence<Is...>) {
  return f(std::integral_constant<size_t, Is>{}...);
}
} // namespace detail

template <size_t N, typename F>
constexpr auto index_apply(F&& f) {
  return detail::index_apply(f, std::make_index_sequence<N>{});
}

/**
 * \brief Apply functor \bF to \bTuple.
 * 
 * @param f - functor to apply, arity of \bf must be 
 *            equal of size of Tuple or must support currying
 * @param t - arguments packed to tuple
 * @return return value of \bF
 */
template <typename F, typename Tuple>
constexpr auto apply(F&& f, Tuple&& t) {
  return index_apply<std::tuple_size<Tuple>>(
    [&](auto... Is) { return f(std::get<Is>(t)...); });
}

/**
 * \brief Returns tuple of I'th elements retrieved from tuples pack \bTuples
 * 
 * @param I - zero-based index of the element into each tuple
 * @param ts - pack of the tuples to retrieve
 * @return tuple of \bI'th elements of the \bTuples
 */
template <size_t I, typename... Tuples>
constexpr auto get_elements(Tuples&&... ts) {
  return std::make_tuple(std::get<I>(std::forward<Tuples>(ts))...);
}

/**
 * \brief Applies comparator \bF for \bI'th elements of the tuples pack \bTuples
 * 
 * @param I - zero-based index of the element into each tuple
 * @param f - comparator to apply, arity of \bf must be 
 *            equal of \bsizeof...(Tuples) or comparator must support currying
 * @param ts - pack of the tuples to scan
 * @return bool
 */
template <size_t I, typename F, typename... Tuples>
constexpr auto compare_elements(F&& f, Tuples&&... ts) -> bool {
  return std::forward<F>(f)(std::get<I>(std::forward<Tuples>(ts))...);
}

namespace {
template <typename F, typename A, typename V>
constexpr auto foldl_elements_impl(F&& f, A const& a, V&& v) -> A {
  return std::forward<F>(f)(a, std::forward<V>(v));
}

template <typename F, typename A, typename V, typename... Vs>
constexpr auto foldl_elements_impl(F&& f, A const& a, V&& v, Vs&&... vs) -> A {
  return foldl_elements_impl(f, std::forward<F>(f)(a, std::forward<V>(v)), vs...);
}

template <typename F, typename A, typename V>
constexpr auto foldr_elements_impl(F&& f, A const& a, V&& v) -> A {
  return std::forward<F>(f)(std::forward<V>(v), a);
}

template <typename F, typename A, typename V, typename... Vs>
constexpr auto foldr_elements_impl(F&& f, A const& a, V&& v, Vs&&... vs) -> A {
  return foldr_elements_impl(f, std::forward<F>(f)(std::forward<V>(v), a), vs...);
}
} // namespace anonymous

/**
 * \brief Applies functor \bF (at left) for all \bI'th elements of the tuples pack \bTuples
 * 
 * @param I - zero-based index of the element into each tuple
 * @param f - functor to apply, arity of \bf must be 
 *            equal of \bsizeof...(Tuples) or must support currying
 * @param a - initial value
 * @param ts - pack of the tuples to scan
 * @return folded value of type \bA
 */
template <size_t I, typename F, typename A, typename... Tuples>
constexpr auto foldl_elements(F&& f, const A& a, Tuples&&... ts) -> A {
  return foldl_elements_impl(f, a, std::get<I>(std::forward<Tuples>(ts))...);
}

/**
 * \brief Applies functor \bF (at right) for all \bI'th elements of the tuples pack \bTuples
 * 
 * @param I - zero-based index of the element into each tuple
 * @param f - functor to apply, arity of the \bf must be 
 *            equal of \bsizeof...(Tuples) or must be curried
 * @param a - initial value
 * @param ts - pack of the tuples to scan
 * @return folded value of type \bA
 */
template <size_t I, typename F, typename A, typename... Tuples>
constexpr auto foldr_elements(F&& f, const A& a, Tuples&&... ts) -> A {
  return foldr_elements_impl(f, a, std::get<I>(std::forward<Tuples>(ts))...);
}

/**
 * \brief Transposes tuples pack \bTuples, size of each tuple in \bTuples 
 *       must be equal \bsizeof...(Tuples)
 * 
 * @param ts - pack of the tuples to transpose
 * @return transposed tuple of \bTuples
 * 
 * Let \bTuples... are <a, b, c>, <d, e, f>, <g, h, i> then 
 * result is tuple<<a, d, g>, <b, e, h>, <c, f, i>>
 */
template <typename... Tuples>
constexpr auto transpose(Tuples&&... ts) {
  return index_apply<sizeof...(Tuples)>(
    [&](auto... Is) { 
      return std::make_tuple(get_elements<Is>(ts...)...); 
    });
}

namespace {
template <size_t I, typename F, typename A, typename Tuple>
constexpr auto foldl_impl(std::integral_constant<size_t, 1>, F&& f, A const& a, Tuple&& t) -> A {
  return a;
}

template <size_t I, typename F, typename A, typename Tuple>
constexpr auto foldl_impl(std::integral_constant<size_t, I>, F&& f, A const& a, Tuple&& t) -> A {
  return foldl_impl<I - 1>(std::integral_constant<size_t, I>{}, f, 
    std::forward<F>(f)(a, std::get<I - 1>(t)), t);
}

template <size_t I, typename F, typename A, typename Tuple>
constexpr auto foldr_impl(std::integral_constant<size_t, 1>, F&& f, A const& a, Tuple&& t) -> A {
  return a;
}

template <size_t I, typename F, typename A, typename Tuple>
constexpr auto foldr_impl(std::integral_constant<size_t, I>, F&& f, A const& a, Tuple&& t) -> A {
  return foldr_impl<I - 1>(std::integral_constant<size_t, I>{}, f, 
    std::forward<F>(f)(std::get<I - 1>(t), a), t);
}
} // namespace anonymous

/**
 * \brief Folds tuple values at left.
 * 
 * @param f - functor to apply, arity of \bf must be 
 *            equal of size of \bTuple or must be curried
 * @param a - initial value
 * @param t - tuple to scan
 * @return folded value of type \bA
 */
template <typename F, typename A, typename Tuple>
constexpr auto foldl(F&& f, A const& a, Tuple&& t) -> A {
  constexpr size_t I = std::tuple_size<Tuple>::value;
  return foldl_impl<I>(std::integral_constant<size_t, I>{}, f, a, t);
}

/**
 * \brief Folds tuple values at right.
 * 
 * @param f - functor to apply, arity of \bf must be 
 *            equal of size of Tuple or must be curried
 * @param a - initial value
 * @param t - tuple to scan
 * @return folded value of type \bA
 */
template <typename F, typename A, typename Tuple>
constexpr auto foldr(F&& f, A const& a, Tuple&& t) -> A {
  constexpr size_t I = std::tuple_size<Tuple>::value;
  return foldr_impl<I>(std::integral_constant<size_t, I>{}, f, a, t);
}

/**
 * \brief Folds \bTuples... at left by using \bF, sizes of all tuples 
 *       in \bTuples... must be equal.
 * 
 * @param f - functor to apply, arity of \bf must be 
 *            equal of sizeof...(Tuples) or must be curried
 * @param a - initial value
 * @param ts - pack of the tuples to scan
 * @return array of folded values of type \bA
 */
template <typename F, typename A, typename... Tuples>
constexpr auto foldl_tuples(F&& f, A const& a, Tuples&&... ts) ->
    std::array<A, sizeof...(Tuples)> {
  return index_apply<sizeof...(Tuples)>(
    [&](auto... Is) -> A { 
      return {foldl_elements<Is>(f, a, ts...)...};
    });
}

/**
 * \brief Folds \bTuples... at right by using \bF, sizes of all tuples 
 *       in \bTuples... must be equal.
 * 
 * @param f - functor to apply, arity of \bf must be 
 *            equal of \bsizeof...(Tuples) or must be curried
 * @param a - initial value
 * @param ts - pack of the tuples to scan
 * @return array of folded values of type \bA
 */
template <typename F, typename A, typename... Tuples>
constexpr auto foldr_tuples(F&& f, A const& a, Tuples&&... ts) -> 
    std::array<A, sizeof...(Tuples)> {
  return index_apply<sizeof...(Tuples)>(
    [&](auto... Is) -> A { 
      return {foldr_elements<Is>(f, a, ts...)...};
    });
}

/**
 * \brief Compares \bTuples... by using \bF, sizes of all tuples 
 *       in \bTuples... must be equal.
 * 
 * @param f - comparator to apply, arity of \bf must be 
 *            equal of \bsizeof...(Tuples) or must be curried
 * @param a - initial value
 * @param ts - pack of the tuples to scan
 * @return array of bools
 */
template <typename F, typename... Tuples> // TODO: replace with foldX_tuples
constexpr auto compare_tuples(F&& f, Tuples&&... ts) -> 
    std::array<bool, sizeof...(Tuples)> {
  return index_apply<sizeof...(Tuples)>(
    [&](auto... Is) -> bool {
      return {compare_elements<Is>(f, ts...)...};
    });
}

/**
 * \brief Packs argument pack to tuple.
 */
template <typename... Ts> using pack = std::tuple<Ts...>;

/**
 * \brief Unpacks \bTuple<...> to type \bT<..., Ts...>.
 * 
 * @return \bT<..., Ts...>{}
 */
template <template <typename...> class T, typename Tuple, typename... Ts>
constexpr auto unpack_at_begin() {
  return index_apply<std::tuple_size<Tuple>{}>(
    [&](auto... Is) { 
      return T<typename std::tuple_element<Is, Tuple>::type..., Ts...>{}; 
    });
}

/**
 * \brief Unpacks \bTuple<...> to type \bT<Ts..., ...>.
 * 
 * @return \bT<Ts..., ...>{}
 */
template <template <typename...> class T, typename Tuple, typename... Ts>
constexpr auto unpack_at_end() {
  return index_apply<std::tuple_size<Tuple>{}>(
    [&](auto... Is) { 
      return T<Ts..., typename std::tuple_element<Is, Tuple>::type...>{}; 
    });
}

/**
 * \brief Unpacks \bTuple<...> to type \bT<...>.
 * 
 * @return T<...>{}
 */
template <template <typename...> class T, typename Tuple>
constexpr auto unpack() {
  return unpack_at_end<T, Tuple>();
}

} // tuples

namespace fn {

/**
 * \brief Function details description
 */
template<typename Sign, typename R, typename... Args>
struct traits_base {
  using signature = Sign;
  using cpp11_signature = R(Args...);
  using is_method = std::is_member_function_pointer<signature>;
  using args = std::tuple<Args...>;
  using arg_types = std::tuple<std::decay_t<Args>...>;
  using arity = std::integral_constant<unsigned, sizeof...(Args)>;
  using result = R;
};

/**
 * \brief Function details specializations
 */
template<typename T>
struct traits : traits<decltype(&T::operator())> {};

template<typename R, typename... Args>
struct traits<R(*)(Args...)> : traits_base<R(*)(Args...), R, Args...> {};
template<typename R, typename... Args>
struct traits<R(* const)(Args...)> : traits_base<R(* const)(Args...), R, Args...> {};
template<typename R, typename... Args>
struct traits<R(*)(Args...) noexcept> : traits_base<R(*)(Args...), R, Args...> {};
template<typename R, typename... Args>
struct traits<R(* const)(Args...) noexcept> : traits_base<R(* const)(Args...), R, Args...> {};
template<typename R, typename C, typename... Args>
struct traits<R(C::*)(Args...)> : traits_base<R(C::*)(Args...), R, Args...> {};
template<typename R, typename C, typename... Args>
struct traits<R(C::* const)(Args...)> : traits_base<R(C::* const)(Args...), R, Args...> {};
template<typename R, typename C, typename... Args>
struct traits<R(C::*)(Args...) const> : traits_base<R(C::*)(Args...) const, R, Args...> {};
template<typename R, typename C, typename... Args>
struct traits<R(C::* const)(Args...) const> : traits_base<R(C::* const)(Args...) const, R, Args...> {};
template<typename R, typename C, typename... Args>
struct traits<R(C::*)(Args...) noexcept> : traits_base<R(C::*)(Args...), R, Args...> {};
template<typename R, typename C, typename... Args>
struct traits<R(C::* const)(Args...) noexcept> : traits_base<R(C::* const)(Args...), R, Args...> {};
template<typename R, typename C, typename... Args>
struct traits<R(C::*)(Args...) const noexcept> : traits_base<R(C::*)(Args...) const, R, Args...> {};
template<typename R, typename C, typename... Args>
struct traits<R(C::* const)(Args...) const noexcept> : traits_base<R(C::* const)(Args...) const, R, Args...> {};

} // namespace fn

namespace ct {

enum class KindOfContainer { ckUndefined, ckStructure, ckStlContainer, 
  ckStlSingleContainer, ckStlSingleFixedContainer, ckStlPairContainer };

template<typename T>
struct structure:std::false_type { 
  static constexpr KindOfContainer kind = KindOfContainer::ckStructure; 
  template<typename U> using type = T; 
};
template <template<typename...> class C, typename... Ts> 
struct single:std::true_type { 
  static constexpr KindOfContainer kind = KindOfContainer::ckStlSingleContainer; 
  template<typename U> using type = C<U, std::allocator<U>>; 
};
template <template<typename, std::size_t> class C, typename T, std::size_t N> 
struct fixed:std::true_type { 
  static constexpr KindOfContainer kind = KindOfContainer::ckStlSingleFixedContainer; 
  template<typename U> using type = C<U, N>; 
};
template <template<typename...> class C, typename K, typename... Ts> 
struct mapping:std::true_type { 
  static constexpr KindOfContainer kind = KindOfContainer::ckStlPairContainer; 
  template<typename U> using type = C<K, U, std::allocator<std::pair<const K, U>>>; 
};

template <typename T> struct container_traits:structure<T>{};
template <typename T, std::size_t N> struct container_traits< 
    std::array<T,N>>:fixed<std::array, T, N>{};
template <typename... Args> struct container_traits< 
    std::vector<Args...>>:single<std::vector, Args...>{};
template <typename... Args> struct container_traits<
    std::deque<Args...>>:single<std::deque, Args...>{};
template <typename... Args> struct container_traits<
    std::list<Args...>>:single<std::list, Args...>{};
template <typename... Args> struct container_traits<
    std::forward_list<Args...>>:single<std::forward_list, Args...>{};
template <typename... Args> struct container_traits<
    std::set<Args...>>:single<std::set, Args...>{};
template <typename... Args> struct container_traits<
    std::multiset<Args...>>:single<std::multiset, Args...>{};
template <typename... Args> struct container_traits<
    std::map<Args...>>:single<std::map, Args...>{};
template <typename... Args> struct container_traits<
    std::multimap<Args...>>:single<std::multimap, Args...>{};
template <typename... Args> struct container_traits<
    std::unordered_set<Args...>>:single<std::unordered_set, Args...>{};
template <typename... Args> struct container_traits<
    std::unordered_multiset<Args...>>:single<std::unordered_multiset, Args...>{};
template <typename... Args> struct container_traits<
    std::unordered_map<Args...>>:mapping<std::unordered_map, Args...>{};
template <typename... Args> struct container_traits<
    std::unordered_multimap<Args...>>:mapping<std::unordered_multimap, Args...>{};
template <typename... Args> struct container_traits<
    std::stack<Args...>>:single<std::stack, Args...>{};
template <typename... Args> struct container_traits<
    std::queue<Args...>>:single<std::queue, Args...>{};
template <typename... Args> struct container_traits<
    std::priority_queue<Args...>>:single<std::priority_queue, Args...>{};

template <typename F, template <typename...> class C, typename... Ts>
auto foldr_impl(F&& f, typename C<Ts...>::value_type const& a, 
    C<Ts...>&& c, typename C<Ts...>::const_iterator it) -> typename C<Ts...>::value_type {
  if(it == c.end()) {
      return a;
  }
  return foldr_impl(f, f(*it, a), std::forward<C<Ts...>>(c), std::next(it));
}

template <typename F, template <typename...> class C, typename... Ts>
auto foldr(F&& f, typename C<Ts...>::value_type const& a, C<Ts...>&& c) 
    -> typename C<Ts...>::value_type {
  return foldr_impl(f, f(*c.begin(), a), std::forward<C<Ts...>>(c), std::next(c.begin()));
}

template <typename F, template <typename...> class C, typename... Ts>
auto foldl_impl(F&& f, typename C<Ts...>::value_type const& a, 
    C<Ts...>&& c, typename C<Ts...>::const_iterator it) -> typename C<Ts...>::value_type {
  if(it == c.end()) {
      return a;
  }
  return foldl_impl(f, f(a, *it), std::forward<C<Ts...>>(c), std::next(it));
}

template <typename F, template <typename...> class C, typename... Ts>
auto foldl(F&& f, typename C<Ts...>::value_type const& a, C<Ts...>&& c) 
    -> typename C<Ts...>::value_type {
  return foldl_impl(f, f(a, *c.begin()), std::forward<C<Ts...>>(c), std::next(c.begin()));
}

} // namespace containers

} // namespace util
} // namespace metaf

#endif /* TYPE_CONSTRS_UTIL_H */

