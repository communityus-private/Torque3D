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

/*
#include "stdafx.h"
*/
#include "platform/threads/mutex.h"
#include "console/engineAPI.h"
#include "platform/threads/thread.h"
#include "platform/threads/mutex/tracedMutexImpl_impl.h"
#include "platform/threads/mutex/untracedMutexImpl_impl.h"

#ifndef TORQUE_SHIPPING
#include <SDL_thread.h>
#endif // !TORQUE_SHIPPING

//#define DEBUG_SPEW

Mutex::Mutex(const char* name) : mMutexImpl(name == nullptr ? "unknown" : name)
{
}

MutexHandle Mutex::internalLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS)
{
	auto lockID = mMutexImpl.lock(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS);
	return MutexHandle(this, lockID);
}

MutexHandle Mutex::internalTryLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS)
{
	auto tryLockResultPair = mMutexImpl.tryLock(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS);
	if (tryLockResultPair.second)
	{
		return MutexHandle(this, tryLockResultPair.first);
	}

	return MutexHandle();
}

void Mutex::_unlock(MutexImpl::LockID lockID)
{
	mMutexImpl.unlock(lockID);
}

void Mutex::checkOwnerThread() const
{
#ifdef _aw_s4_mutexCheck
   SDL_threadID owningThreadID = mMutexImpl.getOwningThreadID();
   AssertISV(owningThreadID == ThreadManager::getCurrentThreadId(), "Owner thread mismatch");
#endif // _aw_s4_mutexCheck
}

MutexHandle::MutexHandle() : MutexHandle(nullptr, Mutex::MutexImpl::LockID())
{
}

MutexHandle::MutexHandle(Mutex* mutex, Mutex::MutexImpl::LockID lockID)
	: mMutex(mutex)
	, mLockID(lockID)
{
}

MutexHandle::~MutexHandle()
{
	unlock();
}

MutexHandle::MutexHandle(MutexHandle&& other) : MutexHandle()
{
	*this = std::move(other);
}

MutexHandle& MutexHandle::operator=(MutexHandle&& rhs)
{
	unlock();

	std::swap(mMutex, rhs.mMutex);
	std::swap(mLockID, rhs.mLockID);
	return *this;
}

bool MutexHandle::isLocked() const
{
	return mMutex != nullptr;
}

void MutexHandle::unlock()
{
	if (mMutex)
	{
		mMutex->_unlock(mLockID);
		mMutex = nullptr;
	}
}

namespace MutexDetails
{
LockableWrapper::LockableWrapper(Mutex& mutex, MUTEX_INTERNAL_TRACE_LOCK_PARAMS)
	: mMutexPtr(&mutex), mLine(line), mFile(file), mFunction(function)
{
}

LockableWrapper::LockableWrapper(LockableWrapper&& rhs)
{
	if (this != &rhs)
	{
		mMutexPtr = rhs.mMutexPtr;
		mHandle = std::move(rhs.mHandle);
		mLine = rhs.mLine;
		mFile = rhs.mFile;
		mFunction = rhs.mFunction;
	}
}

void LockableWrapper::lock()
{
	const auto& function = mFunction;
	const auto& file = mFile;
	const auto& line = mLine;
	mHandle = mMutexPtr->internalLock(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS);
}

bool LockableWrapper::try_lock()
{
	const auto& function = mFunction;
	const auto& file = mFile;
	const auto& line = mLine;
	mHandle = mMutexPtr->internalTryLock(MUTEX_INTERNAL_TRACE_FORWARD_LOCK_ARGS);
	return mHandle.isLocked();
}

void LockableWrapper::unlock()
{
	mHandle.unlock();
}

MutexHandle LockableWrapper::extractHandle()
{
	return std::move(mHandle);
}
}

#ifndef TORQUE_SHIPPING

DefineConsoleFunction(testMutexDeadLockDetection, void, (), , "testMutexDeadLockDetection")
{
	Mutex testMutex1("test mutex 1");
	Mutex testMutex2("test mutex 2");

	std::thread thr1([&]()
	{
		MutexHandle locker1 = TORQUE_LOCK(testMutex1);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		MutexHandle locker2 = TORQUE_LOCK(testMutex2);
	});

	std::thread thr2([&]()
	{
		MutexHandle locker2 = TORQUE_LOCK(testMutex2);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		MutexHandle locker1 = TORQUE_LOCK(testMutex1);
	});

	thr1.join();
	thr2.join();
}

