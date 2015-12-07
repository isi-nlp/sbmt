// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include <Common/stldefs.h>
#include "LangModelKN.h"

#include <vector>
#include <fstream>
#include <stdio.h>

#ifndef _MSC_VER
    #define sscanf_s sscanf
#endif

#include "Common/os-dep.h"
#include "Common/ErrorCode.h"
#ifndef LM_NO_COMMON_LIB
#include "Common/Crypto/CryptoStreamFactory.h"
#endif
#include "Common/Serializer/Serializer.h"
#include "Common/Util.h"

#include "LMVersion.h"

using namespace std;

namespace LW {

	/**
	When we convert a language model to a sorted array binary serialization
	we create temporary streams, one stream per order.
	*/
	struct OrderStreamInfo {
	public:
		/** This class does not assume ownership of the output stream */
		OrderStreamInfo(ostream* pOut, const string& sFileName) {
			m_nCount = 0;
			m_pOut = pOut;
			m_sFileName = sFileName;
		}
		~OrderStreamInfo() {
		}
	public:
		// Temporary (opened) stream to write to (the file name for the stream is below)
		ostream* m_pOut;
		// Temporary file name to write to
		string m_sFileName;
		// Total number of probability entries
		// This is also used by the lower order n-grams as "current child offset"
		unsigned int m_nCount;
	};

