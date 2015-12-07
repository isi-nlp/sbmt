// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include <Common/stldefs.h>
#include "CustomizerLM.h"

#include "CountsReader.h"

using namespace std;

namespace LW {
/**
This class maps IDs from a master vocab space to another (reduced) vocab
Any word that is not found in the second vocab is mapped to <unk>
*/
class IDMapper 
{
public:
	IDMapper(const LWVocab* pMasterVocab, const LWVocab* pVocab);
	~IDMapper();
public:
    typedef LangModelImplBase::word_type word_type;
    typedef word_type const* iterator;
    
	void map(iterator pSource, word_type *pDest, unsigned int nCount);
	word_type map(word_type nSrc);
private:
	/// The array to map the words
	word_type *m_pMap;
	/// The number of elements in the array
	unsigned int m_nMapCount; 
};

IDMapper::IDMapper(const LWVocab* pMasterVocab, const LWVocab* pVocab)
{
	m_pMap = NULL;

	// Allocate space to store the conversion table
	m_nMapCount = pMasterVocab->getNextID();
	m_pMap = new LWVocab::WordID[m_nMapCount];

	// Store <unk> in all entries
	LWVocab::WordID *pWords = m_pMap;
	for (unsigned int i = 0; i < m_nMapCount; i++) {
		*pWords = LWVocab::UNKNOWN_WORD;
		pWords++;
	}

	// Scan all entries of the master vocab
	VocabIterator it(pMasterVocab);
	LWVocab::WordID nWord = it.next();
	for ( ; nWord != LWVocab::INVALID_WORD; nWord = it.next()) {
		const string& sWord = pMasterVocab->getWord(nWord);

		// Find the word in the regular vocab
		LWVocab::WordID nWord2 = pVocab->getID(sWord);

		// Sanity check to avoid mem corruption. This should never fail.
		if (nWord >= m_nMapCount) {
			char szError[1024];
			lw_snprintf(szError, sizeof(szError), "Word out of range in the vocabulary file. Word: %s, ID: %d", sWord.c_str(), nWord);
			throw Exception(ERR_IO, szError);
		}

		// Store the word in the conversion table
		m_pMap[nWord] = nWord2;
	}

}

IDMapper::~IDMapper()
{
	delete m_pMap;
}

void IDMapper::map(iterator pSource, word_type *pDest, unsigned int nCount)
{
	for (unsigned int i = 0; i < nCount; i++) {
		// This should never happen, but if this test failed we would crash
		if (*pSource >= m_nMapCount) {
			char szError[1024];
			lw_snprintf(szError, sizeof(szError), "Vocabulary error: Word value out of range: %d", *pSource);
			throw Exception(ERR_IO, szError);
		}

		*pDest = m_pMap[*pSource];

		pSource++;
		pDest++;
	}
}

IDMapper::word_type IDMapper::map(word_type nSrc)
{
	// This should never happen, but if this test failed we would crash
	if (nSrc >= m_nMapCount) {
		char szError[1024];
		lw_snprintf(szError, sizeof(szError), "Vocabulary error: Word value out of range: %d", nSrc);
		throw Exception(ERR_IO, szError);
	}

	return m_pMap[nSrc];
}


// Required by tries
void clear(CustomizerLMCombinedCount& count) {
		count.m_nGenericCount = 0;
		count.m_nDomainCount = 0;
};

CustomizerLM::CustomizerLM(SmartPtr<LangModel> spGenericLM, SmartPtr<LangModel> spDomainLM, std::istream& combinedCountsIn, unsigned int nDomainReplicationFactor) 
{
	//assert(!spGenericLM.isNull());
	//assert(!spDomainLM.isNull());

	m_spGenericLM = spGenericLM;
	m_spDomainLM = spDomainLM;
	m_nMaxOrder = 0;

	m_pMasterVocab = NULL;
	m_pGenericMapper = NULL;
	m_pDomainMapper = NULL;
	initVocab();

	loadCombinedCounts(combinedCountsIn, nDomainReplicationFactor);
}

void CustomizerLM::loadCombinedCounts(istream& combinedCountsIn, unsigned int nDomainReplicationFactor) 
{
	// TODO: REMOVE THIS
    char *szDupFactor = 0;
    size_t sz;
    _dupenv_s(&szDupFactor, &sz, "DUP_FACTOR");
	if (szDupFactor) {
		nDomainReplicationFactor = atoi(szDupFactor);
        free(szDupFactor);
	}


	// n-gram cache as words
	char* wordNGram[MAX_SUPPORTED_ORDER];
	LWVocab::WordID vocabIDNGram[MAX_SUPPORTED_ORDER];

	CountsReader reader(&combinedCountsIn, true);

	unsigned int nOrder = 1;
	while (reader.moveOrder(nOrder)) {
		// Store max order for later use
		m_nMaxOrder = nOrder;

		do {
			CustomizerLMCombinedCount count;
			count.m_nGenericCount = reader.getCurrentCount();
			count.m_nDomainCount = reader.getCurrentCount2();

			// Domain count is not allowed to be 0
			if (0 == count.m_nDomainCount) {
				throw Exception(ERR_IO, "Domain counts are not allowed to be 0 (read from combined counts file)");
			}

			// Use the domain replication factor during loading so we don't have to multiply
			// while computing probabilities
			count.m_nDomainCount *= nDomainReplicationFactor;

			// Read current n-gram and convert it to Vocab IDs
			reader.getCurrentNGram(wordNGram);
			// Do not insert words in the vocabulary
			m_pMasterVocab->wordsToID(wordNGram, vocabIDNGram, nOrder, false);

			// Ready to insert in the trie
			m_combinedCounts.insertKey(vocabIDNGram, nOrder, count);

		} while (reader.moveNext());

		nOrder++;
	}
}

CustomizerLM::~CustomizerLM()
{
	delete m_pMasterVocab;
	delete m_pGenericMapper;
	delete m_pDomainMapper;
}

LangModel::ProbLog CustomizerLM::getContextProb(iterator pNGram, unsigned int nOrder) 
{
	// Allocate space for the mapped generic and domain n-grams
	LWVocab::WordID pGenericNGram[MAX_SUPPORTED_ORDER];
	LWVocab::WordID pDomainNGram[MAX_SUPPORTED_ORDER];

	// Map the n-grams passed into the word id space of each vocabulary
	m_pGenericMapper->map(pNGram, pGenericNGram, nOrder);
	m_pDomainMapper->map(pNGram, pDomainNGram, nOrder);

	LangModel::ProbLog dProb;

//	if (nOrder == m_nMaxOrder) {
	if (nOrder > 1) {
		// Has the context been seen in the domain counts?
            TrieNode<LWVocab::WordID, CustomizerLMCombinedCount>* pNode = m_combinedCounts.findNode(const_cast<word_type *>(pNGram), nOrder - 1);
		int nGenericCount = 0;
		int nDomainCount = 0;
		if (pNode) {
			nGenericCount = pNode->m_payload.m_nGenericCount;
			nDomainCount = pNode->m_payload.m_nDomainCount;
		}
		
		LangModel::ProbLog dGenericProb = 0;
		LangModel::ProbLog dDomainProb = 0;

		// Always compute the generic probability
		// TODO: Optimize so we don't need to always compute it if the count is 0
		dGenericProb = m_spGenericLM->computeProbability(pGenericNGram[nOrder - 1], pGenericNGram, nOrder - 1);

		if (nDomainCount > 0) {
			dDomainProb = m_spDomainLM->computeProbability(pDomainNGram[nOrder - 1], pDomainNGram, nOrder - 1);

			// Compute weighted probability from domain and generic probs
			double dGenericWeight = (double) nGenericCount / (nGenericCount + nDomainCount);
			double dDomainWeight = 1 - dGenericWeight;

			// dGenericWeight = 0.91;
			// dDomainWeight = 0.4;

			dProb = static_cast<LangModel::ProbLog> (dGenericWeight * dGenericProb + dDomainWeight * dDomainProb);
		}
		else {
			dProb = dGenericProb;
		}
	}
	else {
		dProb = m_spGenericLM->computeProbability(pGenericNGram[nOrder - 1], pGenericNGram, nOrder - 1);
		//if (PROB_LOG_ZERO == dProb) {
		//	// This only happens when none of the words were seen in the generic corpus
		//	// See NOTE above
		//	dProb = m_spDomainLM->computeProbability(pNGram[nOrder - 1], pNGram, nOrder - 1);
		//}
	}

	return dProb;
}

void CustomizerLM::initVocab()
{
	m_pMasterVocab = new LWVocab();

	// Master vocab is empty at this point
	// Initialize it from the generic vocab
	*m_pMasterVocab = *(m_spGenericLM->getVocab());

	// Now we have to add all words in the Domain vocab that don't already exist
	LWVocab* pDomainVocab =  const_cast<LWVocab*>(m_spDomainLM->getVocab());
	VocabIterator domVocabIterator(pDomainVocab);
	// To iterate the vocab, just keep calling next()
	LWVocab::WordID nWord = domVocabIterator.next();
	for ( ; nWord != LWVocab::INVALID_WORD; nWord = domVocabIterator.next()) {
		// Get the word out of the Domain vocab
		const string& sWord = pDomainVocab->getWord(nWord);

		// Make sure it exists in the Master vocab
		m_pMasterVocab->insertWord(sWord);
	}	

	// Initialize the vocabulary mappers
	m_pGenericMapper = new IDMapper(m_pMasterVocab, m_spGenericLM->getVocab());
	m_pDomainMapper = new IDMapper(m_pMasterVocab, m_spDomainLM->getVocab());
}

} // namespace LW
