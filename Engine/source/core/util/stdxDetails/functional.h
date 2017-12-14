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

#ifndef _CORE_UTIL_STDX_DETAILS_FUNCTIONAL_H_
#define _CORE_UTIL_STDX_DETAILS_FUNCTIONAL_H_

namespace StdX
{

namespace ResultTypeDetails
{

struct SubstitutionFailure {};

template <typename T>
struct IsSuccessfulSubstitution : std::true_type {};

template <>
struct IsSuccessfulSubstitution<SubstitutionFailure> : std::false_type {};

// Deduces the type of the expression &T::operator(), the address of the
// T's function call operator. This will fail when T is a polymorphic
// functor.
template <typename T>
struct GetCallOperatorType
{
private:
	template <typename X>
	static auto check(X) -> decltype(&X::operator());

	static SubstitutionFailure check(...);

public:
	using Type = decltype(check(std::declval<T>()));
};


// Get the result type of a function or callable type F. This is different
// than result_of in that F is not a call expression type. This means
// that the result type is not defined for polymorphic functors.
template <typename F, bool = std::is_class<F>::value>
struct ResultTypeImpl
{
	using Type = SubstitutionFailure;
};

template <typename R, typename... Args>
struct ResultTypeImpl<R(Args...), false>
{
    using Type = R;
};

template <typename R, typename... Args>
struct ResultTypeImpl<R(*)(Args...), false>
{
    using Type = R;
};

template <typename R, typename C, typename... Args>
struct ResultTypeImpl<R(C::*)(Args...), false>
{
    using Type = R;
};

template <typename R, typename C, typename... Args>
struct ResultTypeImpl<R(C::*)(Args...) const, false>
{
    using Type = R;
};

// For class types, the argument types are deduced from the member call
// operator, if present.
template <typename T>
struct ResultTypeImpl<T, true> : ResultTypeImpl<typename GetCallOperatorType<T>::Type>
{
};

} // namespace ResultTypeDetails

template <typename T>
using ResultType = typename ResultTypeDetails::ResultTypeImpl<T>::Type;

namespace Traits
{

/**
 * @brief Type trait that allows to check whether entity is callable or not
 * @details Unlike std::is_callable (and StdX::IsCallable) expects an object
 * or something like a pointer to a function, not a call expression as a
 * template parameter.
 *
 * Does not work for polymorphic functors (e.g. generic lambdas)
 */
template <typename T>
struct IsCallableEntity : ResultTypeDetails::IsSuccessfulSubstitution<ResultType<T>> {};

namespace IsCallableDetails
{
// Taken from "True Story: Call Me Maybe" article by Agustín "K-ballo" Bergé
// http://talesofcpp.fusionfenix.com/post-11/true-story-call-me-maybe

template <typename T> using AlwaysVoid = void;

template <typename Expression, typename Enable = void>
struct IsCallableImpl : std::false_type {};

template <typename Fn, typename... Args>
struct IsCallableImpl<Fn(Args...), AlwaysVoid<std::result_of_t<Fn(Args...)>>> : std::true_type {};

} // namespace IsCallableDetails

/**
 * @brief Mirrors std::is_callable type trait
 * @details Will become obsolete when C++17 is available
 *
 * Usage pattern example:
 *
 * void foo(int, bool) {}
 * std::cout << IsCallable<foo(int, bool)>::value << std::endl; // prints 1
 * std::cout << IsCallable<foo(int)>::value << std::endl; // prints 0
 *
 * @tparam T A call expression
 */
template <typename Expression>
struct IsCallable : IsCallableDetails::IsCallableImpl<Expression> {};

} // namespace Traits

} // namespace StdX

#endif // _CORE_UTIL_STDX_DETAILS_FUNCTIONAL_H_
