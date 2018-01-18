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
#include "platform/threads/mutex/threadLockStack.h"

#include "core/util/stdExtensions.h"

#include <mutex>

#include <iomanip>
#include <fstream>
#include <time.h>
#include <sstream>

namespace MutexDetails
{

LockCallDescription::LockCallDescription(MUTEX_INTERNAL_TRACE_LOCK_PARAMS, const char* name)
	: mutexName((name != nullptr && name[0] != '\0') ? name : "unnamed mutex")
	, function(function)
	, file(file)
	, line(line)
{}


LockState::LockState(LockCallDescription description, bool locked)
	: callDescription(description)
	, locked(locked)
{
}

std::string LockState::toString() const
{
	std::stringstream stream;
	stream << "mutex \"" << callDescription.mutexName << "\" "
	       << (locked ? "locked" : "not yet locked") << '\n'
	       << "\t\t\tfile: " << callDescription.file << '\n'
	       << "\t\t\tfunction: " << callDescription.function << '\n'
	       << "\t\t\tline: " << callDescription.line;

	return stream.str();
}

/*static*/ LockState LockState::MakeUnlocked(LockCallDescription desc)
{
	return LockState(desc, false);
}

/*static*/ LockState LockState::MakeLocked(LockCallDescription desc)
{
	return LockState(desc, true);
}


LockStack::Entry::Entry(LockState lockState) : lockState(lockState)
{
}


LockStack::LockStack() : threadID(SDL_ThreadID())
{
	mStack.reserve(InitialLockStackSize);
	GlobalLockStackPool::Instance().registerStack(*this);
}

LockStack::~LockStack()
{
	GlobalLockStackPool::Instance().unregisterStack(*this);
}

/*static*/ LockStack& LockStack::ThreadLocalInstance()
{
	thread_local LockStack instance;
	return instance;
}

LockStack::EntryID LockStack::push(LockState state)
{
	auto lock = StdX::Lock(mSpinLock);
	AssertFatal(!mStack.empty(), "Can't push an empty Stack!");
	AssertFatal(!mStack.back().lockState.locked, "Attemptempting to acquire a Thread Lock more than once!");

	mStack.emplace_back(state);
	return mStack.size() - 1;
}

void LockStack::lock(LockStack::EntryID id)
{
	auto lock = StdX::Lock(mSpinLock);
	// NOTE: Although the presense of `id` parameter in this method makes it
	// possible to mark any entry in mStack as locked, if `id` does not correspond
	// to the last entry, it would indicate that there is an error in higher
	// level logic (because if mutex B locking start after mutex A is locked, there is no
	// way B is locked before A).
	// One might wonder - why have `id` parameter at all in this method?
	// It is present to make public API of LockStack clearer (you lock() what you
	// push()) and to add additional layer of debug checks.
	AssertFatal(!(!mStack.empty() && id == mStack.size() - 1), avar("Improper Thread Locking order of operations: (%i != %i)", id, mStack.size() - 1));
	AssertFatal(mStack.back().lockState.locked, "Thread Lock not Aquired!");
	mStack.back().lockState.locked = true;
}

void LockStack::remove(LockStack::EntryID id)
{
	auto lock = StdX::Lock(mSpinLock);
	mStack[id].isRemoved = true;

	while (!mStack.empty() && mStack.back().isRemoved)
	{
		mStack.pop_back();
	}
}

std::string LockStack::makeDescription() const
{
	auto lock = StdX::Lock(mSpinLock);
	if (mStack.empty())
	{
		return "";
	}

	std::stringstream stream;
	stream << "Thread ID: " << threadID << '\n';
	for (const Entry& entry : mStack)
	{
		if (!entry.isRemoved)
		{
			stream << entry.lockState.toString() << '\n';
		}
	}

	return stream.str();
}


struct GlobalLockStackPool::MutexData
{
   SDL_mutex* mutex;
   SDL_threadID owner_handle;
};


/*static*/ GlobalLockStackPool& GlobalLockStackPool::Instance()
{
	return *Singleton<GlobalLockStackPool>::instance();
}

GlobalLockStackPool::GlobalLockStackPool()
	: mMutexData(std::make_unique<MutexData>())
{
}

void GlobalLockStackPool::registerStack(const LockStack& stack)
{
	auto lock = StdX::Lock(mMutexData->mutex);
	mStacks.push_back(&stack);
}

void GlobalLockStackPool::unregisterStack(const LockStack& stack)
{
	auto lock = StdX::Lock(mMutexData->mutex);
	auto iter = std::find(mStacks.begin(), mStacks.end(), &stack);
	if (iter != mStacks.end())
	{
		StdX::UnorderingErase(&mStacks, iter);
	}
}

void GlobalLockStackPool::dumpToFile(const char* filename) const
{
	std::ofstream file(filename);
	if (!file.is_open())
	{
		return;
	}
	file << "Per-thread mutex lock report" << '\n'
	     << "Time: " << _getCurrentDateTime() << '\n';

	auto lock = StdX::Lock(mMutexData->mutex);
	for (const LockStack* stack : mStacks)
	{
		auto description = stack->makeDescription();
		if (!description.empty())
		{
			file << description << "\n\n";
		}
	}
	lock.release();

	file << std::endl;
}

/*static*/ std::string GlobalLockStackPool::_getCurrentDateTime()
{
	tm tmTimeLock;
	time_t nTime = time(nullptr);
	localtime_s(&tmTimeLock, &nTime);
	std::stringstream stream;
	stream << std::put_time(&tmTimeLock, "%Y-%m-%d %H:%M:%S");

	return stream.str();
}

} // namespace MutexDetails
