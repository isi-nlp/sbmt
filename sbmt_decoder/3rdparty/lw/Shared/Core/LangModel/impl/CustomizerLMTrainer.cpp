// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include <Common/stldefs.h>
#include "LangModel/CustomizerLMTrainer.h"

#include <fstream>

#include "LangModel/LangModelFactory.h"
#ifndef LM_NO_COMMON_LIB
#include "Common/Crypto/CryptoStreamFactory.h"
#endif
#include "CountsReader.h"

#include <Common/Platform.h>

using namespace std;

namespace LW {

CustomizerLMTrainer::CustomizerLMTrainer()
{
	m_pDomainLM = NULL;
}

CustomizerLMTrainer::~CustomizerLMTrainer()
{
	delete m_pDomainLM;
}

void CustomizerLMTrainer::init(istream& vocabularyLMFile)
{
	// The default params are ok
	LangModelParams params;

////	// Create a Lang Model to load the vocabulary only
//	// For now, only accept LangModelSA input files
//	LangModel* pGenericLM = LangModelFactory::getInstance()->create(params, LangModelFactory::LANG_MODEL_SA);

	// Create the Lang Model to train the Domain LM
	if (NULL != m_pDomainLM) {
		delete m_pDomainLM;
	}
	m_pDomainLM = LangModelFactory::getInstance()->create(params, LangModelFactory::LANG_MODEL_TRIE);

//	pGenericLM->read(vocabularyLMFile, LangModel::READ_VOCAB_ONLY);

////
//	// Now clone the vocabulary of the Generic LM into the Domain one, so the vocabularies 
//	// will be in sync.
//	// This runs on a single thread so the Vocab* can be cast to const
//	LWVocab* pGenericVocab = const_cast<LWVocab*> (pGenericLM->getVocab());
//	LWVocab* pDomainVocab = const_cast<LWVocab*> (m_pDomainLM->getVocab());
//	// Clone the domain vocabulary (using LWVocab copy operator)
//	*pDomainVocab = *pGenericVocab;
}

//void CustomizerLMTrainer::trainSentence(const vector<string>& words)
//{
//	// Transfer from a vector to an array of words
//	char* pSentence[MAX_WORDS_IN_SENTENCE];
//	unsigned int nWordCount = 0;
//	vector<string>::const_iterator it;
//	for (it = words.begin(); it != words.end(); it++) {
//		pSentence[nWordCount] = const_cast<char*> (it->c_str());
//		nWordCount++;
//	}
//	
//	m_pDomainLM->learnSentence(pSentence, nWordCount);
//}

void CustomizerLMTrainer::trainSentence(char* szSentence) 
{
	char* pSentence[MAX_WORDS_IN_SENTENCE];
	unsigned int nWordCount = 0;
    char* nextToken;
	char* szWord = strtok_s(szSentence, " ", &nextToken);
	while (szWord && (nWordCount < MAX_WORDS_IN_SENTENCE)) {
		pSentence[nWordCount] = szWord;

		nWordCount++;
		szWord = strtok_s(NULL, " ", &nextToken);
	}

	m_pDomainLM->learnSentence(pSentence, nWordCount);
}

void CustomizerLMTrainer::trainSentence(char** pSentence, unsigned int nWordCount)
{
	m_pDomainLM->learnSentence(pSentence, nWordCount);
}

void CustomizerLMTrainer::finalize(istream& genericCountsIn, std::ostream& domainLMOut, ostream& combinedCountsOut, const string& tempDomainCounts, bool bDeleteTemp) 
{
	// We have to get the counts of of the Domain LM - it was trained by calling trainSentence() 
	// repeatedly and it is currently in memory
	// Write the Domain counts out to disk

	ofstream domainCountsOut(tempDomainCounts.c_str(), ios::binary);
	if (domainCountsOut.fail()) {
		char szError[1024];
		lw_snprintf(
			szError, 
			sizeof(szError), 
			"Cannot open temporary domain counts file for writing. File name: %s",
			tempDomainCounts.c_str()
			);
		throw Exception(ERR_IO, szError);
	}

	// Wrap the temporary counts file in a crypto stream
#ifndef LM_NO_COMMON_LIB
	SmartPtr<ostream> spDomainCountsOutEnc = CryptoStreamFactory::getInstance()->createOStream(domainCountsOut);
#else
	SmartPtr<ostream> spDomainCountsOutEnc = &domainCountsOut;
#endif
	m_pDomainLM->writeCounts(*(spDomainCountsOutEnc.getPtr()));
	spDomainCountsOutEnc = NULL;
	domainCountsOut.close();

	// Open the domain counts again for reading
	ifstream domainCountsIn(tempDomainCounts.c_str(), ios::binary);
	if (domainCountsIn.fail()) {
		char szError[1024];
		lw_snprintf(
			szError, 
			sizeof(szError), 
			"Cannot open temporary domain counts file for reading. File name: %s", 
			tempDomainCounts.c_str()
			);
		throw Exception(ERR_IO, szError);
	}

	// Wrap the temporary counts file in a crypto stream
#ifndef LM_NO_COMMON_LIB
	SmartPtr<istream> spDomainCountsInEnc = CryptoStreamFactory::getInstance()->createIStream(domainCountsIn, true);
#else 
	SmartPtr<istream> spDomainCountsInEnc = &domainCountsIn;
#endif

	// Write the domain language model
	m_pDomainLM->write(domainLMOut, LangModel::FORMAT_BINARY_SA);
//	m_pDomainLM->write(domainLMOut, LangModel::FORMAT_TEXT);

	CountsReader genericCountsReader(&genericCountsIn);
	CountsReader domainCountsReader(spDomainCountsInEnc.getPtr());

	// The words will be read here
	char* genericCountsNGram[MAX_SUPPORTED_ORDER];
	char* domainCountsNGram[MAX_SUPPORTED_ORDER];

	unsigned int nMaxOrder = domainCountsReader.getMaxOrder();

	// Write the counts file header for combined counts
	
	CountsReader::writeHeader(combinedCountsOut, nMaxOrder);

	// Now scan the domain counts and extract all generic counts for matching n-grams
	// We will store the 2 counts side by side
	// The count files are sorted by n-gram so we can perform comparisons
	for (unsigned int nOrder = 1; nOrder <= nMaxOrder; nOrder++) {
		// Skip into to file to the right n-gram order
		bool bHasGenericCounts = genericCountsReader.moveOrder(nOrder);
		bool bHasDomainCounts = domainCountsReader.moveOrder(nOrder);

		// Write the order header in the output file
		CountsReader::writeNGramHeader(combinedCountsOut, nOrder);
		while (bHasDomainCounts) {
			domainCountsReader.getCurrentNGram(domainCountsNGram);

			// Advance generic counts until >= current domain counts
			genericCountsReader.getCurrentNGram(genericCountsNGram);
			while (bHasGenericCounts && (1 == domainCountsReader.compare(genericCountsNGram))) {
				bHasGenericCounts = genericCountsReader.moveNext();
				if (bHasGenericCounts) {
					genericCountsReader.getCurrentNGram(genericCountsNGram);
				};
			};

			if (bHasGenericCounts) {
				// Compare with generic counts for a match
				switch(domainCountsReader.compare(genericCountsNGram)) {
				case 1:
					// This should not happen as we advanced generic counts to >= domain counts
					break;
				case 0:
					// The n-grams match. Store this value
					CountsReader::writeNGram(
						combinedCountsOut, nOrder, domainCountsNGram, 
						genericCountsReader.getCurrentCount(), domainCountsReader.getCurrentCount()
					);
					bHasGenericCounts = genericCountsReader.moveNext();
					break;
				case -1:
					// Generic counts are "ahead". That means we have some domain counts
					// that don't exist in generic counts. We store this pair having 
					// a 0 for generic counts
					CountsReader::writeNGram(
						combinedCountsOut, nOrder, domainCountsNGram,
                        0, domainCountsReader.getCurrentCount()
					);
					break;
				}
			}
			else {
				// No more generic counts. Just write the domain counts.
				CountsReader::writeNGram(
					combinedCountsOut, nOrder, domainCountsNGram,
					0, domainCountsReader.getCurrentCount()
				);
			}
		
			bHasDomainCounts = domainCountsReader.moveNext();
		}
	}

	CountsReader::writeFooter(combinedCountsOut);

	spDomainCountsInEnc = NULL;
	domainCountsIn.close();

	if (bDeleteTemp) {
		remove(tempDomainCounts.c_str());
	}
}

void CustomizerLMTrainer::writeLangModel(ostream& lmModelFile) const
{
}

void CustomizerLMTrainer::writeCounts(ostream& countsFile) const
{
}

} // namespace LW
