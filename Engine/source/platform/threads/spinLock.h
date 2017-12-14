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

#ifndef _PLATFORM_THREADS_SPIN_LOCK_H_
#define _PLATFORM_THREADS_SPIN_LOCK_H_

#include <mutex>
#include <atomic>

/**
 * @brief A strateforward implementation of a spin-lock concept
 * @details Satisfies BasicLockable and Lockable concepts of C++ standard library.
 * Intended to be RAII-used with std::unique_lock, std::lock_guard, StdX::Lock(),
 * Std::TryLock()
 *
 * As opposed to things like mutexes, will not yield thread execution
 * while waiting for a lock. Will block thread execution by spinning inside a
 * loop which results in burning CPU resources aggressively.
 *
 * As std::mutex, has constexpr constructor, which means that static SpinLock-s
 * are initialized as part of static non-local initialization, before any
 * dynamic non-local initialization begins.
 * This makes it safe to lock a SpinLock in a constructor of any static object.
 */
class SpinLock final
{
public:
	constexpr SpinLock() = default;
	SpinLock(const SpinLock&) = delete;
	SpinLock& operator=(const SpinLock&) = delete;
	SpinLock(SpinLock&&) = delete;
	SpinLock& operator=(SpinLock&&) = delete;

	void lock();
	void unlock();
	bool try_lock();

private:
	std::atomic_flag mLocked = ATOMIC_FLAG_INIT;
};


inline void SpinLock::lock()
{
	while (mLocked.test_and_set(std::memory_order_acquire))
		;
}

inline void SpinLock::unlock()
{
	mLocked.clear(std::memory_order_release);
}

inline bool SpinLock::try_lock()
{
	return !mLocked.test_and_set(std::memory_order_acquire);
}

#endif // _PLATFORM_THREADS_SPIN_LOCK_H_
