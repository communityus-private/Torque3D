//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
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

#ifndef _PLATFORM_THREADS_MUTEX_MUTEX_H_
#define _PLATFORM_THREADS_MUTEX_MUTEX_H_

#include "platform/types.h"
#include "platform/threads/mutex/untracedMutexImpl.h"
#include "platform/threads/mutex/tracedMutexImpl.h"
#include "platform/threads/mutex/mutexInternalApiMacros.h"

class MutexHandle;

class Mutex final
{
	friend class MutexHandle;
public:
	explicit Mutex(const char* name);

	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;
	Mutex(Mutex&&) = delete;
	Mutex& operator=(Mutex&&) = delete;

	void checkOwnerThread() const;

	/**
	 * @brief These methods are for internal usage
	 *
	 * @details Use TORQUE_LOCK() and TORQUE_TRY_LOCK() macros for simpler API
	 */
	MutexHandle internalLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS);
	MutexHandle internalTryLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS);

private:
#ifdef _aw_s4_mutexTrace
	using MutexImpl = MutexDetails::TracedMutexImpl;
#else // _aw_s4_mutexTrace
	using MutexImpl = MutexDetails::UntracedMutexImpl;
#endif // _aw_s4_mutexTrace

	void _unlock(MutexImpl::LockID);

	MutexImpl mMutexImpl;
};

/**
 * @brief A movable RAII-object that expresses lock ownership of
 * a single @see Mutex.
 * @details Users should use TORQUE_LOCK()/TORQUE_TRY_LOCK() to create these handles.
 * Locks @see Mutex upon creation and unlocks it either via call to
 * @see unlock or upon destruction
 */
class MutexHandle final
{
	friend class Mutex;
public:
	/**
	 * @brief Creates a MutexHandle that does not own a lock to any mutex
	 */
	MutexHandle();
	~MutexHandle();

	MutexHandle(MutexHandle&&);
	MutexHandle& operator=(MutexHandle&&);

	MutexHandle(const MutexHandle&) = delete;
	MutexHandle& operator=(const MutexHandle&) = delete;

	MutexHandle* operator&() const = delete;

	static void* operator new(size_t size) = delete;
	static void  operator delete(void* ptr) = delete;

	bool isLocked() const;
	void unlock();

private:
	/**
	 * @brief Constructor is not public to make locked MutexHandle construction
	 * possible only from @see Mutex::internalLock()/internalTryLock()
	 */
	MutexHandle(Mutex*, Mutex::MutexImpl::LockID);

	Mutex* mMutex;
	Mutex::MutexImpl::LockID mLockID;
};

#include "platform/threads/mutex/mutexVariadicLockImpl.h"

/**
 * @brief Torque Mutex-specific convenience wrappers for variadic std::lock()
 * and std::try_lock()
 *
 * These macros are intended to be the common usage pattern of @see Mutex-es
 *
 * @details Just as std::lock(), they accept variadic number of input mutexes
 * and use deadlock avoidance algorithm to avoid deadlock. See std::lock()
 * docs for more information
 *
 * They wrap MutexDetails::Lock() and MutexDetails::TryLock(), removing
 * the need to type MUTEX_INTERNAL_TRACE_LOCK_ARGS each time
 *
 * Usage example:
 *	Mutex mutex;
 *
 *	{
 *		MutexHandle lock = TORQUE_LOCK(mutex);
 *		// successfully locked
 *		...
 *	} // `mutex` unlocked
 *
 *	Mutex mutex1, mutex2;
 *
 *	{
 *		std::tuple<MutexHandle, MutexHandle> locks = TORQUE_LOCK(mutex1, mutex2);
 *		// successfully locked
 *		...
 *	} // both `mutex1` and `mutex2` unlocked
 */
#define TORQUE_LOCK(...) MutexDetails::Lock(MUTEX_INTERNAL_TRACE_LOCK_ARGS, __VA_ARGS__)
/**
 * Usage example:
 *	Mutex mutex;
 *
 *	{
 *		MutexHandle lock = TORQUE_TRY_LOCK(mutex);
 *
 *		if (lock.isLocked()) { // successfully locked }
 *		else { // failed to lock the mutex }
 *	} // `mutex` unlocked
 *
 *	Mutex mutex1, mutex2;
 *
 *	{
 *		std::tuple<bool, MutexHandle, MutexHandle> locks = TORQUE_TRY_LOCK(mutex1, mutex2)
 *
 *		if (std::get<0>(locks)) { // successfully locked }
 *		else { // all mutexes are unlocked (!), failed to lock at least one of given mutexes}
 *	} // both `mutex1` and `mutex2` unlocked
 */
#define TORQUE_TRY_LOCK(...) MutexDetails::TryLock(MUTEX_INTERNAL_TRACE_LOCK_ARGS, __VA_ARGS__)

/**
 * @brief Helper macro to create an identifier bound to particular line.
 *
 * This macro creates an identifier consisting of user-defined string and current line number.
 * Keep in mind that all macro definitions are one-liners so multiple uses of this macro in any macro
 * definition will create the same identifier. This helps to avoid shadowing when using nested TORQUE_LOCK_SCOPE
 *
 * Internal use only.
 */
#define TORQUE_LOCK_SCOPE_DETAILS_LINE_BOUND_VAR(var) TORQUE_CONCAT(_DETAILS_##var##, __LINE__)

/**
 * @brief Convenient macro for defining a scope with a set of locked mutexes.
 *
 * The most common usage of TORQUE_LOCK is acquiring a lock and holding it until the current scope ends.
 * In such case you have to define a lock variable where you will hold the result of TORQUE_LOCK even
 * if you don't want to unlock the mutexes explicitly. This can lead to silly mistakes when you just
 * bare call TORQUE_LOCK without keeping the lock in a variable, so mutexes will be unlocked instantly
 * rather than, as you may expect, at the end of the scope.
 *
 * This macro provides its own scope (at least one line if you don't use braces but this is strongly discouraged)
 * where mutexes are locked and you don't need to worry about any special variable to hold locks.
 * Usage example:
 *	Mutex mutex("blabla");
 *	TORQUE_LOCK_SCOPE(mutex)
 *	{
 *		// mutex is locked
 *	}
 *	// mutex is unlocked
 *
 * Of course, you can provide any number of mutexes to lock, since this macro is a wrapper around TORQUE_LOCK
 *
 * Some credits must be given to Andrei Alexandrescu for his ideas regarding implementation of similar macros.
 */
#define TORQUE_LOCK_SCOPE(...) \
	if (bool TORQUE_LOCK_SCOPE_DETAILS_LINE_BOUND_VAR(state) = false) /* define a loop breaker flag */ \
		{} else /* go to actual control flow */ \
		for (auto TORQUE_LOCK_SCOPE_DETAILS_LINE_BOUND_VAR(mutexLock) = TORQUE_LOCK(__VA_ARGS__); /* lock the mutexes */ \
			!TORQUE_LOCK_SCOPE_DETAILS_LINE_BOUND_VAR(state); /* check the loop breaker */ \
			TORQUE_LOCK_SCOPE_DETAILS_LINE_BOUND_VAR(state) = true) /* set the loop breaker so the loop body is performed once */ \
			if (true) /* C4390 if ended with bare semicolon */

#endif // _PLATFORM_THREADS_MUTEX_MUTEX_H_
