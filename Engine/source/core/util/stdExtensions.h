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

#ifndef _CORE_UTIL_STD_EXTENSIONS_H_
#define _CORE_UTIL_STD_EXTENSIONS_H_

#include "core/util/stdxDetails/containerTraits.h"
#include "core/util/stdxDetails/traits.h"
#include "core/util/stdxDetails/functional.h"
#include "core/util/stdxDetails/tuple.h"

#include <vector>
#include <mutex>
#include <tuple>

namespace StdX
{

/**
 * @brief Converts std::unique_ptr of base type to std::unique_ptr of derived type
 * by using static_cast internally
 * @details Ownership of the object is transfered to the returned std::unique_ptr.
 * It is somewhat analogous to std::static_ptr_cast
 */
template<typename Derived, typename Base, typename Deleter>
std::unique_ptr<Derived, Deleter> StaticUptrCast(std::unique_ptr<Base, Deleter> base)
{
	auto deleter = base.get_deleter();
	auto derivedPtr = static_cast<Derived*>(base.release());
	return std::unique_ptr<Derived, Deleter>(derivedPtr, std::move(deleter));
}

/**
 * @details A separate StaticUptrCast() version for std::default_delete<T> is
 * required to provide expected results in use-cases like
 *
 * std::unique_ptr<Base> base(new Derived());
 * auto derived = StaticUptrCast<Derived>(std::move(base));
 *
 * It is desirable for `derived` to have type `std::unique_ptr<Derived>`,
 * but if general version of StaticUptrCast() is used, it will actually
 * be `std::unique_ptr<Derived, std::default_delete<Base>>` which is very
 * inconvenient
 */
template<typename Derived, typename Base>
std::unique_ptr<Derived> StaticUptrCast(std::unique_ptr<Base> base) noexcept
{
	auto derivedPtr = static_cast<Derived*>(base.release());
	return std::unique_ptr<Derived>(derivedPtr);
}

/**
 * @brief Converts std::unique_ptr of base type to std::unique_ptr of derived type
 * by using dynamic_cast internally
 * @details Ownership of the object will be transfered to the returned std::unique_ptr
 * if and only if the dynamic_cast from Base to Derived is successful. Otherwise
 * the input std::unique_ptr will continue to be the owner of the object
 * It is somewhat analogous to std::dynamic_ptr_cast
 */
template<typename Derived, typename Base, typename Deleter>
std::unique_ptr<Derived, Deleter> DynamicUptrCast(std::unique_ptr<Base, Deleter>&& base)
{
	if (auto derived = dynamic_cast<Derived*>(base.get()))
	{
		auto deleter = base.get_deleter();
		base.release();
		return std::unique_ptr<Derived, Deleter>(derived, std::move(deleter));
	}

	return nullptr;
}

/**
 * @details see description to @see StaticUptrCast() version that works only
 * with `std::default_delete<T>` for explanation why this special version of
 * DynamicUptrCast() is required
 */
template<typename Derived, typename Base>
std::unique_ptr<Derived> DynamicUptrCast(std::unique_ptr<Base>&& base) noexcept
{
	if (auto derived = dynamic_cast<Derived*>(base.get()))
	{
		base.release();
		return std::unique_ptr<Derived>(derived);
	}

	return nullptr;
}

/**
 * @brief Erases element from container and breaks ordering of the elements
 * that are stored after it as a side effect
 * @details Does not copy elements after erased element like std::vector::erase()
 * and performs with O(1) complexity
 *
 * @param from Container to erase element from
 * @param what Iterator assotiated with the element that must be erased,
 * will be invalidated
 */
template <typename T>
void UnorderingErase(std::vector<T>* from, typename std::vector<T>::iterator what)
{
	std::swap(*what, from->back());
	from->pop_back();
}

template <typename T>
void UnorderingErase(std::vector<T>* from, size_t index)
{
	std::swap((*from)[index], from->back());
	from->pop_back();
}

/**
 * @brief Pops front element from container and breaks ordering of the elements
 * that are stored after it as a side effect
 * @details Performs with the same complexity as @see UnorderingErase()
 */
template <typename T>
void UnorderingPopFront(std::vector<T>* from)
{
	UnorderingErase(from, from->begin());
}

/**
 * @brief Erases requested element + returns its value ("takes out" element) and
 * breaks ordering of the elements that are stored after it as a side effect
 * @details Performs with the same complexity as @see UnorderingErase()
 *
 * @param from Container to take element out from
 * @param what Iterator assotiated with the element that must be erased,
 * will be invalidated
 * @return Value of the erased element
 */
template <typename T>
T UnorderingTakeOut(std::vector<T>* from, typename std::vector<T>::iterator what)
{
	T ret = std::move(*what);
	UnorderingErase(from, what);
	return ret;
}

template <typename T>
T UnorderingTakeOut(std::vector<T>* from, size_t index)
{
	T ret = std::move((*from)[index]);
	UnorderingErase(from, index);
	return ret;
}

/**
 * @brief Erases requested element + returns its value ("takes out" element) and
 * breaks ordering of the elements that are stored after it as a side effect
 * @details Performs with the same complexity as @see UnorderingErase()
 *
 * @param from Container to take element out from
 * @return Value of the erased element
 */
template <typename T>
T UnorderingTakeOutFront(std::vector<T>* from)
{
	T ret = std::move(from->front());
	UnorderingPopFront(from);
	return ret;
}

/**
 * @brief Erases last element + returns its value ("takes out" element)
 * @details Works for containers that support 'back()' and 'pop_back()'
 * operations (such as vector, list, deque)
 *
 * @param from Container to take element out from
 * @return Value of the erased element
 */
template <typename Container>
typename Container::value_type TakeOutBack(Container* from)
{
	auto ret = std::move(from->back());
	from->pop_back();
	return ret;
}

/**
 * @brief Erases first element + returns its value ("takes out" element)
 * @details Works for containers that support 'front()' and 'pop_front()'
 * operations (such as list, forward_list, deque)
 *
 * @param from Container to take element out from
 * @return Value of the erased element
 */
template <typename Container>
typename Container::value_type TakeOutFront(Container* from)
{
	auto ret = std::move(from->front());
	from->pop_front();
	return ret;
}

/**
 * @brief Checks whether the target value is present in the given [first, last)
 * iterator range
 * @details Performs at most O(last - first) operations
 *
 * @return True if the value is present in the iterator range, false otherwise
 */
template <typename ForwardIterator, typename ValueType>
bool Contains(ForwardIterator first, ForwardIterator last, const ValueType& value)
{
	return std::find(first, last, value) != last;
}

/**
 * @brief Checks whether the target value is present in the given container
 * @details Can be called for sequence containers only. Performs with at most
 * N operations, where N is the size of the container
 *
 * @return True if the value is present in the container, false otherwise
 */
template <typename Container>
std::enable_if_t <
Traits::IsSequence<Container>::value || Traits::IsArray<Container>::value,
       bool >
       Contains(const Container& container, const typename Container::value_type& value)
{
	return Contains(container.begin(), container.end(), value);
}

/**
 * @brief Checks whether the target key is present in the given container
 * @details Can be called for assotiative containers only.
 * For unordered containers: performs with O(1) complexity on average,
 * worst case O(n) complexity where n is the size of the container.
 * For ordered containers: performs with O(log(n)) complexity where n is the size
 * of the container
 *
 * @return True if the key is present in the container, false otherwise
 */
template <typename Container>
std::enable_if_t <
Traits::IsAssociativeContainer<Container>::value,
       bool >
       Contains(const Container& container, const typename Container::key_type& key)
{
	return container.find(key) != container.end();
}

/**
 * @brief Erases elements for which @see predicate returns 'true'
 * from the given container
 * @details Unlike std::remove_if, this algorithm really erases elements from
 * the container and destroys them
 *
 * @return Void
 */
template <typename Container, typename UnaryPredicate>
std::enable_if_t <
Traits::IsVector<Container>::value || Traits::IsDeque<Container>::value,
       void >
       EraseIf(Container* container, UnaryPredicate predicate)
{
	container->erase(
	    std::remove_if(container->begin(), container->end(), predicate),
	    container->end());
}

/**
 * @brief Erases elements for which @see predicate returns 'true'
 * from the given assotiative container
 * @details A SFINAE-overload of EraseIf() for assotiative containers.
 *
 * @return Void
 */
template <typename Container, typename UnaryPredicate>
std::enable_if_t<Traits::IsAssociativeContainer<Container>::value, void>
EraseIf(Container* container, UnaryPredicate predicate)
{
	for (auto iter = container->begin(), end = container->end(); iter != end;)
	{
		if (predicate(*iter))
		{
			iter = container->erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

/**
 * @brief Erases elements for which @see predicate returns 'true'
 * from the given list
 * @details A SFINAE-overload of EraseIf() for std::list and std::forward_list.
 * Although std::list::remove_if does exactly the same thing, this overload is
 * provided for EraseIf() to be easy to use and for it to have the same effect
 * for all containers
 *
 * @return Void
 */
template <typename List, typename UnaryPredicate>
std::enable_if_t <
Traits::IsList<List>::value || Traits::IsForwardList<List>::value,
       void >
       EraseIf(List* list, UnaryPredicate predicate)
{
	list->remove_if(predicate);
}

/**
 * @brief Converts enum or enum class value to its underlying type value
 *
 * This function is very useful when you need to get integral value of some enum class value
 * without writing bulky static_casts.
 *
 * @param enumValue incoming enum value
 * @return underlying integer
 *
 * @note Credits to Scott Meyers
 */
template <typename EnumType> constexpr decltype(auto) ToUType(EnumType enumValue) noexcept
{
	return static_cast<std::underlying_type_t<EnumType>>(enumValue);
}

/**
 * @brief Compares a value with a given set of values
 *
 * This template is useful when you need to write something like "if ((a == 10) || (a == 15) || (a == 20) ... )"
 * Just replace that mess with "if (StdX::IsAnyOf(a, 10, 15, 20...))"
 */
template <typename T, typename U>
constexpr bool IsAnyOf(const T& testValue, const U& referenceValue)
{
	return testValue == referenceValue;
}

template <typename T, typename U, typename... Args>
constexpr bool IsAnyOf(const T& testValue, const U& firstReferenceValue, const Args& ... otherReferenceValues)
{
	return IsAnyOf(testValue, firstReferenceValue)
	       || IsAnyOf(testValue, otherReferenceValues...);
}

/**
* @brief Compares values to check if they are all equal to each other
*
* This template is useful when you need to write something like "if ((a == 10) && (b == 10) && (c == 10) ... )"
* Just replace that mess with "if (StdX::AreEqual(a, b, c, 10...))"
* Keep in mind that it works correctly only with transitive equality (A equals B and B equals C means A equals C)
*/
template <typename T, typename U>
constexpr bool AreEqual(const T& testValue, const U& referenceValue)
{
	return testValue == referenceValue;
}

template <typename T, typename U, typename... Args>
constexpr bool AreEqual(const T& testValue, const U& firstReferenceValue, const Args& ... otherReferenceValues)
{
	return AreEqual(testValue, firstReferenceValue)
	       && AreEqual(testValue, otherReferenceValues...);
}

/**
 * @brief Helper for StdX::Apply
 * @brief Helper namespace for StdX implementation details
 */
namespace Details
{
template <typename Callable, typename Tuple, std::size_t... Index>
constexpr decltype(auto) ApplyImpl(Callable&& callable, Tuple&& tuple, std::index_sequence<Index...>)
{
	return std::invoke(std::forward<Callable>(callable), std::get<Index>(std::forward<Tuple>(tuple))...);
}
} // namespace Details

/**
 * @brief Calls a callable with args passed by a tuple
 *
 * Unpacks tuple into separate arguments and invokes callable with them.
 *
 * This is a copy-paste from http://en.cppreference.com/w/cpp/utility/apply
 * Please, remove this when C++17 arrives (std::apply).
 */
template <typename Callable, typename Tuple>
constexpr decltype(auto) Apply(Callable&& callable, Tuple&& tuple)
{
	return Details::ApplyImpl(std::forward<Callable>(callable), std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>> {});
}

/**
 * @brief Merge two std::vector<T> into one.
 *
 * Move content of sourceVector into end of destVector and clears sourceVector.
 *
 * Two versions of MergeVectors are needed for support rvalue and lvalue parameters of sourceVector.
 */
template <typename T>
void MergeVectors(std::vector<T>* destVector, std::vector<T>&& sourceVector)
{
	destVector->reserve(destVector->size() + sourceVector.size());
	for (auto& val : sourceVector)
	{
		destVector->emplace_back(std::move(val));
	}
	sourceVector.clear();
}

template <typename T>
void MergeVectors(std::vector<T>* destVector, std::vector<T>* sourceVector)
{
	MergeVectors(destVector, std::move(*sourceVector));
}

/**
 * @brief Calls a callable for each passed parameter
 * @details If callable is a C++14 generic lambda (i.e. [](auto&& arg){};),
 * input parameters may be of different types
 */
template <typename Callable, typename... Args>
constexpr void InvokeForEach(const Callable& callable, Args&& ... args)
{
	// The following approach relies on the fact that contents
	// of the array initializer list are evaluated from left to right
	// (which is guaranteed by the C++ standard), so the callable is
	// invoked for each argument in the same order in which the arguments
	// were passed into this function.
	// The more conventional way of doing things in variadic functions
	// would be making a solution via variadic recursion, but
	// there seems to be a consensus that it will produce a more bloated
	// code because of recursive calls that may not be optimized away
	using ExpandAsArrayInitList = int[];
	ExpandAsArrayInitList
	{
		0, // NOTE: forces initializer list to be non-empty
		(callable(std::forward<decltype(args)>(args)), // NOTE: comma operator at the end
		void(), // NOTE: protects from accidental usage of overloaded comma operator
		// since comma operator can't be overloaded for void
		0       // NOTE: dummy value that is actually put into the array initializer
		// list
		)... // NOTE: variadic parameter pack expansion
	};
}

/**
 * @brief A convenience function that allows to construct std::unique_lock
 * by performing lock() operation for a given mutex with less typing
 *
 * @param mutex Must satisfy BasicLockable concept of C++ standard library
 */
/*template <typename Mutex>
std::unique_lock<Mutex> Lock(Mutex& mutex)
{
	return std::unique_lock<Mutex>(mutex);
}*/

/**
 * @brief A convenience function that allows to construct std::unique_lock
 * by performing try_lock() operation for a given mutex with less typing
 *
 * @param mutex Must satisfy Lockable concept of C++ standard library
 */
/*template <typename Mutex>
std::unique_lock<Mutex> TryLock(Mutex& mutex)
{
	return std::unique_lock<Mutexes>(mutex, std::try_to_lock);
}*/

/**
 * @brief A convenience function that allows to construct multiple
 * std::unique_lock-s by calling std::lock() for given mutexes with less typing
 * @details Provides same behaviour (locking order and deadlock avoidance
 * guarantees) as std::lock()
 *
 * @param mutex Must satisfy BasicLockable concept of C++ standard library
 * @return A tuple of std::unique_lock-s for all input mutexes
 */
/*template <typename... Mutexes>
std::enable_if_t < (sizeof...(Mutexes) > 1), std::tuple<std::unique_lock<Mutexes>... >>
        Lock(Mutexes& ... mutexes)
{
	std::lock(mutexes...);
	return {std::unique_lock<Mutexes>(mutexes, std::adopt_lock)...};
}*/

} // namespace StdX

#endif // _CORE_UTIL_STD_EXTENSIONS_H_
