// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef SCORESET_H
#define SCORESET_H 1

#ifdef WIN32
#pragma warning ( disable : 4290 ) 
#endif

#include "LiBE.h"
#include <iostream>
#include <string>
#include <iterator>
#include <vector>
#include <set>
#include <algorithm>
#include <utility>
#include "LiBEDefs.h"
#include "MoreMaths.h"
#include "Projector.h"
#include "Descriptor.h"
#include "LiBEDefs.h"

#define PRINT_SCORESET_ERRORS 1

using namespace std;

//! For simplicity of iteration over the hash_map.
#ifdef WIN32
#define HASHTYPE(ETYPE) typename STL_HASH::hash_map<ETYPE, SCORET >
#define HASHSETTYPE(ETYPE) typename STL_HASH::hash_set<ETYPE  >
#else 
#define HASHTYPE(ETYPE) typename STL_HASH::hash_map<ETYPE, SCORET, STL_HASH::hash<ETYPE>, equal_to<ETYPE>  >
#define HASHSETTYPE(ETYPE) typename STL_HASH::hash_set<ETYPE, STL_HASH::hash<ETYPE>, equal_to<ETYPE>  >
#endif

//! When printing a ScoreSet to an output stream, use this whitespace 
//! delimiter between entries.  This should not be the same as 
//! PAIRDELIMITER in IOFunctor.H or any delimiters used within Events!
#define SCORESETENTRYDELIMITER "\n"
#define SCORESETEVENTSCORETDELIMITER "\t"

//! This is an exception object for when a user requests a key not 
//! in the ScoreSet.
template<class K>
class KeyNotFound {
private:
  K key;  //!< Not necessarily used, but available nonetheless.
public:
  KeyNotFound(const K& k) : key(k){ 
  }
};

//! A set of mappings from unique keys to numeric values.
/*! Currently, ScoreSet is implemented using STL's \c hash_map . */
template<class E>
class ScoreSet {
protected:
  //! All of the data is stored here.
  HASHTYPE(E) theHash;

public:
  typedef HASHTYPE(E)::const_iterator const_iterator;
  typedef HASHTYPE(E)::iterator iterator;

  ScoreSet(); //!< Default constructor.
  template<class F>
  ScoreSet(const ScoreSet<F> &other, Descriptor& descriptors);

  //! Does work for the project-and-aggregate constructor.
  template<class F> 
  ScoreSet<E>& projectUnique(const ScoreSet<F>&, Descriptor&);


  // C++ gives us the following for free.
  // ScoreSet(const ScoreSet<E>&); //!< Copy constructor.
  // ScoreSet<E>& operator=(const ScoreSet<E>&); //!< Assignment operator.

  virtual ~ScoreSet(){}

  SCORET& operator[](const E&); //! subscripting operator

  //! Prints to an output stream.
  friend ostream& operator<<(ostream&out, const ScoreSet<E>&ss) {
    if(ss.empty()) {
      out << "<SCORESET>" << endl;
      out << "</SCORESET>" << endl;
    }
    out << "<SCORESET>" << endl;
    copy(ss.theHash.begin(), ss.theHash.end(), 
	 ostream_iterator<pair<E, SCORET> >(out, SCORESETENTRYDELIMITER)); 
    out << "</SCORESET>" << endl;
    return out;

  }

  //! Reads from an input stream.
  friend istream& operator>>(istream&in, ScoreSet<E>&ss) {
  ss.theHash.clear();
  E e;
  SCORET s;
  
  string st;
  in >> st;
  if(st != "<SCORESET>"){
  //if(strcmp(st, "<SCORESET>")){
    cerr << "Error:  expected <SCORESET>, got " << st << endl;
    exit(1);
  }
 
 if(in.peek() == '<'){
    in >> st;
    if(st != "</SCORESET>"){
      cerr << "Error:  expected </SCORESET>, got " << st << endl;
      exit(1);
    }
    return in;
  }
  while(in >> s >> e){
    ss.increment(e,s);
    in >> ws;
    if(in.peek() == '<'){
      in >> st;
      if(st != "</SCORESET>"){
	cerr << "Error:  expected </SCORESET>, got " << st << endl;
	exit(1);
      }
      return in;
    }
  }
  cerr << "Warning!:  unclosed <SCORESET> tag" << endl;
  return in;

  }

  //! Insert a key-value pair {E, SCORET} into theHash.
  //! Modified 7/24/02, NAS, to return the iterator.
  const_iterator insert(const E, const SCORET); 

