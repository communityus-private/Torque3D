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

#ifndef _PLATFORM_THREADS_MUTEX_UNTRACED_MUTEX_IMPL_H_
#define _PLATFORM_THREADS_MUTEX_UNTRACED_MUTEX_IMPL_H_

#include "platform/threads/mutex/mutexInternalApiMacros.h"

#include <memory>
#include <SDL_thread.h>
#include <SDL_mutex.h>

namespace MutexDetails
{

class UntracedMutexImpl final
{
public:
	/**
	 * @brief Dummy ID to match TracedMutexImpl interface
	 */
	using LockID = EmptyType;

	explicit UntracedMutexImpl(const char* name);
	~UntracedMutexImpl();

	LockID lock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS);
	std::pair<LockID, bool> tryLock(MUTEX_INTERNAL_TRACE_LOCK_PARAMS);
	void unlock(LockID);

   SDL_threadID getOwningThreadID() const;
   SDL_mutex* getRootMutex() const;

private:
	struct MutexData;

	static void _reportAboutPossibleDeadlock();

	const std::unique_ptr<MutexData> mData;
};

} // namespace MutexDetails

#endif // _PLATFORM_THREADS_MUTEX_UNTRACED_MUTEX_IMPL_H_
