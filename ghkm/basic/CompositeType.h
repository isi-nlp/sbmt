// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _CompositeType_H_
#define _CompositeType_H_

#include <iostream>

using namespace std;

//! CompositeType class composes a variable number of different types
//! into one type.
template<class T1 = void,
         class T2 = void,
	 class T3 = void,
	 class T4 = void,
	 class T5 = void,
	 class T6 = void,
	 class T7 = void,
	 class T8 = void,
	 class T9 = void,
	 class T10= void>
struct CompositeType : T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 {

  CompositeType& operator=(const CompositeType& other){
      T1::operator=(other);
      T2::operator=(other);
      T3::operator=(other);
      T4::operator=(other);
      T5::operator=(other);
      T6::operator=(other);
      T7::operator=(other);
      T8::operator=(other);
      T9::operator=(other);
      T10::operator=(other);
      return *this;
  }

  friend ostream& operator<<(ostream& o, const CompositeType& ct) {
      o<<static_cast<const T1&>(ct)<<" "
       <<static_cast<const T2&>(ct)<<" "
       <<static_cast<const T3&>(ct)<<" "
       <<static_cast<const T4&>(ct)<<" "
       <<static_cast<const T5&>(ct)<<" "
       <<static_cast<const T6&>(ct)<<" "
       <<static_cast<const T7&>(ct)<<" "
       <<static_cast<const T8&>(ct)<<" "
       <<static_cast<const T9&>(ct)<<" "
       <<static_cast<const T10&>(ct)<<endl;
      return o;
  }

  friend istream& operator>>(istream& i, CompositeType& ct) {
      i>>static_cast<T1&>(ct)
       >>static_cast<T2&>(ct)
       >>static_cast<T3&>(ct)
       >>static_cast<T4&>(ct)
       >>static_cast<T5&>(ct)
       >>static_cast<T6&>(ct)
       >>static_cast<T7&>(ct)
       >>static_cast<T8&>(ct)
       >>static_cast<T9&>(ct)
       >>static_cast<T10&>(ct);
      return i;
  }
};


// 2
template<class T1, 
         class T2>
struct CompositeType<T1, T2, void, void, void, void, void, void, void, void >
 : T1, T2    {
  CompositeType& operator=(const CompositeType& other){
    T1::operator=(other);
    T2::operator=(other);
    return *this;
  }

  friend ostream& operator<<(ostream& o, const CompositeType& ct) {
      o<<static_cast<const T1&>(ct)<<" "
       <<static_cast<const T2&>(ct)<<endl;
      return o;
  }

  friend istream& operator>>(istream& i, CompositeType& ct) {
      i>>static_cast<T1&>(ct)
       >>static_cast<T2&>(ct);
      return i;
  }
 };

// 3
template<class T1, 
         class T2,
	 class T3>
struct CompositeType<T1, T2, T3, void, void, void, void, void, void, void >
 : T1, T2, T3   {
  CompositeType& operator=(const CompositeType& other){
    T1::operator=(other);
    T2::operator=(other);
    T3::operator=(other);
    return *this;
  }

  friend ostream& operator<<(ostream& o, const CompositeType& ct) {
      o<<static_cast<const T1&>(ct)<<" "
       <<static_cast<const T2&>(ct)<<" "
       <<static_cast<const T3&>(ct)<<endl;
      return o;
  }

  friend istream& operator>>(istream& i, CompositeType& ct) {
      if(i>>static_cast<T1&>(ct)){
	   cerr<<"EEEEEEEEEE"<<static_cast<T1&>(ct)<<endl;
	   if(i>>static_cast<T2&>(ct)){
	       cerr<<"EEEEEEEEEE"<<static_cast<T2&>(ct)<<endl;
	       i>>static_cast<T3&>(ct);
		   cerr<<"EEEEEEEEEE"<<static_cast<T3&>(ct)<<endl;
	   }
      }
      return i;
  }
 };
#endif