DefineConsoleFunction(testMutexRecursiveLock, void, (U32 nLockCount), , "testMutexRecursiveLock")
{
	if (nLockCount == 0)
	{
		Con::errorf("mutex lock count must be > 0");
		return;
	}

	class recursiveLock
	{
	public:
		U32 mIndex;
		const U32 mLockCount;
		Mutex mMutex;
		recursiveLock(U32 nLockCount) : mLockCount(nLockCount), mIndex(0), mMutex("lock count test mutex")
		{

		}

		void doLock()
		{
#ifdef DEBUG_SPEW
			Con::printf("lock mutex #%u", mIndex);
#endif
			MutexHandle locker = TORQUE_LOCK(mMutex);
			++mIndex;
			if (mIndex >= mLockCount)
			{
				return;
			}
			doLock();
		}
	};

	recursiveLock locker(nLockCount);
	locker.doLock();
}
/*
static void testMutexGenericLock(U32 iterationCount)
{
	Mutex mutex1("1"), mutex2("2"), mutex3("3");
	U32 synchronizedData = 0;
	auto func1 = [&](U32 iterationCount)
	{
		for (U32 i = 0; i < iterationCount; ++i)
		{
			TORQUE_LOCK_SCOPE(mutex1, mutex2, mutex3)
			{
#ifdef DEBUG_SPEW
				Con::printf("LOCKED 1ST TYPE. DATA = %u. SEE THREAD ID HERE -->", synchronizedData++);
#endif
			}
		}
	};

	auto func2 = [&](U32 iterationCount)
	{
		for (U32 i = 0; i < iterationCount; ++i)
		{
			std::tuple<MutexHandle, MutexHandle, MutexHandle> locks = TORQUE_LOCK(mutex1, mutex2, mutex3); // note the same sequence of mutexes
#ifdef DEBUG_SPEW
			Con::printf("LOCKED 2ND TYPE. DATA = %u. SEE THREAD ID HERE -->", synchronizedData++);
#endif

			// we are going to unlock mutexes in reverse order - this should not affect deadlock safety
			auto reverseLocks = std::make_tuple(std::move(std::get<2>(locks)), std::move(std::get<1>(locks)), std::move(std::get<0>(locks)));
		}
	};

	std::vector<std::thread> threads;
	constexpr U32 ThreadInTypeCount = 4;
	threads.reserve(ThreadInTypeCount * 2);
	for (U32 i = 0; i < ThreadInTypeCount; ++i)
	{
		threads.emplace_back(func1, iterationCount);
		threads.emplace_back(func2, iterationCount);
	}

	for (auto& t : threads)
	{
		t.join();
	}
}
static void testMutexGenericTryLock(U32 iterationCount)
{
	Mutex mutex1("1"), mutex2("2"), mutex3("3");
	U32 synchronizedData = 0;
	auto func1 = [&](U32 iterationCount)
	{
		for (U32 i = 0; i < iterationCount; ++i)
		{
			auto locks = TORQUE_TRY_LOCK(mutex1, mutex2, mutex3);
			if (std::get<0>(locks))
			{
#ifdef DEBUG_SPEW
				Con::printf("LOCKED 1ST TYPE. DATA = %u. SEE THREAD ID HERE -->", synchronizedData++);
#endif
			}
			else
			{
#ifdef DEBUG_SPEW
				Con::printf("FAILED TO LOCK 1ST TYPE(%d, %d, %d). SEE THREAD ID HERE -->", std::get<1>(locks).isLocked(), std::get<2>(locks).isLocked(), std::get<3>(locks).isLocked());
#endif
			}
		}
	};

	auto func2 = [&](U32 iterationCount)
	{
		for (U32 i = 0; i < iterationCount; ++i)
		{
			auto locks = TORQUE_TRY_LOCK(mutex1, mutex2, mutex3);
			if (std::get<0>(locks))
			{
#ifdef DEBUG_SPEW
				Con::printf("LOCKED 2ND TYPE. DATA = %u. SEE THREAD ID HERE -->", synchronizedData++);
#endif
			}
			else
			{
#ifdef DEBUG_SPEW
				Con::printf("FAILED TO LOCK 2ND TYPE(%d, %d, %d). SEE THREAD ID HERE -->", std::get<1>(locks).isLocked(), std::get<2>(locks).isLocked(), std::get<3>(locks).isLocked());
#endif
			}
		}
	};

	std::vector<std::thread> threads;
	constexpr U32 ThreadInTypeCount = 4;
	threads.reserve(ThreadInTypeCount * 2);
	for (U32 i = 0; i < ThreadInTypeCount; ++i)
	{
		threads.emplace_back(func1, iterationCount);
		threads.emplace_back(func2, iterationCount);
	}

	for (auto& t : threads)
	{
		t.join();
	}
}

DefineConsoleFunction(testMutexGenericLock, void, (U32 iterationCount), , "testMutexGenericLock(iterationCount")
{
	std::thread(testMutexGenericLock, iterationCount).detach();
}

DefineConsoleFunction(testMutexGenericTryLock, void, (U32 iterationCount), , "testMutexGenericLock(iterationCount")
{
	std::thread(testMutexGenericTryLock, iterationCount).detach();
}
*/
#endif // !TORQUE_SHIPPING
