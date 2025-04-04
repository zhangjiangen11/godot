/**************************************************************************/
/*  mutex.cpp                                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "mutex.h"

static Mutex _global_mutex;

void _global_lock() {
	_global_mutex.lock();
}

void _global_unlock() {
	_global_mutex.unlock();
}

#ifdef THREADS_ENABLED

#if defined(__WIN32__) && !defined(PTHREADS_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

SystemMutex::SystemMutex() {
	mutex = new CRITICAL_SECTION;
	InitializeCriticalSection((CRITICAL_SECTION *)mutex);
}
SystemMutex::~SystemMutex() {
	DeleteCriticalSection((CRITICAL_SECTION *)mutex);
	delete (CRITICAL_SECTION *)mutex;
}
void SystemMutex::lock() {
	EnterCriticalSection((CRITICAL_SECTION *)mutex);
}
bool SystemMutex::try_lock() {
	return TryEnterCriticalSection((CRITICAL_SECTION *)mutex) != 0;
}
void SystemMutex::unlock() {
	LeaveCriticalSection((CRITICAL_SECTION *)mutex);
}
#endif

#if defined(__UNIX__) || defined(PTHREADS_WIN32)
#include <pthread.h>
/*! system mutex using pthreads */
SystemMutex::SystemMutex() {
	mutex = new pthread_mutex_t;
	if (pthread_mutex_init((pthread_mutex_t *)mutex, nullptr) != 0) {
		THROW_RUNTIME_ERROR("pthread_mutex_init failed");
	}
}

SystemMutex::~SystemMutex() {
	MAYBE_UNUSED bool ok = pthread_mutex_destroy((pthread_mutex_t *)mutex) == 0;
	assert(ok);
	delete (pthread_mutex_t *)mutex;
	mutex = nullptr;
}

void SystemMutex::lock() {
	if (pthread_mutex_lock((pthread_mutex_t *)mutex) != 0) {
		THROW_RUNTIME_ERROR("pthread_mutex_lock failed");
	}
}

bool SystemMutex::try_lock() {
	return pthread_mutex_trylock((pthread_mutex_t *)mutex) == 0;
}

void SystemMutex::unlock() {
	if (pthread_mutex_unlock((pthread_mutex_t *)mutex) != 0) {
		THROW_RUNTIME_ERROR("pthread_mutex_unlock failed");
	}
}

#endif

template class MutexImpl<THREADING_NAMESPACE::recursive_mutex>;
template class MutexImpl<THREADING_NAMESPACE::mutex>;
template class MutexLock<MutexImpl<THREADING_NAMESPACE::recursive_mutex>>;
template class MutexLock<MutexImpl<THREADING_NAMESPACE::mutex>>;

#endif
