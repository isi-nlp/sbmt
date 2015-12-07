// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _LANG_MODEL_KN_H
#define _LANG_MODEL_KN_H

#include <iostream>

#include "../LangModel.h"
#include "Common/Vocab/Vocab.h"
#include "LangModelImplBase.h"
#include "Trie.h"
#include "Prob.h"

namespace LW {

struct KNDiscount
{
	double d1;
	double d2;
	double d3plus;
//	int minCount;
public:
	KNDiscount() {
		d1 = 0;
		d2 = 0;
		d3plus = 0;
//		minCount = 1;
	}
};

class LangModelKN : public LangModelImplBase
{
protected:
	enum ParseStatus{PS_BEFORE_DATA, PS_READING_COUNTS, PS_BEFORE_READING_NGRAM, PS_READING_NGRAM, PS_DONE};
public:
	LangModelKN(LWVocab* pVocab, const LangModelParams& params);
	virtual ~LangModelKN();
public:
	virtual void clear();
	virtual void learnSentence(iterator pSentence, unsigned int nSentenceSize);
	virtual LangModel::Prob computeTrigramProb(iterator trigramArray);
	virtual LangModel::ProbLog getContextProb(iterator pNGram, unsigned int order);
	virtual void dump(std::ostream& out);
	virtual void read(std::istream& in, LangModel::ReadOptions nOptions = LangModel::READ_NORMAL);
	virtual void readText(std::istream& in, LangModel::ReadOptions nOptions);
	virtual void readBinary(std::istream& in, LangModel::ReadOptions nOptions);
	virtual void readCounts(std::istream& in);
	/// Serializes the LM to a stream
	virtual void write(std::ostream& out, LangModel::SerializeFormat nFormat);
	/// Writes out a text language model. Can be read back by this class
	virtual void writeText(std::ostream& out);
	/// Writes out binary information that can be read back by this class
	virtual void writeBinary(std::ostream& out);
	/// Writes out binary information that can be read by LangModelSA, but not by this class
	virtual void writeBinary2(std::ostream& out);
	/// Writes out counts
	virtual void writeCounts(std::ostream& out);
public:
	unsigned int getMaxOrder() const;
    virtual logprob find_bow(iterator b,iterator end) const
    {
        int ctxlen=m_params.m_nMaxOrder-1;
        if (end-b>ctxlen)
            b=end-ctxlen; //because we're supposed to return BOW_NONE for max order
        ProbNode *n=const_cast<LangModelKN*>(this)->findProbNode(b,end-b);
        if (!n) return BOW_NONE;
        return n->m_bow; 
    }
protected:
	void computeDiscount(KNDiscount& discount, unsigned int nOrder);
	void computeDiscounts();
	double getDiscount(int nCount, KNDiscount& discount, unsigned int nOrder);
	/// Used for intepolation
	double getLowerOrderWeight(int nTotalCount, int nUniqueCount, 
		int nMin2Vocab, int nMin3Vocab, KNDiscount& discount);
	void redistributeProb(double dLeftOverProb);
	void prepare();
	bool computeBOW(unsigned int nOrder);
	ProbNode* findProbNode(iterator pNGram, unsigned int nOrder);
	LangModel::ProbLog* insertProb(iterator pNGram, unsigned int nOrder);
	LangModel::ProbLog* findProb(iterator pNGram, unsigned int nOrder);
    
private:
	/// Flag that is set when discounts are computed for the current train data. If the train
	/// data changes the discounts need to be recomputed
	bool m_bDiscountsComputed;
	/// Vocabulary used
	LWVocab* m_pVocab;
	/// Trie used to hold counts
	TrieNode<LWVocab::WordID, int> m_counts;
	/// Trie used to hold probabilities
	TrieNode<LWVocab::WordID, ProbNode> m_probs;
	/// The number of words in the LM
	unsigned int m_nWordCount;
	/// Discounts for unigrams, bigrams, trigrams. The index represents the order of n-gram, so index 0 is not used.
	KNDiscount m_discounts[MAX_SUPPORTED_ORDER + 1];
	///// Max n-gram order in the LM. For learning, it must be configured. 
	///// If the LM is loaded it will read it from the file
	//unsigned int m_nMaxOrder;

	/// Parameters for the language model
	LangModelParams m_params;
};

} // namespace LW


#endif // _LANG_MODEL_KN
