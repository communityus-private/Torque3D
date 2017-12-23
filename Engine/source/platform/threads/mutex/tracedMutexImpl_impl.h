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

#ifndef _PLATFORM_THREADS_MUTEX_TRACED_MUTEX_IMPL_INC_
#define _PLATFORM_THREADS_MUTEX_TRACED_MUTEX_IMPL_INC_

/**
 * The following code is not inside of a separate .cpp to allow the compiler
 * better optimize Mutex during platform/threads/mutex.cpp translation unit compilation
 */

#include "platform/threads/mutex/tracedMutexImpl.h"

#include "platform/threads/mutex/mutexSettings.h"

#include <mutex>

namespace MutexDetails
{

struct TracedMutexImpl::MutexData
{
	std::recursive_timed_mutex mutex;
	std::thread::id owner_handle;
};

TracedMutexImpl::TracedMutexImpl(const char* name)
	: mData(std::make_unique<MutexData>())
	, mName(name)
{
}

TracedMutexImpl::~TracedMutexImpl() = default;

TracedMutexImpl::LockID TracedMutexImpl::lock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS)
{
	LockCallDescription description(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS, mName.c_str());
#ifdef _aw_s4_deadlockCheck
	
	auto lockID = LockStack::ThreadLocalInstance().push(LockState::MakeUnlocked(description));

	static const auto PotentialDeadLockTimeout = boost::chrono::seconds(DeadLockThresholdInSec);

	// we are forcing here using of system_clock::now() since boost::recursive_timed_mutex::try_lock_for(timeout)
	// is actually try_lock_until(steady_clock::now() + timeout) and steady_clock::now() appears to be VERY SIGNIFICANTLY
	// slower than system_clock::now() on Windows systems run on virtual machines
	if (mData->mutex.try_lock_until(boost::chrono::system_clock::now() + PotentialDeadLockTimeout))
	{
		LockStack::ThreadLocalInstance().lock(lockID);
		return lockID;
	}

	_reportAboutPossibleDeadlock();

	// NOTE: In case there was no actual dead-lock, lock still needs to be
	// acquired to allow mutex to continue to behave as expected from outside
	// if the warning pop-up is skipped by the user
	mData->mutex.lock();
	LockStack::ThreadLocalInstance().lock(lockID);
	return lockID;
#else
	mData->mutex.lock();

	return LockStack::ThreadLocalInstance().push(LockState::MakeLocked(description));
#endif // _aw_s4_deadlockCheck
}

std::pair<TracedMutexImpl::LockID, bool>
TracedMutexImpl::tryLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS)
{
	if (mData->mutex.try_lock())
	{
		LockCallDescription description(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS, mName.c_str());
		auto lockID = LockStack::ThreadLocalInstance().push(LockState::MakeLocked(description));
		return std::make_pair(lockID, true);
	}

	return std::make_pair(TracedMutexImpl::LockID(), false);
}

void TracedMutexImpl::unlock(LockID id)
{
	LockStack::ThreadLocalInstance().remove(id);
	mData->mutex.unlock();
}

std::thread::id TracedMutexImpl::getOwningThreadID() const
{
   return mData->owner_handle;
}

/*static*/ void TracedMutexImpl::_reportAboutPossibleDeadlock()
{
	// NOTE: Report is dumped to a file before a message box is shown
	// to the user to provide more relevant information in situations
	// of false deadlock detections (for example when an operation in a WORKER
	// thread takes longer then DeadLockThresholdInSec and holds a mutex for this
	// period; in this case it may possible to continue playing if main thread is
	// still alive without interacting with the message box which means that
	// global mutex lock situation may be completely different when the message
	// box is finally closed)
	GlobalLockStackPool::Instance().dumpToFile(TraceReportFileName);

	const bool shouldTerminateProcess = _askUserIfShouldTerminateProcess();
	if (shouldTerminateProcess)
	{
		Platform::debugBreak();
	}
}

/*static*/ bool TracedMutexImpl::_askUserIfShouldTerminateProcess()
{
#ifndef TORQUE_SHIPPING
	const bool userChoseToWaitForUnlock = Platform::AlertRetry("Mutex deadlock error", DeadLockUserWarningMsg);
	return !userChoseToWaitForUnlock;
#else
	Platform::AlertOK("Mutex deadlock error", DeadLockUserFatalErrorMsg);
	return true;
#endif // TORQUE_SHIPPING
}

} // namespace MutexDetails

#endif // _PLATFORM_THREADS_MUTEX_TRACED_MUTEX_IMPL_INC_
