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

#ifndef _PLATFORM_THREADS_MUTEX_VARIADIC_LOCK_IMPL_H_
#define _PLATFORM_THREADS_MUTEX_VARIADIC_LOCK_IMPL_H_

#include "platform/threads/mutex/mutexInternalApiMacros.h"
#include "core/util/stdExtensions.h"

#include <tuple>

namespace MutexDetails
{

/**
 * @brief Proxy object for Torque Mutex that satisfies Lockable concept of
 * C++ Standard
 */
class LockableWrapper final
{
public:
	LockableWrapper(Mutex& mutex, MUTEX_INTERNAL_TRACE_LOCK_PARAMS);

	LockableWrapper(const LockableWrapper&) = delete;
	LockableWrapper& operator=(const LockableWrapper&) = delete;

	LockableWrapper(LockableWrapper&& rhs);
	LockableWrapper& operator=(LockableWrapper&& rhs) = delete;

	void lock();
	bool try_lock();
	void unlock();

	MutexHandle extractHandle();

private:
	Mutex* mMutexPtr;
	MutexHandle mHandle;

	U32         mLine;
	const char* mFunction;
	const char* mFile;
};

template <typename... Mutexes>
auto WrapIntoLockable(MUTEX_INTERNAL_TRACE_LOCK_PARAMS, Mutexes&... mutexes)
{
	return std::make_tuple(LockableWrapper(mutexes, MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS)...);
}

struct LockAndExtractHandles
{
	template <typename... Wrappers>
	auto operator()(Wrappers&... wrappers)
	{
		std::lock(wrappers...);
		return std::make_tuple(wrappers.extractHandle()...);
	}
};

struct TryLockAndExtractHandles
{
	template <typename... Wrappers>
	auto operator()(Wrappers&... wrappers)
	{
		int tryLockResult = std::try_lock(wrappers...);
		return std::make_tuple(tryLockResult == -1, wrappers.extractHandle()...);
	}
};

template <typename T>
using Handle = MutexHandle;

/**
 * @brief Calls std::lock() for mutexes incompatible with the Lockable concept of
 * C++ standard. Use @see TORQUE_LOCK() macro for simpler usage
 *
 * @returns tuple of locked MutexHandles
 */
template <typename MutexType, typename... Mutexes>
std::enable_if_t<std::conjunction_v<std::is_same<Mutex, MutexType>, std::is_same<Mutex, Mutexes>...>,
	std::tuple<MutexHandle, Handle<Mutexes>...>>
Lock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS, MutexType& mutex, Mutexes&... mutexes)
{
	auto wrapperTuple = MutexDetails::WrapIntoLockable(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS, mutex, mutexes...);
	return StdX::Apply(MutexDetails::LockAndExtractHandles(), wrapperTuple);
}

/**
 * @brief Special Lock() overload for a single input mutex
 * @details As opposed to variadic Lock(), returns a single MutexHandle
 * instead of a tuple for simpler usage
 */
inline MutexHandle Lock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS, Mutex& mutex)
{
	return mutex.internalLock(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS);
}

template <typename... T>
std::enable_if_t<!std::conjunction_v<std::is_same<Mutex, T>...>, std::tuple<T...>>
Lock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS, T&...)
{
	static_assert(false, "Only Torque Mutex is supported");
}

/**
 * @brief Calls std::try_lock() for mutexes incompatible with the Lockable concept of
 * C++ standard. Use @see TORQUE_TRY_LOCK() macro for simpler usage
 *
 * @returns tuple of try_lock result plus handles for all mutexes
 */
template <typename MutexType, typename... Mutexes>
std::enable_if_t<std::conjunction_v<std::is_same<Mutex, MutexType>, std::is_same<Mutex, Mutexes>...>,
	std::tuple<bool, MutexHandle, MutexDetails::Handle<Mutexes>...>>
TryLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS, MutexType& mutex, Mutexes&... mutexes)
{
	auto wrapperTuple = MutexDetails::WrapIntoLockable(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS, mutex, mutexes...);
	return StdX::Apply(MutexDetails::TryLockAndExtractHandles(), wrapperTuple);
}

/**
 * @brief Special TryLock() overload for a single input mutex
 * @details As opposed to variadic TryLock(), returns a single MutexHandle
 * instead of a tuple for simpler usage
 */
inline MutexHandle TryLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS, Mutex& mutex)
{
	return mutex.internalTryLock(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS);
}

template <typename... T>
std::enable_if_t<!std::conjunction_v<std::is_same<Mutex, T>...>, std::tuple<T...>>
TryLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS, T&... mutexes)
{
	static_assert(false, "Only Torque Mutex is supported");
}

} // namespace MutexDetails

#endif // _PLATFORM_THREADS_MUTEX_VARIADIC_LOCK_IMPL_H_
