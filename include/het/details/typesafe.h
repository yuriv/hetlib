//
// Created by yuri on 3/16/23.
//

#ifndef HETLIB_TYPESAFE_H
#define HETLIB_TYPESAFE_H

namespace metaf::util {

template <typename T> consteval auto type_or_ref() noexcept ->
    std::conditional_t<std::is_lvalue_reference_v<T>,
    std::conditional_t<std::is_const_v<std::remove_reference_t<T>>,
    std::reference_wrapper<std::remove_cvref_t<const T>>,
    std::reference_wrapper<std::remove_cvref_t<T>>
>, T> {return{};}

template <typename T> using safe_ref = decltype(type_or_ref<T>());

} // namespace metf::util

#endif //HETLIB_TYPESAFE_H
