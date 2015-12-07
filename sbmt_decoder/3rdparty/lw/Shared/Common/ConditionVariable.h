// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef LW_CONDITION_VARIABLE
#define LW_CONDITION_VARIABLE 1

#include <Common/Mutex.h>
#include <Common/Atomic.h>
#ifdef _WIN32
#	include <windows.h>
#	include <vector>
#else
#	include <pthread.h>
#endif


namespace LW {



class Mutex;

/// condition variable with "wake all" semantics; gets signalled once
class ConditionVariable
{
public:
	static long getNum() { return num_; }

	ConditionVariable();
	~ConditionVariable();

	bool wait(Mutex* mutex, int msec = 0);
	void wakeup();

private:

#ifdef _WIN32
	std::vector<HANDLE> events_;
#else
	pthread_cond_t cid_;
#endif

	static VolatileInt num_;
};




/// condition variable with "wake one" semantics
class ConditionVariable1
{
public:
	static long getNum() { return num_; }

	ConditionVariable1();
	~ConditionVariable1();

	bool wait(int msec = 0);
	void wakeup();
	void reset();

private:
	static VolatileInt num_;
#ifdef _WIN32
	HANDLE event_;
#else
	bool signalled_;
	pthread_cond_t cid_;
    Mutex mutex_;
#endif
};

};


#endif // LW_CONDITION_VARIABLE
