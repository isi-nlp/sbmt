// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _VOCAB_H
#define _VOCAB_H

// TODO: Fix the issues with tries and remove this definition
#undef VOCAB_USE_STD_MAP
#define VOCAB_USE_STD_MAP

#ifdef VOCAB_USE_STD_MAP
#include <map>
#else
#include "impl/BinHashMap.h"
#endif

#include <string>
#include <vector>

namespace LW {

/*
/// Increments a word ID by one
class VocabIDRegularIncrementer {
	unsigned int increment(unsigned int nCurrentWordID) {
		return ++nCurrentWordID;
	}
};

/// Makes sure every 
class VocabIDIncrementerNotZero {
	unsigned int increment(unsigned int nCurrentWordID) {
		
	}
};
*/

class HashCompareString
{	
public:
	enum
		{	// parameters for hash table
		bucket_size = 4,	// 0 < bucket_size
		min_buckets = 4096};// min_buckets = 2 ^^ N, 0 < N

	size_t operator()(const std::string& key) const
	{	
			size_t hash;
			char* szKey = (char*) key.c_str();

			for (hash = 0; *szKey; szKey++) {
//				hash += (hash << 3) + *szKey;
                            // google for g_str_hash X31_HASH to see why this is better (less collisions, good performance for short strings, faster)
                            hash = 31*hash+*szKey;
                            // or Paul Hsieh's correct (all bits influenced) hash: http://www.azillionmonkeys.com/qed/hash.html
			}

			return hash;
	}

	bool operator()(const std::string& key1, const std::string& key2) const
	{	
		return key1 < key2;
	}
};

template <class T>
struct dummyless
{
    bool operator()(T const& s1, T const& s2) const
    {
        return s1 < s2;
    }
};

class LWVocab
{
	friend class VocabIterator;
public:
	enum SpecialTokens{NONE = 0, UNKNOWN_WORD = 1, START_SENTENCE = 2, END_SENTENCE = 3, INVALID_WORD = 4, FIRST_VALID_WORD_ID = 100};
public:
	typedef unsigned int WordID;
	typedef std::vector<WordID> WordIDVector;
#ifdef VOCAB_USE_STD_MAP
	//typedef std::hash_map<WordID, std::string> ID2WordMap;
	//typedef std::hash_map<std::string, WordID, HashCompareString> Word2IDMap;
	typedef std::map<WordID, std::string,dummyless<WordID> > ID2WordMap;
	typedef std::map<std::string, WordID,dummyless<std::string> > Word2IDMap;
#else
	typedef BinHashMap<WordID, std::string> ID2WordMap;
	typedef BinHashMap<std::string, WordID> Word2IDMap;
#endif
public:
	/// Default constructor
	LWVocab();
	/// Copy constructor
	LWVocab(const LWVocab& src);
	/// Destructor
	virtual ~LWVocab();
	/// Copy operator
	LWVocab& operator=(const LWVocab& src);
	/**
	* Inserts a word if the word does not exist already in the vocabulary
	*/
	WordID insertWord(const std::string& word);
	/**
	* Maps a string to a word ID
	*/
	WordID getID(const std::string& word) const;
	/**
	* Maps a word ID to a string
	*/
	const std::string& getWord(WordID wordID) const;
	/**
	* Maps a word ID to a string. Same as the method above, different signature
	*/
	const std::string& getWord(WordID wordID, bool& bFound) const;
	/**
	* If a word is not found it is inserted if bInsertWords is true
	*/
	void wordsToID(const std::vector<std::string>& sentWords, std::vector<WordID>& sentID, bool bInsertWords);
	/**
	* If a word is not found it is replaced with UNKNOWN_WORD
	*/
	void wordsToID(const std::vector<std::string>& sentWords, std::vector<WordID>& sentID) const;
	/**
	* If a word is not found it is inserted if bInsertWords is true
	*/
	void wordsToID(char** pWords, WordID* pWordIDs, unsigned int nWordCount, bool bInsertWords);
	/**
	* This is a const method if the words do not exist, they are not 
	* inserted, but rather replaced with UNKNOWN_WORD
	*/
	void wordsToID(char** pWords, WordID* pWordIDs, unsigned int nWordCount) const;
	/**
	* Returns true if the word is a non-event
	*/
	bool isNonEvent(WordID wordID) const;
	/**
	* Returns the number of words in the vocabulary
	*/
	size_t getWordCount() const;
	/**
	* Inserts a word forcing the Word ID to a given value. Use carefully!
	*/
	void insertWord(const std::string& word, WordID wordID);
	/**
	Returns the next ID the vocabulary will use when a new word is inserted.
	*/
	WordID getNextID() const {return m_currentID;};
	/**
	Sets the next word ID the vocabulary will use when a new word is inserted.
	Use very carefully and only if you know what you're doing.
	*/
	void setNextID(WordID wordID) {m_currentID = wordID;};
private:
	/// This variable is incremented each time we insert a word in the vocabulary
	WordID m_currentID;
	/// Mapping of Word IDs to words
	ID2WordMap m_mapID2Word;
	/// Mapping of words to WordIDs
	Word2IDMap m_mapWord2ID;
};

class VocabIterator
{
private:
	const LWVocab* m_pVocab;
	LWVocab::Word2IDMap::const_iterator m_iterator;
public:
	VocabIterator(const LWVocab* pVocab);
	void init();
	LWVocab::WordID next();
};

} // namespace LW

#endif // _VOCAB_H
