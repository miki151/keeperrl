// -*- C++ -*-
// Copyright (c) 2015, Just Software Solutions Ltd
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted provided that the
// following conditions are met:
//
// 1. Redistributions of source code must retain the above
// copyright notice, this list of conditions and the following
// disclaimer.
//
// 2. Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following
// disclaimer in the documentation and/or other materials
// provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of
// its contributors may be used to endorse or promote products
// derived from this software without specific prior written
// permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef STX_VARIANT_HPP_INCLUDED
#define STX_VARIANT_HPP_INCLUDED

#ifndef STX_NAMESPACE_NAME
#define STX_NAMESPACE_NAME stx
#endif


#if defined(__has_include) && !defined(STX_NO_STD_VARIANT)
#    if __has_include(<variant>) && (__cplusplus > 201402)
        namespace STX_NAMESPACE_NAME {
            using std::variant;
            using std::visit;
            using std::holds_alternative;
            using std::get_if;
            using std::monostate;
            using std::bad_variant_access;
            using std::variant_size;
            using std::variant_size_v;
            using std::variant_alternative;
            using std::variant_alternative_t;
#           ifndef STX_HAVE_IN_PLACE_T
            using std::in_place_t;
            using std::in_place;
#           define STX_IN_PLACE_T 1
#           endif
            using std::in_place_type_t;
            using std::in_place_type;
            using std::in_place_index_t;
            using std::in_place_index;
        }
#       define STX_HAVE_STD_VARIANT 1
#    endif // __hasinclude(any)
#endif // defined(__hasinclude)


#ifndef STX_HAVE_STD_VARIANT

#include <stddef.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <new>
#include <utility>
#include <limits.h>
#include <memory>
#include <functional>
#include <tuple>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4521)
#pragma warning(disable:4522)
#endif

namespace STX_NAMESPACE_NAME {

#ifndef STX_HAVE_IN_PLACE_T
struct in_place_t {
    explicit in_place_t() = default;
};

constexpr in_place_t in_place{};
#define STX_HAVE_IN_PLACE_T 1
#endif

template <class T> struct in_place_type_t {
    explicit in_place_type_t() = default;
};
template <class T>
constexpr in_place_type_t<T> in_place_type{};

template <size_t I> struct in_place_index_t {
    explicit in_place_index_t() = default;
};
template <size_t I>
constexpr in_place_index_t<I> in_place_index{};


class bad_variant_access : public std::logic_error {
public:
    explicit bad_variant_access(const std::string& what_arg)
            :
            std::logic_error(what_arg) {}

    explicit bad_variant_access(const char* what_arg)
            :
            std::logic_error(what_arg) {}
};

template <ptrdiff_t _Offset, typename _Type, typename ... _Types>
struct __type_index_helper;

template <ptrdiff_t _Offset, typename _Type, typename _Head, typename ... _Rest>
struct __type_index_helper<_Offset, _Type, _Head, _Rest...> {
    static constexpr ptrdiff_t __value =
            __type_index_helper<_Offset + 1, _Type, _Rest...>::__value;
};

template <ptrdiff_t _Offset, typename _Type, typename ... _Rest>
struct __type_index_helper<_Offset, _Type, _Type, _Rest...> {
    static constexpr ptrdiff_t __value = _Offset;
};

template <typename _Type, typename ... _Types>
struct __type_index {
    static constexpr ptrdiff_t __value =
            __type_index_helper<0, _Type, _Types...>::__value;
};

template <ptrdiff_t _Index, typename ... _Types>
struct __indexed_type;

template <typename _Head, typename ... _Rest>
struct __indexed_type<0, _Head, _Rest...> {
    typedef _Head __type;
};

template <typename _Head, typename ... _Rest>
struct __indexed_type<-1, _Head, _Rest...> {
    typedef void __type;
};

template <ptrdiff_t _Index, typename _Head, typename ... _Rest>
struct __indexed_type<_Index, _Head, _Rest...> {
    typedef typename __indexed_type<_Index - 1, _Rest...>::__type __type;
};

template <ptrdiff_t _Index, typename ..._Types>
struct __next_index {
    static constexpr ptrdiff_t __value =
            (_Index >= ptrdiff_t(sizeof...(_Types) - 1)) ? -1 : _Index + 1;
};

template <typename ... _Types>
class variant;

template <typename>
struct variant_size;

template <typename _Type>
struct variant_size<const _Type> : variant_size<_Type> {};

template <typename _Type>
struct variant_size<volatile _Type> : variant_size<_Type> {};

template <typename _Type>
struct variant_size<const volatile _Type> : variant_size<_Type> {};

template <typename... _Types>
struct variant_size<variant<_Types...>>
        : std::integral_constant<size_t, sizeof...(_Types)> {
};

template <size_t _Index, typename _Type>
struct variant_alternative;

template <size_t _Index, typename _Type>
using variant_alternative_t=typename variant_alternative<_Index, _Type>::type;

template <size_t _Index, typename _Type>
struct variant_alternative<_Index, const _Type> {
    using type=std::add_const_t<variant_alternative_t<_Index, _Type>>;
};

template <size_t _Index, typename _Type>
struct variant_alternative<_Index, volatile _Type> {
    using type=std::add_volatile_t<variant_alternative_t<_Index, _Type>>;
};

template <size_t _Index, typename _Type>
struct variant_alternative<_Index, volatile const _Type> {
    using type=std::add_volatile_t<std::add_const_t<variant_alternative_t<_Index, _Type>>>;
};

template <size_t _Index, typename ... _Types>
struct variant_alternative<_Index, variant<_Types...>> {
    using type=typename __indexed_type<_Index, _Types...>::__type;
};

constexpr size_t variant_npos = -1;

template <typename _Type, typename ... _Types>
constexpr _Type& get(variant<_Types...>&);

template <typename _Type, typename ... _Types>
constexpr _Type const& get(variant<_Types...> const&);

template <typename _Type, typename ... _Types>
constexpr _Type&& get(variant<_Types...>&&);

template <typename _Type, typename ... _Types>
constexpr const _Type&& get(variant<_Types...> const&&);

template <ptrdiff_t _Index, typename ... _Types>
constexpr typename __indexed_type<_Index, _Types...>::__type&
get(variant<_Types...>&);

template <ptrdiff_t _Index, typename ... _Types>
constexpr typename __indexed_type<_Index, _Types...>::__type&&
get(variant<_Types...>&&);

template <ptrdiff_t _Index, typename ... _Types>
constexpr typename __indexed_type<_Index, _Types...>::__type const& get(
        variant<_Types...> const&);

template <ptrdiff_t _Index, typename... _Types>
constexpr const typename __indexed_type<_Index, _Types...>::__type&&
get(variant<_Types...> const&&);

template <typename _Type, typename ... _Types>
constexpr std::add_pointer_t<_Type> get_if(variant<_Types...>&);

template <typename _Type, typename ... _Types>
constexpr std::add_pointer_t<_Type const> get_if(variant<_Types...> const&);

template <ptrdiff_t _Index, typename ... _Types>
constexpr std::add_pointer_t<typename __indexed_type<_Index, _Types...>::__type>
get_if(variant<_Types...>&);

template <ptrdiff_t _Index, typename ... _Types>
constexpr std::add_pointer_t<typename __indexed_type<_Index, _Types...>::__type const>
get_if(
        variant<_Types...> const&);

template <ptrdiff_t _Index, typename ... _Types>
struct __variant_accessor;

template <size_t __count,
        bool __larger_than_char = (__count > SCHAR_MAX),
        bool __larger_than_short = (__count > SHRT_MAX),
        bool __larger_than_int = (__count > INT_MAX)>
struct __discriminator_type {
    typedef signed char __type;
};

template <size_t __count>
struct __discriminator_type<__count, true, false, false> {
    typedef signed short __type;
};

template <size_t __count>
struct __discriminator_type<__count, true, true, false> {
    typedef int __type;
};
template <size_t __count>
struct __discriminator_type<__count, true, true, true> {
    typedef signed long __type;
};

template <typename _Type>
struct __stored_type {
    typedef _Type __type;
};

template <typename _Type>
struct __stored_type<_Type&> {
    typedef _Type* __type;
};

template <typename ... _Types>
struct __all_trivially_destructible;

template <>
struct __all_trivially_destructible<> {
    static constexpr bool __value = true;
};

template <typename _Type>
struct __all_trivially_destructible<_Type> {
    static constexpr bool __value =
            std::is_trivially_destructible<typename __stored_type<_Type>::__type>::value;
};

template <typename _Head, typename ... _Rest>
struct __all_trivially_destructible<_Head, _Rest...> {
    static constexpr bool __value =
            __all_trivially_destructible<_Head>::__value &&
                    __all_trivially_destructible<_Rest...>::__value;
};

template <typename _Target, typename ... _Args>
struct __storage_nothrow_constructible {
    static const bool __value =
            noexcept(_Target(std::declval<_Args>()...));
};

template <typename ... _Types>
struct __storage_nothrow_move_constructible;

template <>
struct __storage_nothrow_move_constructible<> {
    static constexpr bool __value = true;
};

template <typename _Type>
struct __storage_nothrow_move_constructible<_Type> {
    static constexpr bool __value =
            std::is_nothrow_move_constructible<
                    typename __stored_type<_Type>::__type>::value;
};

template <typename _Head, typename ... _Rest>
struct __storage_nothrow_move_constructible<_Head, _Rest...> {
    static constexpr bool __value =
            __storage_nothrow_move_constructible<_Head>::__value &&
                    __storage_nothrow_move_constructible<_Rest...>::__value;
};

template <ptrdiff_t _Index, typename ... _Types>
struct __other_storage_nothrow_move_constructible;

template <typename _Head, typename ... _Rest>
struct __other_storage_nothrow_move_constructible<0, _Head, _Rest...> {
    static const bool __value = __storage_nothrow_move_constructible<_Rest...>::__value;
};

template <typename _Head, typename ... _Rest>
struct __other_storage_nothrow_move_constructible<-1, _Head, _Rest...> {
    static const bool __value =
            __storage_nothrow_move_constructible<_Head, _Rest...>::__value;
};

template <ptrdiff_t _Index, typename _Head, typename ... _Rest>
struct __other_storage_nothrow_move_constructible<_Index, _Head, _Rest...> {
    static const bool __value =
            __storage_nothrow_move_constructible<_Head>::__value &&
                    __other_storage_nothrow_move_constructible<
                            _Index - 1, _Rest...>::__value;
};

template <ptrdiff_t _Index, typename ... _Types>
struct __backup_storage_required {
    static const bool __value =
            !__storage_nothrow_move_constructible<
                    typename __indexed_type<_Index, _Types...>::__type>::__value
                    &&
                            !__other_storage_nothrow_move_constructible<_Index, _Types...>::__value;
};

template <ptrdiff_t _Index, ptrdiff_t _Count, typename ... _Types>
struct __any_backup_storage_required_impl {
    static const bool __value =
            __backup_storage_required<_Index, _Types...>::__value ||
                    __any_backup_storage_required_impl<
                            _Index + 1, _Count - 1, _Types...>::__value;
};

template <ptrdiff_t _Index, typename ... _Types>
struct __any_backup_storage_required_impl<_Index, 0, _Types...> {
    static const bool __value = false;
};

template <typename _Variant>
struct __any_backup_storage_required;

template <typename ... _Types>
struct __any_backup_storage_required<variant<_Types...>> {
    static const bool __value =
            __any_backup_storage_required_impl<0, sizeof...(_Types), _Types...>::__value;
};

template <typename ... _Types>
union __variant_data;

template <typename _Type, bool= std::is_literal_type<_Type>::value>
struct __variant_storage {
    typedef _Type __type;

