// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _LANG_MODEL_BDB_DATABASE_H
#define _LANG_MODEL_BDB_DATABASE_H

#ifndef LM_NO_BDB_LIB

#define HAVE_CXX_STDHEADERS 1
#include <db_cxx.h>

#include "LangModel/LangModel.h"
#include "Common/Vocab/Vocab.h"

namespace LW {

class LangModelBDBDatabase;

//class LangModelBDBInfoEntry {
//public:
//	void setKey(const std::string& sValue) {
//		m_sKey = sKey;
//	}
//
//	std::string getKey() {
//		return m_sKey;
//	}
//
//public:
//	unsigned int getKeySize() {
//		return m_sKey.length();
//	}
//	const char* getKeyBuffer() {
//		return m_sKey.data();
//	}
//	unsigned int getDataSize() {
//		return m_sData.length()
//	}
//private:
//	std::string m_sKey;
//	std::string m_sValue;
//};

/**
This class wraps an n-gram in BDB representation.
*/
class LangModelBDBEntry {
public:
	/**
	@param in nOrder The n-gram order for this entry.
	 (The internal storage size is always MAX_SUPPORTED_ORDER as we only store a limited number of instances in memory
	  so we are not concerned with wasted space).
	@param in An aray of words of nOrder length. If NULL, all words are set to 0.
	*/
	LangModelBDBEntry() {
		m_nOrder = 0;
	};

	LangModelBDBEntry(unsigned int nOrder) {
		m_nOrder = nOrder;
		memset(m_pWords, 0, nOrder * sizeof(LWVocab::WordID));
	}

	LangModelBDBEntry(unsigned int nOrder, LWVocab::WordID* pWords) {
		m_nOrder = nOrder;
		memcpy(m_pWords, pWords, nOrder * sizeof(LWVocab::WordID));
	}
public: // Accessor methods
	/**
	n-gram order getter
	*/
	unsigned int getOrder() {return m_nOrder;};
	///**
	//This method returns the size of the key
	//*/
	//unsigned int getKeySize() {
	//	return m_nOrder * sizeof(LWVocab::WordID) * m_nOrder;
	//}
	///**
	//This method returns a pointer of a buffer that stores the key. 
	//We are not performing any internal copying, just returning a pointer
	//to the array of LWVocab::WordID
	//*/
	//char* getKeyBuffer() {
	//	return (char*) m_pWords;
	//}
	///**
	//Returns the size necessary to allocate the data buffer. 
	//This buffer nees to hold m_dProb and m_dBOW so it's a constant
	//*/
	//unsigned int getDataSize() {
	//	return sizeof(m_dProb) + sizeof(m_dBOW);
	//}
	///**
	//This method will marshal m_dProb and m_dBOW into the internal buffer
	//and return a pointer to it. This method is usually called before writing
	//the data to the database. Do not modify m_dProb or m_BOW between calling
	//this method and writing to the database or the modifications will not
	//be picked up.
	//*/
	//char* getDataBuffer() {
	//	marshalDataOut(m_pDataBuffer);
	//	return m_pDataBuffer;
	//}
	/** Getter for words
	*/
	void getWords(LWVocab::WordID* pWords, unsigned int nOrder) {
		memcpy(pWords, m_pWords, nOrder * sizeof(LWVocab::WordID));
	}
	/**
	Setter for words AND n-gram order
	*/
	void setWords(LWVocab::WordID* pWords, unsigned int nOrder) {
		m_nOrder = nOrder;
		memcpy(m_pWords, pWords, nOrder * sizeof(LWVocab::WordID));
	}
	/**
	Probability getter
	*/
	LangModel::ProbLog getProb() {return m_dProb;};
	/**
	Probability setter
	*/
	void setProb(LangModel::ProbLog dValue) {m_dProb = dValue;}
	/**
	BOW getter
	*/
	LangModel::ProbLog getBOW() {return m_dBOW;};
	/**
	BOW Setter
	*/
	void setBOW(LangModel::ProbLog dValue) {m_dBOW = dValue;};
public:
	Dbt& getKeyForReading() {
		m_key.set_data(m_pWords);
		m_key.set_ulen(m_nOrder * sizeof(LWVocab::WordID));
		m_key.set_size(m_key.get_ulen());
		m_key.set_flags(DB_DBT_USERMEM);
		return m_key;
	}

	Dbt& getDataForReading() {
		m_data.set_data(m_pDataBuffer);
		m_data.set_ulen(sizeof(m_dProb) + sizeof(m_dBOW));
		m_data.set_size(m_data.get_ulen());
		m_data.set_flags(DB_DBT_USERMEM);
		return m_data;
	}

	Dbt& getKeyForWriting() {
		m_key.set_data(m_pWords);
		m_key.set_ulen(m_nOrder * sizeof(LWVocab::WordID));
		m_key.set_size(m_key.get_ulen());
		m_key.set_flags(DB_DBT_USERMEM);
		return m_key;
	}

	Dbt& getDataForWriting() {
		memcpy(m_pDataBuffer, &m_dProb, sizeof(m_dProb));
		size_t nOffset = sizeof(m_dProb);
		memcpy(m_pDataBuffer + nOffset, &m_dBOW, sizeof(m_dBOW));

		m_data.set_data(m_pDataBuffer);
		m_data.set_ulen(sizeof(m_dProb) + sizeof(m_dBOW));
		m_data.set_size(m_data.get_ulen());
		m_data.set_flags(DB_DBT_USERMEM);
		return m_data;
	}

	/**
	Call sequence:
	getDataForReading();
	...read data from db...
	afterRead();
	*/
	void afterRead() {
		memcpy(&m_dProb, m_pDataBuffer, sizeof(m_dProb));
		size_t nOffset = sizeof(m_dProb);
		memcpy(&m_dBOW, m_pDataBuffer + nOffset, sizeof(m_dBOW));
	}
private:
	/**
	The order of this n-gram
	*/
	unsigned int m_nOrder;
	/**
	The words in the n-gram in the same order they appear in a normal sentence
	*/
	LWVocab::WordID m_pWords[MAX_SUPPORTED_ORDER];
	/**
	*/
	LangModel::ProbLog m_dProb;
	/**
	*/
	LangModel::ProbLog m_dBOW;
	/**
	This is an internal buffer used to marshal the data (m_dProb and m_dBOW before) writing
	to the database)
	*/
	char m_pDataBuffer[2 * sizeof(LangModel::ProbLog)];
	/**
	Key buffer for DB operations
	*/
	Dbt m_key;
	/**
	Data buffer for DB operations
	*/
	Dbt m_data;
};

/**
This class wraps a BDB table for a given n-gram order (e.g A table for trigrams)
<li> The key is a sequence N word IDs where N is the order.
<li> The data is composed of 2 floats (probability and BOW).
*/
class LangModelBDBDatabase {
public:
	static LangModelBDBDatabase* create(const std::string& sEnvDir, const std::string& sFileName);
	static LangModelBDBDatabase* open(const std::string& sEnvDir, const std::string sFileName);
public:
	bool find(LangModelBDBEntry* pEntry);
	void write(LangModelBDBEntry* pEntry);
private:
	/**
	The n-gram order for which we store the records
	*/
	unsigned int m_nOrder;
	/**
	Berkeley database that stores the probabilities for this order
	*/
	Db m_db;
};

} // namespace LW

// LM_NO_BDB_LIB
#endif

#endif
