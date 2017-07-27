// Original code from here: https://github.com/YeyaSwizaw/cpp-enum-variant
// Modified by Michal Brzozowski as part of KeeperRL
//
//////////////////////////////////////////////////////////////////////////////
//  File: cpp-enum-variant/enum.hpp
//////////////////////////////////////////////////////////////////////////////
//  Copyright 2017 Samuel Sleight
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//////////////////////////////////////////////////////////////////////////////


#pragma once

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "extern/optional.h"

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

template <typename... Lambdas>
LambdaVisitor<Lambdas...> makeVisitor(Lambdas... lambdas) {
  return LambdaVisitor<Lambdas...>(lambdas...);
}

namespace venum {

// Index of T in Ts
template<typename, typename...>
struct IndexOf;

template<typename T, typename U, typename... Ts>
struct IndexOf<T, U, Ts...> {
    static constexpr int value = 1 + IndexOf<T, Ts...>::value;
};

template<typename T, typename... Ts>
struct IndexOf<T, T, Ts...> {
    static constexpr int value = 0;
};

// Variadic Max
template<typename T>
constexpr auto const_max(T a) {
    return a;
}

template<typename T, typename... Args>
constexpr T const_max(T a, T b, Args... args) {
    return a > b ? const_max(a, args...) : const_max(b, args...);
}

// Variadic Or
template<typename... Args>
struct Or;

template<typename T, typename... Args>
struct Or<T, Args...> {
    static constexpr bool value = T::value || Or<Args...>::value;
};

template<typename T>
struct Or<T> {
    static constexpr bool value = T::value;
};

// TypeList
template<typename T, std::size_t n>
struct NthImpl : public NthImpl<typename T::Tail, n - 1> {};

template<typename T>
struct NthImpl<T, 0> {
    using value = typename T::Head;
};

template<typename...>
struct TypeList {};

template<typename H, typename... Ts>
struct TypeList<H, Ts...> {
    using Head = H;
    using Tail = TypeList<Ts...>;

    template<std::size_t n>
    using Nth = typename NthImpl<TypeList<H, Ts...>, n>::value;
};

// Enum implementation
template<typename VariantT, typename... Variants>
class EnumT {
public:
    static constexpr std::size_t storage_size = const_max(sizeof(VariantT), sizeof(Variants)...);
    static constexpr std::size_t storage_align = const_max(alignof(VariantT), alignof(Variants)...);

    static constexpr std::size_t variants = sizeof...(Variants)+1;

private:
    using Self = EnumT<VariantT, Variants...>;
    using VariantList = TypeList<VariantT, Variants...>;

    using StorageT = typename std::aligned_storage<storage_size, storage_align>::type;

    // Implementation detail
    struct impl {
        // Constructor
        template<template<typename...> class Check, bool enable, std::size_t n, typename... Args>
        struct ConstructorT;

        template<template<typename...> class Check, std::size_t n, typename... Args>
        struct ConstructorT<Check, false, n, Args...>
            : public ConstructorT<Check, Check<typename Self::VariantList::template Nth<n + 1>, Args...>::value, n + 1, Args...> {};

        template<template<typename...> class Check, std::size_t n, typename... Args>
        struct ConstructorT<Check, true, n, Args...> {
            static void construct(Self* e) {
                using T = typename Self::VariantList::template Nth<n>;
                ::new (&(e->storage)) T();
                e->tag = n;
            }
            static void construct(Self* e, Args&&... args) {
                using T = typename Self::VariantList::template Nth<n>;
                ::new (&(e->storage)) T(std::forward<Args>(args)...);
                e->tag = n;
            }
            static void construct(Self* e, const Args&... args) {
                using T = typename Self::VariantList::template Nth<n>;
                ::new (&(e->storage)) T(args...);
                e->tag = n;
            }
        };

        // Type Check for Constructor
        template<typename T, typename... Args>
        struct TypeCheck {
            static constexpr bool value = false;
        };

        template<typename T, typename U>
        struct TypeCheck<T, U> {
            static constexpr bool value = std::is_same<typename std::decay<T>::type, typename std::decay<U>::type>::value && std::is_constructible<T, U>::value;
        };

