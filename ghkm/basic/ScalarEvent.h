// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef SCALAREVENT_H
#define SCALAREVENT_H

#ifdef WIN32
#pragma warning ( disable : 4541 ) 
#endif

#include "LiBE.h"
#include "MoreMaths.h"
#include "Event.h"
#include <assert.h>


using namespace std;

//! This class represents a single-featured HomEvent.  
//! Note that it is concrete.
template<class FeatureType>
class ScalarEvent : public Event
{
protected:
  FeatureType val;

public:
  typedef Event::EventTp  EventTp;

   EventTp  type() const { return SCALAR_EVENT;}

  //! Default constructor.
  ScalarEvent(){}

#if 0
  //! clone the object
  virtual Event *clone()  const {
    const ScalarEvent *newv = new ScalarEvent(*this);
    
    if(typeid(this)==typeid(newv))
       return (Event*)newv;
    else
      throw NotYetImplemented();

  };  
#endif
  
  //! Constructor from value.
  ScalarEvent(const FeatureType& v) : val(v) 
  {}


  virtual FeatureType getFeature(const unsigned int& x = 0) const {
    if(x == 0)
      return val;
    else
      throw "OutOfRange";
  }

  //! This one is not checked; it can only be used by a trusted friend.
  //! Simple base Events can't be projected, so this is pure virtual.
  Event* project(Descriptor&) const 
  { throw "cannot project a scalar";}

  virtual void setFeature(const unsigned int& x, const FeatureType& f) 
     {
    if (x == 0)
      val = f;
    else
      throw "OutOfRange";
  }
  
  virtual void setFeature(const FeatureType& f){
    val = f;
  }

  virtual FeatureType value() const{
    return val;
  }

#if 1
  //! reader.
   istream& get(istream& in){
     in >> val;
     return in;  
  }

  //! writer.
  ostream& put(ostream& out) const{
    out << val;
    return out;
  }

  //! writer.
  ostream& serialize(ostream& out) const{
    out << val;
    return out;
  }
#endif

  //! Friend istream reader.
   friend istream& operator>>(istream& in, ScalarEvent<FeatureType>& ve){
     in >> ve.val;
     return in;  
  }

  //! Friend ostream writer.
  friend ostream& operator<<(ostream& out, const ScalarEvent<FeatureType>& ve){
    out << ve.val;
    return out;
  }

  //! The default is that two events are never equal.
  virtual bool operator==(const Event& other) const{
   // if(const ScalarEvent<FeatureType>* se = 
	//static_cast<const ScalarEvent<FeatureType>*>(&other)){
	  if(other.type() == SCALAR_EVENT){
		const ScalarEvent<FeatureType>* se = static_cast<const ScalarEvent<FeatureType>*>(&other);
      return operator==(*se);
    }
    return false;
  }

  //! Test for equality.
  bool operator==(const ScalarEvent<FeatureType>& other) const{
    return(val == other.val);
  }

  bool operator<(const ScalarEvent<FeatureType>& other) const{
    return(val < other.val);
  }

  bool operator>(const ScalarEvent<FeatureType>& other) const{
    return(val > other.val);
  }

  virtual unsigned int size() const {
    return 1;
  }
  
  virtual size_t hash() const {
#ifdef WIN32
	//return hash_value(val);
	  ostringstream ost;
	  ost << val;
	  return hash_value(ost.str());
#else
    STL_HASH::hash<FeatureType> h;
    return h(val);
#endif
  }

  ScalarEvent<FeatureType>& operator[](const unsigned int&) {
    return *this;
  }

  ScalarEvent<FeatureType> operator[](const unsigned int&) const{
    return *this;
  }

};

namespace STL_HASH {
#ifdef WIN32
	template<class T>
		size_t hash_value(const ScalarEvent<T>& se) { return se.hash();}
#else 
  //! Template specialization of STL's hash object (a hash function) for 
  //! ScalarEvents.
  template<> template<class T> struct hash<ScalarEvent<T> > : hash<Event> {};
#endif
}

//! A PointerEvent is intended for use with Vocabularyularies, but can be used to
//! point to anything, especially when there is a concept of null.
class PointerEvent : public ScalarEvent<unsigned int> {
public:
  PointerEvent() : ScalarEvent<unsigned int>() {}

  PointerEvent(const unsigned int& i) : ScalarEvent<unsigned int>(i) {}

  virtual size_t hash() const {
#ifdef WIN32
	  return val;
#else
    STL_HASH::hash<unsigned int> h;
    return h(val);
#endif
  }

#if 0
  
  //! clone the object
  virtual Event *clone()  const {
    const PointerEvent*newv = new PointerEvent(*this);
    
    if(typeid(this)==typeid(newv))
       return (Event*)newv;
    else
      throw NotYetImplemented();

  };  
#endif

  const static PointerEvent null;

  //! The default is that two events are never equal.
  virtual bool operator==(const Event& other) const{
    return false;
  }

  //! To compare with others like me, use the implementer class.
  bool operator==(const PointerEvent& other) const{
    return(val == other.val);
  }

#if 0
  bool equalTo(const Event& other) const 
  {
    if(typeid(*this) != typeid(other)){
	return false;
    } else {
	const PointerEvent* t = static_cast<const PointerEvent*>(&other);
	//dynamic_cast<const PointerEvent*>(&other);
	return *this == *t;
    }
  }
#endif
  PointerEvent operator+(const unsigned int& incr) const{
    return( PointerEvent(val+incr));
  }

  //! For general one-step comparison.
  int cmp(const PointerEvent& other) const {
    return (int)val - (int)other.val;
  }

  //! To compare with others like me, use the implementer class.
  bool operator!=(const PointerEvent& other) const{
    return(val != other.val);
  }

  PointerEvent& operator[](const unsigned int&) {
    return *this;
  } 

  PointerEvent operator[](const unsigned int&) const {
    return *this;
  } 
};



namespace STL_HASH {
#ifdef WIN32
	size_t hash_value(const PointerEvent& pe);
#else 
  //! Template specialization of STL's hash object (a hash function) for 
  //! PointerEvents.
  template<> struct hash<PointerEvent> : hash<ScalarEvent<unsigned int> >
  {};
  //! To cheat the compiler, does nothing. 
  template<> struct hash<void*>
  { 
    size_t operator () (const void* other) { assert(0); return 0;}
  };
#endif
}



#endif
