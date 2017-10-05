// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/** LiBEPair.H
*   Inherited from the stl::pair to provide the hash and serialization
*   method.
*/
#ifndef _LiBEPair_H_
#define _LiBEPair_H_
#include "LiBE.h"
#include <iostream>
#include "LiBE.h"

#ifdef WIN32
#pragma warning ( disable : 4267 )
#endif

using namespace std;

/** LiBEPair is inherited from the stl::pair, and is intended to
*   provide the hash function and the serialization method.
*/
template<class T1, class T2>
class LiBEPair: public pair<T1, T2>
{
    typedef pair<T1, T2> _base;

public:
    /** Empty constructor.*/
    LiBEPair() {}

    /** Constructor.*/
    LiBEPair(const T1& p1, const T2& p2) : _base(p1, p2) {}

    bool operator==(const LiBEPair<T1, T2>& other) const 
    {
        if(this->first == other.first && this->second == other.second){
            return true;
	} else {
          return false;
        }
    }

	//! To compare for ordering, compare the left thing first, then the right.
  bool operator<(const LiBEPair<T1, T2>& other) const{
    if(this->first < other.first){
      return true;
    }
    else if(this->first > other.first){
      return false;
    }
    else return(this->second < other.second);

  }

    
    /** Reads a LiBEPair from one line of the input stream.  */
    friend istream& operator>>(istream& in, LiBEPair<T1, T2>& p){
	in>>p.first>>p.second;
	return in;
    }

    /** 
    *   Outputs a LiBEPair into one line. Fields are separated by
    *   space.
    */
    friend ostream& operator<<(ostream& out, const LiBEPair<T1, T2>& p)
    {
	out<<p.first<<" "<<p.second;
	return out;
    }
};


template<class T>
class Link : public LiBEPair<T, T>
{
    typedef LiBEPair<T, T> _base;

public:
    Link() {}
    Link(const T& p1, const T& p2) :  _base(p1, p2) {}

    /** Returns the size. */
    size_t size() const { return (size_t)2;}

    /** Random access operator. */
    T& operator[](size_t i )  { if(!i) {return this->first;} 
	else {return this->second;}}
    T operator[](size_t i )const{ 
	if(!i){ return this->first;} else {return this->second;}}
};


namespace STL_HASH{

#ifndef WIN32

    /** The hash function for LiBEPair. */
    template<> 
    template<class T1, class T2> 
    struct hash<LiBEPair<T1, T2> > 
    {
	size_t operator()(const LiBEPair<T1, T2>& p) const {
	    hash<T1> h1;
	    hash<T2> h2;

	    return h1(p.first) * 1000 + h2(p.second);
	}
    };
    /** The hash function for Link. */
    template<> 
    template<class T> 
    struct hash<Link<T> > : LiBEPair<T, T> {};

#else
    /** The hash function for LiBEPair. */
    template<class T1, class T2> 
    size_t hash_value(const LiBEPair<T1, T2>& p)  {
	    return hash_value(p.first) * 1000 + hash_value(p.second);
	}
   
    /** The hash function for Link. */
   // template<class T> 
	//	size_t hash_value(const Link<T>& l) 
    
#endif

}  

#endif

