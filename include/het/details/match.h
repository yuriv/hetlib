/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   match.h
 * Author: yuri
 *
 * Created on September 19, 2017, 1:32 PM
 */

#ifndef STAGES_MATCH_H
#define STAGES_MATCH_H

#include <type_traits>
#include <tuple>

namespace stg {
namespace util {

/// \brief Retrieve function traits (\see struct meta::util::fn::traits_base).
//template <typename T> using fn_traits = metaf::util::fn::traits<T>;
//template <typename T> using fn_signature = typename fn_traits<T>::signature;
//template <typename T> using fn_cpp11signature = typename fn_traits<T>::cpp11_signature;
//template <typename T> using fn_args = typename fn_traits<T>::args;
//template <typename T> using fn_arg_types = typename fn_traits<T>::arg_types;

/// \brief Evaluates return value of function in compile-time.
template <typename F, typename... Args>
using fn_evaluate_t = decltype(std::declval<F>()(std::declval<Args>()...));

/// \brief Evaluates return value of member function in compile-time.
template <typename C, typename F, typename... Args>
using fn_member_evaluate_t = 
  decltype(((std::declval<C>()).*std::declval<F>())(std::declval<Args>()...));

template<typename F, typename... Args>
struct fn_is_applicable_fn {
private:
  template <typename, typename = std::void_t<>>
  struct check : std::false_type {};

  template <typename T>
  struct check<T, std::void_t<fn_evaluate_t<T, Args...>>> : std::true_type {};

public:
  using type = check<F>;
  static constexpr bool value = type::value;
};

template<typename F, typename C, typename... Args>
struct fn_is_member_applicable_fn {
private:
  template <typename, typename = std::void_t<>>
  struct check : std::false_type {};

  template <typename T>
  struct check<T, std::void_t<
    fn_member_evaluate_t<C, T, Args...>>> : std::true_type {};

public:
  typedef check<F> type;
  static constexpr bool value = type::value;
};

template<typename F, typename... Args>
struct fn_is_applicable {
private:
  static constexpr bool check_impl(std::true_type) {
    return fn_is_member_applicable_fn<F, Args...>::value;
  }
  static constexpr bool check_impl(std::false_type) {
    return fn_is_applicable_fn<F, Args...>::value;
  }
  static constexpr bool check() {
    return check_impl(std::is_member_function_pointer<F>{});
  }

public:
  typedef typename std::integral_constant<bool, check()>::type type;
  static constexpr bool value = type::value;
};

template<typename... Args>
struct fn_select_applicable {
private:
  template <typename R, typename F, typename... Fs>
  static constexpr decltype(auto) check_impl(R, F, Fs...) {
    static_assert(R::value && !R::value, "can't deduce applicable case, check list of available cases.");
  }
  template <typename F, typename... Fs>
  static constexpr decltype(auto) continuator(std::false_type, F, Fs... fs) {
    return check(fs...);
  }
  template <typename F>
  static constexpr decltype(auto) continuator(std::false_type, F) {
    return [](auto...){ static_assert(!std::is_same_v<F, F>, "alternative not found"); };
  }
  template <typename F, typename... Fs>
  static constexpr decltype(auto) continuator(std::true_type, F f, Fs...) {
    return f;
  }
  template <typename F>
  static constexpr decltype(auto) continuator(std::true_type, F f) {
    return f;
  }
  template <typename F, typename... Fs>
  static constexpr decltype(auto) check_impl(std::false_type, F f, Fs... fs) {
    using match = typename fn_is_applicable_fn<F, Args...>::type;
    return continuator(match{}, f, fs...);
  }

public:
  template <typename... Fs>
  static constexpr decltype(auto) check(Fs... fs) {
    return check_impl(std::false_type{}, fs...);
  }
};

template<typename F, typename Ret, typename... Args>
struct fn_is_eligible_fn {
private:
  template<typename T>
  static constexpr auto check(T&& t) -> typename
    std::is_same<
      decltype(std::forward<T>(t)(std::declval<Args>()...)),
      Ret
    >::type; 

  template<typename T=void>
  static constexpr std::false_type check(...);

public:
  typedef decltype(check(std::declval<F>())) type;
  static constexpr bool value = type::value;
};

// specialization that does the substitution checking
template<typename F, typename Ret, typename... Args>
struct fn_is_eligible_fn<F, Ret(Args...)> {
private:
  template<typename T>
  static constexpr auto check(T&& t) -> typename
    std::is_same<
      decltype(std::forward<T>(t)(std::declval<Args>()...)),
      Ret
    >::type; 

