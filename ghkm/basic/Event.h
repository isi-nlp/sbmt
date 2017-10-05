// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef EVENT_H
#define EVENT_H 1

#ifdef WIN32
#pragma warning ( disable : 4541 ) 
#endif

#include "LiBE.h"
#include "Descriptor.h"
#include <iostream>
#include <typeinfo>
#include <string>
#include <deque>
#include "MoreMaths.h"
#include "LiBEException.h"


#define PRINT_EVENT_ERRORS 1

using namespace std;
using namespace STL_HASH;

//! A useful exception class.
class NoProject {
public:
  NoProject(){
#ifdef PRINT_EVENT_ERRORS
    cerr << "Error:  Can't project ScalarEvents." << endl;
#endif
  }

  NoProject(string str) { cerr<<str<<endl; }
};



//! An abstract object signifying an event.
class Event {
public:
    enum EventTp { BAD_EVENT, SCALAR_EVENT, PAIR_EVENT};


    virtual EventTp  type() const { return BAD_EVENT;}
#if 0
  //! clones the object
  virtual Event *clone() const = 0;
#endif

  virtual ostream& put(ostream&) const{
    throw NotYetImplemented();
  }

  virtual istream& get(istream&) {
    throw NotYetImplemented();
  }


  //! This one is not checked; it can only be used by a trusted friend.
  //! Simple base Events can't be projected, so this is pure virtual.
  virtual Event* project(Descriptor&) const  = 0;

  //! Need to be able to hash any kind of event for it to be stored in a ScoreSet.
  virtual size_t hash() const = 0;

  //! The default is that two events are never equal; subclasses can redefine this.
  virtual bool operator==(const Event& other) const{
    return false;
  }


  virtual bool equalTo(const Event& other) const{
	  throw NotYetImplemented("Event::equalTo");
  }

  virtual ~Event(){}

  //! Serialize the content of the event. The difference between
  //! serialize and put is that, the former outputs the event
  //! content using pure integer representation, but the latter
  //! might use some non-integer characters.
  virtual ostream& serialize(ostream&) const
  { throw NotYetImplemented("Event::serialize"); }

  friend ostream& operator<<(ostream& out, Event*){
    throw NotYetImplemented();
  }

  friend istream& operator>>(istream& in, Event*){
    throw NotYetImplemented();
  }


};


namespace STL_HASH {
#ifndef WIN32
  //! Template specialization of STL's hash object (a hash function) 
  //! for Events.  
  template<> struct hash<Event> {
    size_t operator()(const Event& e) const{
      return e.hash();
    }
  };
#endif
}
#endif







