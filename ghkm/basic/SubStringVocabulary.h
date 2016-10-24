// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _SubStringVocabulary_H_
#define _SubStringVocabulary_H_

#include "StringVocabulary.h"

//! Subset of a StringVocab. An adaptor class of  StringVocab.
class SubStringVocabulary : public StringVocabulary {
public:

    //! Constructor.
    SubStringVocabulary(StringVocabulary& baseV);

    //! Add word.
    VocabularyIndex addWord(const string& word);

    //! Returns the base Vocab.
    const StringVocabulary& baseVocabulary() const  { return _M_baseV; }
    StringVocabulary& baseVocabulary()        { return _M_baseV; }

    SubStringVocabulary& operator=(const SubStringVocabulary& other);

    istream& read(istream& in);

protected:

    //! The based class.
    StringVocabulary&  _M_baseV;
};

#endif
