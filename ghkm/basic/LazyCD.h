// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef LAZYCD_H
#define LAZYCD_H

#ifdef WIN32
#pragma warning ( disable : 4541 ) 
#endif

#include "Histogram.h"
#include "ScalarDescriptor.h"


template<class Num, class Den, class Left> class LazyCD {

protected:

  typedef Den DenTp;
  typedef Left LeftTp;

  //! Histogram of Nums
  Histogram<Num> numerators;
  
  //! Histogram of Dens
  Histogram<Den> denominators;

  //! Descriptor that tells how to get from Num to Den
  Descriptor * numToDenDescriptor;

  //! Descriptor that tells how to get from Num to Left
  Descriptor * leftDescriptor;

   
public: 
  typedef Num NumTp;

  //! Iterator over the events in the Num.
  typedef typename Histogram<Num>::iterator iterator;
  typedef typename Histogram<Num>::const_iterator const_iterator;

  //!Default Constructor.  Never used.
  LazyCD<Num,Den,Left>(); 

  //! Constructor for when starting empty.
  LazyCD<Num, Den, Left>(Descriptor *, Descriptor *);

  //! Constructor for LazyCD.  Descriptor describes how to find Den from Num.
  LazyCD<Num,Den,Left>(Histogram<Num>&, Descriptor*, Descriptor *);

  //!Returns numerators
  const Histogram<Num> *getNum() const;

  //!Returns denominators
  const Histogram<Den> *getDen() const;

  iterator begin() { return numerators.begin(); }
  const iterator begin() const { return numerators.begin(); }
  iterator end() { return numerators.end(); }
  const iterator end() const { return numerators.end(); }

  //!Returns numToDenDescriptor
  Descriptor* getD() const;

  //!Returns   num to left descriptor. 
  Descriptor* getDL() const { return leftDescriptor;}

  //! Increments counts in numerators and appropriate denominators
  virtual void increment(const Num&, const SCORET);
  
  //! Creates the content based on a Histogram.
  //! Note that the descriptors should already been initialized.
  void create(const Histogram<Num>& hist);

  //! Returns value in numerators over appropriate value in denominators.
  virtual SCORET lookup(const Num&) const;
  
  //! Returns the log of numerators val minus the log of denominators val.
  virtual SCORET lookupLog(const Num&) const;

  //! Returns the log of a denominator's count.
  virtual SCORET lookupLogDen(const Den&) const;

  //! Returns a denominator's count.
  virtual SCORET lookupDen(const Den&) const;

  //! Returns the log of a numerator's count.
  virtual SCORET lookupLogNum(const Num&) const;

  //! Returns a numerator's count.
  virtual SCORET lookupNum(const Num&) const;

  //! Returns true if the histogram is Empty.
  bool isEmpty();

  //!Removes all elements from the Histogram.
  void clear();

  //! Merges two LazyCD together.
  LazyCD<Num, Den, Left>& operator+=(const LazyCD<Num, Den, Left>& other);
  
  //! Prints to an output stream.
    friend ostream& operator<<(ostream& out, const LazyCD<Num,Den,Left>& lcd) {
      out << lcd.numerators;
      return out;
  }

  //! Prints to an output stream, but includes denominators, 
  //! so they don't have to be recalculated on input.  Useful for 
  //! reading in very large structures..
    friend ostream& operator<(ostream& out, const LazyCD<Num,Den,Left>& lcd) {
      out << lcd.numerators;
      out << lcd.denominators;
      return out;
  }

  //! Reads from an input stream.
  friend istream& operator>>(istream& in, LazyCD<Num,Den,Left>& lcd) {
  string s; 

  lcd.numerators.clear();
  
  in >> lcd.numerators;
  
  lcd.denominators.clear();
  lcd.denominators.projectUnique(lcd.numerators,*lcd.numToDenDescriptor);

  return in;

  }
  
  //! Reads from an input stream, but expects both Numerators 
  //! and Denominators.  Works in conjunction with < operator.
friend istream& operator>(istream& in, LazyCD<Num,Den,Left>& lcd) {
  string s; 

  lcd.numerators.clear();
  in >> lcd.numerators;
  
  lcd.denominators.clear();
  in >> lcd.denominators;
  
  return in;
  
}

  //! Returns a LazyCD which was made from the input and the Descriptors.
  template <class N,class D, class L>
  LazyCD<Num,Den,Left>& projectUnique(LazyCD<N,D,L>&, Descriptor&, Descriptor& );

  //!Destructor
  virtual ~LazyCD() {}

  //! Random event according to the conditional distribution and a Den.
  Left *generateRandom(const Den& d, SCORET& score) const;
  
  //! Random event according to the backoff distribution.  Here, causes program to die.
  virtual Left *generateRandom_BO(SCORET& score) const;
  
  //! Returns the event with the highest score.
  Num maxEvent() const;

  //! Returns the event with the lowest score.
  Num minEvent() const;

  //! Returns the maximum score over all the events.
  SCORET maxScore() const;

  //! Returns the minimum score over all the events.
  SCORET minScore() const;

  
  //! Compute the entropy of this distribution.
  SCORET entropy() const;

}; 