  template<typename T=void>
  static constexpr std::false_type check(...);

public:
  typedef decltype(check(std::declval<F>())) type;
  static constexpr bool value = type::value;
};

template<typename F, typename Ret, typename C, typename... Args>
struct fn_is_member_eligible_fn {
private:
  template<typename T>
  static constexpr auto check(T* t) -> typename
    std::is_same<
      decltype((t->*std::declval<F>())(std::declval<Args>()... )),
      Ret
    >::type; 

  template<typename>
  static constexpr std::false_type check(...);

public:
  typedef decltype(check<C>(0)) type;
  static constexpr bool value = type::value;
};

// specialization that does the substitution checking
template<typename F, typename Ret, typename C, typename... Args>
struct fn_is_member_eligible_fn<F, C, Ret(Args...)> {
private:
  template<typename T>
  static constexpr auto check(T* t) -> typename
    std::is_same<
      decltype((t->*std::declval<F>())(std::declval<Args>()... )),
      Ret
    >::type;

  template<typename>
  static constexpr std::false_type check(...);

public:
  typedef decltype(check<C>(0)) type;
  static constexpr bool value = type::value;
};

template<typename F, typename Ret, typename... Args>
struct fn_is_eligible {
private:
  static constexpr bool check_impl(std::true_type) {
    return fn_is_member_eligible_fn<F, Ret, Args...>::value;
  }
  static constexpr bool check_impl(std::false_type) {
    return fn_is_eligible_fn<F, Ret, Args...>::value;
  }
  static constexpr bool check() {
    return check_impl(std::is_member_function_pointer<F>{});
  }

public:
  typedef typename std::integral_constant<bool, check()>::type type;
  static constexpr bool value = type::value;
};

template<typename F, typename Ret, typename... Args>
struct fn_is_eligible<F, Ret(Args...)> {
private:
  static constexpr bool check_impl(std::true_type) {
    using C = std::tuple_element_t<0, std::tuple<Args...>>;
    using S = Ret(Args...);
    return fn_is_member_eligible_fn<F, C, S>::value;
  }
  static constexpr bool check_impl(std::false_type) {
    return fn_is_eligible_fn<F, Ret(Args...)>::value;
  }
  static constexpr bool check() {
    return check_impl(std::is_member_function_pointer<F>{});
  }

public:
  typedef typename std::integral_constant<bool, check()>::type type;
  static constexpr bool value = type::value;
};

template<typename Ret, typename... Args>
struct fn_select_eligible {
private:
  template <typename R, typename F, typename... Fs>
  static constexpr decltype(auto) check_impl(R, F, Fs...) {
    static_assert(R::value && !R::value, 
      "Error: can't deduce eligible function, check "
      "list of available functions.");
  }
  template <typename F, typename... Fs>
  static constexpr decltype(auto) continuator(std::false_type, F f, Fs... fs) {
    return check(fs...);
  }
  template <typename F>
  static constexpr decltype(auto) continuator(std::false_type, F) {
    return [](auto...){ return int(-1); };
  }
  template <typename F, typename... Fs>
  static constexpr decltype(auto) continuator(std::true_type, F f, Fs... fs) {
    return f;
  }
  template <typename F>
  static constexpr decltype(auto) continuator(std::true_type, F f) {
    return f;
  }
  template <typename F, typename... Fs>
  static constexpr decltype(auto) check_impl(std::false_type, F f, Fs... fs) {
    using match = typename fn_is_eligible<F, Ret, Args...>::type;
    return continuator(match{}, f, fs...);
  }

public:
  template <typename... Fs>
  static constexpr decltype(auto) check(Fs... fs) {
    return check_impl(std::false_type{}, fs...);
  }
};

/// \brief Checks type \bT if is it \bstd::tuple
//template <typename T> constexpr bool is_tuple = false;
//template <typename... Ts> constexpr bool is_tuple<std::tuple<Ts...>> = true;
//template <typename... Ts> using is_tuple_t = std::bool_constant<is_tuple<Ts...>>;
//
//template <typename F, typename... Args>
//constexpr auto fn_run_with_untupled_args(F && f, Args &&... args) noexcept -> std::invoke_result_t<F, Args...> {
//  static_assert(std::invocable<F, Args...>, "Can't invoke function with arguments provided.");
//  return std::invoke(f, std::forward<Args>(args)...);
//}
//
//template <typename F, typename... Args, std::size_t... Is>
//constexpr auto fn_run_with_tupled_args(F && f, std::tuple<Args...> && tupled_args) noexcept -> std::invoke_result_t<F, std::tuple<Args...>> {
//  static_assert(std::invocable<F, std::tuple<Args...>>, "Can't invoke function with arguments provided.");
//  return std::invoke(f, std::forward<std::tuple<Args...>>(tupled_args));
////  return std::forward<F>(f)(std::get<Is>(std::forward<std::tuple<Args...>>(tupled_args))...);
//}
//
//template <typename Tuple, typename... Fs, std::size_t... Is>
//constexpr auto fn_try_with_tupled_args(std::index_sequence<Is...> &&, Fs &&... fs) noexcept ->
//  decltype(util::fn_select_applicable<std::decay_t<std::tuple_element_t<Is, Tuple>>...>::check(std::forward<Fs>(fs)...)) {
//  static_assert(std::invocable<decltype(util::fn_select_applicable<std::decay_t<std::tuple_element_t<Is, Tuple>>...>::check(std::forward<Fs>(fs)...)), std::tuple_element_t<Is, Tuple>...>,
//                "There are no candidates to apply with arguments provided.");
//  return {}; //util::fn_select_applicable<std::decay_t<std::tuple_element_t<Is, Tuple>>...>::check(std::forward<Fs>(fs)...);
//}
//
//template <typename Arg, typename... Fs>
//constexpr auto fn_try_with_untupled_args(std::false_type, Fs&&... fs) noexcept ->
//  decltype(util::fn_select_applicable<std::decay_t<Arg>>::check(std::forward<Fs>(fs)...)) {
////  static_assert(!std::is_void<decltype(util::fn_select_applicable<std::decay_t<Arg>>::check(std::forward<Fs>(fs)...))>::value,
////                "There are no candidates to apply with arguments provided.");
////  auto i = util::fn_select_applicable<std::decay_t<Arg>>::check(std::forward<Fs>(fs)...);
////  int k = i(std::declval<Arg>());
////  int j = std::declval<Arg>();
//  static_assert(std::invocable<decltype(util::fn_select_applicable<std::decay_t<Arg>>::check(std::forward<Fs>(fs)...)), Arg>,
//                "There are no candidates to apply with arguments provided.");
//  return {}; //util::fn_select_applicable<std::decay_t<Arg>>::check(std::forward<Fs>(fs)...);
//}
//
//template <typename Arg, typename... Fs>
//constexpr auto fn_try_with_untupled_args(std::true_type, Fs&&... fs) noexcept {
////  return fn_try_with_tupled_args<Arg>(std::make_index_sequence<std::tuple_size<Arg>::value>{}, std::forward<Fs>(fs)...);
//  return fn_try_with_untupled_args<Arg>(std::false_type{}, std::forward<Fs>(fs)...);
//}
//
//template<unsigned N, typename F> constexpr bool fn_is_arity_n = bool(fn_traits<std::decay_t<F>>::arity::value == N);
//template<unsigned N, typename... Fs> constexpr bool fn_is_arities_n = std::conjunction_v<std::bool_constant<fn_is_arity_n<N, Fs>>...>;
//template<unsigned N, typename F> using fn_is_arity_t = std::bool_constant<fn_is_arity_n<N, F>>;
//
//template<typename F>
//constexpr auto fn_tuple_args(F&&, std::true_type) {
//  return fn_arg_types<std::decay_t<F>>{};
////  return std::tuple_element_t<0, fn_arg_types<std::decay_t<F>>>{};
//}
//
//template<typename F>
//constexpr auto fn_tuple_args(F&&, std::false_type) {
//  return fn_arg_types<std::decay_t<F>>{};
//}
//
//template<typename... Fs>
//constexpr auto fn_args_ripper(Fs&&... fs) {
//  return std::make_tuple(fn_tuple_args(fs, fn_is_arity_t<1, Fs>{})...);
//}

} // namespace util
} // namespace stg

#endif /* STAGES_MATCH_H */
