// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef LANG_MODEL_IMPL_H
#define LANG_MODEL_IMPL_H

#include <string>
#include <vector>

#include "../LangModel.h"
#include "LangModelImplBase.h"

namespace LW {


class LangModelImpl : public LangModel
{
public: 
	/// The actual implementation of the LM
    LangModelImplBase* m_pImpl; //fixme: inherit?  less indirection?
	/// Indicates if discounts and counts were calculated after learning
	bool m_bPrepared;
	/// Vocabulary used the the LM
	LWVocab* m_pVocab;
	/// This flag tells the destructor whether to delete the vocab or not
	bool m_bOwnVocab;
public:
	LangModelImpl(LangModelImplBase* pImpl, LWVocab* pVocab, bool bOwnVocab = true);
	~LangModelImpl();
public:
	void clear();
	void learnSentence(char** pSentence, unsigned int nSentenceSize, bool bPadWithStartEnd = true);
	ProbLog computeSentenceProbability(char** pSentence, unsigned int nSentenceSize);
	ProbLog computeSequenceProbability(unsigned int* pWords, unsigned int nStartWord, unsigned int nEndWord);
	ProbLog computeProbability(unsigned int nWord, LangModelHistory& historyIn, LangModelHistory& historyOut);
	ProbLog computeProbability(unsigned int nWord, unsigned int* pnContext, unsigned int nContextLength);
	void writeCounts(std::ostream& out);
	void write(std::ostream& out, LangModel::SerializeFormat);
	void read(std::istream& in, LangModel::ReadOptions nOptions = LangModel::READ_NORMAL);
	void readCounts(std::istream& in);
	void dump(std::ostream& out);
	const LWVocab* getVocab() const;
	unsigned int getMaxOrder() const;
	/// Let the LM know that counting is finished so it can compute probabilities.
	void finishedCounts();

    typedef LangModelImplBase::iterator iterator;
    /*
          logprob getContextProb(iterator b,iterator end)
          {
          return m_pImpl->getContextProb(b,end);
          {

      ProbLog find_bow(iterator b,iterator end) const
      {
         return m_pImpl->find_bow(b,end);
       }
      iterator longest_prefix(iterator b,iterator end) const
    {
        return m_pImpl->longest_prefix(b,end);
    }
    
    iterator longest_suffix(iterator b,iterator end) const
    {
        return m_pImpl->longest_suffix(b,end);
    }
    */
    
public:
	LangModelImplBase* getImplementation() {return m_pImpl;};
protected:
	// Prepare discounts and counts after learning
	void prepare();
};

} // namespace LW

#endif // LANG_MODEL_IMPL_H