    static constexpr _Type& __get(__type& __val)
    {
        return __val;
    }

    static constexpr _Type&& __get_rref(__type& __val)
    {
        return std::move(__val);
    }

    static constexpr const _Type& __get(__type const& __val)
    {
        return __val;
    }

    static constexpr const _Type&& __get_rref(__type const& __val)
    {
        return std::move(__val);
    }

    static void __destroy(__type&) {}
};

template <typename _Type>
struct __storage_wrapper {
    typename std::aligned_storage<sizeof(_Type), alignof(_Type)>::type __storage;

    template <typename ... _Args>
    static constexpr void __construct(void* __p, _Args&& ... __args)
    {
        new(__p) _Type(std::forward<_Args>(__args)...);
    }

    template <typename _Dummy = _Type>
    __storage_wrapper(
            typename std::enable_if<std::is_default_constructible<_Dummy>::value,
                    void (__storage_wrapper::*)()>::type = nullptr)
    {
        __construct(&__storage);
    }

    template <typename _Dummy = _Type>
    __storage_wrapper(
            typename std::enable_if<!std::is_default_constructible<_Dummy>::value,
                    void (__storage_wrapper::*)()>::type = nullptr)
    {
    }

    template <typename _First, typename ... _Args>
    __storage_wrapper(_First&& __first, _Args&& ... __args)
    {
        __construct(&__storage, std::forward<_First>(__first),
                    std::forward<_Args>(__args)...);
    }

    _Type& __get()
    {
        return *static_cast<_Type*>(static_cast<void*>(&__storage));
    }

    constexpr _Type const& __get() const
    {
        return *static_cast<_Type const*>(static_cast<void const*>(&__storage));
    }

    void __destroy()
    {
        __get().~_Type();
    }
};

template <typename _Type>
struct __storage_wrapper<_Type&> {
    _Type* __storage;

    template <typename _Arg>
    constexpr __storage_wrapper(_Arg& __arg)
            :
            __storage(&__arg) {}

    _Type& __get()
    {
        return *__storage;
    }

    constexpr _Type const& __get() const
    {
        return *__storage;
    }
};

template <typename _Type>
struct __variant_storage<_Type, false> {
    typedef __storage_wrapper<_Type> __type;

    static constexpr _Type& __get(__type& __val)
    {
        return __val.__get();
    }

    static constexpr _Type&& __get_rref(__type& __val)
    {
        return std::move(__val.__get());
    }

    static constexpr const _Type& __get(__type const& __val)
    {
        return __val.__get();
    }

    static constexpr const _Type&& __get_rref(__type const& __val)
    {
        return std::move(__val.__get());
    }

    static void __destroy(__type& __val)
    {
        __val.__destroy();
    }
};

template <typename _Type, bool __b>
struct __variant_storage<_Type&, __b> {
    typedef _Type* __type;

    static constexpr _Type& __get(__type& __val)
    {
        return *__val;
    }

    static constexpr _Type& __get_rref(__type& __val)
    {
        return *__val;
    }

    static constexpr _Type& __get(__type const& __val)
    {
        return *__val;
    }

    static constexpr _Type& __get_rref(__type const& __val)
    {
        return *__val;
    }

    static void __destroy(__type&) {}
};

template <typename _Type, bool __b>
struct __variant_storage<_Type&&, __b> {
    typedef _Type* __type;

    static constexpr _Type&& __get(__type& __val)
    {
        return static_cast<_Type&&>(*__val);
    }

    static constexpr _Type&& __get_rref(__type& __val)
    {
        return static_cast<_Type&&>(*__val);
    }

    static constexpr _Type&& __get(__type const& __val)
    {
        return static_cast<_Type&&>(*__val);
    }

    static constexpr _Type&& __get_rref(__type const& __val)
    {
        return static_cast<_Type&&>(*__val);
    }

    static void __destroy(__type&) {}
};

template <>
union __variant_data<> {
    constexpr __variant_data() {}
};

template <typename _Type>
union __variant_data<_Type> {
    typename __variant_storage<_Type>::__type __val;
    struct __dummy_type {}__dummy;

    constexpr __variant_data()
            : __dummy() {}

    constexpr __variant_data(in_place_index_t<0>)
            :
            __val() {}

    template <typename _FirstArg>
    constexpr __variant_data(
            typename std::conditional<
                    !std::is_same<std::remove_cv_t<std::remove_reference_t<_FirstArg>>,
                            std::allocator_arg_t>::value,
                    in_place_index_t<0>, __dummy_type>::type,
            _FirstArg&& __firstArg)
            :
            __val(std::forward<_FirstArg>(__firstArg)) {}

    template <typename _FirstArg, typename ... _Rest>
    constexpr __variant_data(
            typename std::conditional<
                    !std::is_same<std::remove_cv_t<std::remove_reference_t<_FirstArg>>,
                            std::allocator_arg_t>::value,
                    in_place_index_t<0>, __dummy_type>::type,
            _FirstArg&& __firstArg, _Rest&& ... __rest)
            :
            __val(std::forward<_FirstArg>(__firstArg),
                  std::forward<_Rest>(__rest)...) {}

    template <typename _Alloc, typename ... _Args>
    constexpr __variant_data(
            typename std::conditional<
                    !std::uses_allocator<_Type, _Alloc>::value,
                    in_place_index_t<0>,
                    __dummy_type>::type,
            std::allocator_arg_t, _Alloc const&,
            _Args&& ... __args)
            :
            __val(std::forward<_Args>(__args)...) {}

    template <typename _Alloc, typename... _Args>
    __variant_data(
            typename std::conditional<
                    std::uses_allocator<_Type, _Alloc>::value &&
                            std::is_constructible<_Type, std::allocator_arg_t,
                                    _Alloc const&, _Args...>::value,
                    in_place_index_t<0>, __dummy_type>::type,
            std::allocator_arg_t, _Alloc const& __alloc, _Args&& ... __args)
            : __val(std::allocator_arg_t(), __alloc,
                    std::forward<_Args>(__args)...) {}

    template <typename _Alloc, typename... _Args>
    __variant_data(
            typename std::conditional<
                    std::uses_allocator<_Type, _Alloc>::value &&
                            !std::is_constructible<_Type, std::allocator_arg_t,
                                    _Alloc const&, _Args...>::value,
                    in_place_index_t<0>, __dummy_type>::type,
            std::allocator_arg_t, _Alloc const& __alloc, _Args&& ... __args)
            : __val(std::forward<_Args>(__args)..., __alloc) {}

    _Type& __get(in_place_index_t<0>)
    {
        return __variant_storage<_Type>::__get(__val);
    }

    constexpr _Type&& __get_rref(in_place_index_t<0>)
    {
        return __variant_storage<_Type>::__get_rref(__val);
    }

    constexpr const _Type& __get(in_place_index_t<0>) const
    {
        return __variant_storage<_Type>::__get(__val);
    }

    constexpr const _Type&& __get_rref(in_place_index_t<0>) const
    {
        return __variant_storage<_Type>::__get_rref(__val);
    }

    void __destroy(in_place_index_t<0>)
    {
        __variant_storage<_Type>::__destroy(__val);
    }
};

template <typename _Type>
union __variant_data<_Type&> {
    typename __variant_storage<_Type&>::__type __val;
    struct __dummy_type {}__dummy;

    constexpr __variant_data()
            : __dummy() {}

    template <typename ... _Args>
    constexpr __variant_data(in_place_index_t<0>, _Args&& ... __args)
            :
            __val(&std::forward<_Args>(__args)...) {}

    _Type& __get(in_place_index_t<0>)
    {
        return __variant_storage<_Type&>::__get(__val);
    }

    constexpr _Type& __get(in_place_index_t<0>) const
    {
        return __variant_storage<_Type&>::__get(__val);
    }

    _Type& __get_rref(in_place_index_t<0>)
    {
        return __variant_storage<_Type&>::__get_rref(__val);
    }

    constexpr _Type& __get_rref(in_place_index_t<0>) const
    {
        return __variant_storage<_Type&>::__get_rref(__val);
    }

    void __destroy(in_place_index_t<0>)
    {
        __variant_storage<_Type&>::__destroy(__val);
    }
};

template <typename _Type>
union __variant_data<_Type&&> {
    typename __variant_storage<_Type&&>::__type __val;
    struct __dummy_type {}__dummy;

    constexpr __variant_data()
            : __dummy() {}

    template <typename _Arg>
    __variant_data(in_place_index_t<0>, _Arg&& __arg)
            :
            __val(&__arg) {}

    _Type&& __get(in_place_index_t<0>)
    {
        return __variant_storage<_Type&&>::__get(__val);
    }

    constexpr _Type&& __get(in_place_index_t<0>) const
    {
        return __variant_storage<_Type&&>::__get(__val);
    }

    _Type&& __get_rref(in_place_index_t<0>)
    {
        return __variant_storage<_Type&&>::__get_rref(__val);
    }

    constexpr _Type&& __get_rref(in_place_index_t<0>) const
    {
        return __variant_storage<_Type&&>::__get_rref(__val);
    }

