// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _LiBEException_H_
#define _LiBEException_H_
#include <iostream>

using namespace std;

//! This exception is convenient for testing code with stubs
class GeneralException {
public:
    GeneralException() {
	cerr << "Exception." << endl;
    }

    GeneralException(char* s) {
	cerr <<s<< endl;
    }
};

//! This exception is convenient for testing code with stubs
class NotYetImplemented {
public:
    NotYetImplemented() {
	cerr << "Not implemented exception." << endl;
    }

    NotYetImplemented(char* s) {
	cerr <<s<<" is unimplemented!"<< endl;
    }
};

//! This exception is convenient for testing code with stubs
class OutOfRange{
public:
    OutOfRange() {
	cerr << "out of range." << endl;
    }

};


#endif