  //! Updates the score of the contained E to be the the larger one.
  //! If E does not exist, then just insert.
  //! Added by Wei Wang 9/23/2003.
  void update(const E&, SCORET);

  /*
  //! Delete a key-value pair {E, SCORET} from theHash.
  void delete(const E); 
  */
  void erase(const E); 
  void erase(iterator it); 

  //! Increment the SCORET for the given key by the given amount.
  virtual void increment(const E, const SCORET = UNITYSCORE);
  virtual void increment(const ScoreSet<E>& other);

  //! Increment the SCORET for the given key by the given amount.
  virtual void decrement(const E, const SCORET = UNITYSCORE) throw(KeyNotFound<E>);


  //! Returns the score associated with the given E.
  virtual SCORET lookup(const E&) const throw(KeyNotFound<E>); 

  //! Return the \c HASHTYPE(E)::const_iterator (into \c theHash ) 
  //!for the given E.
  const_iterator find(const E& e) const{
    return theHash.find(e);
  }
  
  iterator find(const E& e)  {
    return theHash.find(e);
  }

  //! Allows access to the \c hash_map (its first element).
  const_iterator begin() const{ return theHash.begin(); }
  iterator begin() { return theHash.begin(); }

  //! Allows access to the \c hash_map (its last element).
  const_iterator end() const{ return theHash.end(); }
  iterator end() { return theHash.end(); }

  //! Returns \c true iff the ScoreSet contains no scored E objects.
  bool empty() const{ return theHash.empty(); }

  //! Clears the ScoreSet (making it empty).
  void clear(){ theHash.clear(); }

  //! Returns a new ScoreSet such that the scores are the logs of 
  //! this ScoreSet's scores.
  ScoreSet<E> logs() const;

  //! Returns a new ScoreSet such that the scores are the 
  //! exponential function's value of this ScoreSet's scores.
  ScoreSet<E> exps() const;

  //! Destructively create a mixed ScoreSet from a vector of 
  //! ScoreSets with the corresponding coefficients.
  void mixture(const vector<ScoreSet<E>* >&, const vector<SCORET>&, 
	       const SCORET& = ZEROSCORET);
  
  //! Returns the size of the ScoreSet (number of keys).
  int size() const{ return (int)theHash.size(); }

  //! Compute difference in mass between two ScoreSets.
  SCORET massShift(const ScoreSet<E>&) const;

  //! Return the sum of all counts.  Formerly in Histogram.
  SCORET sum() const; 

  //! Read the content from stream based prototypes of type E.
  istream& read(istream& in, E*);
  
};

/*======================================================================*/

/* Project-unique constructor. */
template<class E> template<class F>
ScoreSet<E>::ScoreSet(const ScoreSet<F> &other, 
			Descriptor& descriptors) 
{
    projectUnique(other, descriptors);
}

/* Does projection and aggregation; converts each F in other
   to an E, and aggregates all the Es into this object. */
template<class E> template<class F>
ScoreSet<E>& ScoreSet<E>::projectUnique(const ScoreSet<F> &other, 
					  Descriptor& descriptors){
  theHash.clear();
  HASHTYPE(F)::const_iterator i;
  Projector<F, E> projector(descriptors);
  for(i = other.begin(); i != other.end(); i++)
    increment(projector(i->first), i->second);
  return *this;
}




/* Insert a key-value pair (Event-Score) into theHash. */
template<class E>
typename ScoreSet<E>::const_iterator ScoreSet<E>::insert(const E key, const SCORET value){
  return theHash.insert(make_pair(key, value)).first;
}

//! Updates the score of the contained E to be the the larger one.
//! Added by Wei Wang 9/23/2003.
template<class E>
void ScoreSet<E> :: update(const E& e, SCORET score)
{
  iterator it = find(e);
  if(it != end()){
    score = (it->second > score) ? it->second : score;
  }

  (*this)[e] = score;
}

/* Delete a key-value pair (Event-Score) from theHash. */
template<class E>
void ScoreSet<E>::erase(E key)
{
    theHash.erase(key);
}

template<class E>
void ScoreSet<E>::erase(iterator it)
{
    theHash.erase(it);
}
/* Before this is uncommented, it must be overridden in subclasses to
   do sensible things
template<class E>
void ScoreSet<E>::delete(E key){
  theHash.erase(key);
}
*/

/* Increment the value for the given key by the given amount. */
template<class E>
void ScoreSet<E>::increment(const E key, const SCORET inc){
  if(theHash.find(key) != theHash.end())
    theHash[key] += inc;
  else
    insert(key, inc);
}