        // Helper
        template<std::size_t n, std::size_t m, template<typename, std::size_t> class F, typename... Args>
        struct HelperT {
            static auto call(const std::size_t& tag, Args... args) {
                using T = typename Self::VariantList::template Nth<n>;

                if(tag == n) {
                    return F<T, n>::call(std::forward<Args>(args)...);
                } else {
                    return HelperT<n + 1, m, F, Args...>::call(tag, std::forward<Args>(args)...);
                }
            }
        };

        template<std::size_t n, template<typename, std::size_t> class F, typename... Args>
        struct HelperT<n, n, F, Args...> {
            static auto call(const std::size_t& tag, Args... args) {
                using T = typename Self::VariantList::template Nth<n>;

                if(tag == n) {
                    return F<T, n>::call(std::forward<Args>(args)...);
                } else {
                    throw std::runtime_error("Invalid tag, something has gone horribly wrong");
                }
            }
        };

        template<template<typename, std::size_t> class F, typename... Args>
        using Helper = HelperT<0, Self::variants - 1, F, Args...>;

        // Copy Constructor
        template<typename T, std::size_t n>
        struct CopyConstructorT {
            static void call(const Self& from, Self* to) {
                to->tag = n;
                ::new (&(to->storage)) T(*reinterpret_cast<const T*>(&(from.storage)));
            }
        };

        // Move Constructor
        template<typename T, std::size_t n>
        struct MoveConstructorT {
            static void call(Self&& from, Self* to) {
                to->tag = std::move(n);
                ::new (&(to->storage)) T(std::move(*reinterpret_cast<T*>(&(from.storage))));
            }
        };

        // Destructor
        template<typename T, std::size_t n>
        struct DestructorT {
            static void call(Self* e) {
                reinterpret_cast<T*>(&(e->storage))->~T();
            }
        };

        // Apply
        template<typename T, std::size_t n>
        struct ApplyT {
            template<typename F>
            static auto call(Self* e, F f) {
                return f(*reinterpret_cast<T*>(&(e->storage)));
            }
            template<typename F>
            static auto call(const Self* e, F f) {
                return f(*reinterpret_cast<const T*>(&(e->storage)));
            }
        };

        // Match
        template<typename T, std::size_t n, typename... Fs>
        struct CallNth;

        template<typename T, std::size_t n, typename F, typename... Fs>
        struct CallNth<T, n, F, Fs...> {
            static auto call(T t, F f, Fs... fs) {
                return CallNth<T, n - 1, Fs...>::call(t, std::forward<Fs>(fs)...);
            }
        };

        template<typename T, typename F, typename... Fs>
        struct CallNth<T, 0, F, Fs...> {
            static auto call(T t, F f, Fs... fs) {
                return f(t);
            }
        };

        template<typename T, std::size_t n>
        struct MatchT {
            template<typename... Fs>
            static auto call(Self* e, Fs... fs) {
                return CallNth<T, n, Fs...>::call(*reinterpret_cast<T*>(&(e->storage)), std::forward<Fs>(fs)...);
            }
            template<typename... Fs>
            static auto call(const Self* e, Fs... fs) {
                return CallNth<T, n, Fs...>::call(*reinterpret_cast<const T*>(&(e->storage)), std::forward<Fs>(fs)...);
            }
        };
    };

    template<typename... Args>
    using Constructor = typename std::conditional<
        Or<typename impl::template TypeCheck<VariantT, Args...>, typename impl::template TypeCheck<Variants, Args...>...>::value,
        typename impl::template ConstructorT<impl::template TypeCheck, impl::template TypeCheck<VariantT, Args...>::value, 0, Args...>,
        typename impl::template ConstructorT<std::is_constructible, std::is_constructible<VariantT, Args...>::value, 0, Args...>
    >::type;

    using CopyConstructor = typename impl::template Helper<impl::template CopyConstructorT, const Self&, Self*>;
    using MoveConstructor = typename impl::template Helper<impl::template MoveConstructorT, Self&&, Self*>;
    using Destructor = typename impl::template Helper<impl::template DestructorT, Self*>;