	class VocabTranslator : public KeyTranslator<LWVocab::WordID>
	{
	public:
		VocabTranslator(LWVocab* pVocab) {
			m_pVocab = pVocab;
		}
		virtual const char* translate(LWVocab::WordID key) {
			return m_pVocab->getWord(key).c_str();
		}
	private:
		LWVocab* m_pVocab;
	};




/*
* Structure used for quick sorting children of a node
*/
template <class T>
struct SortStruct
{
	const char* m_pWord; // This must be the first element, as the struct is cast by compareStrings to (char*)
	T* m_pNode;
};
typedef SortStruct<TrieNode<LWVocab::WordID, int> > CountSortStruct;

/// Comparison function for quick sorting. The argumets are SortStruct<> structures.
int compareStrings(const void* pSort1, const void* pSort2)
{
	return strcmp(((CountSortStruct*) pSort1)->m_pWord, ((CountSortStruct*) pSort2)->m_pWord);
}


// Replaces -99 with -infinity
inline LangModel::ProbLog ReadProb(const char* szDouble)
{
	LangModel::ProbLog dValue = static_cast<LangModel::ProbLog> (atof(szDouble));
	if (dValue <= (LangModel::ProbLog) -98) {
		return PROB_LOG_ZERO;
	}
	else {
		return dValue;
	}
}


// Replace -infinity with -99
inline LangModel::ProbLog WriteProb(LangModel::ProbLog dValue)
{
	if (dValue == PROB_LOG_ZERO) {
		return -99;
	}
	else {
		return dValue;
	}
}


LangModelKN::LangModelKN(LWVocab* pVocab, const LangModelParams& params)
:m_counts(0), m_probs(0)
{
	//assert(pVocab);

	m_bDiscountsComputed = false;
	m_nWordCount = 0;
	m_pVocab = pVocab;
	//m_nMaxOrder = nMaxOrder;
	m_params = params;
}

LangModelKN::~LangModelKN()
{
}

void LangModelKN::clear()
{
	m_counts.clear();
	m_probs.clear();
}

void LangModelKN::learnSentence(iterator sentence, unsigned int nSentenceSize)
{
    word_type *pSentence=const_cast<word_type *>(sentence);
	static CountIncrementor<int> incrementor;

	m_bDiscountsComputed = false;

	// Compute counts up to max n-gram order
	for (unsigned int i = 0; i < nSentenceSize; i++) {
		unsigned int nOrder = getMaxOrder();
		// Make sure we don't exceed the end of the sentence
		if (nSentenceSize - i < nOrder) {
			nOrder = nSentenceSize - i;
		}

		m_counts.insertKey(pSentence + i, nOrder, incrementor);
		m_nWordCount++;
	}
}

LangModel::Prob LangModelKN::computeTrigramProb(iterator trigramArray) 
{
	return 0;
}


void LangModelKN::computeDiscount(KNDiscount& discount, unsigned int nOrder)
{
	unsigned int n1 = 0;
	unsigned int n2 = 0;
	unsigned int n3 = 0;
	unsigned int n4 = 0;

	//assert(nOrder >= 1);
	//assert(nOrder <= getMaxOrder());

	LWVocab::WordID pNGram[MAX_SUPPORTED_ORDER];
	TrieCountIterator<LWVocab::WordID, int> it(&m_counts, pNGram, nOrder);
	int* pnCount;
	while ((pnCount = it.next())) {
		// Ignore non events
		if (!m_pVocab->isNonEvent(pNGram[nOrder-1])) {
			switch (*pnCount) {
				case 1: 
					n1++;
					break;
				case 2:
					n2++;
					break;
				case 3:
					n3++;
					break;
				case 4:
					n4++;
					break;
			}
		}
	}

	// Make sure none of them is 0
	// This is just a quick fix to be able to run with small training files for testing
	// Normally, we should stop if any of the counts is 0
	if (n1 == 0 || n2 == 0 || n3 == 0 || n4 == 0) {
		return;
		//n1++;
		//n2++;
		//n3++;
		//n4++;
	}

    double y = (double) n1 / (n1 + 2 * n2);

    discount.d1			= 1 - 2 * y * n2 / n1;
    discount.d2			= 2 - 3 * y * n3 / n2;
    discount.d3plus		= 3 - 4 * y * n4 / n3;

	//// TODO: Get the min count from somewhere. 
	//// For now we just hard code it to be the same as SRI defaults
	//switch (nOrder) {
	//case 1:
	//	discount.minCount = 1;
	//	break;
	//case 2:
	//	discount.minCount = 1;
	//	break;
	//case 3:
	//	discount.minCount = 2;
	//	break;
	//default:
	//	discount.minCount = 2;
	//	break;
	//}
}

void LangModelKN::computeDiscounts() 
{
	LWVocab::WordID pNGram[MAX_SUPPORTED_ORDER];

	// Fix the counts first
	for (unsigned int nOrder = 1; nOrder < getMaxOrder(); nOrder++) {
		{
			// Make count 0 for all n-grams that do not start with <s>
			TrieCountIterator<LWVocab::WordID, int> it(&m_counts, pNGram, nOrder);
			int* pnCount;
			while ((pnCount = it.next())) {
				if (pNGram[0] != LWVocab::START_SENTENCE) {
					*pnCount = 0;
				}
			}
		}

		{
			// for abc increment c(bc) for each unique a
			TrieCountIterator<LWVocab::WordID, int> it(&m_counts, pNGram, nOrder + 1);
			int* pnCount;
			int* pnLowCount;
			while ((pnCount = it.next())) {
				if (*pnCount > 0 && pNGram[1] != LWVocab::START_SENTENCE) {
					pnLowCount = m_counts.findPayload(pNGram + 1, nOrder);
					if (pnLowCount) {
						(*pnLowCount)++;
					}
				}
			}
		}
	}

	for (unsigned int i = 1; i <= getMaxOrder(); i++) {
		computeDiscount(m_discounts[i], i);
	}

	m_bDiscountsComputed = true;
}

double LangModelKN::getDiscount(int nCount, KNDiscount& discount, unsigned int nOrder)
{
	if (nCount < 1) {
		// Leave count unmodified
		return 1;
	}
	//if (nCount < discount.minCount) {
	if (nCount < (int) m_params.m_nCountCutoffs[nOrder]) {
		// Pretend it did not show up at all
		return 0;
	}

	// The switch statement does not return directly as the compiler generates
	// an "Internal compiler error" in release mode (???)
	double dReturn = 0;
	switch (nCount) {
		case 1:
			dReturn =  (nCount - discount.d1) / nCount;
			break;
		case 2:
			dReturn = (nCount - discount.d2) / nCount;
			break;
		default:
			dReturn =  (nCount - discount.d3plus) / nCount;
			break;
	}

	return dReturn;
}

double LangModelKN::getLowerOrderWeight(
	int nTotalCount, 
	int nObservedVocab, 
	int nMin2Vocab, 
	int nMin3Vocab, 
	KNDiscount& discount)
{
	return 
	(
		discount.d1 * (nObservedVocab - nMin2Vocab)
		+ discount.d2 * (nMin2Vocab - nMin3Vocab) 
		+ discount.d3plus * nMin3Vocab
	) 
	/ nTotalCount;
}

/**
* Called by LangModelImpl after learning is finished
*/
void LangModelKN::prepare() 
{
	//// REMOVE ME
	//VocabTranslator translator(m_pVocab);
	//m_counts.dump(cerr, "", translator);

	bool bInterpolate = true;
    char *noInterp = 0;
    size_t sz;
    _dupenv_s(&noInterp, &sz, "NO_LM_INTERP");
	if (noInterp) {
		bInterpolate = false;
        free(noInterp);
	}

	computeDiscounts();

	unsigned int nVocabSize = static_cast<unsigned int>(m_pVocab->getWordCount());
	// Adjust for <s> </s> <unk>
	if (nVocabSize > 3) {
		nVocabSize -= 3;
	}

	// For each n-gram order 
	LWVocab::WordID pNGram[MAX_SUPPORTED_ORDER];
	for (unsigned int nOrder = 1; nOrder <= getMaxOrder(); nOrder++) {
		// Enumerate all lower order n-grams
		TrieNodeHorzIterator<LWVocab::WordID, int> lowIter(&m_counts, pNGram, nOrder - 1);
		TrieNode<LWVocab::WordID, int>* pNode;
		while ((pNode = lowIter.next())) {
			// Enumerate all words following the key for this node
			LWVocab::WordID pWord[1];
			TrieCountIterator<LWVocab::WordID, int> wordIter(pNode, pWord, 1);
			int* pnWordCount;
			int nTotalCount = 0;
			int nUniqueCount = 0;
			int nTwoOrMoreCount = 0;
			int nThreeOrMoreCount = 0;

			// First time calculate counts only 
			while ((pnWordCount = wordIter.next())) {
				if (!m_pVocab->isNonEvent(pWord[0])) {
					nTotalCount += *pnWordCount;
					nUniqueCount++;
					if (*pnWordCount >= 2) {
						nTwoOrMoreCount++;

						if (*pnWordCount >= 3) {
							nThreeOrMoreCount++;
						}
					}
				};
			}


			// Loop and attempt fixing until the sum of probabilities is less than 1
			bool bProbTotalOk = true;
			do {
				// Now calculate probabilities
				double dTotalProb = 0;
				wordIter.reset();
				while ((pnWordCount = wordIter.next())) {
					double dDiscount;
					LangModel::ProbLog dProbLog;
					if (m_pVocab->isNonEvent(pWord[0])) {
						// We give this event 0 probability. 
						// This probability will be adjusted later on by redistribution.
						dDiscount = 1;
						dProbLog = PROB_LOG_ZERO;
					}
					else {
						// This is a fix by Manuel. When we are performing n-gram counts on a file
						// this fix is not necessary, but when we read the counts from an outside
						// source, nTotalCount could be 0. 
						// e.g. C(ab) will be 0 if there are no recorded n-grams of the form xab (they were removed by cutoffs)
						// again, this will never happen if we perform the counts ourselves
						if (0 == nTotalCount) {
							// This will avoid storing the probability for this n-gram (see below)
							//dDiscount = 0;
                                                    goto prune_ngram; // Jonathan Graehl: avoid uninit. var warning w/ -Wall
                                                    
						}
						else {
							// Get the discount
							dDiscount = getDiscount(*pnWordCount, m_discounts[nOrder], nOrder);
							LangModel::Prob dProb = static_cast<LangModel::Prob> (dDiscount * *pnWordCount / nTotalCount);

							if (bInterpolate) {
								// Interpolation
								double dLowerOrderWeight = getLowerOrderWeight(
									nTotalCount, nUniqueCount, nTwoOrMoreCount, nThreeOrMoreCount, m_discounts[nOrder]);

								double dLowerOrderProb;
								if (nOrder > 1) {
									dLowerOrderProb = getContextProb(pNGram + 1, nOrder - 1);
								}
								else {
									dLowerOrderProb = -log10((double) nVocabSize);
								}

								dProb += dLowerOrderWeight * LangModel::ProbLogToProb(dLowerOrderProb);
							}
							dProbLog = LangModel::ProbToProbLog(dProb);
							// Compute total probability for verification
							dTotalProb += dProb;
						}
					}

					// Store probability
					// Complete the key
					if (0 == dDiscount) {
                                        prune_ngram:
                                            pNGram[nOrder - 1] = pWord[0];
                                            // Remove probability from the trie to save space
						m_probs.removeKey(pNGram, nOrder);
					}
					else {
                                            pNGram[nOrder - 1] = pWord[0];
                                            // Insert the key
                                            ProbNode probNode(dProbLog);
                                            m_probs.insertKey(pNGram, nOrder, probNode);
					}
				}
				
				// This is a hack credited to Doug Paul (by Roni Rosenfeld in
				// his CMU tools).  It may happen that no probability mass
				// is left after totalling all the explicit probs, typically
				// because the discount coefficients were out of range and
				// forced to 1.0.  To arrive at some non-zero backoff mass
				// we try incrementing the denominator in the estimator by 1.
				if (dTotalProb > 1 - PROB_EPSILON) {
					nTotalCount++;
					bProbTotalOk = false;
				}
				else {
					bProbTotalOk = true;
				}
			} while (!bProbTotalOk);

		}
		// Do not compute BOWs for 3-grams
		computeBOW(nOrder);
	}

	// Fix by Manuel, approved by Daniel
	// In order to penalize <unk> words, we give them a sufficiently low probability
	// The one we have computed so far is now low enough the the Lang Model likes <unk> words too much
	LWVocab::WordID key = LWVocab::UNKNOWN_WORD;
	TrieNode<LWVocab::WordID, ProbNode>* pUnknown = m_probs.findNode(&key, 1);
	if (pUnknown) {
		pUnknown->m_payload.m_prob = -10;
	}
}


LangModel::ProbLog LangModelKN::getContextProb(iterator pContext, unsigned int nOrder) 
{
	// We only support up to getMaxOrder() n-grams
	if (nOrder > getMaxOrder()) {
		unsigned int nOldOrder = nOrder;
		nOrder = getMaxOrder();
		// If the size of the context is reduced, move the starting pointer appropriately
		pContext = pContext + (nOldOrder - nOrder);
	}

	LangModel::ProbLog dReturn;
	ProbNode* probNode = findProbNode(pContext, nOrder);
	if (probNode) {
		// Found the probability for n-gram
		dReturn = probNode->m_prob;
	}
	else if (nOrder > 0) {
		// Prob not found
		// Find Back Off Weight
		ProbNode* pParent = findProbNode(pContext, nOrder - 1);
		if (pParent) {
			// Recurse to a shorter n-gram
			dReturn = pParent->m_bow + getContextProb(pContext + 1, nOrder - 1);
		}
		else {
			// No BOW. Just use the probablity of the shorter context
			dReturn = getContextProb(pContext + 1, nOrder - 1);
		}
	}
	else {
		// No probability found. None of the words have been seen during training, or they have been discarded
		dReturn = PROB_LOG_ZERO; // Minus infinity
	}

	return dReturn;
}

void LangModelKN::dump(ostream& out)
{
	LWVocab::WordID key[MAX_SUPPORTED_ORDER];
	TrieNode<LWVocab::WordID, ProbNode>* pNode;
	for (unsigned int i = 1; i <= getMaxOrder(); i++) {
		TrieNodeHorzIterator<LWVocab::WordID, ProbNode> it(&m_probs, key, i);
		while((pNode = it.next())) {
			// Dump n-gram as Word IDs
			out << "[";
			unsigned int j;
			for (j = 0; j < i; j++) {
				out << key[j] << " ";
			}
			out << "] ";

			// Dump each n-gram
			for (j = 0; j < i; j++) {
				const string& sWord = m_pVocab->getWord(key[j]);
				out << sWord.c_str() << " ";
			}
			// Dump probability
			out << pNode->m_payload.m_prob << " ";

			// Dump BOW
			if (pNode->m_payload.m_bow != 0) {
				out << pNode->m_payload.m_bow;
			}

			out << endl;
		}

	}
}

void LangModelKN::read(std::istream& inStream, LangModel::ReadOptions nOptions) {
#ifndef LM_NO_COMMON_LIB
	// The auto_pointer will take care of automatically destructing the stream in case somebody throws
	auto_ptr<istream> pCryptoStream(CryptoStreamFactory::getInstance()->createIStream(inStream, true));
	// Use a reference, looks nicer in the code
	istream& in = *pCryptoStream;
#else 
	istream& in = inStream;
#endif

	// Try to detect the type of the stream
	char chFirstByteBinary = (char) (VERSION_MAGIC_NUMBER & 0xFF);
	char ch = in.peek();
	if (chFirstByteBinary == ch) {
		readBinary(in, nOptions);
	}
	else {
		readText(in, nOptions);
	}
}

void LangModelKN::readText(std::istream& in, LangModel::ReadOptions nOptions)
{
	// Current order of ngram we are reading
//	unsigned int nOrder = 0;
	// Input buffer
	char line[1024];
	// The number of counts, by order
	unsigned int counts[MAX_SUPPORTED_ORDER + 1];
	// The current order we are processing
	unsigned int nCurrentOrder=0; // note: will always be initialized before use provided parse BEFORE event called first, but shut up, please -Wall
	// The current status of the parse
	ParseStatus nParseStatus = PS_BEFORE_DATA;

	// Initialize counts with 0
	for (unsigned int i = 0; i <= MAX_SUPPORTED_ORDER; i++) {
		counts[i] = 0;
	}

	// Clear the probability trie
	m_probs.clear();

	// We read the max order from the file
	m_params.m_nMaxOrder = 0;

//        cerr << "reading from input" << endl;

//        int i = 0;
   unsigned int nLineNumber = 0;
	while (in && nParseStatus!=PS_DONE) {
//                if (!(i%1000)) { cerr << "."; }
//                i++;
		safeGetLine(in, line, sizeof(line));
		nLineNumber++;



		// Append NULL char at the end, just in case
		line[sizeof(line) - 1] = '\0';

		// We are in binary mode and we might receive <cr> chars. Remove them
		for (char* ptr = line; *ptr != '\0'; ptr++) {
			if (*ptr == 13) {
				*ptr = '\0';
			}
		}

		switch (nParseStatus) {
			case PS_BEFORE_DATA:
				{
					if (0 == strncmp(line, "\\data\\", 6)) {
						// Found "\data\"
						nParseStatus = PS_READING_COUNTS;
					}
				}
				break;
			case PS_READING_COUNTS:
				{
					// Stop at the first empty line
					if(0 == strlen(line)) {
						// Done with reading counts. Move to the next status
						nParseStatus = PS_BEFORE_READING_NGRAM;
					}
					else {
						// Read the count
						unsigned int nReadOrder = 0;
						unsigned int nReadCount = 0;
						sscanf_s(line, "ngram %d=%d", &nReadOrder, &nReadCount);
						if (nReadOrder <= 0 ||  nReadCount <= 0 || nReadOrder > MAX_SUPPORTED_ORDER) {
							// TODO: Throw "Invalid line: expected 'ngram x-count'
						}
						counts[nReadOrder] = nReadCount;
//                                                cerr << "got counts: " << nReadCount << endl;
					}
				}
				break;
			case PS_BEFORE_READING_NGRAM:
				{
					unsigned int nReadOrder = 0;
					// If we did not find the \ keep going. The "x-ngrams:" line not reached yet
					if (0 == strncmp(line, "\\end\\", 5)) {
						// "end of LM" marker found
						nParseStatus = PS_DONE;
					}
					else if (0 == strncmp(line, "\\", 1)) {
						sscanf_s(line, "\\%d-grams:", &nReadOrder);
						if (0 == nReadOrder || nReadOrder > MAX_SUPPORTED_ORDER) {
							// TODO: Throw "Invalid line: expected '\\x-grams:'
						}
						else {
							// The following lines are going to be the probabilities for order nReadOrder
							nCurrentOrder = nReadOrder;
							// Increase max order if necessary, as we find it in the file
							if (m_params.m_nMaxOrder < nReadOrder) {
								m_params.m_nMaxOrder = nReadOrder;
							}

							nParseStatus = PS_READING_NGRAM;
						}
					}
				}
				break;
			case PS_READING_NGRAM:
				{
					// First empty line says we are done
					if (0 == strlen(line)) {
						nParseStatus = PS_BEFORE_READING_NGRAM;
					}
					else {
						LWVocab::WordID key[MAX_SUPPORTED_ORDER];
						// Get the probability
                        char* nextToken;
						char* szProb = strtok_s(line, " \t", &nextToken);
						// Now get the n-gram words
						for (unsigned int i = 0; i < nCurrentOrder; i++) {
							char* szWord = strtok_s(NULL, " \t", &nextToken);
							if (NULL == szWord) {
								char szError[4096];
								lw_snprintf(szError, sizeof(szError), "Bad text Language Model: expected 'n-gram' at line %d", nLineNumber);
								throw Exception(ERR_IO, szError);
							}
							// Make sure every word is in the vocabulary and construct the key to this n-gram
							key[i] = m_pVocab->insertWord(szWord);
						}
						// If the caller just wanted to extract the vocabulary, skip over probabilities
						if (LangModel::READ_VOCAB_ONLY != nOptions) {
							// Do we have a BOW?
							char* szBOW = strtok_s(NULL, " \t", &nextToken);

							if (NULL == szProb) {
								szProb = "";
							}
							if (NULL == szBOW) {
								szBOW = "";
							}

							// Convert 0 probabilities (written -99, the same SRI does)
							ProbNode probNode;
							probNode.m_prob = ReadProb(szProb);
							probNode.m_bow = ReadProb(szBOW);

							// Insert in the trie
							m_probs.insertKey(key, nCurrentOrder, probNode);
						}
					}
				}
				break;
                default:
                    throw Exception(ERR_IO,"Impossible LangModelKN parse status");
                    
		}
	}

	// Check consistency
	if (PS_DONE != nParseStatus) {
		throw Exception(ERR_IO, "Text language model not properly terminated by '\\end\\'.");
	}
}

void LangModelKN::readBinary(std::istream& inStream, LangModel::ReadOptions nOptions)
{

// The values below are for reading FORMAT_BINARY_TRIE
//	[C1C1] [version]
//	[C1C2] [Vocabulary Count]
//	[Vocab ID] [Vocab String]
//	...
//	[C1C3] [N-Gram Order] [N-Gram Count]
//	[Word ID]...[Word ID] [Prob - log, float] [Back Off] - log, float]
//	...
//	[C1C3] [0] // 0 means end

	// Clear the probability trie
	m_probs.clear();
	// TODO: We should clear the vocabulary as well here. Normally this is not important
	// as the language model is only read once.
	// m_pVocab->clear();

	ISerializer in(&inStream);

	// Read magic number
	unsigned int nMagic;
	in.read(nMagic);
	if (nMagic != VERSION_MAGIC_NUMBER) {
		throw InvalidLangModelVersionException(ERR_IO, "Invalid Language Model file: Bad version magic number.");
	}

	// Read version
	unsigned int nVersion;
	in.read(nVersion);
	if (nVersion != CURRENT_VERSION) {
		throw InvalidLangModelVersionException(ERR_IO, "Unsupported version number in Language Model file.");
	}

	// Read vocabulary header
	in.read(nMagic);
	if (nMagic != VOCAB_MAGIC_NUMBER) {
		throw Exception(ERR_IO, "Invalid Language Model file: Bad Vocabulary magic number.");
	}

	// Read Vocabulary count
	unsigned int nVocabCount = 0;
	in.read(nVocabCount);

//	cerr << "Reading " << nVocabCount << " vocabulary words..." << endl;
	LWVocab::WordID nWordID;
	string sWord;
	for (unsigned int i = 0; i < nVocabCount; i++) {
		
		in.read(nWordID);
		in.read(sWord);

//		if (i%1000)
//			cerr << ".";
//		cerr << sWord << endl;

		m_pVocab->insertWord(sWord, nWordID);
	}

//	cerr << endl;
	// Test if the caller only wanted the vocabulary extracted (no probabilities)
	if (LangModel::READ_VOCAB_ONLY != nOptions) {
//		cerr << "Reading probabilities..." << endl;
		bool bDone = false;
		while (!bDone) {
			unsigned int nGramOrder;
			unsigned int nGramCount;

			// Read magic number for probabilities
//			cerr << "Reading magic number for probs..." << endl;
			in.read(nMagic);
			if (nMagic != PROB_MAGIC_NUMBER) {
				throw Exception(ERR_IO, "Invalid Language Model file: Bad magic number in probability header.");
			}

//			cerr << "Reading n-gram order for probs..." << endl;
			in.read(nGramOrder);
	//		cerr << "Order: " << nGramOrder << endl;
	//		cerr << "Reading n-gram count for probs..." << endl;
			in.read(nGramCount);

//			cerr << "Order: " << nGramOrder << " Count: " << nGramCount << endl;
			if (nGramOrder > MAX_SUPPORTED_ORDER) {
				char buffer[500];
				sprintf_s(buffer, 500, "n-gram order (%d) found in the Language Model file is higher than maximum supported order (%d)", nGramOrder, (unsigned int) MAX_SUPPORTED_ORDER);
				throw Exception(ERR_IO, buffer);
			}

			if (0 == nGramOrder) {
				bDone = true;
			}
			else {
				unsigned int wordIDArray[MAX_SUPPORTED_ORDER];
				float dProb;
				float dBOW;

				for (unsigned int i = 0; i < nGramCount; i++) {
					for (unsigned int j = 0; j < nGramOrder; j++) {
						// Read n-gram words
						in.read(*(wordIDArray + j));
					}
				
					in.read(dProb);
					in.read(dBOW);

					// By convention -98 or less means PROB_LOG_ZERO
					if (dProb <= -98) {
						dProb = PROB_LOG_ZERO;
					};

					ProbNode probNode;
					probNode.m_prob = dProb;
					probNode.m_bow = dBOW;
					m_probs.insertKey(wordIDArray, nGramOrder,  probNode);
				}
			}
//			cerr << "Done reading order: " << nGramOrder << endl;
		}
	}
}



void LangModelKN::readCounts(std::istream& in)
{
	// Current order of ngram we are reading
//	unsigned int nOrder = 0;
	// Input buffer
	char line[1024];
	// The number of counts, by order
	unsigned int counts[MAX_SUPPORTED_ORDER + 1];
	// The current order we are processing
	unsigned int nCurrentOrder = 0; // to shut up -Wall (relies on parse BEFORE happening before)
	// The current status of the parse
	ParseStatus nParseStatus = PS_BEFORE_DATA;

	// Initialize counts with 0
	for (int i = 0; i <= MAX_SUPPORTED_ORDER; i++) {
		counts[i] = 0;
	}

	// Clear the probability trie
	m_probs.clear();
	m_counts.clear();

	// Max order will be read from the file
	m_params.m_nMaxOrder = 0;

	while (in) {
		safeGetLine(in, line, sizeof(line));
		// Append NULL char at the end, just in case
		line[sizeof(line) - 1] = '\0';

		switch(nParseStatus) {
			case PS_BEFORE_DATA:
				if (0 == strncmp(line, "\\data\\", 6)) {
					// Found "\data\"
					nParseStatus = PS_READING_COUNTS;
				}
				break;
			case PS_READING_COUNTS:
				{
					// Stop at the first empty line
					if(0 == strlen(line)) {
						// Done with reading counts. Move to the next status
						nParseStatus = PS_BEFORE_READING_NGRAM;
					}
					else {
						// Read the count
						int nReadOrder = 0;
						int nReadCount = 0;
						sscanf_s(line, "ngram %d=%d", &nReadOrder, &nReadCount);
						if (nReadOrder <= 0 ||  nReadCount <= 0 || nReadOrder > MAX_SUPPORTED_ORDER) {
							// TODO: Throw "Invalid line: expected 'ngram x-count'
						}
						counts[nReadOrder] = nReadCount;
					}
				}
				break;
			case PS_BEFORE_READING_NGRAM:
				{
					unsigned int nReadOrder = 0;
					// If we did not find the \ keep going. The "x-ngrams:" line not reached yet
					if (0 == strncmp(line, "\\", 1)) {
						sscanf_s(line, "\\%d-grams:", &nReadOrder);
						if (0 == nReadOrder || nReadOrder > MAX_SUPPORTED_ORDER) {
							// TODO: Throw "Invalid line: expected '\\x-grams:'
						}
						else {
							// The following lines are going to be the probabilities for order nReadOrder
							nCurrentOrder = nReadOrder;
							// Increase max order as we find it in the file
							if (m_params.m_nMaxOrder < nReadOrder) {
								m_params.m_nMaxOrder = nReadOrder;
							}
							nParseStatus = PS_READING_NGRAM;
						}
					}
				}
				break;
			case PS_READING_NGRAM:
				{
					// First empty line says we are done
					if (0 == strlen(line)) {
						nParseStatus = PS_BEFORE_READING_NGRAM;
					}
					else {
						LWVocab::WordID key[MAX_SUPPORTED_ORDER];
						// Get the count
                        char* nextToken;
						char* szCount = strtok_s(line, " \t", &nextToken);
						// Now get the n-gram words
						for (unsigned int i = 0; i < nCurrentOrder; i++) {
							char* szWord = strtok_s(NULL, " \t", &nextToken);
							if (NULL == szWord) {
								// TODO: Throw "Expected n-gram"
							}
							// Make sure every word is in the vocabulary and construct the key to this n-gram
							key[i] = m_pVocab->insertWord(szWord);
						}

						if (NULL == szCount) {
							szCount = "";
						}

						int nCount = atoi(szCount);

						// nCount should always be > 0, but test, just in case
						// an invalid file is passed. Inserting a 0 count 
						// would cause problems when computing probabilities
						if (nCount > 0) {
							// Insert in the trie
							m_counts.insertKey(key, nCurrentOrder, nCount);
						}
					}
				}
				break;
                default:
                    throw Exception(ERR_IO,"Impossible LangModelKN parse status");            
		}
	}

	// Check consistency
	if (PS_BEFORE_READING_NGRAM != nParseStatus) {
		// Something went really wrong
		// TODO: Throw "Invalid input file".
	}
}

void LangModelKN::write(std::ostream& out, LangModel::SerializeFormat nFormat) 
{
	switch (nFormat) {
		case LangModel::FORMAT_TEXT:
			writeText(out);
			break;
		case LangModel::FORMAT_BINARY_TRIE:
			writeBinary(out);
			break;
		case LangModel::FORMAT_BINARY_SA:
			writeBinary2(out);
			break;
		default:
			throw Exception(ERR_IO, "Unsupported file format for serialization.");
	}
}

void LangModelKN::writeText(std::ostream& out)
{
	out << endl;
	out << "\\data\\" << endl;
//	bool bDone = false;
	// Write out the counts for each n-gram
	unsigned int nOrder = 1;
	unsigned int nCount;
	do {
		nCount = m_probs.getChildCount(nOrder);
		if (nCount > 0) {
			out << "ngram " << nOrder << "=" << nCount << endl;
			nOrder++;
		}
	} while (nCount > 0);
	out << endl;

	unsigned int nMaxOrder = nOrder - 1;

	// Write the probabilities and BOW
	LWVocab::WordID key[MAX_SUPPORTED_ORDER];
	for (nOrder = 1; nOrder <= nMaxOrder; nOrder++) {
		out << "\\" << nOrder << "-grams:" << endl;
		// Write out each n-gram
		TrieNodeHorzIterator<LWVocab::WordID, ProbNode> it(&m_probs, key, nOrder);
		TrieNode<LWVocab::WordID, ProbNode>* pNode;
		while ((pNode = it.next())) {
			// Write probability
			out << WriteProb(pNode->m_payload.m_prob);
			// Write the actual n-gram - use Vocab to translate the index into a real word
			for (unsigned int i = 0; i < nOrder; i++) {
				out << "\t" << m_pVocab->getWord(key[i]);
			}
			// Write BOW (if any);
			if (pNode->m_payload.m_bow != 0) {
				out << "\t" << WriteProb(pNode->m_payload.m_bow);
			}
			out << endl;
		}
		out << endl;
	}

	out << "\\end\\" << endl;
}

void LangModelKN::writeBinary(std::ostream& outStream)
{
	// See readBinary() for the data structure description

	OSerializer out(&outStream);

	// Write header
	out.write((unsigned int) VERSION_MAGIC_NUMBER);
	out.write((unsigned int) CURRENT_VERSION);

	// Write Vocabulary Header
	out.write((unsigned int) VOCAB_MAGIC_NUMBER);
	out.write((unsigned int) m_pVocab->getWordCount());

	// Write Vocabulary
	VocabIterator vocabIterator(m_pVocab);
	LWVocab::WordID nWordID;
	while (LWVocab::INVALID_WORD != (nWordID = vocabIterator.next())) {
		const string& sWord = m_pVocab->getWord(nWordID);
		// write it out
		out.write(nWordID);
		out.write(sWord);
	}

	// What is the highest order currently supported?
	unsigned int nCount = 123; // Some non-zero value
	unsigned int nMaxOrder;
	for (nMaxOrder = 1; nCount != 0; nMaxOrder++) {
		nCount = m_probs.getChildCount(nMaxOrder);
	}

	// Write all probabilities for each order
	bool bDone = false;
	for (unsigned int nOrder = 1; (nOrder <= nMaxOrder) & !bDone; nOrder++) {
		nCount = m_probs.getChildCount(nOrder);

		if (nCount > 0) {
			// Write order header
			out.write((unsigned int) PROB_MAGIC_NUMBER);
			out.write((unsigned int) nOrder);
			out.write((unsigned int) nCount);

			// Write all n-grams
			LWVocab::WordID key[MAX_SUPPORTED_ORDER];
			TrieNodeHorzIterator<LWVocab::WordID, ProbNode> it(&m_probs, key, nOrder);
			TrieNode<LWVocab::WordID, ProbNode>* pNode;
			// Iterate through all nodes having order nOrder
			while((pNode = it.next())) {
				// Write the words
				for (unsigned i = 0; i < nOrder; i++) {
					out.write((unsigned int) key[i]);
				}

				float dProb = pNode->m_payload.m_prob;
				float dBOW = pNode->m_payload.m_bow;

				// Convert to -99 if PROB_LOG_ZERO
				if (dProb == PROB_LOG_ZERO) {
					dProb = -99;
				}
				// BOW is ok, does not need to be converted
	
				// Write the probabilities
				out.write((float) dProb);
				out.write((float) dBOW);
			}
			
		}
		else {
			// Exit, there are no n-grams for this order
			bDone = true;
		}
	}
	// Write the end of the probs
	out.write((unsigned int) PROB_MAGIC_NUMBER);
	// Order 
	out.write((unsigned int) 0);
	// Count
	out.write((unsigned int) 0);
}

int nodeSorterFunction(const void* p1, const void* p2)
{
	TrieNode<LWVocab::WordID, ProbNode>** ppNode1 = (TrieNode<LWVocab::WordID, ProbNode>**)p1;
	TrieNode<LWVocab::WordID, ProbNode>** ppNode2 = (TrieNode<LWVocab::WordID, ProbNode>**)p2;

	//TrieNode<LWVocab::WordID, ProbNode>* pNode1 = (TrieNode<LWVocab::WordID, ProbNode>*)(*p1);
	//TrieNode<LWVocab::WordID, ProbNode>* pNode2 = (TrieNode<LWVocab::WordID, ProbNode>*)*p2;

	if ((*ppNode1)->m_key > (*ppNode2)->m_key) {
		return 1;
	}
	else if ((*ppNode1)->m_key < (*ppNode2)->m_key) {
		return -1;
	}
	else {
		return 0;
	}
}


void writeBinaryNode2(vector<OrderStreamInfo>& vStreamInfo, TrieNode<LWVocab::WordID, ProbNode>& node, unsigned int nCurrentOrder)
{
	// Check if we exceeded the order. Should never happen, but we don't want to crash.
	if (vStreamInfo.size() < nCurrentOrder) {
		return;
	}

	typedef TrieNode<LWVocab::WordID, ProbNode> Node;
	typedef TrieNodeChildIterator<LWVocab::WordID, ProbNode> NodeIterator;
	typedef Sorter<Node, NodeIterator, nodeSorterFunction> NodeSorter;

	// Create a sorter to scan the node in sorted order
	TrieNodeChildIterator<LWVocab::WordID, ProbNode> it(&node);
	NodeSorter sorter(it);
	// Now the sorter will return the elements in sorted order

	OrderStreamInfo& info = vStreamInfo[nCurrentOrder - 1];
	// pParentInfo is a pointer to the same structure for one order lower 
	// This pointer might be NULL if current order is 1
	OrderStreamInfo* pChildInfo = NULL;
	if (nCurrentOrder < vStreamInfo.size()) {
		pChildInfo = &(vStreamInfo[nCurrentOrder]);
	}

	OSerializer out(info.m_pOut);

	Node* pChild;
	while ((pChild = sorter.next())) {
		// The default is "no children"
		unsigned int nChildrenOffset;

		// In order to have offset for children a node must not be of the highest order
		// and it must have children. getChildCount() would be enough, but we test the pointer too, just in case :-)
		if (pChildInfo) {
			nChildrenOffset = pChildInfo->m_nCount;
		}
		else {
			nChildrenOffset = 0;
		}

		// Convert to -99 if PROB_LOG_ZERO
		if (pChild->m_payload.m_prob == PROB_LOG_ZERO) {
			pChild->m_payload.m_prob = -99;
		}
		// BOW is ok, does not need to be converted

		// Write the key and payload to the stream
		out.write(pChild->m_key);
		out.write(pChild->m_payload.m_prob);
		out.write(pChild->m_payload.m_bow);
		out.write(nChildrenOffset);
		info.m_nCount++;

		// Recurse for every child (incrementing the order so the child
		// gets written on the corresponding stream for its order)
		writeBinaryNode2(vStreamInfo, *pChild, nCurrentOrder + 1);
	}
}


void LangModelKN::writeBinary2(std::ostream& outStream)
{
	// See readBinary() for the data structure description

	OSerializer out(&outStream);

	// Write header
	out.write((unsigned int) VERSION_MAGIC_NUMBER2);
//	out.write((unsigned int) CURRENT_VERSION2);
	out.write((unsigned int) CURRENT_VERSION2_1);

	out.write((unsigned int) getMaxOrder());

	// Write Vocabulary Header
	out.write((unsigned int) VOCAB_MAGIC_NUMBER2);
	out.write((unsigned int) m_pVocab->getWordCount());

	// Write Vocabulary
	VocabIterator vocabIterator(m_pVocab);
	LWVocab::WordID nWordID;
	while (LWVocab::INVALID_WORD != (nWordID = vocabIterator.next())) {
		const string& sWord = m_pVocab->getWord(nWordID);
		// write it out
		out.write(nWordID);
		out.write(sWord);
	}

	// What is the highest order currently supported?
	unsigned int nMaxOrder = 1;
	for (unsigned int i = 1; i <= MAX_SUPPORTED_ORDER; i++) {
		if (m_probs.getChildCount(i) > 0) {
			nMaxOrder = i;
		}
		else {
			// Force exit
			i = MAX_SUPPORTED_ORDER + 1;
		}
	}

	string sTempDir = m_params.m_sTempDir;
	// Make sure path ends with the file path separator
	if (!sTempDir.empty()) {
		if (FILE_PATH_SEPARATOR != sTempDir[sTempDir.length() - 1]) {
			sTempDir += FILE_PATH_SEPARATOR;
		}
	}

	// Create output streams for each order
	vector<OrderStreamInfo> infoVector;
	for (unsigned int i = 0; i < nMaxOrder; i++) {
		char szFileName[512];
		string sTempDirTemplate = sTempDir + "temp-bin%d.blm"; 
		lw_snprintf(szFileName, sizeof(szFileName)-1, sTempDirTemplate.c_str(), i + 1);
		ofstream* pOut = new ofstream(szFileName, ios::binary);
		OrderStreamInfo info(pOut, szFileName);
		infoVector.push_back(info);
	}
 
	// Start with the root node and write each order
	// on its own stream
	writeBinaryNode2(infoVector, m_probs, 1);

	// Close all temporary files
	for (size_t i = 0; i < infoVector.size(); i++) {
		infoVector[i].m_pOut->flush();
		delete infoVector[i].m_pOut;
	}

	// Reopen the streams and concatenate them for each order
	char concatBuffer[10000];
	for (size_t i = 0; i < infoVector.size(); i++) {
		// Write the signature for the corresponding order
		out.write((unsigned int) PROB_MAGIC_NUMBER2);
		// Write the n-gram order
		out.write((unsigned int) (i + 1));
		// Write the number of entries
		out.write((unsigned int) infoVector[i].m_nCount);
		// Now copy the binary stream as it was created by writeBinaryNode2()
		ifstream in(infoVector[i].m_sFileName.c_str(), ios::binary);
		if (in.fail()) {
			char szError[500];
			lw_snprintf(szError, sizeof(szError)-1, "Could not open temporary input file %s.", infoVector[i].m_sFileName.c_str());
			throw Exception(ERR_IO, szError);
		}
		while (in) {
			in.read(concatBuffer, sizeof(concatBuffer));
			outStream.write(concatBuffer, in.gcount());			
		}
	}
	// Write the end of the probs
	out.write((unsigned int) PROB_MAGIC_NUMBER2);
	// Order 
	out.write((unsigned int) 0);
	// Count
	out.write((unsigned int) 0);

	// Delete all temporary files
	for (size_t i = 0; i < infoVector.size(); i++) {
		remove(infoVector[i].m_sFileName.c_str());
	}

}

void writeNodeCounts(ostream& out, LWVocab& vocab, TrieNode<LWVocab::WordID, int>* pParent, vector<CountSortStruct>& parentKeys, unsigned int nLevel) 
{
	nLevel--;

	// Sort children
	// Get the children
	unsigned int nChildCount = pParent->getChildCount();
	CountSortStruct* pBuffer;
	// For speed, allocate on the stack if the size is reasonable
	CountSortStruct pStackBuffer[1024];
	// If the size is large, allocate on the heap
	CountSortStruct* pHeapBuffer = NULL;
	if (nChildCount > 1024) {
		pHeapBuffer = (CountSortStruct*) malloc(nChildCount * sizeof(CountSortStruct));
		pBuffer = pHeapBuffer;
	}
	else {
		pBuffer = pStackBuffer;
	}

	// Put the word keys in the array
	if (nChildCount > 0) {
		TrieNodeChildIterator<LWVocab::WordID, int> it(pParent);
		TrieNode<LWVocab::WordID, int>* pNode;
		CountSortStruct* pCurrent = pBuffer;
		while ((pNode = it.next())) {
			pCurrent->m_pWord = vocab.getWord(pNode->m_key).c_str();
			pCurrent->m_pNode = pNode;

			pCurrent++;
		}

		// Sort the array
		qsort(pBuffer, nChildCount, sizeof(CountSortStruct), compareStrings);
	}

	if (nLevel == 0) {
		// We arrived at the desired depth
		// Write all nodes
		CountSortStruct* pSortStruct = pBuffer;
		for (unsigned int i = 0; i < nChildCount; i++) {
			// Write count
			out << pSortStruct->m_pNode->m_payload;
			// Write words from parents
			for (size_t i = 0; i < parentKeys.size(); i++) {
				out << "\t" << parentKeys[i].m_pWord;
			}
			// Write current word
			out << "\t" << pSortStruct->m_pWord << endl;;

			pSortStruct++;
		}
	}
	else {
		// We have not reached the desired level yet
		for (unsigned int i = 0; i < nChildCount; i++) {
			// Put this node on the stack
			parentKeys.push_back(pBuffer[i]);

			// Recurse
			writeNodeCounts(out, vocab, pBuffer[i].m_pNode, parentKeys, nLevel); 

			// Cleanu the stack
			parentKeys.pop_back();
		}
	}


	// Delete temporary array (if allocated)
	if (pHeapBuffer) {
		free(pHeapBuffer);
	}
}

void LangModelKN::writeCounts(std::ostream& out) 
{
	out << endl;
	out << "\\data\\" << endl;
//	bool bDone = false;
	// Write out the counts for each n-gram
	unsigned int nOrder = 1;
	unsigned int nCount;
	do {
		nCount = m_counts.getChildCount(nOrder);
		if (nCount > 0) {
			out << "ngram " << nOrder << "=" << nCount << endl;
			nOrder ++;
		}
	} while (nCount > 0);
	out << endl;

	unsigned int nMaxOrder = nOrder - 1;

	// Write the counts for each n-gram
//	LWVocab::WordID key[MAX_SUPPORTED_ORDER];
	for (nOrder = 1; nOrder <= nMaxOrder; nOrder++) {
		out << "\\" << nOrder << "-grams:" << endl;

		// Initialize empty stack of parent node keys
		vector<CountSortStruct> parentKeys;

		// This will write out the keys for a specified deapth
		writeNodeCounts(out, *m_pVocab, &m_counts, parentKeys, nOrder);

		//// Write out each n-gram
		//TrieNodeIterator<Vocab::WordID, int> it(&m_counts, key, nOrder);
		//TrieNode<Vocab::WordID, int>* pNode;
		//while (pNode = it.next()) {
		//	// Write count
		//	out << pNode->m_payload;
		//	// Write the actual n-gram - use Vocab to translate the index into a real word
		//	for (unsigned int i = 0; i < nOrder; i++) {
		//		out << "\t" << m_pVocab->getWord(key[i]);
		//	}
		//	out << endl;
		//}
		out << endl;
	}

	out << "\\end\\" << endl;
}

unsigned int LangModelKN::getMaxOrder() const
{
	return m_params.m_nMaxOrder;
}


bool LangModelKN::computeBOW(unsigned int nOrder) 
{
	//assert(nOrder > 0);

	// Iterate through all nodes of order nOrder - 1
	LWVocab::WordID pNGram[MAX_SUPPORTED_ORDER];
	TrieNodeHorzIterator<LWVocab::WordID, ProbNode> it(&m_probs, pNGram, nOrder - 1);
	TrieNode<LWVocab::WordID, ProbNode>* pNode;
	while ((pNode = it.next())) {
		LangModel::Prob dNumerator = 1;
		LangModel::Prob dDenominator = 1;
		// Iterate through all words that are children of this node (their order is nOrder)
		// NOTE: We set the iterator to return keys in the same pNGram, right after the keys
		// returned by the outer iterator. This way pNGram points to the complete context of each word
		// We have to be very careful with pNGram pointers :-)
		TrieNodeHorzIterator<LWVocab::WordID, ProbNode> wordIterator(pNode, pNGram + nOrder - 1, 1);		
		TrieNode<LWVocab::WordID, ProbNode>* pWordNode;
		while ((pWordNode = wordIterator.next())) {
			LangModel::ProbLog dWordProb = pWordNode->m_payload.m_prob;
			// Subtract probability
			dNumerator -= LangModel::ProbLogToProb(dWordProb);

			// Subtract the probability of the word in a shorter context
			if (nOrder > 1) {
				dDenominator -= LangModel::ProbLogToProb(getContextProb(pNGram + 1, nOrder - 1));
			}
		}

		// Fix anomalies
		if (dNumerator < 0 /* && dNumerator > -PROB_EPSILON */) {
			dNumerator = 0;
		}
		if (dDenominator < 0 /*&& dDenominator > -PROB_EPSILON */) {
			dDenominator = 0;
		}

		// TODO: Use return codes here
		if (dNumerator < 0) {
			//assert(false);
			return false;
		}
		else if (dDenominator <= 0) {
			if (dNumerator > PROB_EPSILON) {
				return false;
			}
			else {
				// They are both 0, we can handle this case
				dNumerator = 0;
				dDenominator = 0;
			}
		}

		// Perform prob distribution if order = 1
		if (1 == nOrder) {
			redistributeProb(dNumerator);
		} else {
			LangModel::ProbLog dBow;
			if (dNumerator == 0 && dDenominator == 0) {
				dBow = PROB_LOG_ONE;
			}
			else {
				dBow = LangModel::ProbToProbLog(dNumerator) - LangModel::ProbToProbLog(dDenominator);
			}
			// Store BOW
			pNode->m_payload.m_bow = dBow;
		}
	}

	// Success
	return true;
}

void LangModelKN::redistributeProb(double dLeftOverProb) 
{
	int nWordCount = 0;
	int nZeroProbCount = 0;

	// Count all words in the vocabulary and words that have 0 probs
	LWVocab::WordID word;
	VocabIterator it(m_pVocab);
	while (LWVocab::INVALID_WORD != (word = it.next())) {
		if (!m_pVocab->isNonEvent(word)) {
			nWordCount++;

			LangModel::ProbLog* pProb = findProb(&word, 1);
			if (!pProb || (*pProb == PROB_LOG_ZERO)) {
				nZeroProbCount++;
			}

			if (!pProb) {
				// Insert prob if it does not exist
				ProbNode probNode(PROB_LOG_ZERO);
				m_probs.insertKey(&word, 1, probNode);
			}
		}
	}

	// Are there any 0 probability words?
	it.init();
	if (nZeroProbCount > 0) {
		// Distribute probability over all 0 prob words
		LangModel::ProbLog dProbIncrement = static_cast<LangModel::ProbLog> (LangModel::ProbToProbLog(static_cast<LangModel::Prob> (dLeftOverProb / nZeroProbCount)));
		while (LWVocab::INVALID_WORD != (word = it.next())) {
			if (!m_pVocab->isNonEvent(word)) {
				LangModel::ProbLog* pProb = findProb(&word, 1);
				if (!pProb) {
					// Insert probability if it does not exist
					pProb = insertProb(&word, 1);
					*pProb = PROB_LOG_ZERO;
				}
				if (PROB_LOG_ZERO == *pProb) {
					*pProb = dProbIncrement;
				}
			}
		}
	}
	else {
		// Distribute the prob to all words
		LangModel::ProbLog dProbIncrement = static_cast<LangModel::ProbLog> (LangModel::ProbToProbLog(static_cast<LangModel::Prob> (dLeftOverProb / nWordCount)));
		while ((word = it.next())) {
			if (!m_pVocab->isNonEvent(word)) {
				LangModel::ProbLog* pProb = insertProb(&word, 1);
				(*pProb) += dProbIncrement;
			}
		}
	}
}

ProbNode* LangModelKN::findProbNode(iterator pNGram, unsigned int nOrder)
{
	TrieNode<LWVocab::WordID, ProbNode>* pNode = m_probs.findNode(pNGram, nOrder);
	if (pNode) {
		return &(pNode->m_payload);
	}
	else {
		return NULL;
	}
}

LangModel::ProbLog* LangModelKN::insertProb(iterator pNGram, unsigned int nOrder)
{
	TrieNode<LWVocab::WordID, ProbNode>* pNode = m_probs.insertKey(pNGram, nOrder);

	return &(pNode->m_payload.m_prob);
}

LangModel::ProbLog* LangModelKN::findProb(iterator pNGram, unsigned int nOrder)
{
	TrieNode<LWVocab::WordID, ProbNode>* pNode = m_probs.findNode(pNGram, nOrder);
	if (pNode) {
		return &(pNode->m_payload.m_prob);
	}
	else {
		return NULL;
	}
}

} // namespace LW