/*Default Constructor.  Never used.*/
template<class Num, class Den, class Left>
LazyCD<Num,Den,Left>::LazyCD(){

}  



/* Constructor for LazyCD.  Descriptor describes how to find Den from Num.*/
template<class Num, class Den, class Left>
LazyCD<Num,Den,Left>::LazyCD(Histogram<Num>& hist, Descriptor *desc, Descriptor *ldesc): denominators(hist,*desc){
  numToDenDescriptor = desc;
  leftDescriptor = ldesc;
  numerators = hist;
 
}  

//! Creates the content based on a Histogram.
//! Note that the descriptors should already been initialized.
template<class Num, class Den, class Left>
void LazyCD<Num,Den,Left>:: create(const Histogram<Num>& hist)
{
    clear();
    numerators = hist;
    denominators.projectUnique(hist, *numToDenDescriptor); 
}


/*Returns numerators*/
template<class Num, class Den, class Left>
const Histogram<Num> *LazyCD<Num,Den,Left>::getNum() const{
  return &numerators;
}

/*Returns denominators*/
template<class Num, class Den, class Left>
const Histogram<Den> *LazyCD<Num,Den,Left>::getDen() const{
  return &denominators;
}

/*Returns denominators*/
template<class Num, class Den, class Left>
Descriptor* LazyCD<Num,Den,Left>::getD() const{
  return numToDenDescriptor;
}


/* Increment the SCORET for the given key by the given amount,
  as well as the appropriate denominator.*/

template<class Num, class Den, class Left>
void LazyCD<Num,Den,Left>::increment(const Num& key, const SCORET value){
    
  numerators.increment(key,value);
    
  //find a new Den from Num using descriptor d.

  //Den* key2 = dynamic_cast<Den*>(key.project(*numToDenDescriptor));
  Den* key2 = static_cast<Den*>(key.project(*numToDenDescriptor));

  //increment appropriate denominator value.
  denominators.increment(*key2,value);
  
  delete key2;
}

/* returns value in numerators matching Num over appropriate value in denominators. */
template<class Num, class Den, class Left>
SCORET LazyCD<Num,Den,Left>::lookup(const Num& key) const{
  
  HASHTYPE(Num)::const_iterator i;
  SCORET score1 = ZEROSCORET, score2 = ZEROSCORET;
  if((i = numerators.find(key)) != numerators.end())
    score1 = i->second;
  else return ZEROSCORET;

  //find a new Den from Num using descriptor d.
 
  //Den* key2 = dynamic_cast<Den*>(key.project(*numToDenDescriptor));
  Den* key2 = static_cast<Den*>(key.project(*numToDenDescriptor));
  
  HASHTYPE(Den)::const_iterator j;
  if((j = denominators.find(*key2)) != denominators.end())
    score2 = j->second;
  else{
    delete key2; 
        cerr << "RHS of conditional item, " << *key2 << " (" << key << ") not found in LazyCD!" << endl;
    return ZEROSCORET;
  }
  delete key2;

  if(score1 == ZEROSCORET){
      return score1;
  } else {
      return score1/score2;
  }
}

