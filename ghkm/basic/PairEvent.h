// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/*! \file PairEvent.H
 * \brief Definition of class PairEvent.
 */

#ifndef PAIREVENT_H
#define PAIREVENT_H 1

#include "LiBE.h"
#include <utility>
#include "Event.h"
#include "ScalarEvent.h"
#include "Projector.h"
#include "MoreMaths.h"

#define PAIREVENTDELIM " "
#define DEFAULTBIGVAL 99999999

using namespace std;

//! This class represents a concrete pair-valued Event, 
//! where the two element of the pair need not be of the same type.
//! U and V must both be Events.
template<class U, class V = U>
class PairEvent : public pair<U, V>, public virtual Event{

public:
    typedef Event::EventTp  EventTp;

    EventTp type() const {return PAIR_EVENT;}

  //! Default constructor.
  PairEvent(){}

  //! Constructor.
  PairEvent(const pair<U,V>& pa) : pair<U,V>(pa) {}

  //! Constructor.
  PairEvent(const U& u, const V& v) : pair<U, V>(u, v){}  
#if 0
  //! clone the object
  virtual Event *clone() const{
    const PairEvent<U,V> *newv = new PairEvent<U,V>(*this);
    if(typeid(this)==typeid(newv)) return (Event*)newv;
    else throw NotYetImplemented();
  };  

#endif
  //! Unsafe projector.  Assumes Descriptor is of the right type!
  virtual Event* project(Descriptor& D) const {
	  // if(ScalarDescriptor * s = dynamic_cast<ScalarDescriptor *>(&D)){
	  if(D.type() == Descriptor::SCALAR_DESC){
		ScalarDescriptor * s = static_cast<ScalarDescriptor *>(&D);
        
      switch (s->value()) {
      case 0: 
	return (Event *) new U(this->first);
      case 1:
	return (Event *) new V(this->second);
      default:
	cerr << "default" << endl;
	cerr << s->value() << " is out of range" << endl;
	throw "out of range"; 
      }
    } 
    else{
      cerr << "Bad descriptor in Pair Event" << endl;
      //string s2 = typeid(D).name();
      //cerr << s2 << endl;

      throw "bad descriptor";
    }
    
  }
  
  //! A hashing function.
  virtual size_t hash() const{
#ifdef WIN32
	return (size_t) (hash_value(this->first) * getBigValForHash()) + hash_value(this->second);
#else
    STL_HASH::hash<U> u;
    STL_HASH::hash<V> v;
    return (size_t) (u(this->first) * getBigValForHash()) + v(this->second);
#endif
  }

  //! So each class can have its own static member that it accesses (or some
  //! other way to compute this), we have a virtual method.
  virtual size_t getBigValForHash() const { 
#ifdef PEDANTIC_WARNINGS
    cerr << "Warning:  using PairEvent's getBigValForHash" << endl; 
#endif
    return DEFAULTBIGVAL;
  }

  //! Output
  friend ostream& operator<<(ostream& out, const PairEvent<U, V>& pe){
    out << pe.first << PAIREVENTDELIM << pe.second;
    return out;
  }

  ostream& put(ostream& out) const{ out<<*this; return out; }

  //! Input.
  friend istream& operator>>(istream& in, PairEvent<U, V>& pe){
    in >> pe.first >> pe.second;
    return in;
  }
  istream& get(istream& in) { in>>*this; return in; }

  //! The default is that two events are never equal.
  virtual bool operator==(const Event& other) const{
    //cerr << "PROBLEM" << endl;
    return false;
  }

  //! To compare with others like me, use the implementer class.
  bool operator==(const PairEvent<U, V>& other) const{
    //    cerr << "comparing " << *this << " to " << other << " : " <<
    //  (first == other.first && second == other.second ? "equal" : "NOT equal") << endl;
    return(this->first == other.first && this->second == other.second);
  }

  //! To compare for ordering, compare the left thing first, then the right.
  bool operator<(const PairEvent<U, V>& other) const{
    if(this->first < other.first){
      return true;
    }
    else if(this->first > other.first){
      return false;
    }
    else return(this->second < other.second);

  }

  //! To compare for ordering, compare the left thing first, then the right.
  bool operator>(const PairEvent<U, V>& other) const{
    return (other < *this);
  }

  ostream& serialize(ostream& out) const{   
        out << this->first << PAIREVENTDELIM << this->second << PAIREVENTDELIM;
    return out; 
  }


  virtual ~PairEvent(){}
  
  typedef U FirstType;
  typedef V SecondType;

};


namespace STL_HASH {
#ifdef WIN32 
	template<class U, class V>
	size_t hash_value(const PairEvent<U, V>& pe) { return pe.hash();}
#else 
  //! Template specialization of STL's hash object (a hash function) 
  //! for PairEvents.
  template<> template<class U, class V> struct hash<PairEvent<U, V>  > : hash<Event> {};
#endif
}

#endif


 