    void __destroy(in_place_index_t<0>)
    {
        __variant_storage<_Type&&>::__destroy(__val);
    }
};

template <typename _Head, typename ... _Rest>
union __variant_data<_Head, _Rest...> {
    __variant_data<_Head> __head;
    __variant_data<_Rest...> __rest;

    constexpr __variant_data()
            :
            __head() {}

    template <typename ... _Args>
    constexpr __variant_data(in_place_index_t<0>, _Args&& ... __args)
            :
            __head(in_place_index<0>, std::forward<_Args>(__args)...) {}

    template <size_t _Index, typename ... _Args>
    constexpr __variant_data(in_place_index_t<_Index>, _Args&& ... __args)
            :
            __rest(in_place_index<_Index - 1>, std::forward<_Args>(__args)...) {}

    _Head& __get(in_place_index_t<0>)
    {
        return __head.__get(in_place_index<0>);
    }

    constexpr _Head&& __get_rref(in_place_index_t<0>)
    {
        return __head.__get_rref(in_place_index<0>);
    }

    constexpr const _Head& __get(in_place_index_t<0>) const
    {
        return __head.__get(in_place_index<0>);
    }

    constexpr const _Head&& __get_rref(in_place_index_t<0>) const
    {
        return __head.__get_rref(in_place_index<0>);
    }

    template <size_t _Index>
    typename __indexed_type<_Index - 1, _Rest...>::__type& __get(
            in_place_index_t<_Index>)
    {
        return __rest.__get(in_place_index<_Index - 1>);
    }

    template <size_t _Index>
    constexpr typename __indexed_type<_Index - 1, _Rest...>::__type&&
    __get_rref(
            in_place_index_t<_Index>)
    {
        return __rest.__get_rref(in_place_index<_Index - 1>);
    }

    template <size_t _Index>
    constexpr const typename __indexed_type<_Index - 1, _Rest...>::__type&
    __get(
            in_place_index_t<_Index>) const
    {
        return __rest.__get(in_place_index<_Index - 1>);
    }

    template <size_t _Index>
    constexpr const typename __indexed_type<_Index - 1, _Rest...>::__type&&
    __get_rref(
            in_place_index_t<_Index>) const
    {
        return __rest.__get_rref(in_place_index<_Index - 1>);
    }

    void __destroy(in_place_index_t<0>)
    {
        __head.__destroy(in_place_index<0>);
    }

    template <size_t _Index>
    void __destroy(in_place_index_t<_Index>)
    {
        __rest.__destroy(in_place_index<_Index - 1>);
    }
};

template <ptrdiff_t... _Indices>
struct __index_sequence {
    typedef __index_sequence<_Indices..., sizeof...(_Indices)> __next;
    static constexpr size_t __length = sizeof...(_Indices);
};

template <typename ... _Types>
struct __type_indices;

template <>
struct __type_indices<> {
    typedef __index_sequence<> __type;
};

template <typename _Type>
struct __type_indices<_Type> {
    typedef __index_sequence<0> __type;
};

template <typename _Type, typename ... _Rest>
struct __type_indices<_Type, _Rest...> {
    typedef typename __type_indices<_Rest...>::__type::__next __type;
};

template <typename _Variant>
struct __variant_indices;

template <typename ... _Types>
struct __variant_indices<variant<_Types...>> {
    typedef typename __type_indices<_Types...>::__type __type;
};

template <typename _Variant,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __move_construct_op_table;

template <typename _Variant, ptrdiff_t ... _Indices>
struct __move_construct_op_table<_Variant, __index_sequence<_Indices...>> {
    typedef void(* const __func_type)(_Variant*, _Variant&);

    template <ptrdiff_t _Index>
    static void __move_construct_func(
            _Variant* __lhs, _Variant& __rhs)
    {
        __lhs->template __emplace_construct<_Index>(
                std::move(get<_Index>(__rhs)));
    }

    static const __func_type __apply[sizeof...(_Indices)];
};

template <typename _Variant, ptrdiff_t ... _Indices>
const typename __move_construct_op_table<_Variant, __index_sequence<_Indices...>>::
__func_type
        __move_construct_op_table<_Variant, __index_sequence<_Indices...>>::__apply[
        sizeof...(_Indices)] = {
        &__move_construct_func < _Indices > ...
};

template <typename _Variant, typename _Alloc,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __move_construct_alloc_op_table;

template <typename _Variant, typename _Alloc, ptrdiff_t ... _Indices>
struct __move_construct_alloc_op_table<_Variant, _Alloc, __index_sequence<_Indices...>> {
    typedef void(* const __func_type)(_Variant*, _Alloc const&, _Variant&);

    template <ptrdiff_t _Index>
    static void __move_construct_func(
            _Variant* __lhs, _Alloc const& __alloc, _Variant& __rhs)
    {
        __lhs->template __emplace_construct<_Index>(
                std::allocator_arg_t(), __alloc,
                std::move(get<_Index>(__rhs)));
    }

    static const __func_type __apply[sizeof...(_Indices)];
};

template <typename _Variant, typename _Alloc, ptrdiff_t ... _Indices>
const typename __move_construct_alloc_op_table<_Variant, _Alloc, __index_sequence<_Indices...>>::
__func_type
        __move_construct_alloc_op_table<_Variant, _Alloc, __index_sequence<_Indices...>>::__apply[
        sizeof...(_Indices)] = {
        &__move_construct_func < _Indices > ...
};

template <typename _Variant,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __move_assign_op_table;

template <typename _Variant, ptrdiff_t ... _Indices>
struct __move_assign_op_table<_Variant, __index_sequence<_Indices...>> {
    typedef void(* const __func_type)(_Variant*, _Variant&);

    template <ptrdiff_t _Index>
    static void __move_assign_func(
            _Variant* __lhs, _Variant& __rhs)
    {
        get<_Index>(*__lhs) = std::move(get<_Index>(__rhs));
    }

    static const __func_type __apply[sizeof...(_Indices)];
};

template <typename _Variant, ptrdiff_t ... _Indices>
const typename __move_assign_op_table<_Variant, __index_sequence<_Indices...>>::
__func_type
        __move_assign_op_table<_Variant, __index_sequence<_Indices...>>::__apply[
        sizeof...(_Indices)] = {
        &__move_assign_func < _Indices > ...
};

template <typename _Variant,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __copy_construct_op_table;

template <typename _Variant, ptrdiff_t ... _Indices>
struct __copy_construct_op_table<_Variant, __index_sequence<_Indices...>> {
    typedef void(* const __func_type)(_Variant*, _Variant const&);

    template <ptrdiff_t _Index>
    static void __copy_construct_func(
            _Variant* __lhs, _Variant const& __rhs)
    {
        __lhs->template __emplace_construct<_Index>(
                get<_Index>(__rhs));
    }

    static const __func_type __apply[sizeof...(_Indices)];
};

template <typename _Variant, ptrdiff_t ... _Indices>
const typename __copy_construct_op_table<_Variant, __index_sequence<_Indices...>>::
__func_type
        __copy_construct_op_table<_Variant, __index_sequence<_Indices...>>::__apply[
        sizeof...(_Indices)] = {
        &__copy_construct_func < _Indices > ...
};

template <typename _Variant, typename _Alloc,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __copy_construct_alloc_op_table;

template <typename _Variant, typename _Alloc, ptrdiff_t ... _Indices>
struct __copy_construct_alloc_op_table<_Variant, _Alloc, __index_sequence<_Indices...>> {
    typedef void(* const __func_type)(_Variant*, _Alloc const&,
                                      _Variant const&);

    template <ptrdiff_t _Index>
    static void __copy_construct_func(
            _Variant* __lhs, _Alloc const& __alloc, _Variant const& __rhs)
    {
        __lhs->template __emplace_construct<_Index>(
                std::allocator_arg_t(), __alloc,
                get<_Index>(__rhs));
    }

    static const __func_type __apply[sizeof...(_Indices)];
};

template <typename _Variant, typename _Alloc, ptrdiff_t ... _Indices>
const typename __copy_construct_alloc_op_table<_Variant, _Alloc, __index_sequence<_Indices...>>::
__func_type
        __copy_construct_alloc_op_table<_Variant, _Alloc, __index_sequence<_Indices...>>::__apply[
        sizeof...(_Indices)] = {
        &__copy_construct_func < _Indices > ...
};

template <typename _Variant,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __copy_assign_op_table;

template <typename _Variant, ptrdiff_t ... _Indices>
struct __copy_assign_op_table<_Variant, __index_sequence<_Indices...>> {
    typedef void(* const __func_type)(_Variant*, _Variant const&);

    template <ptrdiff_t _Index>
    static void __copy_assign_func(
            _Variant* __lhs, _Variant const& __rhs)
    {
        get<_Index>(*__lhs) = get<_Index>(__rhs);
    }

    static const __func_type __apply[sizeof...(_Indices)];
};

template <typename _Variant, ptrdiff_t ... _Indices>
const typename __copy_assign_op_table<_Variant, __index_sequence<_Indices...>>::
__func_type
        __copy_assign_op_table<_Variant, __index_sequence<_Indices...>>::__apply[
        sizeof...(_Indices)] = {
        &__copy_assign_func < _Indices > ...
};

template <typename _Variant,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __destroy_op_table;

template <typename _Variant, ptrdiff_t ... _Indices>
struct __destroy_op_table<_Variant, __index_sequence<_Indices...>> {
    typedef void(* const __func_type)(_Variant*);

    template <ptrdiff_t _Index>
    static void __destroy_func(
            _Variant* __self)
    {
        if (__self->__index >= 0) {
            __self->__storage.__destroy(in_place_index<_Index>);
        }
    }

    static const __func_type __apply[sizeof...(_Indices)];
};

template <typename _Variant, ptrdiff_t ... _Indices>
const typename __destroy_op_table<_Variant, __index_sequence<_Indices...>>::
__func_type
        __destroy_op_table<_Variant, __index_sequence<_Indices...>>::__apply[
        sizeof...(_Indices)] = {
        &__destroy_func < _Indices > ...
};

template <typename _Variant,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __swap_op_table;

template <typename _Variant, ptrdiff_t ... _Indices>
struct __swap_op_table<_Variant, __index_sequence<_Indices...>> {
    typedef void(* const __func_type)(_Variant&, _Variant&);

    template <ptrdiff_t _Index>
    static void __swap_func(
            _Variant& __lhs, _Variant& __rhs)
    {
        using std::swap;
        swap(get<_Index>(__lhs), get<_Index>(__rhs));
    }

