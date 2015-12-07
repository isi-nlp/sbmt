// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef WIN32
#pragma implementation
#endif

#include "Common/logprob.h"

/////////////////// Static members for LogProb class ////////////////////////
	template<class T>
		const T LogProbBase<T>::LOG_VALUE_ONE = 0;
	template<class T>
		const T LogProbBase<T>::LOG_VALUE_ZERO = -pow(2., 125);

	template<class T>
		const LogProbBase<T> LogProbBase<T>::one = LogProbBase<T>(true, LogProbBase<T>::LOG_VALUE_ONE);
	template<class T>
		const LogProbBase<T> LogProbBase<T>::zero = LogProbBase<T>(true, LogProbBase<T>::LOG_VALUE_ZERO);


/// Macro to implement static data members for specific underlying data types that hold 
/// store the probability (e.g float)
#define IMPLEMENT(T) \
	template <> const T LogProbBase<T>::LOG_VALUE_ONE = 0; \
	template <> const T LogProbBase<T>::LOG_VALUE_ZERO = (T) pow(-2., 23); \
	template <> const LogProbBase<T> LogProbBase<T>::one = LogProbBase<T>(true, LogProbBase<T>::LOG_VALUE_ONE); \
	template <> const LogProbBase<T> LogProbBase<T>::zero = LogProbBase<T>(true, LogProbBase<T>::LOG_VALUE_ZERO);

IMPLEMENT(float)
IMPLEMENT(double)








