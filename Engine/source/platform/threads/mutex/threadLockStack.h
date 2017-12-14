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

#ifndef _PLATFORM_THREADS_MUTEX_THREAD_LOCK_STACK_H_
#define _PLATFORM_THREADS_MUTEX_THREAD_LOCK_STACK_H_

#include "platform/types.h"
#include "platform/threads/spinLock.h"
#include "platform/threads/mutex/mutexInternalApiMacros.h"

#include <string>
#include <vector>

namespace MutexDetails
{

/**
 * @brief Struct that contains data about invocation of mutex lock method
 * @details Is used to produce a report about current lock state of a thread
 */
struct LockCallDescription final
{
	LockCallDescription(MUTEX_INTERNAL_TRACE_LOCK_PARAMS, const char* mutexName);

	const U32         line;
	const char* const function;
	const char* const file;
	const char* const mutexName; /**< Non-owning pointer to the name of the assotiated mutex.
	                                  Fully copying a name would be safer but would produce too
	                                  much overhead for such common operation as mutex lock */
};

struct LockState final
{
	LockState(LockCallDescription, bool locked);

	static LockState MakeUnlocked(LockCallDescription);
	static LockState MakeLocked(LockCallDescription);

	std::string toString() const;

	const LockCallDescription callDescription;
	/**
	 * If set to true, indicates that lock was successfully acquired.
	 * If set to false, indicates that attempt to acquire lock is happening
	 */
	bool locked = false;
};

/**
 * @brief A stack that stores information about the sequence of mutex locks
 * that were acquired in the current moment in a specific thread
 */
class LockStack final
{
public:
	using EntryID = size_t;

	static LockStack& ThreadLocalInstance();

	/**
	 * @brief Pushes a @see LockState on the stack
	 * @details Should be assotiated with invokation of mutex lock method.
	 * It is the responsibility of the user to call @see remove() for each push()
	 * call it did before the assotiated mutex is destroyed.
	 * If present, last pushed @see LockState must be marked as locked
	 * (@see LockState::locked) before the new one is pushed
	 */
	EntryID push(LockState);
	/**
	 * @brief Sets @see LockState::locked of the @see LockState assotiated with
	 * the given EntryID to 'true'
	 * @details Should be used in situations when the last pushed @see LockState was
	 * pushed in an unlocked state
	 */
	void lock(EntryID);
	/**
	 * @brief Removes the @see LockState assotiated with the given EntryID
	 * from the stack
	 * @details Should be assotiated with invokation of mutex unlock method.
	 * It is the responsibility of the user to call this method for each push()
	 * call it did before the assotiated mutex is destroyed. The order of remove()
	 * calls may be different from the order of push() calls
	 */
	void remove(EntryID);

	/**
	 * @brief Generates a report about which mutexes are currently locked
	 * in the assotiated thread
	 */
	std::string makeDescription() const;

	const std::thread::id threadID;

private:
	struct Entry
	{
		Entry(LockState);

		/**
		 * isRemoved flag is a way of implementing @see remove() that allows
		 * unlocks to happen in an order different to the order of locks.
		 *
		 * If unlocks happen in the reversed order of locks (which is the most
		 * common scenario), Entry will just be poped from the back of the
		 * @see mStack.
		 * If unlocks happen in a different order, @see remove()-ed Entry
		 * is marked as removed to indicate that it is not a part of lock stack
		 * anymore, while not invalidating still locked EntryID-s
		 */
		bool isRemoved = false;
		LockState lockState;
	};

	static constexpr size_t InitialLockStackSize = 10;
	static_assert(InitialLockStackSize > 0, "LockStack::InitialLockStackSize must be > 0");

	LockStack();
	~LockStack();

	std::vector<Entry> mStack;
	// NOTE: Spin-lock is used instead of a full mutex because 99% of the
	// calls to public API of this class will be invoked from a single thread
	// (the thread this stack corresponds to, @see threadID), so spin-lock
	// will be very fast. The only reason the syncronization is needed at all
	// is to allow thread-safe calls to @see makeDescription() which
	// @see GlobalLockStackPool does upon request to generate a report file.
	// Generation of this report should happen incredibly rarely and in emergency
	// situations only (like deadlock report generation)
	mutable SpinLock mSpinLock;
};

class GlobalLockStackPool final
{
	friend Singleton<GlobalLockStackPool>;
public:
	static GlobalLockStackPool& Instance();

	void registerStack(const LockStack&);
	void unregisterStack(const LockStack&);

	void dumpToFile(const char* filename) const;

private:
	struct MutexData;

	GlobalLockStackPool();
	~GlobalLockStackPool() = default;

	static std::string _getCurrentDateTime();

	mutable std::unique_ptr<MutexData> mMutexData;
	std::vector<const LockStack*> mStacks;
};

} // namespace MutexDetails

#endif // _PLATFORM_THREADS_MUTEX_THREAD_LOCK_STACK_H_
