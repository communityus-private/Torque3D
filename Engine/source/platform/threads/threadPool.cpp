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
#include "threadPool.h"

#include "console/console.h"
#include "console/consoleTypes.h"

bool ThreadPool::smForceAllMainThread;

void ThreadWorkerThunk(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	pool->_processBackgroundThreadWorkers();
}

void ThreadPool::_processBackgroundThreadWorkers()
{
	for (;;)
	{
		WorkItem* workItem = nullptr;

		{
         int result = SDL_TryLockMutex(mMutex);

         if (mIsStopped || mWorkItems.empty())
			{
            SDL_CondWait(mCondition, mMutex);
         }

			if (mIsStopped && mWorkItems.empty())
			{
				Con::printf("Thread worker terminated");
            SDL_UnlockMutex(mMutex);
				return;
			}

			workItem = mWorkItems.front();
			mWorkItems.pop();
			mBackgroundWorkItemCount.store(mWorkItems.size(), std::memory_order_relaxed);

         SDL_UnlockMutex(mMutex);
		}

		if (workItem != nullptr)
		{
         mNumberOfActiveWorkThreads++;
			workItem->process();
			workItem->release();
         mNumberOfActiveWorkThreads--;
         SDL_CondSignal(mWorkerThreadCondition);
		}
	}
}

ThreadPool* ThreadPool::instance()
{
	static ThreadPool threadPool;
	return &threadPool;
}

ThreadPool::ThreadPool()
	: mMainThreadMutex("ThreadPool_mMainThreadMutex")
	, mPhysicalThreadsMutex("ThreadPool_mPhysicalThreadsMutex")
{
   mMutex = SDL_CreateMutex();
   mCondition = SDL_CreateCond();
   mWorkerThreadCondition = SDL_CreateCond();
	int32_t numPhysicalThreads = std::thread::hardware_concurrency();
	int32_t numBackgroundThreads = std::max(numPhysicalThreads - 1, 1);

   mNumberOfActiveWorkThreads = 0;

	mPhysicalThreads.reserve(numBackgroundThreads);

	for (int32_t i = 0; i < numBackgroundThreads; ++i)
	{
		auto thread = std::make_unique<Thread>(ThreadWorkerThunk, this, false);
		StringTableEntry threadName = StringTable->insert(String::ToString("WorkerThread%d", i).c_str());
		thread->setName(threadName);
		thread->start(this);
		mPhysicalThreads.emplace_back(std::move(thread));
	}

	Con::printf("Thread pool initialized, threads: %i", numBackgroundThreads);
}

void ThreadPool::shutdown()
{
	{
		SDL_LockMutex(mMutex);
		MutexHandle mutexHandle = TORQUE_LOCK(mMainThreadMutex);

		mIsStopped  = true;

		// process remaining items
		while (!mWorkItems.empty())
		{
			WorkItem* item = mWorkItems.front();
			if (item != nullptr)
			{
				item->process();
				item->release();
			}
			mWorkItems.pop();
		}

		// process remaining main thread items
		while (!mMainThreadWorkItems.empty())
		{
			WorkItem* item = mMainThreadWorkItems.front();
			if (item != nullptr)
			{
				item->process();
				item->release();
			}
			mMainThreadWorkItems.pop();
		}
		mutexHandle.unlock();
	}

   SDL_CondSignal(mCondition);

	MutexHandle pHandle = TORQUE_LOCK(mPhysicalThreadsMutex);
	for (auto& physicalThread : mPhysicalThreads)
	{
		physicalThread->stop();
		physicalThread->join();
		physicalThread.reset();
	}

	mPhysicalThreads.clear();
	pHandle.unlock();

   SDL_DestroyMutex(mMutex);
   SDL_DestroyCond(mCondition);

	Con::printf("Thread pool shut down");
}

void ThreadPool::queueWorkItem(WorkItem* item)
{
	bool executeRightAway = (getForceAllMainThread());

	{
		item->addRef();
		item->mStatus.store(WorkItem::StatusPending, std::memory_order_relaxed);

		if (!item->isMainThreadOnly() && !executeRightAway)
		{
         SDL_LockMutex(mMutex);
			mWorkItems.push(item);
         SDL_CondSignal(mCondition);
			mBackgroundWorkItemCount.store(mWorkItems.size(), std::memory_order_relaxed);
         SDL_UnlockMutex(mMutex);
		}
		else
		{
			MutexHandle mutexHandle = TORQUE_LOCK(mMainThreadMutex);
			mMainThreadWorkItems.push(item);
			mMainThreadWorkItemCount.store(mMainThreadWorkItems.size(), std::memory_order_relaxed);
		}
	}
}

bool ThreadPool::processMainThreadItem()
{
	WorkItem* workItem = nullptr;
	{
		MutexHandle mutexHandle = TORQUE_LOCK(mMainThreadMutex);

		if (!mIsStopped && !mMainThreadWorkItems.empty())
		{
			workItem = mMainThreadWorkItems.front();
			mMainThreadWorkItems.pop();
			mMainThreadWorkItemCount.store(mMainThreadWorkItems.size(), std::memory_order_relaxed);
		}
	}

	if (workItem != nullptr)
	{
		workItem->process();
		workItem->release();
		return true;
	}

	return false;
}

void ThreadPool::processAllMainThreadItems()
{
	WorkItem* workItem = nullptr;
	{
		MutexHandle mutexHandle = TORQUE_LOCK(mMainThreadMutex);

		while (!mMainThreadWorkItems.empty())
		{
			workItem = mMainThreadWorkItems.front();
			mMainThreadWorkItems.pop();

			if (workItem)
			{
				workItem->process();
				workItem->release();
			}
		}

		mMainThreadWorkItemCount.store(0, std::memory_order_relaxed);
	}
}

size_t ThreadPool::getNumThreads() const
{
	MutexHandle handle = TORQUE_LOCK(mPhysicalThreadsMutex);
	return mPhysicalThreads.size();
}

bool ThreadPool::isPoolThreadID(SDL_threadID threadID) const
{
	auto threadHasTargetID = [threadID](const std::unique_ptr<Thread>& thread)
	{
		return thread != nullptr && thread->getId() == threadID;
	};

	MutexHandle handle = TORQUE_LOCK(mPhysicalThreadsMutex);
	auto iter = std::find_if(mPhysicalThreads.begin(), mPhysicalThreads.end(), threadHasTargetID);

   bool isPoolThreadId = iter != mPhysicalThreads.end();
   handle.unlock();

   return isPoolThreadId;
}

bool ThreadPool::waitForWorkItems() const
{
	MutexHandle handle;

   while (!mWorkItems.empty() || mNumberOfActiveWorkThreads > 0)
   {
	  handle = TORQUE_TRY_LOCK(mPhysicalThreadsMutex);
      SDL_CondWait(mWorkerThreadCondition, mMainThreadMutex);
   }

   //Just to be sure so we can proceed
   handle.unlock();
   return true;
}

namespace Command
{
namespace Task
{

/*static*/ bool PushToWorkerThread::isInTargetThread() const
{
	return ThreadPool::instance()->isPoolThreadID(ThreadManager::getCurrentThreadId());
}

} // namespace Thread
} // namespace Command
