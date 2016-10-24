// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _LiBEFunctions_H_
#define _LiBEFunctions_H_

#include <functional>
#include <string>
#include "Lcsr.h"

using namespace std;

namespace LiBE {

template<class ArgTp, class ResultTp>
class UnaryFunction : public unary_function<ArgTp, ResultTp>
{
public:
    virtual ResultTp operator()(ArgTp&arg) const = 0;

};

template<class ArgTp>
class Predicate 
{
public:
    virtual bool operator()(const ArgTp& arg) const = 0;
};

template<class _Arg1, class _Arg2>
class BinaryPredicate : public binary_function<_Arg1, _Arg2, bool>
{
public:
    virtual bool operator()(const _Arg1& arg1, const _Arg2& arg2) const = 0;
};

class StringApproxEqual : public BinaryPredicate<string, string>
{
public:
    bool operator()(const string& w, const string& v) const {
	Lcsr lcsr;
	SCORET ratio = lcsr(w, v, true);
	if(ratio >= 0.85) { return true;}
	else {return false;}
    }
};

};


#endif