    static const __func_type __apply[sizeof...(_Indices)];
};

template <typename _Variant, ptrdiff_t ... _Indices>
const typename __swap_op_table<_Variant, __index_sequence<_Indices...>>::
__func_type
        __swap_op_table<_Variant, __index_sequence<_Indices...>>::__apply[
        sizeof...(_Indices)] = {
        &__swap_func < _Indices > ...
};

template <typename _Variant,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __equality_op_table;

template <typename _Variant, ptrdiff_t ... _Indices>
struct __equality_op_table<_Variant, __index_sequence<_Indices...>> {
    typedef bool(* const __compare_func_type)(_Variant const&, _Variant const&);

    template <ptrdiff_t _Index>
    static constexpr bool __equality_compare_func(
            _Variant const& __lhs, _Variant const& __rhs)
    {
        return get<_Index>(__lhs) == get<_Index>(__rhs);
    }

    static constexpr __compare_func_type __equality_compare[sizeof...(_Indices)] = {
            &__equality_compare_func<_Indices>...
    };
};

template <typename _Variant, ptrdiff_t ... _Indices>
constexpr typename __equality_op_table<_Variant, __index_sequence<_Indices...>>::
__compare_func_type
        __equality_op_table<_Variant, __index_sequence<_Indices...>>::__equality_compare[
        sizeof...(_Indices)];

template <typename _Variant,
        typename _Indices=typename __variant_indices<_Variant>::__type>
struct __less_than_op_table;

template <typename _Variant, ptrdiff_t ... _Indices>
struct __less_than_op_table<_Variant, __index_sequence<_Indices...>> {
    typedef bool(* const __compare_func_type)(_Variant const&, _Variant const&);

    template <ptrdiff_t _Index>
    static constexpr bool __less_than_compare_func(
            _Variant const& __lhs, _Variant const& __rhs)
    {
        return get<_Index>(__lhs) < get<_Index>(__rhs);
    }

    static constexpr __compare_func_type __less_than_compare[sizeof...(_Indices)] = {
            &__less_than_compare_func<_Indices>...
    };
};

template <typename _Variant, ptrdiff_t ... _Indices>
constexpr typename __less_than_op_table<_Variant, __index_sequence<_Indices...>>::
__compare_func_type
        __less_than_op_table<_Variant, __index_sequence<_Indices...>>::__less_than_compare[
        sizeof...(_Indices)];

template <typename _Variant>
struct __variant_storage_type;

template <typename _Derived, bool __trivial_destructor>
struct __variant_base {
    ~__variant_base()
    {
        static_cast<_Derived*>(this)->__destroy_self();
    }
};

template <typename _Derived>
struct __variant_base<_Derived, true> {
};

template <ptrdiff_t _Offset, typename _CurrentSequence,
        typename _Type, typename ... _Types>
struct __all_indices_helper;

template <ptrdiff_t _Offset, ptrdiff_t ... _Indices,
        typename _Type, typename ... _Rest>
struct __all_indices_helper<
        _Offset, __index_sequence<_Indices...>,
        _Type, _Type, _Rest...> {
    typedef typename __all_indices_helper<
            _Offset
                    + 1, __index_sequence<_Indices..., _Offset>, _Type, _Rest...>::__type
            __type;
};

template <ptrdiff_t _Offset, typename _CurrentSequence,
        typename _Type, typename _Head, typename ... _Rest>
struct __all_indices_helper<_Offset, _CurrentSequence, _Type, _Head, _Rest...> {
    typedef typename __all_indices_helper<
            _Offset + 1, _CurrentSequence, _Type, _Rest...>::__type __type;
};

template <ptrdiff_t _Offset, typename _CurrentSequence, typename _Type>
struct __all_indices_helper<_Offset, _CurrentSequence, _Type> {
    typedef _CurrentSequence __type;
};

template <typename _Type, typename ... _Types>
struct __all_indices {
    typedef typename __all_indices_helper<
            0, __index_sequence<>, _Type, _Types...>::__type __type;
};

template <typename ... _Sequences>
struct __combine_sequences;

template <ptrdiff_t ... _Indices1, ptrdiff_t ... _Indices2>
struct __combine_sequences<
        __index_sequence<_Indices1...>, __index_sequence<_Indices2...>> {
    typedef __index_sequence<_Indices1..., _Indices2...> __type;
};

template <typename _Sequence, typename ... _Rest>
struct __combine_sequences<_Sequence, _Rest...> {
    typedef typename __combine_sequences<
            _Sequence,
            typename __combine_sequences<_Rest...>::__type>::__type __type;
};

template <typename _Indices>
struct __first_index;

template <ptrdiff_t _FirstIndex, ptrdiff_t ... _Rest>
struct __first_index<__index_sequence<_FirstIndex, _Rest...>> {
    static constexpr ptrdiff_t __value = _FirstIndex;
};

template <ptrdiff_t _Offset, typename _CurrentSequence,
        typename _Type, typename ... _Types>
struct __constructible_matches_helper;

template <ptrdiff_t _Offset, typename _Sequence, typename _Type>
struct __constructible_matches_helper<
        _Offset, _Sequence, _Type> {
    typedef _Sequence __type;
};

template <bool _Accept, ptrdiff_t _Entry>
struct __sequence_or_empty {
    typedef __index_sequence<> __type;
};

template <ptrdiff_t _Entry>
struct __sequence_or_empty<true, _Entry> {
    typedef __index_sequence<_Entry> __type;
};

template <ptrdiff_t _Offset, typename _CurrentSequence,
        typename _Type, typename _Head, typename ... _Rest>
struct __constructible_matches_helper<
        _Offset, _CurrentSequence, _Type, _Head, _Rest...> {
    typedef
    typename __constructible_matches_helper<
            _Offset + 1,
            typename __combine_sequences<
                    _CurrentSequence,
                    typename __sequence_or_empty<
                            std::is_constructible<_Head, _Type>::value,
                            _Offset>::__type>::__type,
            _Type, _Rest...>::__type __type;
};

template <typename _Type, typename ... _Types>
struct __constructible_matches {
    typedef typename __constructible_matches_helper<
            0, __index_sequence<>, _Type, _Types...>::__type __type;
};

template <typename _Type, typename ... _Types>
struct __type_index_to_construct {
    typedef typename __all_indices<_Type, _Types...>::__type __direct_matches;
    typedef typename __all_indices<
            typename std::remove_const<
                    typename std::remove_reference<_Type>::type
            >::type, _Types...>::__type __value_matches;
    typedef typename __all_indices<
            _Type,
            typename std::remove_const<
                    typename std::remove_reference<_Types>::type
            >::type...>::__type __rref_matches;

    typedef typename __constructible_matches<_Type, _Types...>::__type
            __constructibles;

    static_assert(
            (__direct_matches::__length > 0) ||
                    (__value_matches::__length > 0) ||
                    (__rref_matches::__length > 0) ||
                    (__constructibles::__length == 1),
            "For conversion construction of variants, exactly one type must be constructible");

    typedef typename __combine_sequences<
            __direct_matches, __value_matches, __rref_matches,
            __constructibles>::__type __all_matches;

    static constexpr ptrdiff_t __value = __first_index<__all_matches>::__value;
};

struct __replace_construct_helper {
    template <
            ptrdiff_t _Index,
            bool __construct_directly,
            bool __indexed_type_has_nothrow_move,
            bool __other_types_have_nothrow_move>
    struct __helper;

    template <typename _Variant,
            typename _Indices=typename __variant_indices<_Variant>::__type>
    struct __op_table;
};

template <
        ptrdiff_t _Index,
        bool __other_types_have_nothrow_move>
struct __replace_construct_helper::__helper<
        _Index, false, true, __other_types_have_nothrow_move> {

    template <typename _Variant, typename ... _Args>
    static void __trampoline(_Variant& __v, _Args&& ... __args)
    {
        __v.template __two_stage_replace<_Index>(__args...);
    }
};

template <
        ptrdiff_t _Index,
        bool __indexed_type_has_nothrow_move,
        bool __other_types_have_nothrow_move>
struct __replace_construct_helper::__helper<
        _Index, true, __indexed_type_has_nothrow_move,
        __other_types_have_nothrow_move> {

    template <typename _Variant, typename ... _Args>
    static void __trampoline(_Variant& __v, _Args&& ... __args)
    {
        __v.template __direct_replace<_Index>(std::forward<_Args>(__args)...);
    }
};

template <
        ptrdiff_t _Index>
struct __replace_construct_helper::__helper<
        _Index, false, false, true> {

    template <typename _Variant, typename ... _Args>
    static void __trampoline(_Variant& __v, _Args&& ... __args)
    {
        __v.template __local_backup_replace<_Index>(
                std::forward<_Args>(__args)...);
    }
};

template <
        ptrdiff_t _Index>
struct __replace_construct_helper::__helper<
        _Index, false, false, false> {

    template <typename _Variant, typename ... _Args>
    static void __trampoline(_Variant& __v, _Args&& ... __args)
    {
        __v.template __direct_replace<_Index>(std::forward<_Args>(__args)...);
    }
};

template <typename _Variant, ptrdiff_t ... _Indices>
struct __replace_construct_helper::__op_table<_Variant, __index_sequence<_Indices...>> {
    typedef void(* const __move_func_type)(_Variant*, _Variant&);

    typedef void(* const __copy_func_type)(_Variant*, _Variant const&);

    template <ptrdiff_t _Index>
    static void __move_assign_func(
            _Variant* __lhs, _Variant& __rhs)
    {
        __lhs->template __replace_construct<_Index>(
                std::move(get<_Index>(__rhs)));
        __rhs.__destroy_self();
    }

    template <ptrdiff_t _Index>
    static void __copy_assign_func(
            _Variant* __lhs, _Variant const& __rhs)
    {
        __lhs->template __replace_construct<_Index>(get<_Index>(__rhs));
    }

    static const __move_func_type __move_assign[sizeof...(_Indices)];
    static const __copy_func_type __copy_assign[sizeof...(_Indices)];
};

template <typename _Variant, ptrdiff_t ... _Indices>
const typename __replace_construct_helper::__op_table<
        _Variant, __index_sequence<_Indices...>>::__move_func_type
        __replace_construct_helper::__op_table<
                _Variant, __index_sequence<_Indices...>>::__move_assign[
        sizeof...(_Indices)] = {
        &__move_assign_func<_Indices>...
};

template <typename _Variant, ptrdiff_t ... _Indices>
const typename __replace_construct_helper::__op_table<
        _Variant, __index_sequence<_Indices...>>::__copy_func_type
        __replace_construct_helper::__op_table<
                _Variant, __index_sequence<_Indices...>>::__copy_assign[
        sizeof...(_Indices)] = {
        &__copy_assign_func<_Indices>...
};

template <ptrdiff_t _Index, ptrdiff_t _MaskIndex, typename _Storage>
struct __backup_storage_ops {
    static void __move_construct_func(
            _Storage* __dest, _Storage& __source)
    {
        new(__dest) _Storage(
                in_place_index<_Index>,
                std::move(__source.__get(in_place_index<_Index>)));
    }

