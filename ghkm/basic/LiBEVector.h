// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/** LiBEVector.H
*   Inherited from the stl::vector to provide the hash and serialization
*   method.
*/
#ifndef _LiBEVector_H_
#define _LiBEVector_H_
#include <vector>
#include <iostream>
#include <iterator>
#include "LiBE.h"

using namespace std;

/** LiBEVector is inherited from the stl::vector, and is intended to
*   provide the hash function and the serialization method.
*/
template<class T>
class LiBEVector: public vector<T>
{
    typedef vector<T> _base;
public:
    typedef typename vector<T>::const_iterator const_iterator;
    typedef typename vector<T>::iterator iterator;

    /** Constructor. */
    LiBEVector()  : _base() {}
    LiBEVector(size_t size)  : _base(size) {}

    /** Reads a LiBEVector from one line of the input stream.  */
#ifdef WIN32
    friend istream& operator>>(istream& in, LiBEVector<T>& v)
 {
    v.clear();
    string buffer;
    getline(in, buffer,'\n');
    istringstream ist(buffer);
    copy(istream_iterator<T>(ist), istream_iterator<T>(), back_inserter(v)); 
    return in;
}
#else
	friend istream& operator>>(istream& in, LiBEVector<T>& v) {
    v.clear();
    string buffer;
    getline(in, buffer,'\n');
    istringstream ist(buffer);
    copy(istream_iterator<T>(ist), istream_iterator<T>(), back_inserter(v)); 
    
    return in;
	}
#endif

    /** 
    *   Outputs a LiBEVector into one line. Fields are separated by
    *   space.
    */
    friend ostream& operator<<(ostream& out, const LiBEVector<T>& v)
    {
	typename LiBEVector<T>::const_iterator i;
	for(i = v.begin(); i != v.end(); i++) {
	    out << *i << ' ';
	}
	out<<endl;
	return out;
    }
};

#ifdef WIN32
#else
#endif

#if 0
 namespace std{

	 // the less functor.
	 template<class T>  less<LiBEVector<T> > {
		 bool operator(const LiBEVector<T>& v1, const LiBEVector<T>& v2) const
		 { return v1 < v2;}
	 };
 }
#endif

namespace STL_HASH{
    /** The hash function for LiBEVector. */
// for WIN32
#ifndef WIN32
    template<> 
    template<class T> 
    struct hash<LiBEVector<T> > 
    {
	size_t operator()(const LiBEVector<T>& v) {
	    ostringstream ost;
	    copy(v.begin(), v.end(), ostream_iterator<T>(ost, ","));
	    STL_HASH::hash<char*> h;
	    return h(ost.str().c_str());
	}
    };
#else 
// for GNU.
	template<class T>
		size_t hash_value(const LiBEVector<T>& v) {
			ostringstream ost;
	        copy(v.begin(), v.end(), ostream_iterator<T>(ost, ","));
	        STL_HASH::hash<char*> h;
	        return hash_value(ost.str().c_str());
		}
#endif
}  

#endif

