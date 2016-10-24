// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef PROJECTOR_H
#define PROJECTOR_H 1

#include "Descriptor.h"
#include "Event.h"

//! This class allows for fast projection of a collection of (homogeneous)
//! events (of type E1 to type E2).  
/*! 
  It calls upon E1's unsafe project method
  after a single check that the descriptor is of an appropriate type - that
  check happens at construction.  It contains a reference to a Descriptor,
  so that any type of Descriptor may be used; note that the Projector had 
  better not survive the Descriptor!
  */
template<class E1=void, class E2=void> 
class Projector {
private:
  Descriptor& descriptor;
  
public:

  Projector(Descriptor& d) 
    : descriptor(d){ }
  
  E2 operator()(const E1& event) const{
    E2* ptr = static_cast<E2*>(event.project(descriptor));
	//dynamic_cast<E2*>(event.project(descriptor));
    E2 ret = *ptr;
    delete ptr;
    return ret;
  }

  Descriptor& getD() const{ return descriptor;}
};


//! An specification that don't know the type of
//! event that is being projected.
template<>
class Projector<>{
private:
  Descriptor* _M_desc;

public:
  //! Constructor.
  Projector(Descriptor* d) : _M_desc(d) {}

  //! Destructor.
  ~Projector() { if(_M_desc) delete _M_desc;}

  //! Project an event. If the event pointer is non-null, then simply 
  //! call the virtual function of this event. Or else, returns null.
  Event* operator()(Event* e) { 
    // if this descriptor is null, then return null
    if(! _M_desc){
      return NULL;;
    } else if(e){
      return e->project(*_M_desc);
    } else {
      return NULL;
    }
  }
  Descriptor* getD() const{ return _M_desc;}
};


#endif