    static void __destroy_func(_Storage* __obj)
    {
        __obj->__destroy(in_place_index<_Index>);
    };
};

template <ptrdiff_t _Index, typename _Storage>
struct __backup_storage_ops<_Index, _Index, _Storage> {
    static void __move_construct_func(
            _Storage* __dest, _Storage& __source)
    {
        throw std::bad_alloc();
    };

    static void __destroy_func(_Storage* __obj)
    {
        throw std::bad_alloc();
    };
};

template <ptrdiff_t _MaskIndex, typename _Storage, typename _Indices>
struct __backup_storage_op_table;

template <ptrdiff_t _MaskIndex, typename _Storage, ptrdiff_t ... _Indices>
struct __backup_storage_op_table<
        _MaskIndex, _Storage, __index_sequence<_Indices...>> {
    typedef void (* __move_func_type)(_Storage* __dest, _Storage& __source);

    typedef void (* __destroy_func_type)(_Storage* __obj);

    template <size_t _Index>
    struct __helper {
        typedef __backup_storage_ops<_Index, _MaskIndex, _Storage> __ops;
    };

    static const __move_func_type __move_ops[sizeof...(_Indices)];
    static const __destroy_func_type __destroy_ops[sizeof...(_Indices)];
};

template <ptrdiff_t _MaskIndex, typename _Storage, ptrdiff_t ... _Indices>
const typename __backup_storage_op_table<
        _MaskIndex, _Storage, __index_sequence<_Indices...>>::__move_func_type
        __backup_storage_op_table<
                _MaskIndex, _Storage, __index_sequence<_Indices...>>::__move_ops[
        sizeof...(_Indices)] = {
        &__helper<_Indices>::__ops::__move_construct_func...
};

template <ptrdiff_t _MaskIndex, typename _Storage, ptrdiff_t ... _Indices>
const typename __backup_storage_op_table<
        _MaskIndex, _Storage, __index_sequence<_Indices...>>::__destroy_func_type
        __backup_storage_op_table<
                _MaskIndex, _Storage, __index_sequence<_Indices...>>::__destroy_ops[
        sizeof...(_Indices)] = {
        &__helper<_Indices>::__ops::__destroy_func...
};

template <ptrdiff_t _Index, typename ... _Types>
struct __backup_storage {
    typedef __variant_data<_Types...> __storage_type;

    typedef __backup_storage_op_table<
            _Index, __storage_type, typename __type_indices<_Types...>::__type>
            __op_table_type;

    ptrdiff_t __backup_index;
    __storage_type& __live_storage;
    __storage_type __backup;

    __backup_storage(ptrdiff_t __live_index_, __storage_type& __live_storage_)
            :
            __backup_index(__live_index_), __live_storage(__live_storage_)
    {
        if (__backup_index >= 0) {
            __op_table_type::__move_ops[__backup_index](
                    &__backup, __live_storage);
            __op_table_type::__destroy_ops[__backup_index](
                    &__live_storage);
        }
    }

    void __destroy()
    {
        if (__backup_index >= 0)
            __op_table_type::__destroy_ops[__backup_index](
                    &__backup);
        __backup_index = -1;
    }

