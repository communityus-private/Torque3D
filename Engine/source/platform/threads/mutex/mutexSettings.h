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

#ifndef _PLATFORM_THREADS_MUTEX_MUTEX_SETTINGS_H_
#define _PLATFORM_THREADS_MUTEX_MUTEX_SETTINGS_H_

#include  <thread>
std::thread::id gNullThreadID;

namespace MutexDetails
{

constexpr size_t DeadLockThresholdInSec = 30;

const char* const DeadLockUserWarningMsg =
	"Mutex was not able to obtain a lock for alarmingly long, the application is "
	"possibly in a deadlock state. You may choose to retry to acquire the "
	"lock and if this was a false deadlock detection case, the application will "
	"proceed to function after some time. If you choose to retry and the deadlock is "
	"real, you will not be asked to retry again and the application will hang and "
	"won't return to a responsive state.\n\n"
	"Would you like to retry to acquire the lock?";

const char* const DeadLockUserFatalErrorMsg =
	"Mutex was not able to obtain a lock for alarmingly long, the application is "
	"possibly in a deadlock state";

} // namespace MutexDetails

#endif // _PLATFORM_THREADS_MUTEX_MUTEX_SETTINGS_H_
