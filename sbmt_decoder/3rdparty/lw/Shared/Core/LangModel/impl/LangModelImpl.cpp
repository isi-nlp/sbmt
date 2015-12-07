// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include "LangModelImpl.h" 

// REMOVE ME
#include <iostream>
#include <stdio.h>
#include <cstring>

#include <Common/os-dep.h>
using namespace std;

#include "LangModelImplBase.h"
#include "LangModel/LangModelHistory.h"

namespace LW {

LangModelImpl::LangModelImpl(LangModelImplBase* pImpl, LWVocab* pVocab, bool bOwnVocab)
{
	//assert(pImpl);
	//assert(pVocab);

	m_bPrepared = false;
	m_pImpl = pImpl;
	m_pVocab = pVocab;
	m_bOwnVocab = bOwnVocab;
}

LangModelImpl::~LangModelImpl()
{
	delete m_pImpl;
	if (m_bOwnVocab) {
		delete m_pVocab;
	}
}

void LangModelImpl::clear()
{
	m_pImpl->clear();
}

void LangModelImpl::learnSentence(char** pSentence, unsigned int nSentenceSize, bool bPadWithStartEnd)
{
	if (0 == nSentenceSize) {
		return;
	}

	LWVocab::WordID buffer[MAX_WORDS_IN_SENTENCE + 2];

	if (nSentenceSize > MAX_WORDS_IN_SENTENCE) {
		// Just truncate and go ahead
		nSentenceSize = MAX_WORDS_IN_SENTENCE;
	}

	unsigned int nWordCount;

	if (bPadWithStartEnd) {
		// Start with <s>
		buffer[0] = LWVocab::START_SENTENCE;

		// Add the rest of the words
		m_pVocab->wordsToID(pSentence, &(buffer[1]), nSentenceSize, true);

		// Append </s>
		buffer[nSentenceSize + 1] = LWVocab::END_SENTENCE;

		// Compute word count;
		nWordCount = nSentenceSize + 2;
	}
	else {
		// Don't pad, just convert words into IDs
		m_pVocab->wordsToID(pSentence, buffer, nSentenceSize, true);
		// Compute word count;
		nWordCount = nSentenceSize;
	}

	m_pImpl->learnSentence(buffer, nWordCount);
}


LangModel::ProbLog LangModelImpl::computeSentenceProbability(char** pSentence, unsigned int nWordCount)
{
	if (!m_bPrepared) {
		prepare();
	}

	LWVocab::WordID buffer[MAX_WORDS_IN_SENTENCE + 2];

	if (nWordCount > MAX_WORDS_IN_SENTENCE) {
		//assert(false);
		// In a release version, just truncate and go ahead
		nWordCount = MAX_WORDS_IN_SENTENCE;
	}

	// Start with <s>
	buffer[0] = LWVocab::START_SENTENCE;

	// Add the rest of the words
	m_pVocab->wordsToID(pSentence, &(buffer[1]), nWordCount, false);

	// Append </s>
	buffer[nWordCount + 1] = LWVocab::END_SENTENCE;

	// The sentence size is now nWordCount + 2
	// The last word index is sentence size - 1
	
	return computeSequenceProbability(buffer, 0, nWordCount + 2 - 1);
}

LangModel::ProbLog LangModelImpl::computeSequenceProbability(unsigned int* pWords, unsigned int nStartWord, unsigned int nEndWord)
{
	if (!m_bPrepared) {
		prepare();
	}

	// Deal with the case when the sequence has one word only and the word is <s>
	// The decoder makes these kinds of calls :-)
	if ((nStartWord == nEndWord) && (pWords[nStartWord] == LWVocab::START_SENTENCE)) {
		return 0;
	}

	//// REMOVE ME
	//cerr << "+++++ Sequence: [";
	//for (int i = 0; i <= nEndWord; i++) {
	//	if (i == nStartWord) {
	//		cerr << "]";
	//	}
	//	cerr << " " << m_pVocab->getWord(pWords[i]);
	//}

	// Compute sentence probability
	LangModel::ProbLog dProbLog = 0;

	// In case the sentence sarts with <s> ...
	// Start with <s> w0 (the first 2 words - nContextLength gets incremented before the call)
	// in order to avoid the probability of <s> alone (which is 0)
	// So for 3-grams we go / <s> w0 / <s> w0 w1 / w0 w1 w2 / ... / w(n-2) w(n-1) w(n)

	// If the sentence does not start with <s> ...
	// For 3-grams comtpue / w0 / w0 w1 / w0 w1 w2 / ... / w(n-2) w(n-1) w(n)

	int i;
	// The probability of <start> by itself is 0, so avoid it by starting with the next word.
	if (LWVocab::START_SENTENCE == pWords[nStartWord]) {
		nStartWord++;
	}

	unsigned int nMaxOrder = m_pImpl->getMaxOrder();

	for (i = nStartWord - nMaxOrder + 1 ; i <= (int) ((nEndWord + 1) - nMaxOrder); i++) {
		int nEnd = i + nMaxOrder;
		int nStart = i;
		if (nStart < 0) {
			nStart = 0;
		}
		LangModel::ProbLog dProb = m_pImpl->getContextProb(pWords + nStart, nEnd - nStart);
		// printf("  %lf\n", dProb);
		dProbLog += dProb;
	}

	return dProbLog;
}

LangModel::ProbLog LangModelImpl::computeProbability(unsigned int nWord, LangModelHistory& historyIn, LangModelHistory& historyOut)
{
	// Copy the history into the out one
	historyOut = historyIn;

	// Append the new word at the end of history
	historyOut.addWord(nWord);

	// We have to make one call only to the LM 
	LangModel::ProbLog dProb = m_pImpl->getContextProb(historyOut.getWordBuffer(), historyOut.getWordCount());

	// Multiply the probability already in the history with the new one
	LangModel::ProbLog dHistProb = historyIn.getProb() + dProb;

	// ... and store it
	historyOut.setProb(dHistProb);

	return dProb;
}


LangModel::ProbLog LangModelImpl::computeProbability(unsigned int nWord, unsigned int* pnContext, unsigned int nContextLength)
{
	unsigned int pnBuffer[MAX_SUPPORTED_ORDER];
	int nOrigContextLength = nContextLength;
	if (nContextLength > MAX_SUPPORTED_ORDER - 1) {
		nContextLength = MAX_SUPPORTED_ORDER - 1;
	}

	// Copy the whole thing into a contiguous area. Context first, word next
	memcpy(pnBuffer, pnContext + (nOrigContextLength - nContextLength), sizeof(unsigned int) * nContextLength);
	pnBuffer[nContextLength] = nWord;

	return m_pImpl->getContextProb(pnBuffer, nContextLength + 1);
}
void LangModelImpl::prepare() 
{
	if (!m_bPrepared) {
		m_pImpl->prepare();
		m_bPrepared = true;
	};
}

void LangModelImpl::write(std::ostream& out, LangModel::SerializeFormat nFormat)
{
	if (!m_bPrepared) {
		prepare();
	}
	m_pImpl->write(out, nFormat);
}

void LangModelImpl::writeCounts(std::ostream& out)
{
	m_pImpl->writeCounts(out);
}

void LangModelImpl::read(std::istream& in, LangModel::ReadOptions nOptions) 
{
	// We don't want to compute from counts, but read it from a file
	m_bPrepared = true;

	m_pImpl->read(in);
}

void LangModelImpl::readCounts(std::istream& in) 
{
	m_bPrepared = false;

	m_pImpl->readCounts(in);
}


void LangModelImpl::dump(std::ostream& out)
{
	if (!m_bPrepared) {
		prepare();
	}
	m_pImpl->dump(out);
}

const LWVocab* LangModelImpl::getVocab() const
{
	return m_pVocab;
}

unsigned int LangModelImpl::getMaxOrder() const
{
	return m_pImpl->getMaxOrder();
}

void LangModelImpl::finishedCounts()
{
	prepare();
}

} // namespace LW