    ~__backup_storage()
    {
        if (__backup_index >= 0) {
            __op_table_type::__move_ops[__backup_index](
                    &__live_storage, __backup);
            __destroy();
        }
    }
};

template <typename ... _Types>
struct __all_move_constructible;

template <typename _Head, typename ... _Rest>
struct __all_move_constructible<_Head, _Rest...> {
    static constexpr bool value = std::is_move_constructible<_Head>::value
            && __all_move_constructible<_Rest...>::value;
};

template <>
struct __all_move_constructible<> :
        std::true_type {
};

template <typename ... _Types>
struct __all_move_assignable;

template <typename _Head, typename ... _Rest>
struct __all_move_assignable<_Head, _Rest...> {
    static constexpr bool value = std::is_move_assignable<_Head>::value
            && __all_move_assignable<_Rest...>::value;
};

template <>
struct __all_move_assignable<> :
        std::true_type {
};

template <typename ... _Types>
struct __all_copy_assignable;

template <typename _Head, typename ... _Rest>
struct __all_copy_assignable<_Head, _Rest...> {
    static constexpr bool value = std::is_copy_assignable<_Head>::value
            && __all_copy_assignable<_Rest...>::value;
};

template <>
struct __all_copy_assignable<> :
        std::true_type {
};

namespace __swap_test_detail {
using std::swap;

template <typename _Other>
struct __swap_result {};

template <typename>
static char __test(...);

template <typename _Other>
static std::pair<char, std::pair<char, __swap_result<decltype(
swap(std::declval<_Other&>(), std::declval<_Other&>()))>>>
__test(_Other*);
}

template <typename _Type>
struct __is_swappable {
    static constexpr bool value =
            sizeof(__swap_test_detail::__test<_Type>(0)) != 1;
};

template <typename ... _Types>
struct __all_swappable;

template <typename _Head, typename ... _Rest>
struct __all_swappable<_Head, _Rest...> {
    static constexpr bool value =
            __is_swappable<_Head>::value && __all_swappable<_Rest...>::value;
};

template <>
struct __all_swappable<> :
        std::true_type {
};

template <bool _MoveConstructible, typename ... _Types>
struct __noexcept_variant_move_construct_impl {};

template <typename _Head, typename ... _Rest>
struct __noexcept_variant_move_construct_impl<true, _Head, _Rest...> {
    static constexpr bool value = noexcept(_Head(std::declval<_Head&&>()))
            && __noexcept_variant_move_construct_impl<true, _Rest...>::value;
};

template <>
struct __noexcept_variant_move_construct_impl<true> {
    static constexpr bool value = true;
};

template <typename ... _Types>
struct __noexcept_variant_move_construct :
        __noexcept_variant_move_construct_impl<__all_move_constructible<_Types...>::value, _Types...> {
};

template <bool _MoveAssignable, typename ... _Types>
struct __noexcept_variant_move_assign_impl {};

template <typename _Head, typename... _Rest>
struct __noexcept_variant_move_assign_impl<true, _Head, _Rest...> {
    static constexpr bool value =
            std::is_nothrow_move_assignable<_Head>::value &&
                    std::is_nothrow_move_constructible<_Head>::value &&
                    __noexcept_variant_move_assign_impl<true, _Rest...>::value;
};

template <>
struct __noexcept_variant_move_assign_impl<true> {
    static constexpr bool value = true;
};

template <typename... _Types>
struct __noexcept_variant_move_assign
        : __noexcept_variant_move_assign_impl<
                __all_move_assignable<_Types...>::value &&
                        __all_move_constructible<_Types...>::value,
                _Types...> {
};

template <typename ... _Types>
struct __all_copy_constructible;

template <typename _Head, typename ... _Rest>
struct __all_copy_constructible<_Head, _Rest...> {
    static constexpr bool value = std::is_copy_constructible<_Head>::value
            && __all_copy_constructible<_Rest...>::value;
};

template <>
struct __all_copy_constructible<> :
        std::true_type {
};

template <bool _CopyConstructible, typename ... _Types>
struct __noexcept_variant_const_copy_construct_impl {};

template <typename _Head, typename ... _Rest>
struct __noexcept_variant_const_copy_construct_impl<true, _Head, _Rest...> {
    static constexpr bool value = noexcept(_Head(std::declval<_Head const&>()))
            && __noexcept_variant_const_copy_construct_impl<true, _Rest...>::value;
};

template <>
struct __noexcept_variant_const_copy_construct_impl<true> {
    static constexpr bool value = true;
};

template <typename ... _Types>
struct __noexcept_variant_const_copy_construct :
        __noexcept_variant_const_copy_construct_impl<__all_copy_constructible<_Types...>::value, _Types...> {
};

template <bool _CopyNon_Constructible, typename ... _Types>
struct __noexcept_variant_non_const_copy_construct_impl {};

template <typename _Head, typename ... _Rest>
struct __noexcept_variant_non_const_copy_construct_impl<true, _Head, _Rest...> {
    static constexpr bool value = noexcept(_Head(std::declval<_Head&>()))
            && __noexcept_variant_non_const_copy_construct_impl<true, _Rest...>::value;
};

template <>
struct __noexcept_variant_non_const_copy_construct_impl<true> {
    static constexpr bool value = true;
};

template <typename ... _Types>
struct __noexcept_variant_non_const_copy_construct :
        __noexcept_variant_non_const_copy_construct_impl<__all_copy_constructible<_Types...>::value, _Types...> {
};

template <bool _Swappable, typename ... _Types>
struct __noexcept_variant_swap_impl {};

template <typename _Head, typename... _Rest>
struct __noexcept_variant_swap_impl<true, _Head, _Rest...> {
    static constexpr bool value =
            noexcept(std::swap(std::declval<_Head&>(), std::declval<_Head&>())) &&
                    __noexcept_variant_swap_impl<true, _Rest...>::value;
};

template <>
struct __noexcept_variant_swap_impl<true> {
    static constexpr bool value = true;
};

template <typename ... _Types>
struct __noexcept_variant_swap :
        __noexcept_variant_swap_impl<__all_swappable<_Types...>::value, _Types...> {
};

template <typename ... _Types>
class variant :
        private __variant_base<
                variant<_Types...>, __all_trivially_destructible<_Types...>::__value> {
    typedef __variant_base<variant<_Types...>, __all_trivially_destructible<_Types...>::__value> __base_type;
    friend __base_type;
    friend struct __copy_construct_op_table<variant>;
    template <typename _Variant, typename _Alloc, typename _Indices>
    friend
    struct __copy_construct_alloc_op_table;
    friend struct __copy_assign_op_table<variant>;
    friend struct __move_construct_op_table<variant>;
    template <typename _Variant, typename _Alloc, typename _Indices>
    friend
    struct __move_construct_alloc_op_table;
    friend struct __move_assign_op_table<variant>;
    friend struct __destroy_op_table<variant>;

    template <ptrdiff_t _Index, typename ... _Types2>
    friend
    struct __variant_accessor;

    friend struct __replace_construct_helper;

    typedef __variant_data<_Types...> __storage_type;
    __storage_type __storage;
    typename __discriminator_type<sizeof ... (_Types)>::__type __index;

    template <size_t _Index, typename ... _Args>
    size_t __emplace_construct(_Args&& ... __args)
    {
        new(&__storage) __storage_type(
                in_place_index<_Index>, std::forward<_Args>(__args)...);
        return _Index;
    }

    void __destroy_self()
    {
        if (valueless_by_exception())
            return;
        __destroy_op_table<variant>::__apply[index()](this);
        __index = -1;
    }

    ptrdiff_t __move_construct(variant& __other)
    {
        ptrdiff_t const __other_index = __other.index();
        if (__other_index == -1)
            return -1;
        __move_construct_op_table<variant>::__apply[__other_index](this,
                                                                   __other);
        __other.__destroy_self();
        return __other_index;
    }

    template <typename _Alloc>
    ptrdiff_t __move_construct(_Alloc const& __alloc, variant& __other)
    {
        ptrdiff_t const __other_index = __other.index();
        if (__other_index == -1)
            return -1;
        __move_construct_alloc_op_table<variant, _Alloc>::__apply[__other_index](
                this, __alloc, __other);
        __other.__destroy_self();
        return __other_index;
    }

    ptrdiff_t __copy_construct(variant const& __other)
    {
        ptrdiff_t const __other_index = __other.index();
        if (__other_index == -1)
            return -1;
        __copy_construct_op_table<variant>::__apply[__other_index](this,
                                                                   __other);
        return __other_index;
    }

    template <typename _Alloc>
    ptrdiff_t __copy_construct(_Alloc const& __alloc, variant const& __other)
    {
        ptrdiff_t const __other_index = __other.index();
        if (__other_index == -1)
            return -1;
        __copy_construct_alloc_op_table<variant, _Alloc>::__apply[__other_index](
                this, __alloc, __other);
        return __other_index;
    }

    template <size_t _Index, typename ... _Args>
    void __replace_construct(_Args&& ... __args)
    {
        typedef typename __indexed_type<_Index, _Types...>::__type __this_type;
        __replace_construct_helper::__helper<
                _Index,
                __storage_nothrow_constructible<__this_type, _Args...>::__value
                        ||
                                (sizeof...(_Types) == 1),
                __storage_nothrow_move_constructible<__this_type>::__value,
                __other_storage_nothrow_move_constructible<
                        _Index, _Types...>::__value
        >::__trampoline(*this, std::forward<_Args>(__args)...);
    }

    template <size_t _Index, typename ... _Args>
    void __two_stage_replace(_Args&& ... __args)
    {
        typedef typename __indexed_type<_Index, _Types...>::__type __type;
        __variant_data<__type> __local(
                in_place_index<0>, std::forward<_Args>(__args)...);
        __destroy_self();
        __emplace_construct<_Index>(
                std::move(__local.__get(in_place_index<0>)));
        __index = _Index;
        __local.__destroy(in_place_index<0>);
    }

    template <size_t _Index, typename ... _Args>
    void __local_backup_replace(_Args&& ... __args)
    {
        __backup_storage<_Index, _Types...> __backup(__index, __storage);
        __emplace_construct<_Index>(std::forward<_Args>(__args)...);
        __index = _Index;
        __backup.__destroy();
    }

    template <size_t _Index, typename ... _Args>
    void __direct_replace(_Args&& ... __args)
    {
        __destroy_self();
        __emplace_construct<_Index>(std::forward<_Args>(__args)...);
        __index = _Index;
    }

    struct __private_type {};

public:
    constexpr variant()
    noexcept(noexcept(typename __indexed_type<0, _Types...>::__type()))
            :
            __storage(in_place_index<0>),
            __index(0) {}

    constexpr variant(
            typename std::conditional<__all_move_constructible<_Types...>::value, variant, __private_type>::type&& __other)
    noexcept(__noexcept_variant_move_construct<_Types...>::value)
            :
            __index(__move_construct(__other)) {}

    constexpr variant(
            typename std::conditional<!__all_move_constructible<_Types...>::value, variant, __private_type>::type&& __other) = delete;

    constexpr variant(
            typename std::conditional<__all_copy_constructible<_Types...>::value, variant, __private_type>::type& __other)
    noexcept(__noexcept_variant_non_const_copy_construct<_Types...>::value)
            :
            __index(__copy_construct(__other)) {}

    constexpr variant(
            typename std::conditional<!__all_copy_constructible<_Types...>::value, variant, __private_type>::type& __other) = delete;

    constexpr variant(
            typename std::conditional<__all_copy_constructible<_Types...>::value, variant, __private_type>::type const& __other)
    noexcept(__noexcept_variant_const_copy_construct<_Types...>::value)
            :
            __index(__copy_construct(__other)) {}

    constexpr variant(
            typename std::conditional<!__all_copy_constructible<_Types...>::value, variant, __private_type>::type const& __other) = delete;

    template <typename _Type, typename ... _Args>
    explicit constexpr variant(in_place_type_t<_Type>, _Args&& ... __args)
            :
            __storage(
                    in_place_index<__type_index<_Type, _Types...>::__value>,
                    std::forward<_Args>(__args)...),
            __index(__type_index<_Type, _Types...>::__value)
    {
        static_assert(std::is_constructible<_Type, _Args...>::value,
                      "Type must be constructible from args");
    }

    template <size_t _Index, typename ... _Args>
    explicit constexpr variant(in_place_index_t<_Index>, _Args&& ... __args)
            :
            __storage(in_place_index<_Index>, std::forward<_Args>(__args)...),
            __index(_Index)
    {
        static_assert(
                std::is_constructible<typename __indexed_type<_Index, _Types...>::__type, _Args...>::value,
                "Type must be constructible from args");
    }

    template <typename _Type>
    constexpr variant(_Type&& __x)
            :
            __storage(
                    in_place_index<
                            __type_index_to_construct<_Type, _Types...>::__value>,
                    std::forward<_Type>(__x)),
            __index(__type_index_to_construct<_Type, _Types...>::__value) {}

    template <typename _Type,
            typename _Enable=
            typename std::enable_if<
                    (__constructible_matches<std::initializer_list<_Type>, _Types...>::__type::__length
                            > 0)
            >::type>
    constexpr variant(std::initializer_list<_Type> __x)
            :
            __storage(
                    in_place_index<
                            __type_index_to_construct<std::initializer_list<_Type>, _Types...>::__value>,
                    __x),
            __index(__type_index_to_construct<std::initializer_list<_Type>, _Types...>::__value) {}

    template <typename _Alloc>
    variant(std::allocator_arg_t, _Alloc const& __alloc)
            :
            __storage(in_place_index<0>, std::allocator_arg_t(), __alloc),
            __index(0) {}

    template <typename _Alloc, size_t _Index, typename ... _Args>
    variant(
            std::allocator_arg_t, _Alloc const& __alloc,
            in_place_index_t<_Index>, _Args&& ... __args)
            :
            __storage(in_place_index<_Index>, std::allocator_arg_t(), __alloc,
                      std::forward<_Args>(__args)...),
            __index(_Index)
    {
        using __constructed_type=typename __indexed_type<_Index, _Types...>::__type;
        static_assert(
                (std::uses_allocator<__constructed_type, _Alloc>::value &&
                        (std::is_constructible<__constructed_type, std::allocator_arg_t, _Alloc, _Args...>::value
                                ||
                                        std::is_constructible<__constructed_type, _Args..., _Alloc>::value))
                        ||
                                (!std::uses_allocator<__constructed_type, _Alloc>::value
                                        &&
                                                std::is_constructible<__constructed_type, _Args...>::value),
                "Type must be uses-allocator constructible from args or not use allocator");
    }

    template <typename _Alloc, typename _Type, typename ... _Args>
    variant(
            std::allocator_arg_t, _Alloc const& __alloc,
            in_place_type_t<_Type>, _Args&& ... __args)
            :
            __storage(
                    in_place_index<__type_index<_Type, _Types...>::__value>,
                    std::allocator_arg_t(), __alloc,
                    std::forward<_Args>(__args)...),
            __index(__type_index<_Type, _Types...>::__value)
    {
        using __constructed_type=_Type;
        static_assert(
                (std::uses_allocator<__constructed_type, _Alloc>::value &&
                        (std::is_constructible<__constructed_type, std::allocator_arg_t, _Alloc, _Args...>::value
                                ||
                                        std::is_constructible<__constructed_type, _Args..., _Alloc>::value))
                        ||
                                (!std::uses_allocator<__constructed_type, _Alloc>::value
                                        &&
                                                std::is_constructible<__constructed_type, _Args...>::value),
                "Type must be uses-allocator constructible from args or not use allocator");
        static_assert(std::is_constructible<_Type, _Args...>::value,
                      "Type must be constructible from args");
    }

    template <typename _Alloc>
    variant(
            std::allocator_arg_t, _Alloc const& __alloc, variant& __other)
            :
            __index(__copy_construct(__alloc, __other)) {}

    template <typename _Alloc>
    variant(
            std::allocator_arg_t, _Alloc const& __alloc, variant const& __other)
            :
            __index(__copy_construct(__alloc, __other)) {}

    template <typename _Alloc>
    variant(
            std::allocator_arg_t, _Alloc const& __alloc, variant&& __other)
            :
            __index(__move_construct(__alloc, __other)) {}

    template <typename _Type>
    variant& operator=(_Type&& __x)
    {
        constexpr size_t _Index =
                __type_index_to_construct<_Type, _Types...>::__value;
        if (_Index == __index) {
            get<_Index>(*this) = std::forward<_Type>(__x);
        }
        else {
            __replace_construct<_Index>(std::forward<_Type>(__x));
        }
        return *this;
    }

    variant& operator=(
            typename std::conditional<
                    !(__all_copy_constructible<_Types...>::value &&
                            __all_move_constructible<_Types...>::value &&
                            __all_copy_assignable<_Types...>::value),
                    variant, __private_type>::type const& __other) = delete;

    variant& operator=(
            typename std::conditional<
                    __all_copy_constructible<_Types...>::value &&
                            __all_move_constructible<_Types...>::value &&
                            __all_copy_assignable<_Types...>::value,
                    variant, __private_type>::type const& __other)
    {
        if (__other.valueless_by_exception()) {
            __destroy_self();
        }
        else if (__other.index() == index()) {
            __copy_assign_op_table<variant>::__apply[index()](this, __other);
        }
        else {
            __replace_construct_helper::__op_table<variant>::__copy_assign[
                    __other.index()](this, __other);
        }
        return *this;
    }

    variant& operator=(
            typename std::conditional<
                    !(__all_copy_constructible<_Types...>::value &&
                            __all_move_constructible<_Types...>::value &&
                            __all_copy_assignable<_Types...>::value),
                    variant, __private_type>::type& __other) = delete;

    variant& operator=(
            typename std::conditional<
                    __all_copy_constructible<_Types...>::value &&
                            __all_move_constructible<_Types...>::value &&
                            __all_copy_assignable<_Types...>::value,
                    variant, __private_type>::type& __other)
    {
        if (__other.valueless_by_exception()) {
            __destroy_self();
        }
        else if (__other.index() == index()) {
            __copy_assign_op_table<variant>::__apply[index()](this, __other);
        }
        else {
            __replace_construct_helper::__op_table<variant>::__copy_assign[
                    __other.index()](this, __other);
        }
        return *this;
    }

    variant& operator=(
            typename std::conditional<
                    !(__all_move_constructible<_Types...>::value &&
                            __all_move_assignable<_Types...>::value),
                    variant, __private_type>::type&& __other) = delete;

    variant& operator=(
            typename std::conditional<
                    __all_move_constructible<_Types...>::value &&
                            __all_move_assignable<_Types...>::value,
                    variant, __private_type>::type&&
            __other) noexcept(__noexcept_variant_move_assign<_Types...>::value)
    {
        if (__other.valueless_by_exception()) {
            __destroy_self();
        }
        else if (__other.index() == index()) {
            __move_assign_op_table<variant>::__apply[index()](this, __other);
            __other.__destroy_self();
        }
        else {
            __replace_construct_helper::__op_table<variant>::__move_assign[
                    __other.index()](this, __other);
        }
        return *this;
    }

    template <typename _Type, typename ... _Args>
    void emplace(_Args&& ... __args)
    {
        __direct_replace<__type_index<_Type, _Types...>::__value>(
                std::forward<_Args>(__args)...);
    }

    template <size_t _Index, typename ... _Args>
    void emplace(_Args&& ... __args)
    {
        __direct_replace<_Index>(std::forward<_Args>(__args)...);
    }

    constexpr bool valueless_by_exception() const noexcept
    {
        return __index == -1;
    }

    constexpr ptrdiff_t index() const noexcept
    {
        return __index;
    }

    void swap(
            typename std::conditional<
                    __all_swappable<_Types...>::value &&
                            __all_move_constructible<_Types...>::value,
                    variant, __private_type>::type
            & __other) noexcept(__noexcept_variant_swap<_Types...>::value)
    {
        if (__other.index() == index()) {
            if (!valueless_by_exception())
                __swap_op_table<variant>::__apply[index()](*this, __other);
        }
        else {
            variant __temp(std::move(__other));
            __other.__index = __other.__move_construct(*this);
            __index = __move_construct(__temp);
        }
    }
};

template <>
class variant<> {
public:
    variant() = delete;

