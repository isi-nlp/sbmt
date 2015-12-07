// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#include <Common/ConditionVariable.h>
#include <Common/Mutex.h>
#include <Common/Atomic.h>
#include <Common/LWAssert_.h>
#include <Common/TraceCommon.h>
#if defined(linux) || defined(__APPLE__) 
#	include <sys/time.h>
#endif


namespace LW {

VolatileInt
ConditionVariable::num_ = 0;

VolatileInt
ConditionVariable1::num_ = 0;


#ifdef _WIN32

ConditionVariable::ConditionVariable()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable{0x%x}::ConditionVariable", _T_PTR(this)));
}
	
ConditionVariable::~ConditionVariable()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable{0x%x}::~ConditionVariable", _T_PTR(this)));
}

bool 
ConditionVariable::wait(Mutex* mutex, int msec)
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable{0x%x}::wait", _T_PTR(this)));

	HANDLE h = CreateEvent(0, true, false, 0);
	atomicIncrement(num_);
	events_.push_back(h);

	mutex->unlock();
	bool res = (WAIT_TIMEOUT != WaitForSingleObject(h, msec ? msec : INFINITE));
	mutex->lock();

	for (std::vector<HANDLE>::iterator i = events_.begin(); i != events_.end(); i++)
		if (*i == h) {
			events_.erase(i);
			break;
		}
	
	CloseHandle(h);
	atomicDecrement(num_);

	return res;
}


void 
ConditionVariable::wakeup()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable{0x%x}::wakeup", _T_PTR(this)));

	for (size_t i = 0; i < events_.size(); i++)
		SetEvent(events_[i]);
	events_.clear();
}



ConditionVariable1::ConditionVariable1()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::ConditionVariable1", _T_PTR(this)));

	event_ = CreateEvent(0, true, false, 0);
	atomicIncrement(num_);
}
	
ConditionVariable1::~ConditionVariable1()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::~ConditionVariable1", _T_PTR(this)));

	CloseHandle(event_);
	atomicDecrement(num_);
}

bool 
ConditionVariable1::wait(int msec)
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::wait", _T_PTR(this)));

	return (WAIT_TIMEOUT != WaitForSingleObject(event_, msec ? msec : INFINITE));
}

void 
ConditionVariable1::reset()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::reset", _T_PTR(this)));

	ResetEvent(event_);
}


void 
ConditionVariable1::wakeup()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::wakeup", _T_PTR(this)));

	SetEvent(event_);
}


#else // not windows

ConditionVariable::ConditionVariable()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable{0x%x}::ConditionVariable", _T_PTR(this)));

	pthread_cond_init(&cid_, 0);
	atomicIncrement(num_);
}
	
ConditionVariable::~ConditionVariable()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable{0x%x}::~ConditionVariable", _T_PTR(this)));

	pthread_cond_destroy(&cid_);
	atomicDecrement(num_);
}

bool 
ConditionVariable::wait(Mutex* mutex, int msec)
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable{0x%x}::wait", _T_PTR(this)));

	if (msec) {
		struct timeval tv;
		gettimeofday(&tv, 0);
    	struct timespec t = {tv.tv_sec + msec / 1000, tv.tv_usec * 1000 + (msec % 1000) * 1000000};
		int ret = pthread_cond_timedwait(&cid_, &mutex->h_, &t);
	    TRACE(lib_com.cnd, ("out of wait: ret=%d", ret));
		return ret == 0;
	}

    else {
		pthread_cond_wait(&cid_, &mutex->h_);
	    TRACE(lib_com.cnd, ("out of wait", _T_PTR(this)));
		return true;
	}
}


void 
ConditionVariable::wakeup()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable{0x%x}::wakeup", _T_PTR(this)));

    pthread_cond_broadcast(&cid_);
}



ConditionVariable1::ConditionVariable1()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::ConditionVariable1", _T_PTR(this)));

	signalled_ = false;
	pthread_cond_init(&cid_, 0);
	atomicIncrement(num_);
}
	
ConditionVariable1::~ConditionVariable1()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::~ConditionVariable1", _T_PTR(this)));

	pthread_cond_destroy(&cid_);
	atomicDecrement(num_);
}

bool 
ConditionVariable1::wait(int msec)
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::wait", _T_PTR(this)));

	mutex_.lock();
	while (!signalled_) {
		if (msec) {
    		struct timespec t = {msec / 1000, (msec % 1000) * 1000000};
			if (pthread_cond_timedwait(&cid_, &mutex_.h_, &t) != 0) {
				mutex_.unlock();
				TRACE(lib_com.cnd, ("timeout waiting", _T_PTR(this)));
				return false;
			}
		}

    	else 
			pthread_cond_wait(&cid_, &mutex_.h_);
	}

	mutex_.unlock();
	TRACE(lib_com.cnd, ("out of wait", _T_PTR(this)));
	return true;
}


void 
ConditionVariable1::wakeup()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::wakeup", _T_PTR(this)));

	mutex_.lock();
	signalled_ = true;
	pthread_cond_signal(&cid_);
	mutex_.unlock();
}


void 
ConditionVariable1::reset()
{
	TRACE_CALL(lib_com.cnd, ("ConditionVariable1{0x%x}::reset", _T_PTR(this)));

	mutex_.lock();
	signalled_ = false;
	mutex_.unlock();
}


#endif


};
