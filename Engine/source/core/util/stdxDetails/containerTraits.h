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

#ifndef _CORE_UTIL_STDX_DETAILS_CONTAINER_TRAITS_H_
#define _CORE_UTIL_STDX_DETAILS_CONTAINER_TRAITS_H_

#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <type_traits>

namespace StdX
{
namespace Traits
{

template <typename>
struct IsTuple : std::false_type {};

template <typename... T>
struct IsTuple<std::tuple<T...>> : std::true_type {};

template <typename T, typename... Types>
struct IsArray : std::false_type {};

template <typename T, std::size_t N>
struct IsArray<std::array<T, N>> : std::true_type {};

template <typename T, typename... Types>
struct IsDeque : std::false_type {};

template <typename... Types>
struct IsDeque<std::deque<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsForwardList : std::false_type {};

template <typename... Types>
struct IsForwardList<std::forward_list<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsList : std::false_type {};

template <typename... Types>
struct IsList<std::list<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsVector : std::false_type {};

template <typename... Types>
struct IsVector<std::vector<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsMap : std::false_type {};

template <typename... Types>
struct IsMap<std::map<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsSet : std::false_type {};

template <typename... Types>
struct IsSet<std::set<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsUnorderedMap : std::false_type {};

template <typename... Types>
struct IsUnorderedMap<std::unordered_map<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsUnorderedSet : std::false_type {};

template <typename... Types>
struct IsUnorderedSet<std::unordered_set<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsMultiSet : std::false_type {};

template <typename... Types>
struct IsMultiSet<std::multiset<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsMultiMap : std::false_type {};

template <typename... Types>
struct IsMultiMap<std::multimap<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsUnorderedMultiMap : std::false_type {};

template <typename... Types>
struct IsUnorderedMultiMap<std::unordered_multimap<Types...>> : std::true_type {};


template <typename T, typename... Types>
struct IsUnorderedMultiSet : std::false_type {};

template <typename... Types>
struct IsUnorderedMultiSet<std::unordered_multiset<Types...>> : std::true_type {};


// NOTE: std::array is not included in the list due to the fact that
// it only partially satisfies Sequence requirements
template <typename T>
struct IsSequence
	: std::disjunction<IsDeque<T>, IsForwardList<T>, IsList<T>, IsVector<T>> {};

template <typename T>
struct IsOrderedAssociativeContainer
	: std::disjunction<IsSet<T>, IsMultiSet<T>, IsMap<T>, IsMultiMap<T>> {};

template <typename T>
struct IsUnorderedAssociativeContainer
	: std::disjunction <
	  IsUnorderedSet<T>,
	  IsUnorderedMultiSet<T>,
	  IsUnorderedMap<T>,
	  IsUnorderedMultiMap<T>
	  > {};

template <typename T>
struct IsAssociativeContainer
	: std::disjunction <
	  IsOrderedAssociativeContainer<T>,
	  IsUnorderedAssociativeContainer<T>
	  > {};

template <typename T>
struct IsContainer
	: std::disjunction<IsSequence<T>, IsAssociativeContainer<T>, IsArray<T>> {};

} // namespace Traits
} // namespace StdX

#endif // _CORE_UTIL_STDX_DETAILS_CONTAINER_TRAITS_H_
