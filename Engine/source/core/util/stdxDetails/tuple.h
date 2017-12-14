//-----------------------------------------------------------------------------
/// Copyright (c) 2013-2017 BitBox, Ltd.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------
#pragma once

#ifndef _CORE_UTIL_STDX_DETAILS_TUPLE_H_
#define _CORE_UTIL_STDX_DETAILS_TUPLE_H_

#include <tuple>

namespace StdX
{

namespace TupleDetails
{

template <typename Tuple, size_t... Indices>
auto TailImpl(const Tuple& tpl, std::index_sequence<Indices...>)
{
	return std::make_tuple(std::get < Indices + 1 > (tpl)...);
}

template <typename Tuple, size_t... Indices>
auto TailImpl(Tuple&& tpl, std::index_sequence<Indices...>)
{
	return std::make_tuple(std::move(std::get < Indices + 1 > (tpl))...);
}

template <typename T>
struct TailTypeImpl
{
};

template <typename Head, typename... Tail>
struct TailTypeImpl<std::tuple<Head, Tail...>>
{
	using Type = std::tuple<Tail...>;
};

template <typename... T>
struct IsAppliableImpl : std::false_type {};

template <typename Callable, typename... TupleElements>
struct IsAppliableImpl<Callable, std::tuple<TupleElements...>> : StdX::Traits::IsCallable<Callable(TupleElements...)> {};

} // namespace TupleDetails

template <typename Head, typename... Tail>
std::tuple<Tail...> TupleTail(const std::tuple<Head, Tail...>& tpl)
{
	return TupleDetails::TailImpl(tpl, std::make_index_sequence<sizeof...(Tail)>());
}

template <typename Head, typename... Tail>
std::tuple<Tail...> TupleTail(std::tuple<Head, Tail...>&& tpl)
{
	return TupleDetails::TailImpl(std::move(tpl), std::make_index_sequence<sizeof...(Tail)>());
}

template <typename T>
using TupleTailType = typename TupleDetails::TailTypeImpl<T>::Type;

template <size_t N, typename... T>
using ParameterPackNthType = std::tuple_element_t<N, std::tuple<T...>>;

namespace Traits
{

/**
 * @brief Allows to check whether std::apply may be called for given
 * callable object with given tuple
 */
template <typename Callable, typename Tuple>
using IsAppliable = TupleDetails::IsAppliableImpl<Callable, Tuple>;

} // namespace Traits

} // namespace StdX

#endif // _CORE_UTIL_STDX_DETAILS_TUPLE_H_
