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

#include "platform/threads/threadSafeRefCount.h"
#include "platform/threads/thread.h"
#include "platform/threads/mutex/mutex.h"
#include "core/util/command.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <atomic>
#include <future>

class ThreadPool final
{
	friend void ThreadWorkerThunk(void*);
public:

	class WorkItem : public ThreadSafeRefCount<WorkItem>
	{
		friend class ThreadPool;

	private:

		inline void process()
		{
			mStatus.store(StatusRunning, std::memory_order_relaxed);
			execute();
			mStatus.store(StatusFinished, std::memory_order_relaxed);
		}

	protected:

		enum Flags
		{
			FlagDoesSynchronousIO   = (1U << 0),
			FlagMainThreadOnly      = (1U << 1)
		};

		enum Status
		{
			StatusNone              = 0,
			StatusPending           = 1,
			StatusRunning           = 2,
			StatusFinished          = 3
		};

		uint32_t            mFlags = 0;
		std::atomic<U32>    mStatus;

	public:

		virtual ~WorkItem() {}

		virtual void execute() = 0;

		inline bool isMainThreadOnly()      const { return (mFlags & FlagMainThreadOnly); }
		inline bool doesSynchronousIO()     const { return (mFlags & FlagDoesSynchronousIO); }

		inline bool isPending()             const { return mStatus.load(std::memory_order_relaxed) == StatusPending; }
		inline bool isRunning()             const { return mStatus.load(std::memory_order_relaxed) == StatusRunning; }
		inline bool isFinished()            const { return mStatus.load(std::memory_order_relaxed) == StatusFinished; }
	};

private:
	void _processBackgroundThreadWorkers();

	std::mutex                  mMutex;
	Mutex                       mMainThreadMutex;
	std::condition_variable     mCondition;
	std::queue<WorkItem*>       mWorkItems;
	std::queue<WorkItem*>       mMainThreadWorkItems;
	bool                        mIsStopped = false;

	// TODO [snikitin@outlook.com] Explore possability of migration to std::thread
	//std::vector<std::thread>    mPhysicalThreads;
	mutable Mutex                        mPhysicalThreadsMutex;
	std::vector<std::unique_ptr<Thread>> mPhysicalThreads;

	// statistics
	std::atomic<size_t>         mBackgroundWorkItemCount;
	std::atomic<size_t>         mMainThreadWorkItemCount;

public:

	static ThreadPool* instance();

	ThreadPool();
	inline ~ThreadPool() {}

	size_t getNumThreads() const;

	inline size_t getBackgroundWorkItemCount() const   { return mBackgroundWorkItemCount.load(std::memory_order_relaxed); }
	inline size_t getMainThreadWorkItemCount() const   { return mMainThreadWorkItemCount.load(std::memory_order_relaxed); }
	inline size_t getTotalWorkItemCount()      const   { return getBackgroundWorkItemCount() + getMainThreadWorkItemCount(); }

	void initialize();
	void shutdown();

	bool isShuttingDown() { return mIsStopped; }

	void queueWorkItem(WorkItem* item);

	bool processMainThreadItem();
	void processAllMainThreadItems();
	bool isPoolThreadID(U32 threadID) const;
};

namespace Async
{
namespace Details
{
template <typename Callable, typename... Args>
class ThreadPoolWorkItem final : public ThreadPool::WorkItem
{
public:
	ThreadPoolWorkItem(Callable&& callable, Args&& ... args)
		: mCallable(std::forward<Callable>(callable))
		, mArgs(std::forward<Args>(args)...)
	{
	}

	void execute() override
	{
		StdX::Apply(std::move(mCallable), std::move(mArgs));
		mPromise.set_value();
	}

	static std::future<void> LaunchWorkItem(Callable&& callable, Args&& ... args)
	{
		auto workItem = std::make_unique<Details::ThreadPoolWorkItem<Callable, Args...>>(std::forward<Callable>(callable), std::forward<Args>(args)...);
		auto future = workItem->mPromise.get_future();
		ThreadPool::instance()->queueWorkItem(workItem.release());
		return future;
	}

private:
	std::decay_t<Callable> mCallable;
	std::tuple<std::decay_t<Args>...> mArgs;
	std::promise<void> mPromise;
};
} // namespace Details

namespace Force
{
/**
* @brief Calls given function with given arguments asynchronously in a worker thread
*
* This is a Force-version which is absolutely asynchronous, i.e. it will queue the task in a new thread pool work item
* even if the call is performed in any worker thread.
*
* @see Async::CallInWorkerThread (non-Force version)
*
* @return std::future which will be fulfilled when the task is completed
*/
template <typename Callable, typename... Args> std::future<void> CallInWorkerThread(Callable&& callable, Args&& ... args)
{
	return Async::Details::ThreadPoolWorkItem<Callable, Args...>::LaunchWorkItem(std::forward<Callable>(callable), std::forward<Args>(args)...);
}
} // namespace Force
/**
 * @brief Calls given function with given arguments asynchronously in a worker thread
 *
 * This function is pretty useful, when you need to execute some code in a parallel thread.
 * It uses ThreadPool and does not create any new threads. Keep in mind that all passed entities (callable and args)
 * are stored in underlying work item using perfect forwarding, so if you forget to std::move something, it is likely to be copied.
 *
 * @note This is a non-Force version - if you use it in thread pool worker thread, it will call requested code synchronously, i.e. immediately
 *
 * Typical usage:
 *    auto asyncFunction = [](std::vector<U32> someData) { ... async code here ... };
 *    Async::CallInWorkerThread(asyncFunction, std::move(someData));
 *
 * @return std::future which will be fulfilled when the task is completed.
 *    If the call was made from a worker thread, fulfilled std::future will be
 *    returned immediately
 */
template <typename Callable, typename... Args> std::future<void> CallInWorkerThread(Callable&& callable, Args&& ... args)
{
	if (ThreadPool::instance()->isPoolThreadID(ThreadManager::getCurrentThreadId()))
	{
		callable(std::forward<Args>(args)...);
		std::promise<void> promise;
		promise.set_value();
		return promise.get_future();
	}
	else
	{
		return Force::CallInWorkerThread(std::forward<Callable>(callable), std::forward<Args>(args)...);
	}
}
} // namespace Async

namespace Command
{

namespace Details
{
template <typename... Tasks>
class Cmd;
} // namespace Details

namespace Task
{

struct PushToWorkerThread
{
	template <typename... Tasks>
	void moveToTargetThread(Details::Cmd<Tasks...> cmd) const
	{
		auto execute = [](Details::Cmd<Tasks...> cmd)
		{
			Details::Cmd<Tasks...>::ContinueExecution(std::move(cmd));
		};
		Async::Force::CallInWorkerThread(execute, std::move(cmd));
	}

	bool isInTargetThread() const;
};

template <typename ExecuteFunc, typename RollbackFunc>
auto WorkerThread(ExecuteFunc&& execute, RollbackFunc&& rollback)
{
	return Details::Task::GenericTask<ExecuteFunc, RollbackFunc, PushToWorkerThread>(
		std::forward<ExecuteFunc>(execute),
		std::forward<RollbackFunc>(rollback),
		PushToWorkerThread());
}

template <typename ExecuteFunc>
auto WorkerThread(ExecuteFunc&& execute)
{
	return WorkerThread(std::forward<ExecuteFunc>(execute), [](){});
}

} // namespace Task

} // namespace Command
