// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#include <string>
#include "SubStringVocabulary.h"

using namespace std;

// Constructor.
SubStringVocabulary::SubStringVocabulary(StringVocabulary& baseV)
    : _M_baseV(baseV)
{}


// Add word.
VocabularyIndex SubStringVocabulary :: addWord(const string& word)
{
    /*
     * Add the word into the base vocab.
     */
    VocabularyIndex wid = _M_baseV.addWord(word);
	

    /*
     * Add it into base class.
     */
    _byWord[word] = wid;
    _byIndex[wid] = &(_byWord.find(word)->first);

    return wid;
}

// Assignment. 
SubStringVocabulary& SubStringVocabulary::operator=(const SubStringVocabulary& other)
{
    // base vocab.
    baseVocabulary() = const_cast<StringVocabulary&>(other.baseVocabulary());

    // words.
    StringVocabulary::const_iterator it;
    for(it = other.begin(); it != other.end(); ++it){
	addWord(it->first);
    }
    return *this;
}


istream& SubStringVocabulary::read(istream& in)
{
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
	  vector<string> strvec = split(buffer, "\t");
	  if(strvec.size() >= 2){
	      // the index 
	      VocabularyIndex ix = atoi(strvec[0].c_str());
	      VocabularyIndex ix1 = addWord(strvec[1]);
	      if(ix != ix1){
		  cerr<<ix<<" "<<ix1<<" "<<strvec[1]<<endl;
		  cerr<<"The subvocab and the base vocab are inconsistent."
		      <<endl;
	      }
	  } else if(strvec.size() == 1){
	    addWord(strvec[0]);
	  } else {
	      cerr<<"Expected number of fields in SubStringVocabulary."<<endl;
	  }
      }
    }
    return in;

    
}