template<class E>
void ScoreSet<E>::increment(const ScoreSet<E>& other)
{
    const_iterator it ;
    for(it = other.begin(); it != other.end(); ++it){
	increment(it->first, it->second);
    }
}

/* Increment the value for the given key by the given amount. */
template<class E>
void ScoreSet<E>::decrement(const E key, const SCORET inc)throw(KeyNotFound<E>){
  if(theHash.find(key) == theHash.end())
    throw KeyNotFound<E>(key);
  theHash[key] -= inc;
}


/* Default constructor. */
template<class E>
ScoreSet<E>::ScoreSet(){}


/* Subscripting operator. */
template<class E>
SCORET& ScoreSet<E>::operator[](const E& sub){
  return theHash[sub];
}


/* Needed for the above. */
template<class E>
ostream& operator<<(ostream& out, const pair<E, SCORET>& p){
  out << p.second << SCORESETEVENTSCORETDELIMITER << p.first;
  return out;
}


//! Read the content from stream based object prototypes.
template<class E>
istream& ScoreSet<E>::read(istream& in, E*e)
{

  theHash.clear();
  SCORET s;
  
  string st;
  in >> st;
  if(st != "<SCORESET>"){
  //if(strcmp(st, "<SCORESET>")){
    cerr << "Error:  expected <SCORESET>, got " << st << endl;
    exit(1);
  }
  if(in.peek() == '\n'){
    getline(in, st, '\n');
  }
  if(in.peek() == '<'){
    in >> st;
    if(st != "</SCORESET>"){
      cerr << "Error:  expected </SCORESET>, got " << st << endl;
      exit(1);
    }
    return in;
  }
  while(in >> s >> *e){
    //ss.insert(e, s);
    increment(*e,s);
    in >> ws;
    if(in.peek() == '<'){
      in >> st;
      if(st != "</SCORESET>"){
	cerr << "Error:  expected </SCORESET>, got " << st << endl;
	exit(1);
      }
      return in;
    }
  }
  cerr << "Warning!!:  unclosed <SCORESET> tag" << endl;
  return in;
}

/*! Elements in the hash are to be compared by the SCORET, not the E,
 * so < is explicitly defined for pairs where the second member is a
 * SCORET.  This overrides the default!  */
template<class E>
bool operator<(const pair<E, SCORET>& p1, const pair<E, SCORET>& p2){
  return(p1.second < p2.second);
}

/*! Elements in the hash are to be compared by the SCORET, not the E,
 * so > is explicitly defined for pairs where the second member is a
 * SCORET.  This overrides the default! */
template<class E>
bool operator>(const pair<E, SCORET>& p1, const pair<E, SCORET>& p2){
  return(p1.second > p2.second);
}

/*! It is useful to be able to sum elements in the hash by their
  SCORETs, ignoring the key.  This function returns a SCORET (the sum of
  the first argument and the second half of the second argument). */
template<class E>
SCORET operator+(const SCORET& s, const pair<E, SCORET>& p){
  return (s + p.second);
}
/*! It is useful to be able to sum elements in the hash by their
  SCORETs, ignoring the key.  This function returns a pair with its
  SCORET set to the sum of \c p 's SCORET and \s . */
template<class E>
pair<E, SCORET> operator+(const pair<E, SCORET>& p, const SCORET& s){
  return make_pair(p.first, (s + p.second));
}
/*! It is useful to be able to multiply elements in the hash by their
  SCORETs, ignoring the key.  This function returns a SCORET (the
  product of the first argument and the second half of \c p ). */
template<class E>
SCORET operator*(const SCORET& s, const pair<E, SCORET>& p){
  return (s * p.second);
}

/*! It is useful to be able to multiply elements in the hash by their
  SCORETs, ignoring the key.  This function returns a pair with its
  SCORET set to the product of \c p 's SCORET and \c s .*/
template<class E>
pair<E, SCORET> operator*(const pair<E, SCORET>& p, const SCORET& s){
  return make_pair(p.first, (s * p.second));
}


/* Returns the score associated with the given E.  If E is not found,
   returns ZEROSCORET.  This has some bad implications and should be
   fixed in future versions; see the technical report for
   discussion. */
template<class E>
SCORET ScoreSet<E>::lookup(const E& key) const throw(KeyNotFound<E>){
  typename ScoreSet<E>::const_iterator i;
  if((i = theHash.find(key)) == theHash.end()) throw KeyNotFound<E>(key);
  else return i->second;
}



#endif

