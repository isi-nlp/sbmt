// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/*! \file MoreMath.H
 * \brief some common math functions
 */

#ifndef MOREMATHS_H
#define MOREMATHS_H 1

#include "LiBE.h"
#include <functional>
#include <math.h>
#include "LiBEDefs.h"
#include "LiBEException.h"

using namespace std;

//! Functor for taking the log of x with some given base.
template<class T>
class Log : public unary_function<T, T> {
private:
  const T denom;
public:
  //constructor
  Log(const T& logbase) : denom(log(logbase)) {} ;

  const T operator()(const T& x){ return (T)log(x) / denom; }
};

//! Functor for taking the exponential function of x with some given base.
template<class T>
class Exp : public unary_function<T, T> {
private:
  const T base;
public:
  //constructor
  Exp(const T& b) : base(b) {} ;

  const T operator()(const T& x){ return (T)pow(base, x); }
};

////! Shorthand for XOR operation.
//bool XOR(const bool& x, const bool& y) {
//  return (x != y);
//}

/* This is a redundant version of the function below
//! See Manning & Schuetze p.337.
double logAdd(double x, double y){
  static double LOGBIG = log(pow(10.0, 30.0)); // around log(10^30)
  double z;
  if(y - x > LOGBIG) return y;
  else if(x - y > LOGBIG) return x;
  else{
    z = min(x, y);
    return z + log(exp(x - z) + exp(y - z));
  }
}
*/

//! Compute the sum of two logs
template<class T>
T logAdd(const T& x, const T& y){
  const SCORET negInf = (SCORET)log(ZEROSCORE);
  if(y == negInf || x - y > (T)20.0)
    return x;
  if(x == negInf || y - x > (T)20.0)
    return y;
  Exp<T> expfun((SCORET)LOGBASE);
  Log<T> logfun((SCORET)LOGBASE);
  if(x > y)
    return logfun((T)1.0 + expfun(y - x)) + x;
  else
    return logfun((T)1.0 + expfun(x - y)) + y;
}

//! Compute the sum of two logs
template<class T>
class LogAdd {
public:
    T operator()(const T& x, const T& y){
	  const SCORET negInf = (SCORET)log(ZEROSCORE);
	  if(y == negInf || x - y > (T)20.0) return x;
	  if(x == negInf || y - x > (T)20.0) return y;
	  Exp<T> expfun((SCORET)LOGBASE);
	  Log<T> logfun((SCORET)LOGBASE);
	  if(x > y) return logfun((T)1.0 + expfun(y - x)) + x;
	  else return logfun((T)1.0 + expfun(x - y)) + y;
    }
};
//! Subtract one log from another.
template<class T>
T logSub(const T& x, const T& y){
  const SCORET negInf = log(ZEROSCORE);
  if(y == negInf || x - y > (T)20.0)
    return x;
  if(x == negInf || y - x > (T)20.0)
    throw OutOfRange();
  Exp<T> expfun(LOGBASE);
  Log<T> logfun(LOGBASE);
  if(x > y)
    return logfun((T)1.0 - expfun(y - x)) + x;
  else
    throw OutOfRange();
}

//! log of ratio of two factorials
//! computes log( x! / y! ), where y <= x
//! = log(x) + log(x-1) + ... + log(x-y+1)
template<class T>
double logFactRatio(const unsigned int x, const unsigned int y){
  Log<T> logfun(LOGBASE);
  double lfr = 0;
  for(unsigned int i = x; i > y; --i) {
    lfr += logfun(i);
  };
  return lfr;
}

#ifdef max
#	undef max
#endif
#ifdef min
#	undef min
#endif


//! Three-way max.
template<class T>
T max(const T& a, const T& b, const T& c){
  return max(max(a, b), c);
}

//! Three-way min.
template<class T>
T min(const T& a, const T& b, const T& c){
  return min(min(a, b), c);
}

#endif
