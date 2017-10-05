// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/*! \file Histogram.H
 * \brief Definition of class Histogram.
 */

#ifndef HISTOGRAM_H
#define HISTOGRAM_H 1

#include "LiBE.h"
#include <algorithm>
#include <numeric>
#include "ScoreSet.h"
#include "Projector.h"

using namespace std;

// #define PRINT_GSQUARED_INFO 1

//! A ScoreSet with no negative scores.
/*! 
  Lots of extra functionality is added, like aggregation and
  accumulation from an ObservationSequence (or any iterate-able
  object). */
template<class E> class Histogram : public ScoreSet<E> {

public:
  typedef typename ScoreSet<E>::const_iterator const_iterator;
  typedef typename ScoreSet<E>::iterator iterator;


  //! Allows access to the \c hash_map (its first element).
  const_iterator begin() const{ return ScoreSet<E>::begin(); }
  iterator begin() { return ScoreSet<E>::begin(); }

  //! Allows access to the \c hash_map (its last element).
  const_iterator end() const{ return ScoreSet<E>::end(); }
  iterator end() { return ScoreSet<E>::end(); }


  //! Default constructor.
  Histogram() {}

  //! Copy constructor.
  Histogram(const Histogram<E>& orig) : ScoreSet<E>(orig) {} 

  //! Virtual destructor needed due to virtual inheritance.
  virtual ~Histogram(){} 

  //! Projection-and-aggregation constructor.
  /*! Project from the argument Histogram; this takes each F object and
    projects it using a Projector built from the Descriptor; this creates
    (presumably) an E object.  Each E object value is aggregated in the new
    Histogram. */
  template<class F> 
  Histogram<E>(const Histogram<F>&, Descriptor&);

  //! Does work for the project-and-aggregate constructor.
  template<class F> 
  Histogram<E>& projectUnique(const Histogram<F>&, Descriptor&);

  //! Assignment operator.
  Histogram<E>& operator=(const Histogram<E>&); 

  //! Accumulate into the Histogram, iteratively, 
  //! using a given start and finish.
  template<class I> void accumulate(I, I, const SCORET& = UNITYSCORE);

};

/* Sum all elements of the Histogram. */
// moved to ScoreSet


/* Project-unique constructor. */
template<class E> template<class F>
Histogram<E>::Histogram(const Histogram<F> &other, 
			Descriptor& descriptors) 
  : ScoreSet<E>(){
    projectUnique(other, descriptors);
}

/* Does projection and aggregation; converts each F in other
   to an E, and aggregates all the Es into this object. */
template<class E> template<class F>
Histogram<E>& Histogram<E>::projectUnique(const Histogram<F> &other, 
					  Descriptor& descriptors){
  this->theHash.clear();
  HASHTYPE(F)::const_iterator i;
  Projector<F, E> projector(descriptors);
  for(i = other.begin(); i != other.end(); i++)
    increment(projector(i->first), i->second);
  return *this;
}

/* Assignment operator. */
template<class E>
Histogram<E>& Histogram<E>::operator=(const Histogram<E>& orig){
  ScoreSet<E>::operator=(orig);
  return *this;
}


/*! Without clearing the Histogram, the values of the E types in the 
  iterate-able object (from \c start to \c finish) are incremented 
  for each time they are seen while iterating.  \c factor is a value 
  that indicates how much each E should have its score incremented 
  in the Histogram. */
template<class E> template<class I>
void Histogram<E>::accumulate(I start, I finish, const SCORET& factor){ 
  I i;
  for(i = start; i != finish; i++)
    increment((*i), factor);
}




#endif
