#pragma once

#include "stdafx.h"

template <class T>
class OptionalNameValuePair {
  private:
    using Type = typename std::conditional<std::is_array<typename std::remove_reference<T>::type>::value,
                                           typename std::remove_cv<T>::type,
                                           typename std::conditional<std::is_lvalue_reference<T>::value,
                                                                     T,
                                                                     typename std::decay<T>::type>::type>::type;

    OptionalNameValuePair & operator=( OptionalNameValuePair const & ) = delete;

  public:
    OptionalNameValuePair( char const * n, T && v ) : name(n), value(std::forward<T>(v)) {}

    char const * name;
    Type value;
};

template <typename Archive, typename T>
void serialize(Archive& ar1, OptionalNameValuePair<T>& elem) {
  ar1(elem.value);
}

template<class T> inline
OptionalNameValuePair<T>
make_optional_nvp( const char * name, T && value) {
  return {name, std::forward<T>(value)};
}


template <class T>
class SkipPrettyValue {
  private:
    using Type = typename std::conditional<std::is_array<typename std::remove_reference<T>::type>::value,
                                           typename std::remove_cv<T>::type,
                                           typename std::conditional<std::is_lvalue_reference<T>::value,
                                                                     T,
                                                                     typename std::decay<T>::type>::type>::type;

    SkipPrettyValue & operator=( SkipPrettyValue const & ) = delete;

  public:
    SkipPrettyValue(T && v) : value(std::forward<T>(v)) {}
    Type value;
};

template <typename Archive, typename T>
void serialize(Archive& ar1, SkipPrettyValue<T>& elem) {
  ar1(elem.value);
}

template<class T> inline
SkipPrettyValue<T>
make_skip_value(T && value) {
  return {std::forward<T>(value)};
}