/* returns the log of numerators val minus the log of denominators val. */
template<class Num, class Den, class Left>
SCORET LazyCD<Num,Den,Left>::lookupLog(const Num&key) const {

  HASHTYPE(Num)::const_iterator i;
  SCORET score1 = ZEROSCORET, score2 = ZEROSCORET;
  if((i = numerators.find(key)) != numerators.end())
    score1 = log(i->second);
  else return (SCORET)log(ZEROSCORET);

  //find a new Den from Num using descriptor d.

  //Den* key2 = dynamic_cast<Den*>(key.project(*numToDenDescriptor));
  Den* key2 = static_cast<Den*>(key.project(*numToDenDescriptor));

  HASHTYPE(Den)::const_iterator j;
  if((j = denominators.find(*key2)) != denominators.end())
    score2 = log(j->second);
  else{
    delete key2;
    cerr << "RHS of conditional item, " << *key2 << " (" << key << ") not found in LazyCD!" << endl;
    exit(1);
  }  
  delete key2;

  if(score1 == log(0.0) && score2 == log(0.0)){
      return (SCORET)log(0.0);
  }
  else return score1-score2;
}

//! Returns the log of a denominator's count.
template<class Num, class Den, class Left>
SCORET LazyCD<Num, Den, Left>::lookupLogDen(const Den& d) const{
  HASHTYPE(Den)::const_iterator i;
  if((i = denominators.find(d)) != denominators.end())
    return log(i->second);
  else return (SCORET)log(ZEROSCORET);
}

//! Returns a denominator's count.
template<class Num, class Den, class Left>
SCORET LazyCD<Num, Den, Left>::lookupDen(const Den& d) const{
  HASHTYPE(Den)::const_iterator i;
  if((i = denominators.find(d)) != denominators.end())
    return i->second;
  else return ZEROSCORET;
}

//! Returns the log of a numerator's count.
template<class Num, class Den, class Left>
SCORET LazyCD<Num, Den, Left>::lookupLogNum(const Num& n) const{
  HASHTYPE(Num)::const_iterator i;
  if((i = numerators.find(n)) != numerators.end())
    return (SCORET)log(i->second);
  else return (SCORET)log(ZEROSCORET);
}

//! Returns a numerator's count.
template<class Num, class Den, class Left>
SCORET LazyCD<Num, Den, Left>::lookupNum(const Num& n) const{
  HASHTYPE(Num)::const_iterator i;
  if((i = numerators.find(n)) != numerators.end())
    return i->second;
  else return ZEROSCORET;
}

template<class Num, class Den, class Left>
bool LazyCD<Num, Den, Left>::isEmpty(){
  return numerators.empty();
  }

/* Clears both numerators and denominators.*/
template<class Num, class Den, class Left>
void LazyCD<Num,Den,Left>::clear() {
  numerators.clear();
  denominators.clear();

}


/*Merges two LazyCD together*/
template<class Num, class Den, class Left>
LazyCD<Num, Den, Left>& 
LazyCD<Num, Den, Left>::operator+=(const LazyCD<Num, Den, Left>& other)
{
 typename Histogram<Num>::const_iterator iterNum;
 for(iterNum = other.getNum()->begin(); iterNum != other.getNum()->end(); iterNum++){
   increment(iterNum->first, iterNum->second);
 }
 return *this;
}

/* Constructor for empty. */
template<class Num, class Den, class Left>
LazyCD<Num, Den, Left>::LazyCD(Descriptor *desc, Descriptor *ldesc){
  numToDenDescriptor = desc;
  leftDescriptor = ldesc;
}

/* Calls projectUnique on the Numerator and then rebuilds the Denominator..*/
template<class Num, class Den, class Left> template<class N, class D, class L>
LazyCD<Num,Den,Left>&  LazyCD<Num,Den,Left>::projectUnique(LazyCD<N,D,L> &lcd, Descriptor& desc,Descriptor& desc2) {

  
  //if(ScalarDescriptor* s = dynamic_cast<ScalarDescriptor *>(&desc2)) {
	if(desc2.type() == Descriptor::SCALAR_DESC){
		ScalarDescriptor* s = static_cast<ScalarDescriptor *>(&desc2);
    numToDenDescriptor= new ScalarDescriptor(*s);
  }
  else{
    throw "BAD DESC TYPE IN LAZY CD PROJECT UNIQUE" ;
  }
 
  numerators.projectUnique(*(lcd.getNum()), desc);
 
  denominators.clear();
 
  denominators.projectUnique(numerators,desc2);
 
  return *this;
}


