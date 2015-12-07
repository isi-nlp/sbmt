// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#include <Common/Mutex.h>
#include <Common/Atomic.h>
#include <Common/TraceCommon.h>

namespace LW {


VolatileInt Mutex::num_ = 0;

Mutex::Mutex()
{
	TRACE_CALL(lib_com.mutex, ("Mutex{0x%x}::Mutex", _T_PTR(this)));
#ifdef _WIN32
	h_ = CreateMutex(0, false, 0);
	atomicIncrement(num_);
#	ifdef MUTEX_PERFMON
	time_locked_.QuadPart = 0;
	time_waited_.QuadPart = 0;
#	endif
#endif
#if (defined(__unix__) || defined(__MACH__))
    pthread_mutex_init(&h_, 0);
#endif
}

Mutex::~Mutex()
{
	TRACE_CALL(lib_com.mutex, ("Mutex{0x%x}::~Mutex", _T_PTR(this)));
#ifdef _WIN32
	CloseHandle(h_);
	atomicDecrement(num_);
#endif
#if (defined(__unix__) || defined(__MACH__))
    pthread_mutex_destroy(&h_);
#endif
}

void 
Mutex::lock()
{
	TRACE_CALL(lib_com.mutex, ("Mutex{0x%x}::lock", _T_PTR(this)));
#ifdef _WIN32
#	ifdef MUTEX_PERFMON
	LARGE_INTEGER time_in;
	QueryPerformanceCounter(&time_in);
#	endif
	WaitForSingleObject(h_, INFINITE);
#	ifdef MUTEX_PERFMON
	QueryPerformanceCounter(&last_locked_);
	time_waited_.QuadPart += last_locked_.QuadPart - time_in.QuadPart;
#	endif

#endif
#if (defined(__unix__) || defined(__MACH__))
    pthread_mutex_lock(&h_);
#endif
}

void 
Mutex::unlock()
{
	TRACE_CALL(lib_com.mutex, ("Mutex{0x%x}::unlock", _T_PTR(this)));
#ifdef _WIN32
#	ifdef MUTEX_PERFMON
	LARGE_INTEGER time_out;
	QueryPerformanceCounter(&time_out);
	time_locked_.QuadPart += time_out.QuadPart - last_locked_.QuadPart;
#	endif
	ReleaseMutex(h_);
#endif
#if (defined(__unix__) || defined(__MACH__))
    pthread_mutex_unlock(&h_);
#endif
}


};
