// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

//#include "LangModelBDBDatabase.h"
//
//namespace LW {
//LangModelBDBEntry::LangModelBDBEntry(unsigned int nOrder, LWVocab::WordID* pWords)
//{
//	LW_ASSERT(nOrder > 0);
//
//	m_nOrder = nOrder;
//
//	if (pWords) {
//		memcpy(m_pWords, pWords, nOrder * sizeof(LWVocab::WordID));
//	}
//	else {
//		memset(m_pWords, 0, nOrder * sizeof(LWVocab::WordID));
//	}
//
//	m_dProb = 0;
//	m_dBOW = 0;
//}
//
/////////////////////////////////////////////////////////////////////////////////////
//
//bool LangModelBDBDatabase::find(LangModelBDBEntry* pEntry) 
//{
//	// Make sure the database and the n-gram have the same order
//	LW_ASSERT(pEntry->m_nOrder == m_nOrder);
//
//	Dbt key, data;
//
//	// Initialize the key
//	key.set_data(pEntry->getKeyBuffer());
//	key.set_ulen(pEntry->getKeySize());
//	key.set_flags(DB_DBT_USERMEM);
//
//	// Perform the search
//	m_db.get(NULL, &key, &data, 0);
//
//	// Marshal the data from database buffer into our structure
//	void* pData = data.get_data();
//	pEntry->marshalDataIn(pData);
//
//}
//
//void LangModelBDBDatabase::write(LangModelBDBEntry* pEntry)
//{
//	// Make sure the database and the n-gram have the same order
//	LW_ASSERT(pEntry->m_nOrder == m_nOrder);
//
//	Dbt key, data;
//
//	key.set_data(pEntry->getKeyBuffer());
//	key.set_ulen(pEntry->getKeySize());
//	key.set_flags(DB_DBT_USERMEM);
//
//	data.set_data(pEntry->getDataBuffer());
//	data.set_ulen(pEntry->getDataSize());
//	data.set_flags(DB_DBT_USERMEM);
//
//	int nRet = m_db.put(NULL, &key, &data, 0);
//}
//
//} // namespace
