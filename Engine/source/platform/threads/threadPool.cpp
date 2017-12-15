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
#include "threadPool.h"

#include "console/console.h"

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
			std::unique_lock<std::mutex> lock(mMutex);

			mCondition.wait(lock, [this]()
			{
				return mIsStopped || !mWorkItems.empty();
			});

			if (mIsStopped && mWorkItems.empty())
			{
				Con::printf("Thread worker terminated");
				return;
			}

			workItem = mWorkItems.front();
			mWorkItems.pop();
			mBackgroundWorkItemCount.store(mWorkItems.size(), std::memory_order_relaxed);
		}

		if (workItem != nullptr)
		{
			workItem->process();
			workItem->release();
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
	int32_t numPhysicalThreads = std::thread::hardware_concurrency();
	int32_t numBackgroundThreads = std::max(numPhysicalThreads - 1, 1);

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
		std::unique_lock<std::mutex> lock(mMutex);
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
	}
	mCondition.notify_all();

	auto handle = TORQUE_LOCK(mPhysicalThreadsMutex);
	for (auto& physicalThread : mPhysicalThreads)
	{
		physicalThread->stop();
		physicalThread->join();
		physicalThread.reset();
	}

	mPhysicalThreads.clear();
	handle.unlock();

	Con::printf("Thread pool shut down");
}

void ThreadPool::queueWorkItem(WorkItem* item)
{
	{
		item->addRef();
		item->mStatus.store(WorkItem::StatusPending, std::memory_order_relaxed);

		if (!item->isMainThreadOnly())
		{
			std::unique_lock<std::mutex> lock(mMutex);
			mWorkItems.push(item);
			mCondition.notify_one();
			mBackgroundWorkItemCount.store(mWorkItems.size(), std::memory_order_relaxed);
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
	auto handle = TORQUE_LOCK(mPhysicalThreadsMutex);
	return mPhysicalThreads.size();
}

bool ThreadPool::isPoolThreadID(U32 threadID) const
{
	auto threadHasTargetID = [threadID](const std::unique_ptr<Thread>& thread)
	{
		return thread != nullptr && thread->getId() == threadID;
	};

	auto handle = TORQUE_LOCK(mPhysicalThreadsMutex);
	auto iter = std::find_if(mPhysicalThreads.begin(), mPhysicalThreads.end(), threadHasTargetID);
	return iter != mPhysicalThreads.end();
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
