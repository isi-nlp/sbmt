// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _CUSTOMIZER_LM_H_
#define _CUSTOMIZER_LM_H_

namespace LW {

struct CustomizerLMCombinedCount {
	int m_nGenericCount;
	int m_nDomainCount;
};
// Required by tries
void clear(CustomizerLMCombinedCount& count);

} // namespace LW

// DO NOT MOVE THIS INCLUDE. THE LINUX COMPILER IS BRAIN DEAD AND IT WON'T FIND
// clear() method above
#include "Trie.h"

#include "LangModelImplBase.h"
#include "Common/SmartPtr.h"

namespace LW {

class IDMapper;

class CustomizerLM : public LangModelImplBase
{
public:
	/// This class will not own any of the Langauge Models passed to it
	/// The caller must destroy them after used
	/// This is done because typically the Generic LM is expensive to load
	/// and it is usually shared.
	CustomizerLM(SmartPtr<LangModel> spGenericLM, SmartPtr<LangModel> spDomainLM, std::istream& combinedCountsIn, unsigned int nDomainReplicationFactor = 10);
	///
	virtual ~CustomizerLM();
public:
	/// Clears all counts and probabilities and frees memory associated with them
	virtual void clear() {
		throw Exception(ERR_NOT_IMPLEMENTED, "Method clear()not implemented by class CustomizerLangModel");
	};
	/// Performs training o a sentence
	virtual void learnSentence(iterator pSentence, unsigned int nSentenceSize) {
		throw Exception(ERR_NOT_IMPLEMENTED, "Method learnSentence()not implemented by class CustomizerLangModel");
	};
	/// Called after learning is finishted to prepare counts, etc.
	virtual void prepare() {
		// Do nothing
	};
	/// Returns the probability of a sequence of words of size order
	virtual LangModel::ProbLog getContextProb(iterator pNGram, unsigned int nOrder);
	/// Loads the language model from a file (previously saved with write())
	virtual void read(std::istream& in, LangModel::ReadOptions nOptions = LangModel::READ_NORMAL) {
		throw Exception(ERR_NOT_IMPLEMENTED, "Method read()not implemented by class CustomizerLangModel");
	};
	/// Loads the language model counts from a file (previously saved with writeCounts())
	virtual void readCounts(std::istream& in) {
		throw Exception(ERR_NOT_IMPLEMENTED, "Method readCounts()not implemented by class CustomizerLangModel");
	};
	/// Writes the language model to a file
	virtual void write(std::ostream& out, LangModel::SerializeFormat nFormat) {
		throw Exception(ERR_NOT_IMPLEMENTED, "Method write()not implemented by class CustomizerLangModel");
	};
	/// Writes the language model counts to a file
	virtual void writeCounts(std::ostream& out) {
		throw Exception(ERR_NOT_IMPLEMENTED, "Method writeCounts()not implemented by class CustomizerLangModel");
	};
	/// Writes the language model in human readable format
	virtual void dump(std::ostream& out) {
		throw Exception(ERR_NOT_IMPLEMENTED, "Method dump()not implemented by class CustomizerLangModel");
	};
	/// Returns the actual max order of the LM
	virtual unsigned int getMaxOrder() const {
		return m_spDomainLM->getMaxOrder();
	}
public:
	const LWVocab* getMasterVocab() {
		return m_pMasterVocab;
	}
    virtual logprob  find_bow(iterator b,iterator end) const
    {
        throw Exception(ERR_NOT_IMPLEMENTED, "Method find_bow() not implemented by class CustomizerLangModel");
        return 0;
    }

protected:
	/// This method loads the combined counts (generic/domain from a text file)
	void loadCombinedCounts(std::istream& combinedCountsIn, unsigned int nDomainReplicationFactor);
	/// This method will create a master vocab from the Generic and Domain vocabs
	/// and it will take care of mappings between the two vocabs
	void initVocab();
public:
	typedef TrieNode<LWVocab::WordID, CustomizerLMCombinedCount> CombinedCountTrie;

private:
	/// Generic Language Model
	SmartPtr<LangModel> m_spGenericLM;
	/// Customizer Language Model
	SmartPtr<LangModel> m_spDomainLM;
	/// Combined counts
	CombinedCountTrie m_combinedCounts;
	/// Max order as obtained from the counts file
	unsigned int m_nMaxOrder;
	/// This vocabulary contains all words in Generic and Domain vocab
	LWVocab* m_pMasterVocab;
	/// Maps words from Master to Generic Vocab
	IDMapper* m_pGenericMapper;
	/// Maps words from Master to Domain Vocab
	IDMapper* m_pDomainMapper;
};

} // namespace LW

#endif

