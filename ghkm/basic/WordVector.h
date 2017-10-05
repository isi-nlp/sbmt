// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/** @file :  WordVector - a vector of words such that we can 
*                         convert each the ID of each words into string.
*   $Id: WordVector.h,v 1.1 2006/08/22 18:38:04 wang11 Exp $ 
*   $Header: /home/nlg-03/wang11/cvs-repository/dev/basic/src/WordVector.h,v 1.1 2006/08/22 18:38:04 wang11 Exp $
*/
#ifndef _WordVector_H_
#define _WordVector_H_


#include <vector>
#include <string>
#include <assert.h>
#include <iostream>
#include "strmanip.h"
#include "LiBEDefs.h"
#include "StringVocabulary.h"

using namespace std;

/** The StringVocabulary is used just to privde the base class type.*/
class WordVector : public vector<string>, public StringVocabulary
{
    typedef vector<string> 	_base;

public:
    WordVector()  {}

    size_t size() const { return _base::size();}
    void clear() { return _base::clear(); StringVocabulary::clear();}

    string getWord(const EID& id, bool& exist) const {
	string wd = "";
	if(id < _base::size()) {
	    exist = true;
	    wd = (*this)[id];
	}
	return wd;
    }

    EID getIndex(const string& wd) const { assert(0);}


    istream& get(istream& in) {
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
	      EID vi = atoi(fields[0].c_str());
	      if(vi != size()){
		  cerr<<"The word id is not contiguous!"<<endl;
		  exit(1);
	      }
	      (*this).push_back(fields[1]);
	    }
	  }
	}
	return in;
    }

    ostream& put(ostream& out) const {
	size_t i = 0;
	out<<"<VOCAB>"<<endl;
	for(i = 0; i < size(); ++i){
	    out<<i<<"\t"<<(*this)[i]<<endl;
	}
	out<<"</VOCAB>"<<endl;
	return out;
    }

    friend istream& operator>>(istream& in, WordVector& wdv)  
    { return wdv.get(in);}

    friend ostream& operator <<(ostream& out, const WordVector& wdv)
    { return wdv.put(out);}
};

#endif
