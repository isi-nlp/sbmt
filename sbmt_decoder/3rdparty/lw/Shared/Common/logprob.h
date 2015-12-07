// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _LOGPROB_H
#define _LOGPROB_H

#ifndef _WIN32
#pragma interface
#endif
// Routines to perform integer exponential arithmetic.
// A number x is represented as n, where x = b**n
// It is assumed that b > 1, something like b = 1.001

#include <iostream>
#include <math.h>
#include <algorithm>
#include <vector>

using namespace std;

//#ifdef NEW_LOG_PROB
/////////////////////////////////////////////////////////////////////////////////////
/*
This class is used to encapsulate probabilities as logs and to perform
mathematical operations on them.
*/
template<class T>
class LogProbBase {
private:
	static const T LOG_VALUE_ONE, LOG_VALUE_ZERO;
protected:
	/// Creating from another log value
	LogProbBase(bool bFromLog, T logValue) {
		m_logValue = logValue;
	}
public:
	/// Copy constructor
	LogProbBase(const LogProbBase<T>& src) {
		operator=(src);
	}

	/// Default constructor
	LogProbBase() {
		m_logValue = LOG_VALUE_ZERO;
	}

	/// Constructor from double
	LogProbBase(double d) {
		if (0 == d) {
			m_logValue = LOG_VALUE_ZERO;
		}
		else {
			m_logValue = (T) log(d);
		};
	}

	operator double() const {
		return exp((double) m_logValue);
	}

	LogProbBase<T> operator* (const LogProbBase<T>& logProb) const {
		return LogProbBase<T>(true, addLogProb(m_logValue, logProb.m_logValue));
	}

	LogProbBase<T> operator* (double d) const {
		return LogProbBase<T>(true, addLogProb(m_logValue, LogProbBase<T>(d)));
	}

	LogProbBase<T>& operator*= (const LogProbBase<T>& logProb) {
		m_logValue = addLogProb(m_logValue, logProb.m_logValue);

		return *this;
	}

	LogProbBase<T>& operator=(const LogProbBase<T>& logProb) {
		m_logValue = logProb.m_logValue;

		return *this;
	}

	bool operator<	(const LogProbBase<T>& src) const {
		return m_logValue < src.m_logValue; 
	}

	bool operator<=	(const LogProbBase<T>& src) const { 
		return m_logValue <= src.m_logValue;
	}

	bool operator>	(const LogProbBase<T>& src)  const {
		return m_logValue >  src.m_logValue;
	}

	bool operator>=	(const LogProbBase<T>& src) const {
		return m_logValue >= src.m_logValue;
	}

	bool operator==	(const LogProbBase<T>& src) const {
		return m_logValue == src.m_logValue;
	}

	bool operator!=	(const LogProbBase<T>& src) const {
		return m_logValue != src.m_logValue;
	}

	bool operator<	(double d) const {
		return ((double)*this) < d;  
	}

	bool operator<=	(double d) const {
		return ((double)*this) <= d; 
	
	}

	bool operator>	(double d)  const {
		return ((double)*this) > d; 
	}

	bool operator>= (double d) const {
		return ((double)*this) >= d; 
	}

	bool operator== (double d) const {
		return ((double)*this) == d; 
	}

	bool operator!= (double d) const {
		return ((double)*this) != d; 
	}
public:
	/// Initialized the probablity to 1
	void SetOne() {
		m_logValue = LOG_VALUE_ONE;
	}

	/// Initializes the probability with 0
	void SetZero() {
		m_logValue = LOG_VALUE_ZERO;
	}
protected:
	/// Helper class than ensures that the probabilities stay in an acceptable range
	static T addLogProb(T a, T b) {
		T sum = a + b;
		if (sum < LOG_VALUE_ZERO) {
			return LOG_VALUE_ZERO;
		}
		else {
			return sum;
		}
	}
private:
	// The log value
	T m_logValue;
public:
	/// Constant class used for comparison and assignmetns
	static const LogProbBase<T> zero;
	/// Constant class used for comparison and assignmetns
	static const LogProbBase<T> one;
};

template <class T>
inline ostream &operator<< (ostream& out, const LogProbBase<T> &src) 
{
	return out << (double) src;   
}

typedef LogProbBase<double> LogProb;
typedef vector<LogProb> LOGPROBVECT;

#endif