//! Returns the max event in the lazy CD. 
template<class Num, class Den, class Left>
Num LazyCD<Num, Den, Left>::maxEvent() const{

  SCORET max = 0;
  SCORET temp;
  Num event;
  
  typename Histogram<Num>::const_iterator i;
  for(i = numerators.begin(); i != numerators.end(); i++){
    temp = this->lookup((i)->first);
    if(temp > max){
      max = temp;
      event = i->first;
    }
  }
  return event;
}  

//! Returns the min event in the lazy CD. 
template<class Num, class Den, class Left>
Num LazyCD<Num, Den, Left>::minEvent() const{

  SCORET min = 999999.0;
  SCORET temp;
  Num event;
  
  typename Histogram<Num>::const_iterator i;
  for(i = numerators.begin(); i != numerators.end(); i++){
    temp = this->lookup((i)->first);
    if(temp < min){
      min = temp;
      event = i->first;
    }
  }
  return event;
}  

template<class Num, class Den, class Left>
SCORET LazyCD<Num, Den, Left>::maxScore() const
{
  return lookup(maxEvent());
}

template<class Num, class Den, class Left>
SCORET LazyCD<Num, Den, Left>::minScore() const
{
  return lookup(minEvent());
}

template<class Num, class Den, class Left>
Left *LazyCD<Num, Den, Left>::generateRandom_BO(SCORET& score) const {
  return 0;
}


//! Random event according to the conditional distribution and a Den.
template<class Num, class Den, class Left>
Left *LazyCD<Num,Den,Left>::generateRandom(const Den& rhs, SCORET& score) const {
  typename Histogram<Den>::const_iterator j = denominators.find(rhs);
  if(j == denominators.end()){
    //    cerr << rhs << endl;
    return generateRandom_BO(score);
  }
  SCORET rhsScore = j->second;
  SCORET r = rhsScore * (SCORET)rand() / (SCORET)RAND_MAX, s = ZEROSCORET;
  typename Histogram<Num>::const_iterator i = numerators.begin();
  Den *td;

  //  cerr << "random = " << r << "; rhsScore = " << rhsScore << endl;
  do {
    td = static_cast<Den *>(i->first.project(*numToDenDescriptor));
		//dynamic_cast<Den *>(i->first.project(*numToDenDescriptor));
    if(*td == rhs){
      s += i->second;
      //  cerr << " found " << 
      //	cerr << "   Pr( " << *dynamic_cast<Left *>(i->first.project(*leftDescriptor)) << " | " << *dynamic_cast<Den *>(i->first.project(*numToDenDescriptor)) << " ) = " 
      //	   << i->second << " (s is now " << s << ")" << endl;
    }
    delete td;
  } while(s < r && (++i != numerators.end()));
  if(i == numerators.end()){
    cerr << "LazyCD::generateRandom:  Never found left side for " << rhs << "!" << endl;
    return 0;
  }
  score = i->second;
  Left *ret = static_cast<Left *>(i->first.project(*leftDescriptor));
	  //dynamic_cast<Left *>(i->first.project(*leftDescriptor));
  //cerr << "ret is " << *ret<< endl;
  return ret;
}

template<class Num, class Den, class Left>
SCORET LazyCD<Num,Den,Left>:: entropy() const
{
    Log<SCORET> logfun(2.0);
     typename Histogram<Num> ::   const_iterator i;
    SCORET ent = 0.0;
    SCORET total_counts = 0;
    for(i = getNum()->begin(); i != getNum()->end(); ++i){
	ent += i->second * logfun(lookup(i->first));
	total_counts += i->second;
    }

    return ent/total_counts;
}

#endif









  
  
  

  
