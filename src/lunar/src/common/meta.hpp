
// Copyright (C) 2022 fleroviux

#pragma once

#include <utility>
#include <type_traits>

namespace lunar {

namespace detail {
template <typename T, T Begin,  class Func, T ...Is>
constexpr void static_for_impl( Func &&f, std::integer_sequence<T, Is...> ) {
  ( f( std::integral_constant<T, Begin + Is>{ } ),... );
}
} // namespace lunar::detail

template <typename T, T Begin, T End, class Func >
constexpr void static_for( Func &&f ) {
  detail::static_for_impl<T, Begin>( std::forward<Func>(f), std::make_integer_sequence<T, End - Begin>{ } );
}

template<typename T, typename... Args>
struct is_one_of {
  static constexpr bool value = (... || std::is_same_v<T, Args>);
};

template<typename T, typename... Args>
inline constexpr bool is_one_of_v = is_one_of<T, Args...>::value;

} // namespace lunar
