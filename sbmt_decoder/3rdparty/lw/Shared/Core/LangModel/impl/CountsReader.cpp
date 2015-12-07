// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include <Common/stldefs.h>
#include "CountsReader.h"
#include <stdio.h>

#ifndef LM_NO_COMMON_LIB
#include "Common/Crypto/CryptoStreamFactory.h"
#endif

#include "Common/Util.h"

using namespace std;

namespace LW {

#ifndef _MSC_VER
    #define sscanf_s sscanf
#endif


CountsReader::CountsReader(istream* pInput, bool bHasDoubleCounts)
{
	//assert(pInput);

	// Wrap the stream into a crypto stream
#ifndef LM_NO_COMMON_LIB
	m_pInput = CryptoStreamFactory::getInstance()->createIStream(*pInput, true);
#else
	m_pInput = pInput;
#endif

	m_bHasDoubleCounts = bHasDoubleCounts;
	m_bHasPendingData = false;
	m_nMaxOrder = 0;
	m_nCurrentOrder = 0;
	m_nExpectedOrder = 0;
	m_nCurrentLineNumber = 0;

	m_nPendingCount = 0;
	m_nPendingCount2 = 0;

	// Reset all summary counts (one per n-gram order);
	for (int i = 0; i < MAX_SUPPORTED_ORDER; i++) {
		m_countSummary[i] = 0;
	}

	m_nParseStatus = PS_BEFORE_DATA;

	// Read the first count and put it in the buffer
	moveNext();
}


CountsReader::~CountsReader()
{
	// This is a crypto stream. We own it.
	delete m_pInput;
}

void CountsReader::readCounts(
	std::istream& in, 
	LWVocab& vocab, 
	bool bUpdateVocab, 
	TrieNode<LWVocab::WordID, int>& counts, 
	unsigned int* pnMaxOrderFound)
{
	CountsReader reader(&in);

	// Actual words on the current line
	char* ppWords[MAX_SUPPORTED_ORDER];
	// Words on the current line converted to Vocab IDs
	LWVocab::WordID currentNGram[MAX_SUPPORTED_ORDER];
	// Current order of the n-grams we are reading
	unsigned int nCurrentOrder = 1;

	// Loop for each order
	while (reader.moveOrder(nCurrentOrder)) {
		// Loop for each n-gram.
		// moveNext() should not be called now, as it was called by moveOrder()
		do {
			// Retrive the count from the file
			int nCount = reader.getCurrentCount();
			// Retrive the words from the file
			reader.getCurrentNGram(ppWords);

			// Update the vocabulary if the caller allowed it
			if (bUpdateVocab) {
				for (unsigned int i = 0; i < nCurrentOrder; i++) {
					vocab.insertWord(ppWords[i]);
				}
			}
			// Convert words from strings to Vocab IDs
			vocab.wordsToID(ppWords, currentNGram, nCurrentOrder);

			// Load n-gram in the trie
			counts.insertKey(currentNGram, nCurrentOrder, nCount);
		} while (!reader.moveNext());

		nCurrentOrder++;
	}

	// We quit the loop because nCurrentOrder did not exist. Adjust max order
	*pnMaxOrderFound = nCurrentOrder - 1;

}


bool CountsReader::fail()
{
	return m_pInput->fail();
}

bool CountsReader::moveOrder(unsigned int nOrder) 
{
	// No pending data - end of file
	if (!m_bHasPendingData) {
		return false;
	}

	// Tell moveNext() to not go beyond this
	m_nExpectedOrder = nOrder;

	// Do we have the right order in the pending data already?
	if (m_nCurrentOrder == nOrder) {
		return true;
	}

	// Read counts until we reach the right order
	// This skips n-grams so the caller should make sure we never get to loop
	while (m_bHasPendingData && (m_nCurrentOrder < nOrder)) {
		moveNext();
	}

	return (m_nCurrentOrder == nOrder);
}

unsigned int CountsReader::getCurrentOrder() const
{
	return m_nCurrentOrder;
}

void CountsReader::getCurrentNGram(char** ppWords) const
{
	//assert(m_bHasPendingData);
	memcpy(ppWords, m_pPendingWords, m_nCurrentOrder * sizeof(char*));
}

int CountsReader::getCurrentCount() const 
{
	//assert(m_bHasPendingData);
	return m_nPendingCount;
}

int CountsReader::getCurrentCount2() const
{
	//assert(m_bHasPendingData);
	return m_nPendingCount2;
}

bool CountsReader::moveNext()
{
	// Detect attempts to keep moving beyond "out of the current order"
	if (m_nExpectedOrder < m_nCurrentOrder) {
		return false;
	}

	m_bHasPendingData = false;

	while ((m_nParseStatus != PS_DONE) && (!m_pInput->fail())) {
		m_nCurrentLineNumber++;
		safeGetLine(*m_pInput, m_currentLine, sizeof(m_currentLine));

		// Append NULL char at the end, just in case
		m_currentLine[sizeof(m_currentLine) - 1] = '\0';

		// Nuke <cr> in case they are left over at the end of the buffer
		size_t nLineLength = strlen(m_currentLine);
		while ((nLineLength > 0) && (13 == m_currentLine[nLineLength - 1])) {
			m_currentLine[nLineLength - 1] = 0;
			nLineLength--;
		}


		switch(m_nParseStatus) {
			case PS_BEFORE_DATA:
				if (0 == strncmp(m_currentLine, "\\data\\", 6)) {
					// Found "\data\" 
					m_nParseStatus = PS_READING_COUNTS;
				}
				break;
			case PS_READING_COUNTS:
				{
					// Stop at the first empty line
					if(0 == strlen(m_currentLine)) {
						// Done with reading counts. Move to the next status
						m_nParseStatus = PS_BEFORE_READING_NGRAM;
					}
					else {
						// Read the count
						unsigned int nReadOrder = 0;
						unsigned int nReadCount = 0;
						sscanf_s(m_currentLine, "ngram %d=%d", &nReadOrder, &nReadCount);
//						if (nReadOrder <= 0 ||  nReadCount <= 0 || nReadOrder > MAX_SUPPORTED_ORDER) {
						// We can tolerate an nReadCount = 0 (the writer did not know it at the time of the writing)
						if (nReadOrder <= 0 || nReadOrder > MAX_SUPPORTED_ORDER) {
							char szError[1024];
							lw_snprintf(szError, sizeof(szError), "Invalid line in count file. Expected 'ngram x-count' at line %d", m_nCurrentLineNumber);
							throw Exception(ERR_IO, szError);
						}
						m_countSummary[nReadOrder] = nReadCount;
						if (m_nMaxOrder < nReadOrder) {
							m_nMaxOrder = nReadOrder;
						}
					}
				}
				break;
			case PS_BEFORE_READING_NGRAM:
				{
					if (0 == strncmp(m_currentLine, "\\end\\", 5)) {
						m_nParseStatus = PS_DONE;
					}
					else {
						unsigned int nReadOrder = 0;
						// If we did not find the \ keep going. The "x-ngrams:" line not reached yet
						if (0 == strncmp(m_currentLine, "\\", 1)) {
							sscanf_s(m_currentLine, "\\%d-grams:", &nReadOrder);
							if (0 == nReadOrder || nReadOrder > MAX_SUPPORTED_ORDER) {
								char szError[1024];
								lw_snprintf(
									szError, 
									sizeof(szError), 
									"Invalid line in counts file: Expected '\\\\x-grams:' at line number %d",
									m_nCurrentLineNumber
									);

								throw Exception(ERR_IO, szError);
							}
							else {
								// The following lines are going to be the probabilities for order nReadOrder
								m_nCurrentOrder = nReadOrder;
								// Increase max order as we find it in the file
								if (m_nMaxOrder < nReadOrder) {
									m_nMaxOrder = nReadOrder;
								}
								m_nParseStatus = PS_READING_NGRAM;
							}
						}
					}
				}
				break;
			case PS_READING_NGRAM:
				{
					// First empty line says we are done with this order of n-grams
					if (0 == strlen(m_currentLine)) {
						m_nParseStatus = PS_BEFORE_READING_NGRAM;
						m_bHasPendingData = true;
						return false;
					}
					else {
						// Get the count
                        char *nextToken = m_currentLine;
						char const* szCount = strtok_s(m_currentLine, " \t", &nextToken);
						// Do we have double counts? If yes, parse the next count
						if (m_bHasDoubleCounts) {
							char const* szCount2 = strtok_s(NULL, " \t", &nextToken);
							if (NULL == szCount2) {
								// Better safe than sorry
								szCount2 = "";
							}
							m_nPendingCount2 = atoi(szCount2);
						}

						// Now get the n-gram words
						for (unsigned int i = 0; i < m_nCurrentOrder; i++) {
							char* szWord = strtok_s(NULL, " \t", &nextToken);
							if (NULL == szWord) {
								char szError[1024];
								lw_snprintf(szError, sizeof(szError), "Invalid line in counts file: Expected 'n-gram' at line number %d", m_nCurrentLineNumber);
								throw Exception(ERR_IO, szError);
							}
							// Copy the words in the buffer
							m_pPendingWords[i] = szWord;
						}

						// Read counts
						if (NULL == szCount) {
							// Better safe than sorry
							szCount = "";
						}
						m_nPendingCount = atoi(szCount);

						m_bHasPendingData = true;

						return true;
					}
				}
				break;
                default:
                    throw Exception(ERR_IO,"Impossible CountsReader parse status");            
                                
		}
	}

	// Check consistency
	if (PS_DONE != m_nParseStatus) {
		throw Exception(ERR_IO, "Unexpected end of counts file");
	}

	return false;
}

int CountsReader::compare(char** ppWords) const
{
	for (unsigned int i = 0; i < m_nCurrentOrder; i++) {
		int nCompare = strcmp(m_pPendingWords[i], ppWords[i]);
		if (nCompare < 0) {
			return -1;
		}
		else if (nCompare > 0) {
			return 1;
		}
	}

	// If we get here, all strings are equal
	return 0;
}

unsigned int CountsReader::getMaxOrder() const
{
	return m_nMaxOrder;
}

void CountsReader::getCountSummary(unsigned int* pnCountSummary) const
{
	for (unsigned int i = 1; i <= m_nMaxOrder; i++) {
		pnCountSummary[i] = m_countSummary[i];
	}
}

bool CountsReader::getMinFlag() const
{
	return m_bMinFlag;
}

void CountsReader::setMinFlag(bool bMinFlag)
{
	m_bMinFlag = bMinFlag;
}

void CountsReader::writeHeader(ostream& out, unsigned int nMaxOrder, unsigned int* pnCountSummary)
{
	out << endl;
	out << "\\data\\" << endl;

	for (unsigned int i = 1; i <= nMaxOrder; i++) {
		out << "ngram " << i << "=";
		// Did not caller pass the counts?
		if (NULL != pnCountSummary) {
			out << pnCountSummary[i];
		}
		else {
			out << "?";
		}
			
		out << endl;
	}
}

void CountsReader::writeNGramHeader(ostream& out, unsigned int nOrder)
{
	out << endl;
	out << "\\" << nOrder << "-grams:" << endl;
}

void CountsReader::writeFooter(ostream& out)
{
	out << endl;
	out << "\\end\\" << endl;
}

void CountsReader::writeNGram(std::ostream& out, unsigned int nOrder, char** nGram, int nCount)
{
	out << nCount << '\t';

	for (unsigned int i = 0; i < nOrder; i++) {
		out << nGram[i] << '\t';
	}

	out << endl;
}

void CountsReader::writeNGram(std::ostream& out, unsigned int nOrder, char** nGram, int nCount, int nCount2)
{
	out << nCount << '\t';
	out << nCount2 << '\t';

	for (unsigned int i = 0; i < nOrder; i++) {
		out << nGram[i] << '\t';
	}

	out << endl;
}



} // namespace LW
