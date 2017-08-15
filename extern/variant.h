#pragma once

#include "variant.hpp"
#include "optional.h"

namespace variant_helpers {

template <typename... Lambdas>
struct LambdaVisitor;

template <typename Lambda1, typename... Lambdas>
struct LambdaVisitor<Lambda1 , Lambdas...> : public Lambda1, public LambdaVisitor<Lambdas...> {
    using Lambda1::operator();
    using LambdaVisitor<Lambdas...>::operator();
    LambdaVisitor(Lambda1 l1, Lambdas... lambdas)
      : Lambda1(l1), LambdaVisitor<Lambdas...> (lambdas...)
    {}
};

template <typename Lambda1>
struct LambdaVisitor<Lambda1> : public Lambda1 {
    using Lambda1::operator();
    LambdaVisitor(Lambda1 l1) :Lambda1(l1)
    {}
};

template<typename Ret, typename Arg, typename... Rest>
Arg first_argument_helper(Ret(*) (Arg, Rest...));

template<typename Ret, typename F, typename Arg, typename... Rest>
Arg first_argument_helper(Ret(F::*) (Arg, Rest...));

template<typename Ret, typename F, typename Arg, typename... Rest>
Arg first_argument_helper(Ret(F::*) (Arg, Rest...) const);

template <typename F>
decltype(first_argument_helper(&F::operator())) first_argument_helper(F);

template <typename T>
using first_argument = decltype(first_argument_helper(std::declval<T>()));

template <int N, typename Variant, class F1>
auto callFun(Variant&& v, F1&& f1) {
  using arg = typename std::remove_cv_t<std::remove_reference_t<first_argument<F1>>>;
  static_assert(std::is_same<arg, stx::variant_alternative_t<N, typename std::remove_reference_t<Variant>::base>>::value,
      "Bad function argument");
  return std::forward<F1>(f1)(stx::get<N>(std::forward<Variant>(v)));
}

template<int N, typename Variant, class FLast,
    typename = typename std::enable_if<N == std::remove_reference_t<Variant>::num_types - 1, void>::type>
auto matchN(Variant&& v, FLast&& f) {
  return callFun<N>(std::forward<Variant>(v), std::forward<FLast>(f));
}

template<int N, typename Variant, class F1,
    typename = typename std::enable_if<N + 1 <= std::remove_reference_t<Variant>::num_types - 1, void>::type,
    typename ... Fs>
auto matchN(Variant&& v, F1&& f1, Fs&&... fs) {
  if (N == v.index()) {
    return callFun<N>(std::forward<Variant>(v), std::forward<F1>(f1));
  } else
    return matchN<N+1>(std::forward<Variant>(v), std::forward<Fs>(fs)...);
}

}

template <typename... Lambdas>
variant_helpers::LambdaVisitor<Lambdas...> makeVisitor(Lambdas... lambdas) {
  return variant_helpers::LambdaVisitor<Lambdas...>(lambdas...);
}

template <typename ...Arg>
class variant : public stx::variant<Arg...> {
  public:
  using base = stx::variant<Arg...>;
  using stx::variant<Arg...>::variant;
  constexpr static int num_types = stx::variant_size<base>::value;

  template<typename... Fs>
  auto match(Fs&&... fs) const {
    return variant_helpers::matchN<0>(*this, std::forward<Fs>(fs)...);
  }

  template<typename... Fs>
  auto match(Fs&&... fs) {
    return variant_helpers::matchN<0>(*this, std::forward<Fs>(fs)...);
  }

  template<typename... Fs>
  auto visit(Fs... fs) {
    auto f = variant_helpers::LambdaVisitor<Fs...>(fs...);
    return stx::visit(f, *this);
  }

  template<typename... Fs>
  auto visit(Fs... fs) const {
    auto f = variant_helpers::LambdaVisitor<Fs...>(fs...);
    return stx::visit(f, *this);
  }

  template<typename T>
  bool contains() const {
    return stx::__type_index<T, Arg...>::__value == this->index();
  }

  template<typename T>
  optional<T&> getReferenceMaybe() {
    if (auto val = stx::get_if<T>(*this))
      return *val;
    else
      return none;
  }

  template<typename T>
  optional<const T&> getReferenceMaybe() const {
    if (auto val = stx::get_if<T>(*this))
      return *val;
    else
      return none;
  }

  template<typename T>
  optional<T> getValueMaybe() const {
    if (auto val = stx::get_if<T>(*this))
      return *val;
    else
      return none;
  }

  size_t getHash() const {
    return visit([&](const auto& elem) { return combineHash(this->index(), elem); });
  }

};

template<typename ...Arg>
inline std::ostream& operator <<(std::ostream& d, const variant<Arg...>& v) {
  v.visit([&](const auto& elem) { d << elem; });
  return d;
}
