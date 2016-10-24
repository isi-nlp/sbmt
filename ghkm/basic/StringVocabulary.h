// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _StrVocabulary_H_
#define _StrVocabulary_H_

#include <string>
#include "Vocabulary.h"

#define VOCAB_SEP "\t"

using namespace std;

#ifndef _string_hash_
#define _string_hash_
namespace STL_HASH{

#ifndef WIN32
  template<>  struct hash<string> {
    size_t operator()(const string& str) const{
      const char* str1 = str.c_str();
      hash<const char*> ha;
      return ha(str1);
    }
  };
#endif

}

#endif

//! String Vocabularyulary class. Implemented by  overloading
//! the input and output interfaces of class Vocabulary.
//! The reason to rewrite these interfaces is that
//! generally in a string vocabulary file, each line contains
//! a word (string-word), so the input of string-word should 
//! be line by line instead of string by string.
class StringVocabulary : public Vocabulary<string> {
    typedef Vocabulary<string> _base;
public:
  typedef Vocabulary<string>::const_iterator const_iterator;

  static const string bos;
  static const string eos;

  StringVocabulary(size_t startInx = 0) : _base(startInx) {}

    static size_t ngramLength(const VocabularyIndex* ngram)
    {
	size_t i = 0;
	while(ngram[i] != VocabularyNone){
	    ++i;
	}
	return i;
    }

    //! Returns the base Vocabulary.
    virtual const StringVocabulary& baseVocabulary() const  { return *this; }
    virtual      StringVocabulary& baseVocabulary()        { return *this; }

  friend istream& operator>>(istream& in, StringVocabulary& vcb) 
  { return vcb.read(in);  }
  
  friend ostream& operator<<(ostream& out, const StringVocabulary& vcb)
  { return vcb.write(out);  }
  

  ostream& write(ostream& out) const {
    out<<"<VOCAB>"<<endl;
    unsigned int counter = 0;
    unsigned int i = 0;
    while(1) {
      MAPTYPE<VocabularyIndex, const string*> :: const_iterator iter;
      if((iter = _byIndex.find(i)) != _byIndex.end()){
	out<<iter->first<<VOCAB_SEP<<*(iter->second)<<endl;
	counter++;
      } 
      if(counter == _byIndex.size()) break;
      i++;
    }
    out<<"</VOCAB>"<<endl;
    return out;
  }

  istream& read(istream& in) {
    clear();
    string buffer;
    in >> buffer;
    in >> ws;

    if(buffer != "<VOCAB>"){
      cerr<<"Error: expected <VOCAB>, got "<<buffer<<endl;
      exit(1);
    }
    while(getline(in, buffer, '\n')) {
      if(buffer == "</VOCAB>"){
	return in;
      }
      else {
	vector<string> fields = split(buffer.c_str(), "\t ");
	if(fields.size() >= 2) {
	  VocabularyIndex vi = atoi(fields[0].c_str());
	  _byWord[fields[1]] = vi;
	  _byIndex[vi] = &(_byWord.find(fields[1])->first);
	  if (_startIndex > vi)
	    _startIndex = vi;
	  if (_curIndex <= vi)
	    _curIndex = vi + 1;
	}
      }
    }
    return in;
  }
};


typedef StringVocabulary StringVocab;


#endif
