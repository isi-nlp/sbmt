// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef LW_MUTEX_H
#define LW_MUTEX_H

#ifdef _WIN32
#  include <windows.h>
//#  define MUTEX_PERFMON
#endif
#if (defined(__unix__) || defined(__MACH__))
#  include <pthread.h>
#endif
#include <Atomic.h>

namespace LW {

class Mutex 
{
public:

	static long getNum() { return num_; }

	Mutex();
	~Mutex();

	void lock();
	void unlock();

#ifdef MUTEX_PERFMON
	LARGE_INTEGER getTimeLocked() const { return time_locked_; }
	LARGE_INTEGER getTimeWaited() const { return time_waited_; }
#endif

private:

#ifdef _WIN32
	HANDLE h_;

#	ifdef MUTEX_PERFMON
	LARGE_INTEGER time_locked_;
	LARGE_INTEGER time_waited_;
	LARGE_INTEGER last_locked_;
#	endif

#endif
#if (defined(__unix__) || defined(__MACH__))
    pthread_mutex_t h_;
    friend class ConditionVariable;
    friend class ConditionVariable1;
#endif
	static VolatileInt num_; ///< handle count
};

class MutexLocker
{
public:
	MutexLocker(Mutex* mutex)
	{
		mutex_ = mutex;
		locked_ = false;
		lock();
	}

	~MutexLocker() { unlock(); }

	void lock() 
    { 
       	mutex_->lock(); 
		locked_ = true;
    }
	void unlock() 
    { 
		if (locked_) {
			mutex_->unlock(); 
			locked_ = false;
		}
    }

private:
    bool locked_;
	Mutex* mutex_;
};


#ifdef _WIN32
class CriticalSection
{
public:
	CriticalSection() {	InitializeCriticalSection(&cs_); }
	~CriticalSection() { DeleteCriticalSection(&cs_); }
	void lock() { EnterCriticalSection(&cs_); }
	void unlock() { LeaveCriticalSection(&cs_); }

private:
	CRITICAL_SECTION cs_;
};
#else
typedef Mutex CriticalSection;
#endif

};

#endif // LW_MUTEX_H
