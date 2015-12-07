// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _LANG_MODEL_IMPL_BASE
#define _LANG_MODEL_IMPL_BASE

#include <vector>

#include "Common/Vocab/Vocab.h"
#include "Core/LangModel/LangModel.h"

#include <cmath>
#define BOW_NONE -HUGE_VAL

namespace LW {

class LangModelImplBase 
{
public:	
    typedef LWVocab::WordID word_type;
    typedef word_type const* iterator;
	virtual ~LangModelImplBase() {};
	/// Clears all counts and probabilities and frees memory associated with them
	virtual void clear() = 0;
	/// Performs training o a sentence
	virtual void learnSentence(iterator pSentence, unsigned int nSentenceSize) = 0;
	/// Called after learning is finishted to prepare counts, etc.
	virtual void prepare() = 0;
    typedef LangModel::ProbLog logprob;
    
    /// Returns the probability of a sequence of words of size order
    virtual logprob getContextProb(iterator pNGram, unsigned int nOrder) = 0;

    /// Returns probability of [b,end) = p(end[-1] | b[0],b[1],...,end[-2])
    logprob getContextProb(iterator b,iterator end) 
    {
        return getContextProb(b,end-b);
    }
    
    /// looks for exactly the string [b,end) and returns BOW_NONE if no LM entry
    /// exists.  otherwise returns the bo for that history [b,end)
    virtual logprob find_bow(iterator b,iterator end) const = 0;

    /// looks for exactly the string [b,end) and returns BOW_NONE if no LM entry
    /// exists.  otherwise returns the prob for the final word of that phrase
    virtual logprob find_prob(iterator b,iterator end) const 
    {
        throw Exception(ERR_NOT_IMPLEMENTED,"find_prob() not implemented");
    }
    
    //FIXME: rename method - doesn't return maxorder string (no backoff)
    /// returns r such that [b,r) is the longest string which has an LM backoff
    virtual iterator longest_prefix(iterator b,iterator end) const 
    {
        for (;end>b;--end)
            if (find_bow(b,end)!=BOW_NONE)
                break;
        return end;
    }

    //FIXME: rename method - is shortening a history, and doesn't return maxorder strings
    /// returns r such that [r,end) is the longest string which has an LM backoff
    virtual iterator longest_suffix(iterator b,iterator end) const
    {
        iterator b2=end-(getMaxOrder()-1);
        if (b<b2) b=b2; // need to do this since we decided find_bow only works on n-1 and down
#if 1
        for (;b<end;++b)
            if (find_bow(b,end)!=BOW_NONE)
                break;
        return b;
#else 
        unsigned n=end-b; // BUG: this is computed as n=(char*)end-(char*)b - but we use word_type directly below and its size is 4!
        word_type a[n];
        word_type *ae=a+n;
        for (word_type *p=ae;b<end;)
            *--p=*b++;
        return end-(longest_prefix(a,ae)-a);
#endif
    }
    
    
	///// Returns the probability of a word, given a context
	//virtual logprob getContextProb(LWVocab::WordID nWord, iterator pnContext, unsigned int nOrder) = 0;
	/// Loads the language model from a file (previously saved with write())
	virtual void read(std::istream& in, LangModel::ReadOptions nOptions = LangModel::READ_NORMAL) = 0;
	/// Loads the language model counts from a file (previously saved with writeCounts())
	virtual void readCounts(std::istream& in) = 0;
	/// Writes the language model to a file
	virtual void write(std::ostream& out, LangModel::SerializeFormat nFormat) = 0;
	/// Writes the language model counts to a file
	virtual void writeCounts(std::ostream& out) = 0;
	/// Writes the language model in human readable format
	virtual void dump(std::ostream& out) = 0;
	/// Returns the actual max order of the LM
	virtual unsigned int getMaxOrder() const = 0;
};

} // namespace LW

#endif // _LANG_MODEL_IMPL_BASE
