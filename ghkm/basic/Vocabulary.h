// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _Vocabulary_H_
#define _Vocabulary_H_

#ifdef WIN32
#pragma warning ( disable : 4267 )
#endif

#include "LiBE.h"
#include "LiBEDefs.h"
#include <iostream>
#include <map>
#include "strmanip.h"

using namespace std;
using STL_HASH::hash_map;


#ifdef WIN32
#define MAPTYPE map
#else
#define MAPTYPE hash_map
#endif


typedef unsigned int    VocabularyIndex;
extern const VocabularyIndex VocabularyNone;

// the start (begin) of sentnce
extern const EID BOS;

// the end of sentnce
extern const EID EOS;

//! A generic Vocabularyulary class with template parameterized
//! by the type of its content. The main functionality of
//! this class is to assign an 'index' to a 'word', and provide
//! interfaces for the word<->index conversion.
template<class X> class Vocabulary 
{
public:
  typedef typename MAPTYPE<X, VocabularyIndex>::const_iterator const_iterator;

  const_iterator begin() const { return _byWord.begin();}
  const_iterator end()   const { return _byWord.end();  }
	
  //! Constructor. Specifying the start index at the same time.
  Vocabulary(size_t startInx = 0) : _startIndex(startInx), _curIndex(startInx) {}

  //! Destructor.
  virtual ~Vocabulary() {}

  //! Returns the index of the word. Returns VocabularyNone if not found.
  virtual VocabularyIndex  getIndex(const X& word) const ;

  //! Returns the word of the index.
  //! If not found, exist is set to be false.
  virtual X getWord(const VocabularyIndex& index, bool& exist ) const;

  //! Adds a word into Vocabulary, and returns its index.
  virtual VocabularyIndex  addWord(const X& word); 
  // add a word , and make its index to be 'index'.
  virtual VocabularyIndex  addWord(const X& word, VocabularyIndex index); 
    
  //! Returns the number of entries in the Vocabulary.
  virtual size_t size() const { return _byIndex.size(); }
	
  //! Tells whether the vocab is empty
  virtual bool empty() const { return _byIndex.empty(); }


  //! Clear the content.
  virtual void clear();


  //! Loads Vocabulary from istream.
  virtual istream& read(istream& in);

  //! Dumps Vocabulary to ostream.
  virtual ostream& write(ostream& out) const;

  //! Assignment.
  Vocabulary<X>& operator=(const Vocabulary<X>& other);

  // output into stream.
  friend ostream& operator<< (ostream& out, const Vocabulary<X>& vcb) {
  unsigned int counter = 0;
  unsigned int i = vcb._startIndex;
  out<<"<VOCAB>"<<endl;
  while(1) {
    typename MAPTYPE<VocabularyIndex,const X*> :: const_iterator iter;
    if((iter = vcb._byIndex.find(i)) != vcb._byIndex.end()){
      out<<*(iter->second)<<endl;
      counter++;
    }
    if(counter == vcb._byIndex.size()) break;
    i++;
  }
  out<<"</VOCAB>"<<endl;
  return out;
  }

  // read vocab from stream.
  friend istream& operator>>(istream& in, Vocabulary<X>& vcb) {
  vcb.clear();

  string str;
  in >> str;
  if(str != "<VOCAB>"){
    cerr<<"Error: expected <VOCAB>, got "<<str<<endl;
    exit(-1);
  }
	
  if(in.peek() == '\n') getline(in, str, '\n');
  vcb._curIndex = vcb._startIndex;
  
  if(in.peek() == '<'){
    in >> str;
    if(str != "</VOCAB>"){
      cerr<<"Error: expected </VOCAB>, got "<<str<<endl;
      exit(-1);
    }
    return in;
  }

  X x;
  while(in>>x) {
    vcb._byWord[x] = vcb._curIndex;
    vcb._byIndex[vcb._curIndex] =  &(vcb._byWord.find(x)->first);
    vcb._curIndex++;
    
    if(in.peek() == '\n') getline(in, str, '\n');
    
    if(in.peek() == '<'){
      in >> str;
      if(str != "</VOCAB>"){
	cerr<<"Error: expected </VOCAB>, got "<<str<<endl;
	exit(-1);
      }
      if(in.peek() == '\n') in >> ws;
      return in;
    }
    
  }

  return in;

  }


  static size_t ngramLength(const VocabularyIndex* ngram);

  //! Start index.
  size_t _startIndex;

  //! (One past the ) current highest index.
  size_t _curIndex;

protected:

  //! Stored by index.
  MAPTYPE<VocabularyIndex, const X*> _byIndex;
      
  //! Stored by word.
  MAPTYPE<X, VocabularyIndex> _byWord;

};