    constexpr bool valueless_by_exception() const noexcept
    {
        return true;
    }

    constexpr ptrdiff_t index() const noexcept
    {
        return -1;
    }

    void swap(variant&) {}
};

template <typename... _Types>
typename std::enable_if<__all_swappable<_Types...>::value &&
        __all_move_constructible<_Types...>::value,
        void>::type
swap(variant<_Types...>& __lhs, variant<_Types...>& __rhs) noexcept(
__noexcept_variant_swap<_Types...>::value)
{
    __lhs.swap(__rhs);
}

template <ptrdiff_t _Index, typename ... _Types>
struct __variant_accessor {
    typedef typename __indexed_type<_Index, _Types...>::__type __type;

    static constexpr __type& get(variant<_Types...>& __v)
    {
        return __v.__storage.__get(in_place_index<_Index>);
    }

    static constexpr __type const& get(variant<_Types...> const& __v)
    {
        return __v.__storage.__get(in_place_index<_Index>);
    }

    static constexpr __type&& get(variant<_Types...>&& __v)
    {
        return __v.__storage.__get_rref(in_place_index<_Index>);
    }

    static constexpr const __type&& get(variant<_Types...> const&& __v)
    {
        return __v.__storage.__get_rref(in_place_index<_Index>);
    }
};

template <typename _Type, typename ... _Types>
constexpr _Type& get(variant<_Types...>& __v)
{
    return get<__type_index<_Type, _Types...>::__value>(__v);
}

template <typename _Type, typename ... _Types>
constexpr _Type&& get(variant<_Types...>&& __v)
{
    return get<__type_index<_Type, _Types...>::__value>(std::move(__v));
}

template <typename _Type, typename ... _Types>
constexpr _Type const& get(variant<_Types...> const& __v)
{
    return get<__type_index<_Type, _Types...>::__value>(__v);
}

template <typename _Type, typename ... _Types>
constexpr const _Type&& get(variant<_Types...> const&& __v)
{
    return get<__type_index<_Type, _Types...>::__value>(std::move(__v));
}

template <ptrdiff_t _Index, typename ... _Types>
constexpr typename __indexed_type<_Index, _Types...>::__type const& get(
        variant<_Types...> const& __v)
{
    return *((_Index != __v.index()) ? throw bad_variant_access(
            "Bad variant index in get") :
             &__variant_accessor<_Index, _Types...>::get(__v));
}

template <ptrdiff_t _Index, typename ... _Types>
constexpr typename __indexed_type<_Index, _Types...>::__type&
get(variant<_Types...>& __v)
{
    return *((_Index != __v.index()) ? throw bad_variant_access(
            "Bad variant index in get") :
             &__variant_accessor<_Index, _Types...>::get(__v));
}

template <ptrdiff_t _Index, typename ... _Types>
constexpr typename __indexed_type<_Index, _Types...>::__type&&
get(variant<_Types...>&& __v)
{
    return __variant_accessor<_Index, _Types...>::get(
            (((_Index != __v.index()) ? throw bad_variant_access(
                    "Bad variant index in get") : 0), std::move(__v)));
}

template <ptrdiff_t _Index, typename ... _Types>
constexpr const typename __indexed_type<_Index, _Types...>::__type&&
get(variant<_Types...> const&& __v)
{
    return __variant_accessor<_Index, _Types...>::get(
            (((_Index != __v.index()) ? throw bad_variant_access(
                    "Bad variant index in get") : 0), std::move(__v)));
}

template <typename _Type, typename ... _Types>
constexpr std::add_pointer_t<_Type> get_if(variant<_Types...>& __v)
{
    return (__type_index<_Type, _Types...>::__value != __v.index()) ? nullptr
                                                                    : &get<_Type>(
                    __v);
}

template <typename _Type, typename ... _Types>
constexpr std::add_pointer_t<_Type const> get_if(variant<_Types...> const& __v)
{
    return (__type_index<_Type, _Types...>::__value != __v.index()) ? nullptr
                                                                    : &get<_Type>(
                    __v);
}

template <ptrdiff_t _Index, typename ... _Types>
constexpr std::add_pointer_t<typename __indexed_type<_Index, _Types...>::__type>
get_if(variant<_Types...>& __v)
{
    return ((_Index != __v.index()) ? nullptr :
            &__variant_accessor<_Index, _Types...>::get(__v));
}

template <ptrdiff_t _Index, typename ... _Types>
constexpr std::add_pointer_t<typename __indexed_type<_Index, _Types...>::__type const>
get_if(
        variant<_Types...> const& __v)
{
    return ((_Index != __v.index()) ? nullptr :
            &__variant_accessor<_Index, _Types...>::get(__v));
}

template <typename _Type, typename ... _Types>
constexpr bool holds_alternative(variant<_Types...> const& __v) noexcept
{
    return __v.index() == __type_index<_Type, _Types...>::__value;
}

template <typename _Visitor, typename ... _Types>
struct __visitor_return_type;

template <typename _Visitor>
struct __visitor_return_type<_Visitor> {
    typedef decltype(std::declval<_Visitor&>()()) __type;
};

template <typename _Visitor, typename _Head, typename ... _Rest>
struct __visitor_return_type<_Visitor, _Head, _Rest...> {
    typedef decltype(std::declval<_Visitor&>()(std::declval<_Head&>())) __type;
};

template <typename _Visitor, typename ... _Types>
struct __visitor_table {
    typedef variant<_Types...> __variant_type;
    typedef typename __visitor_return_type<_Visitor, _Types...>::__type __return_type;

    typedef __return_type (* __func_type)(_Visitor&, __variant_type&);

    typedef __return_type (* __const_func_type)(_Visitor&,
                                                __variant_type const&);

    typedef __return_type (* __rv_func_type)(_Visitor&, __variant_type&);

    template <typename _Type>
    static constexpr __return_type
    __trampoline_func(_Visitor& __visitor, __variant_type& __v)
    {
        return __visitor(get<_Type>(__v));
    }

    template <typename _Type>
    static constexpr __return_type
    __trampoline_rv_func(_Visitor& __visitor, __variant_type& __v)
    {
        return __visitor(get<_Type>(std::move(__v)));
    }

    template <typename _Type>
    static constexpr __return_type
    __trampoline_const_func(_Visitor& __visitor, __variant_type const& __v)
    {
        return __visitor(get<_Type>(__v));
    }

