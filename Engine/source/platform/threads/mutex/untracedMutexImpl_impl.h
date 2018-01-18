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

#ifndef _PLATFORM_THREADS_MUTEX_UNTRACED_MUTEX_IMPL_INC_
#define _PLATFORM_THREADS_MUTEX_UNTRACED_MUTEX_IMPL_INC_

/**
 * The following code is not inside of a separate .cpp to allow the compiler
 * better optimize Mutex during platform/threads/mutex.cpp translation unit compilation
 */

#include "platform/threads/mutex/untracedMutexImpl.h"

#include "platform/threads/mutex/mutexSettings.h"

#include <mutex>

namespace MutexDetails
{

struct UntracedMutexImpl::MutexData
{
	std::recursive_timed_mutex mutex;
   SDL_threadID owner_handle = 0;
};

UntracedMutexImpl::UntracedMutexImpl(const char*)
	: mData(std::make_unique<MutexData>())
{
	// NOTE: Constructor has string parameter in its signature for interface
	// compatibility with TracedMutexImpl
}

UntracedMutexImpl::~UntracedMutexImpl() = default;

UntracedMutexImpl::LockID UntracedMutexImpl::lock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS)
{
#ifdef _aw_s4_deadlockCheck
	static const auto PotentialDeadLockTimeout = boost::chrono::seconds(DeadLockThresholdInSec);

	// we are forcing here using of system_clock::now() since boost::recursive_timed_mutex::try_lock_for(timeout)
	// is actually try_lock_until(steady_clock::now() + timeout) and steady_clock::now() appears to be VERY SIGNIFICANTLY
	// slower than system_clock::now() on Windows systems run on virtual machines
	if (mData->mutex.try_lock_until(boost::chrono::system_clock::now() + PotentialDeadLockTimeout))
	{
		return LockID();
	}

	_reportAboutPossibleDeadlock();
#endif // _aw_s4_deadlockCheck
	mData->owner_handle = SDL_ThreadID();
	mData->mutex.lock();

	return LockID();
}

std::pair<UntracedMutexImpl::LockID, bool>
UntracedMutexImpl::tryLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS)
{
	bool locked = mData->mutex.try_lock();
	if (locked)
		mData->owner_handle = SDL_ThreadID();
	return std::make_pair(LockID(), locked);
}

void UntracedMutexImpl::unlock(LockID)
{
	mData->mutex.unlock();
	mData->owner_handle = 0;
}

SDL_threadID UntracedMutexImpl::getOwningThreadID() const
{
   return mData->owner_handle;
}

/*static*/ void UntracedMutexImpl::_reportAboutPossibleDeadlock()
{
#ifndef TORQUE_SHIPPING
	const bool userChoseToRetry = Platform::AlertRetry("Mutex deadlock error", DeadLockUserWarningMsg);
	if (!userChoseToRetry)
	{
		Platform::debugBreak();
	}
#else
	Platform::AlertOK("Mutex deadlock error", DeadLockUserFatalErrorMsg);
	Platform::debugBreak();
#endif // TORQUE_SHIPPING
}

} // namespace MutexDetails

#endif // _PLATFORM_THREADS_MUTEX_UNTRACED_MUTEX_IMPL_INC_
