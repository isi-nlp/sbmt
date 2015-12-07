// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include "../Vocab.h"

#include <Common/os-dep.h>
#ifndef assert
#define assert(x)
#endif
using namespace std;

namespace LW {

LWVocab::LWVocab()
{
	m_currentID = FIRST_VALID_WORD_ID;
	insertWord("<unk>", UNKNOWN_WORD);
	insertWord("<s>", START_SENTENCE);
	insertWord("</s>", END_SENTENCE);
}

LWVocab::LWVocab(const LWVocab& src) {
	operator=(src);
}

LWVocab::~LWVocab()
{
}

LWVocab& LWVocab::operator=(const LWVocab& src)
{
#ifndef VOCAB_USE_STD_MAP
#error "Please make sure that the maps below are safe to copy then remove this line"
#endif
	m_currentID = src.m_currentID;

	// Copy the maps by hand
	{ 
		ID2WordMap::const_iterator it;
		for (it = src.m_mapID2Word.begin(); it != src.m_mapID2Word.end(); it++) {
			m_mapID2Word[it->first] = it->second;
		}
	} 
	{
		Word2IDMap::const_iterator it;
		for (it = src.m_mapWord2ID.begin(); it != src.m_mapWord2ID.end(); it++) {
			m_mapWord2ID[it->first] = it->second;
		}
	}

	return *this;
}

LWVocab::WordID LWVocab::insertWord(const string& word)
{
	WordID nReturn = 0;
	// Check if exists
	Word2IDMap::const_iterator it = m_mapWord2ID.find(word);
	if (it == m_mapWord2ID.end()) {
		// Does not exist. Add it.
#ifdef VOCAB_USE_STD_MAP
		m_mapWord2ID.insert(Word2IDMap::value_type(word, m_currentID));
		m_mapID2Word.insert(ID2WordMap::value_type(m_currentID, word));
#else
		m_mapWord2ID.insert2(word)->second = m_currentID;
		m_mapID2Word.insert2(m_currentID)->second = word;
#endif
		nReturn = m_currentID;
		m_currentID++;
	}
	else {
		// Already exists
		nReturn = (*it).second;
	}

	return nReturn;
}

void LWVocab::insertWord(const string& word, WordID wordID) {
#ifdef VOCAB_USE_STD_MAP
	m_mapWord2ID.insert(Word2IDMap::value_type(word, wordID));
	m_mapID2Word.insert(ID2WordMap::value_type(wordID, word));
#else
#error "Must define VOCAB_USE_STD_MAP"
	m_mapWord2ID.insert2(word)->second = wordID;
	m_mapID2Word.insert2(wordID)->second = word;
#endif

	if (m_currentID <= wordID) {
		m_currentID = wordID + 1;
	}
}

LWVocab::WordID LWVocab::getID(const string& word) const
{
	// This method will fail is the word does not exist
	Word2IDMap::const_iterator it = m_mapWord2ID.find(word);
	if (it != m_mapWord2ID.end()) {
		return (*it).second;
	}
	else {
		return UNKNOWN_WORD;
	}
}

const string& LWVocab::getWord(WordID id) const
{
	bool bFound;
	return getWord(id, bFound);
}

const string& LWVocab::getWord(WordID id, bool& bFound) const
{
	static string sInvalidWord;

	// This method might fail if the word ID does not exist in the vocabulary
	ID2WordMap::const_iterator it = m_mapID2Word.find(id);
	if (it == m_mapID2Word.end()) {
		bFound = false;
		return sInvalidWord;
	}
	else {
		bFound = true;
		return (*it).second;
	}
}

void LWVocab::wordsToID(const std::vector<std::string>& sentWords, std::vector<WordID>& sentID, bool bInsertWords)
{
	sentID.clear();

	vector<string>::const_iterator it;
	for (it = sentWords.begin(); it != sentWords.end(); it++) {
		// Convert each word to vocabulary ID
		WordID wordID;
		if (bInsertWords) {
			wordID = insertWord(*it);
		}
		else {
			wordID = getID(*it);
		}
		// ... and append it to the vector if IDs
		sentID.push_back(wordID);
	}
}

void LWVocab::wordsToID(const std::vector<std::string>& sentWords, std::vector<WordID>& sentID) const
{
	// We can cast to const as we know for sure this method will not modify
	// any internal variables if bInsertWords is false
	const_cast<LWVocab*>(this)->wordsToID(sentWords, sentID, false);
}



void LWVocab::wordsToID(char** pWords, WordID* pWordIDs, unsigned int nWordCount, bool bInsertWords)
{
	for (unsigned int i = 0; i < nWordCount; i++, pWords++, pWordIDs++) {
		// Convert each word to vocabulary ID
		WordID wordID;
		if (bInsertWords) {
			wordID = insertWord(*pWords);
		}
		else {
			wordID = getID(*pWords);
		}
		*pWordIDs = wordID;
	}
}

// const version of the previous method
void LWVocab::wordsToID(char** pWords, WordID* pWordIDs, unsigned int nWordCount) const
{
	for (unsigned int i = 0; i < nWordCount; i++, pWords++, pWordIDs++) {
		// Convert each word to vocabulary ID
		*pWordIDs = getID(*pWords);
	}
}

size_t LWVocab::getWordCount() const
{
	return m_mapID2Word.size();
}

bool LWVocab::isNonEvent(WordID wordID) const {
	switch (wordID) {
	case START_SENTENCE:
		return true;
	default:
		return false;
	}
}

VocabIterator::VocabIterator(const LWVocab* pVocab)
{
	assert(pVocab);

	m_pVocab = pVocab;
	init();
}

void VocabIterator::init()
{
	m_iterator = m_pVocab->m_mapWord2ID.begin();
}

LWVocab::WordID VocabIterator::next() {
	LWVocab::WordID nRetValue;
	if (m_iterator != m_pVocab->m_mapWord2ID.end()) {
		nRetValue = (*m_iterator).second;
		m_iterator++;
	}
	else {
		nRetValue =  LWVocab::INVALID_WORD;
	}

	return nRetValue;
}

} // namespace LW