    static constexpr __func_type __trampoline[sizeof...(_Types)] = {
            &__trampoline_func<_Types>...
    };
    static constexpr __const_func_type __trampoline_const[sizeof...(_Types)] = {
            &__trampoline_const_func<_Types>...
    };
    static constexpr __rv_func_type __trampoline_rv[sizeof...(_Types)] = {
            &__trampoline_rv_func<_Types>...
    };
};

template <typename _Visitor, typename ... _Types>
constexpr typename __visitor_table<_Visitor, _Types...>::__func_type __visitor_table<_Visitor, _Types...>::__trampoline[sizeof...(_Types)];

template <typename _Visitor, typename ... _Types>
constexpr typename __visitor_table<_Visitor, _Types...>::__const_func_type __visitor_table<_Visitor, _Types...>::__trampoline_const[sizeof...(_Types)];

template <typename _Visitor, typename ... _Types>
constexpr typename __visitor_table<_Visitor, _Types...>::__rv_func_type __visitor_table<_Visitor, _Types...>::__trampoline_rv[sizeof...(_Types)];

template <typename _Visitor, typename ... _Types>
constexpr typename __visitor_return_type<_Visitor, _Types...>::__type
visit(_Visitor&& __visitor, variant<_Types...>& __v)
{
    if (__v.valueless_by_exception())
        throw bad_variant_access("Visiting of empty variant");
    return __visitor_table<_Visitor, _Types...>::__trampoline[__v.index()](
            __visitor, __v);
}

template <typename _Visitor, typename... _Types>
constexpr typename __visitor_return_type<_Visitor, _Types...>::__type
visit(_Visitor&& __visitor, const variant<_Types...>& __v)
{
    if (__v.valueless_by_exception())
        throw bad_variant_access("Visiting of empty variant");
    return __visitor_table<
            _Visitor, _Types...>::__trampoline_const[__v.index()](__visitor,
                                                                  __v);
}

template <typename _Visitor, typename... _Types>
constexpr typename __visitor_return_type<_Visitor, _Types...>::__type
visit(_Visitor&& __visitor, variant<_Types...>&& __v)
{
    if (__v.valueless_by_exception())
        throw bad_variant_access("Visiting of empty variant");
    return __visitor_table<
            _Visitor, _Types...>::__trampoline_rv[__v.index()](__visitor, __v);
}

template <typename _Visitor, typename _First, typename _Second,
        typename... _Variants>
struct __multi_visitor_return_type {
    typedef decltype(
    std::declval<_Visitor&>()(
            get<0>(std::declval<_First>()), get<0>(std::declval<_Second>()),
            get<0>(std::declval<_Variants>())...)) __type;
};

template <size_t _Count, typename _CurrentSequence=__index_sequence<>>
struct __make_index_sequence {
    typedef typename __make_index_sequence<
            _Count - 1, typename _CurrentSequence::__next>::type type;
};

template <typename _CurrentSequence>
struct __make_index_sequence<0, _CurrentSequence> {
    typedef _CurrentSequence type;
};

template <typename _ReturnType, typename _Visitor, typename _Args,
        ptrdiff_t... _Indices>
constexpr _ReturnType __apply_visitor_impl(
        _Visitor& __visitor, _Args& __args, __index_sequence<_Indices...>)
{
    return __visitor(std::get<_Indices>(__args)...);
}

template <typename _ReturnType, typename _Visitor, typename _Args>
constexpr _ReturnType __apply_visitor(_Visitor& __visitor, _Args&& __args)
{
    return __apply_visitor_impl<_ReturnType>(
            __visitor, __args,
            typename __make_index_sequence<std::tuple_size<_Args>::value>::type());
}

template <typename _Visitor, typename _ReturnType, typename _Args, typename ... _Variants>
struct __multivisitor;

template <typename ... _Variants>
struct __mv_variant_list;

template <typename _First, typename ... _Variants>
struct __mv_variant_list<_First, _Variants...> {
    _First& __first;
    __mv_variant_list<_Variants...> __rest;

    constexpr __mv_variant_list(_First& __first_, _Variants& ... __variants_)
            :
            __first(__first_), __rest(__variants_...) {}

    constexpr _First& __get_first() const
    {
        return __first;
    }

    constexpr __mv_variant_list<_Variants...> __get_remainder() const
    {
        return __rest;
    }
};

template <>
struct __mv_variant_list<> {
};

template <typename _Tuple, typename _Extra>
struct __add_to_tuple;

template <typename ... _TupleTypes, typename _Extra>
struct __add_to_tuple<std::tuple<_TupleTypes...>, _Extra> {
    typedef std::tuple<_TupleTypes..., _Extra&&> type;

    template <ptrdiff_t ... _Indices>
    static constexpr type
    __add_impl(std::tuple<_TupleTypes...>& __args, _Extra&& __extra,
               __index_sequence<_Indices...>)
    {
        return type(std::get<_Indices>(__args)...,
                    std::forward<_Extra>(__extra));
    }

    static constexpr type
    __add(std::tuple<_TupleTypes...>& __args, _Extra&& __extra)
    {
        return __add_impl(
                __args, std::forward<_Extra>(__extra),
                typename __make_index_sequence<sizeof...(_TupleTypes)>::type());
    }
};

template <typename _Visitor, typename _ReturnType, typename _Args>
struct __multivisitor<_Visitor, _ReturnType, _Args> {
    _Visitor& __visitor;
    _Args __args;

    constexpr __multivisitor(
            _Visitor& __visitor_, _Args&& __args_,
            __mv_variant_list<> __variants)
            :
            __visitor(__visitor_), __args(__args_) {}

    constexpr __multivisitor(_Visitor& __visitor_)
            :
            __visitor(__visitor_), __args() {}

    template <typename _VisitVal>
    constexpr _ReturnType operator()(_VisitVal&& __thisval)
    {
        typedef __add_to_tuple<_Args, _VisitVal> __add_args;
        return __apply_visitor<_ReturnType>(
                __visitor,
                __add_args::__add(__args, std::forward<_VisitVal>(__thisval)));
    }
};

template <typename _Visitor, typename _ReturnType, typename _Args,
        typename _First, typename... _Variants>
struct __multivisitor<_Visitor, _ReturnType, _Args, _First, _Variants...> {

    _Visitor& __visitor;
    _Args __args;
    _First&& __first;
    __mv_variant_list<_Variants...> __remaining_variants;

    constexpr __multivisitor(
            _Visitor& __visitor_, _Args&& __args_,
            __mv_variant_list<_First, _Variants...> __variants)
            : __visitor(__visitor_), __args(__args_),
              __first(__variants.__get_first()),
              __remaining_variants(__variants.__get_remainder()) {}

    constexpr __multivisitor(
            _Visitor& __visitor_, _First&& __first_,
            _Variants&& ... __variants_)
            :
            __visitor(__visitor_), __args(),
            __first(std::forward<_First>(__first_)),
            __remaining_variants(std::forward<_Variants>(__variants_)...) {}

    template <typename _VisitVal>
    constexpr _ReturnType operator()(_VisitVal&& __thisval)
    {
        typedef __add_to_tuple<_Args, _VisitVal> __add_args;
        typedef typename __add_args::type __args_type;

        return visit(
                __multivisitor<_Visitor, _ReturnType, __args_type, _Variants...>(
                        __visitor,
                        __add_args::__add(__args,
                                          std::forward<_VisitVal>(__thisval)),
                        std::move(__remaining_variants)),
                std::forward<_First>(__first));
    }
};

template <typename _Visitor, typename _First, typename ... _Variants>
constexpr typename __multi_visitor_return_type<_Visitor, _First, _Variants...>::__type
__multivisit(_Visitor& __visitor, _First&& __first, _Variants&& ... __v)
{
    typedef typename __multi_visitor_return_type<
            _Visitor, _First, _Variants...>::__type __return_type;
    return visit(
            __multivisitor<_Visitor, __return_type, std::tuple<>, _Variants...>(
                    __visitor, std::forward<_Variants>(__v)...),
            __first);
}

template <typename _Visitor, typename ... _Variants>
constexpr typename __multi_visitor_return_type<_Visitor, _Variants...>::__type
visit(_Visitor&& __visitor, _Variants&& ... __v)
{
    return __multivisit(__visitor, std::forward<_Variants>(__v)...);
}

template <typename ... _Types>
constexpr bool
operator==(variant<_Types...> const& __lhs, variant<_Types...> const& __rhs)
{
    return (__lhs.index() == __rhs.index()) &&
            ((__lhs.index() == -1) ||
                    __equality_op_table<variant<_Types...>>::__equality_compare[__lhs.index()](
                            __lhs, __rhs));
}

template <typename ... _Types>
constexpr bool
operator!=(variant<_Types...> const& __lhs, variant<_Types...> const& __rhs)
{
    return !(__lhs == __rhs);
}

template <typename ... _Types>
constexpr bool
operator<(variant<_Types...> const& __lhs, variant<_Types...> const& __rhs)
{
    return (__lhs.index() < __rhs.index()) ||
            ((__lhs.index() == __rhs.index()) &&
                    ((__lhs.index() != -1) &&
                            __less_than_op_table<variant<_Types...>>::
                            __less_than_compare[__lhs.index()](__lhs, __rhs)));
}

template <typename ... _Types>
constexpr bool
operator>(variant<_Types...> const& __lhs, variant<_Types...> const& __rhs)
{
    return __rhs < __lhs;
}

template <typename ... _Types>
constexpr bool
operator>=(variant<_Types...> const& __lhs, variant<_Types...> const& __rhs)
{
    return !(__lhs < __rhs);
}

template <typename ... _Types>
constexpr bool
operator<=(variant<_Types...> const& __lhs, variant<_Types...> const& __rhs)
{
    return !(__lhs > __rhs);
}

struct monostate {};

constexpr inline bool
operator==(monostate const&, monostate const&) { return true; }

constexpr inline bool
operator!=(monostate const&, monostate const&) { return false; }

constexpr inline bool
operator>=(monostate const&, monostate const&) { return true; }

constexpr inline bool
operator<=(monostate const&, monostate const&) { return true; }

constexpr inline bool
operator>(monostate const&, monostate const&) { return false; }

constexpr inline bool
operator<(monostate const&, monostate const&) { return false; }

struct __hash_visitor {
    template <typename _Type>
    size_t operator()(_Type const& __x)
    {
        return std::hash<_Type>()(__x);
    }
};

} // end namespace

namespace std {

template<>
struct hash<STX_NAMESPACE_NAME::monostate>{
    size_t operator()(STX_NAMESPACE_NAME::monostate) noexcept{
        return 42;
    }
};

template<typename ... _Types>
struct hash<STX_NAMESPACE_NAME::variant<_Types...>>{
    size_t operator()(STX_NAMESPACE_NAME::variant<_Types...> const &v) noexcept {
        return std::hash<ptrdiff_t>()(v.index()) ^
                STX_NAMESPACE_NAME::visit(STX_NAMESPACE_NAME::__hash_visitor(), v);
    }
};

template<typename ... _Args,typename _Alloc>
struct uses_allocator<STX_NAMESPACE_NAME::variant<_Args...>,_Alloc>:
    true_type{};

}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // STX_HAVE_STD_VARIANT

#endif // STX_VARIANT_HPP_INCLUDED