    template<typename F>
    using Apply = typename impl::template Helper<impl::template ApplyT, Self*, F>;

    template<typename F>
    using ApplyConst = typename impl::template Helper<impl::template ApplyT, const Self*, F>;

    template<typename... Fs>
    using Match = typename impl::template Helper<impl::template MatchT, Self*, Fs...>;

    template<typename... Fs>
    using MatchConst = typename impl::template Helper<impl::template MatchT, const Self*, Fs...>;

    std::size_t tag;
    StorageT storage;


public:
    // Try to default-construct the first type
    EnumT() {
        Constructor<VariantT>::construct(this);
    }

    EnumT(Self&& other) noexcept {
        MoveConstructor::call(other.tag, std::forward<Self>(other), this);
    }

    EnumT(const Self& other) {
        CopyConstructor::call(other.tag, other, this);
    }

    template<typename T>
    EnumT(T&& t, typename std::enable_if<std::is_rvalue_reference<T&&>::value >::type* = 0,
          typename std::enable_if<!std::is_const<T>::value>::type* = 0) {
        Constructor<T>::construct(this, std::forward<T>(t));
    }

    template<typename T>
    EnumT(const T& t) {
        Constructor<T>::construct(this, t);
    }

    EnumT& operator=(const Self& other) {
        if(this->tag != other.tag) {
            Destructor::call(this->tag, this);
        }

        CopyConstructor::call(other.tag, other, this);
        return *this;
    }

    EnumT& operator=(Self&& other) noexcept {
        if(this->tag != other.tag) {
            Destructor::call(this->tag, this);
        }

        MoveConstructor::call(other.tag, std::forward<Self>(other), this);
        return *this;
    }

    template<typename... Fs>
    auto visit(Fs... fs) {
        auto f = LambdaVisitor<Fs...>(fs...);
        return Apply<decltype(f)>::call(this->tag, this, std::move(f));
    }

    template<typename... Fs>
    auto visit(Fs... fs) const {
      auto f = LambdaVisitor<Fs...>(fs...);
      return ApplyConst<decltype(f)>::call(this->tag, this, std::move(f));
    }

    // Apply to a function based on the contained type
    template<typename... Fs>
    auto match(Fs... fs) {
        return Match<Fs...>::call(this->tag, this, std::forward<Fs>(fs)...);
    }

    template<typename... Fs>
    auto match(Fs... fs) const {
        return MatchConst<Fs...>::call(this->tag, this, std::forward<Fs>(fs)...);
    }

    // Returns the identifying tag
    std::size_t which() const {
        return tag;
    }

    // Returns true if the contained value is of type T
    template<typename T>
    bool contains() const {
        return tag == IndexOf<T, VariantT, Variants...>::value;
    }

    template<typename T>
    optional<T&> getReferenceMaybe() {
        if(tag == IndexOf<T, VariantT, Variants...>::value) {
            return *reinterpret_cast<T*>(&storage);
        } else {
            return none;
        }
    }

    template<typename T>
    optional<const T&> getReferenceMaybe() const {
        if(tag == IndexOf<T, VariantT, Variants...>::value) {
            return *reinterpret_cast<const T*>(&storage);
        } else {
            return none;
        }
    }

    template<typename T>
    optional<T> getValueMaybe() const {
        if(tag == IndexOf<T, VariantT, Variants...>::value) {
            return *reinterpret_cast<const T*>(&storage);
        } else {
            return none;
        }
    }

    bool operator == (const Self& other) const {
      return tag == other.tag && visit([&](const auto& o1) {
          return other.template getReferenceMaybe<
              typename std::remove_const<typename std::remove_reference<decltype(o1)>::type>::type>() == o1; } );
    }

    size_t getHash() const {
      return visit([&](const auto& elem) { return combineHash(tag, elem); });
    }

    ~EnumT() {
        Destructor::call(this->tag, this);
    }
};

}

template<typename ...Arg>
using variant = venum::EnumT<Arg...>;

template<typename ...Arg>
inline std::ostream& operator <<(std::ostream& d, const variant<Arg...>& v) {
  v.visit([&](const auto& elem) { d << elem; });
  return d;
}