////////////////////////////////////////////////////////////////////////

template<class X> istream& Vocabulary<X>::read(istream& in)
{
  _curIndex = _startIndex;
  X x;
  while(in>>x) {
    _byWord[x] = _curIndex;
    _byIndex[_curIndex] =  &(_byWord.find(x)->first);
    _curIndex++;
  }
  return in;
}

template<class X> 
X Vocabulary<X>:: getWord(const VocabularyIndex& index, bool& exist) const
{
  X x;
  typename MAPTYPE<VocabularyIndex,const X*>::const_iterator iter;
  iter = _byIndex.find(index);
  if(iter != _byIndex.end()) { exist = true; return *(iter->second);}
  else { exist = false; return x;}
}

// adds a word, giving it an index that's one higher than the current
// highest, regardless of whether there are any "missing" indexes.
template<class X> VocabularyIndex  Vocabulary<X>:: addWord(const X& word) 
{
  VocabularyIndex index  = getIndex(word);
  if(index == VocabularyNone){
    _byWord[word] = _curIndex;
    _byIndex[_curIndex] =  &(_byWord.find(word)->first);
    _curIndex++;
    return _curIndex - 1;
  }
  else {
    return _byWord[word];	
  }
}

// adds a word, giving it an index that's one higher than the current
// highest, regardless of whether there are any "missing" indexes.
template<class X> VocabularyIndex  Vocabulary<X>:: 
addWord(const X& word, VocabularyIndex inx) 
{
  VocabularyIndex index  = getIndex(word);
  if(index == VocabularyNone){
      _byWord[word] = inx;
      _byIndex[inx] = &(_byWord.find(word)->first);
      if (_startIndex > inx)
	_startIndex = inx;
      if (_curIndex <= inx)
	    _curIndex = inx + 1;
    return inx;
  }
  else {
      if(inx != index){
	  cerr<<"Error: inconsistent indexes in Vocabulary::addWord."<<endl;
	  exit(1);
      }
    return inx;
  }
}

template<class X> ostream& Vocabulary<X>::write(ostream& out) const
{
  unsigned int counter = 0;
  unsigned int i = _startIndex;
  while(1) {
    typename MAPTYPE<VocabularyIndex,const  X*> :: const_iterator iter;
    if((iter = _byIndex.find(i)) != _byIndex.end()){
      out<<*(iter->second)<<endl;
      counter++;
    }
    if(counter == _byIndex.size()) break;
    i++;
  }
  return out;
}


template<class X> VocabularyIndex Vocabulary<X>::getIndex(const X& word) const
{
  typename MAPTYPE<X, VocabularyIndex>::const_iterator iter;
  X key = word;
  iter = _byWord.find(key);
  if(iter != _byWord.end()) return iter->second;
  else return VocabularyNone;
}

template<class X> Vocabulary<X>& Vocabulary<X>::operator=(const Vocabulary<X>& other)
{
  clear();
  _startIndex = other._startIndex;
  _curIndex   = other._curIndex;
  const_iterator iter;
  for(iter = other.begin(); iter != other.end(); iter++) {
    _byWord[iter->first] = iter->second;
    _byIndex[iter->second] =  &(_byWord.find(iter->first)->first);
  }
  return *this;
}

template<class X> void Vocabulary<X>::clear()
{
  _byIndex.clear();
      
  _byWord.clear();

  _curIndex = _startIndex;
}

#endif
