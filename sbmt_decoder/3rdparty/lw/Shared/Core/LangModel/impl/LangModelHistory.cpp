// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include "LangModel/LangModelHistory.h"
#include <cstring>

namespace LW {

LangModelHistory::LangModelHistory(unsigned int nMaxHistorySize, LWVocab::WordID* pWordHistory)
{
	m_pWordHistory = pWordHistory;
	m_nWordHistorySize = nMaxHistorySize;
	m_nWordHistoryCount = 0;
	m_prob = 0;
}

void LangModelHistory::addWord(LWVocab::WordID nWord)
{	
	if (this->m_nWordHistoryCount < m_nWordHistorySize) {
		// Just add the word
		m_pWordHistory[m_nWordHistoryCount] = nWord;
		m_nWordHistoryCount++;
	}
	else {
		// Shift the words to make room for this one
		for (unsigned int i = 1; i < m_nWordHistorySize; i++) {

			m_pWordHistory[i - 1] = m_pWordHistory[i];
		}
		m_pWordHistory[m_nWordHistorySize - 1] = nWord;
	}
}

LangModel::ProbLog LangModelHistory::getProb() const
{
	return m_prob;
}

void LangModelHistory::setProb(LangModel::ProbLog newProb)
{
	m_prob = newProb;
}

LWVocab::WordID* LangModelHistory::getWordBuffer()
{
	return m_pWordHistory;
}

LWVocab::WordID LangModelHistory::getWordCount() const
{
	return m_nWordHistoryCount;
}

LangModelHistory& LangModelHistory::operator=(const LangModelHistory& src)
{
	// The internal buffer size should be equivalent
	// It makes no sense to make an assigment if they are not
	if(m_nWordHistorySize == src.m_nWordHistorySize) {
		memcpy(m_pWordHistory, src.m_pWordHistory, src.m_nWordHistoryCount * sizeof(LWVocab::WordID));
	}

	m_nWordHistoryCount = src.m_nWordHistoryCount;
	m_prob = src.m_prob;

	// m_nWordHistorySize member remains unaffected, as it actually reflects the size of the buffer.

	return *this;
}

} // namespace LW
