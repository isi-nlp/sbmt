// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef LM_NO_BDB_LIB

#include "LangModelBDB.h"

#include <exception>
#include <string>
#include <assert.h>
#include <qfileinfo.h>

#include "Common/Util.h"
#include "Common/Platform.h"
#include "Common/Assert.h"

#include "LangModelBDBDatabase.h"

#define DB_NAME_INFO "info"
#define DB_NAME_VOCAB "vocab"
#define DB_NAME_NGRAM "ngram-%d"

#define PROP_LM_ORDER "lm-order"

using namespace std;

// No need to call this directly. Call HANDLE_DB_EXCEPTION instead
#define HANDLE_DB_EXCEPTION_BY_TYPE(exceptionType, operation, finalBlock) \
	catch (exceptionType& e) { \
        finalBlock; \
		char szError[4096]; \
		lw_snprintf(szError, sizeof(szError), \
			"Language Model Berkeley DB Error: Operation: %s. Error: %s. Database Information: [%s]", \
			(char*) operation, \
			e.what(), \
			getDebugInfo().c_str()); \
		throw Exception(ERR_INTERNAL_ERROR, szError); \
	}

// We have to use a macro as the exception handling is WAY TOO LONG AND UGLY to keep repeating everywhere
// Operation is a char* that defines what we were doing when the exception occurred
// finalBlock is usually empty ("") and it contains cleanup code that should be executed before we re-throw the 
// exception. A poor man's Java finally
#define HANDLE_DB_EXCEPTION(operation, finalBlock) \
	HANDLE_DB_EXCEPTION_BY_TYPE(DbException, operation, (finalBlock)) \
	HANDLE_DB_EXCEPTION_BY_TYPE(std::exception, operation, (finalBlock)) 

#define RETHROW_EXCEPTION(operation, e) \
	char szError[1024]; \
	lw_snprintf(szError, sizeof(szError), \
		"Langauge Model Berkeley DB Error: Operation: %s; Error: %s; Database Information: [%s]", \
		(const char*) operation, \
		e.what(), \
		getDebugInfo().c_str()); \
	throw Exception(ERR_INTERNAL_ERROR, szError);

namespace LW {

enum ParseStatus{PS_BEFORE_DATA, PS_READING_COUNTS, PS_BEFORE_READING_NGRAM, PS_READING_NGRAM, PS_DONE};

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


LangModelBDB::LangModelBDB(LWVocab* pVocab)
{
	m_pEnv = NULL;
	m_nMaxOrder = 0;
	if (pVocab) {
		m_bOwnVocab = false;
		m_pVocab = pVocab;
	}
	else {
		m_bOwnVocab = true;
		m_pVocab = new LWVocab();
	}
	m_pInfoDB = NULL;
	m_pVocabDB = NULL;
}

LangModelBDB::~LangModelBDB()
{
	if (m_pVocab && m_bOwnVocab) {
		delete m_pVocab;
	}

	// REMOVE THIS
	return;

	try {
		if (m_pInfoDB) {
			m_pInfoDB->close(0);
			delete m_pInfoDB;
		}
		if (m_pVocabDB) {
			m_pVocabDB->close(0);
			delete m_pVocabDB;
		}
		for (size_t i = 0; i < m_dbVector.size(); i++) {
			m_dbVector[i]->close(0);
			delete m_dbVector[i];
		}

		if (m_pEnv) {
			m_pEnv->close(0);
			delete m_pEnv;
		}
	}
	catch(DbException& e) {
		cerr << "DbException: " << e.what() << endl;
	}
}

void LangModelBDB::open(const std::string& sFullPath)
{
	string sHomeDir;
	string sFile;
	breakFilePath(sFullPath, sHomeDir, sFile);

	m_sDebugInfo = "Database home: " + sHomeDir + " Database file: " + sFile;
	m_sFileName = sFile;

	// Open the environment
	m_pEnv = openEnv(sHomeDir);

	m_pInfoDB = openDB(m_pEnv, sFile, DB_NAME_INFO);
	bool bFound = readProperty(PROP_LM_ORDER, m_nMaxOrder);
	if (!bFound) {
		throw Exception(ERR_IO, "Invalid Language Model: Cannot find Language Model order in the database.");
	}
	if (m_nMaxOrder < 1 || m_nMaxOrder > MAX_SUPPORTED_ORDER) {
		char szError[1024];
		lw_snprintf(szError, sizeof(szError), "Invalid Language Model order found in the database: %d", m_nMaxOrder);
		throw Exception(ERR_IO, szError);
	}

	// Open vocabulary
	m_pVocabDB = openDB(m_pEnv, sFile, DB_NAME_VOCAB);
	readVocab();

	// Open n-gram databases
	for (unsigned int i = 1; i <= m_nMaxOrder; i++) {
		Db* pDB = openDB(m_pEnv, sFile, getDBNameForOrder(i));
		m_dbVector.push_back(pDB);
	}
}

void LangModelBDB::create(const std::string& sFullPath, unsigned int nOrder) 
{
	string sHomeDir;
	string sFile;
	breakFilePath(sFullPath, sHomeDir, sFile);

	if (nOrder > MAX_SUPPORTED_ORDER) {
		char szError[1024];
		lw_snprintf(szError, sizeof(szError), "Invalid n-gram order for language model creation: %d. Expected range 0-%d",
			nOrder,
			(unsigned int) MAX_SUPPORTED_ORDER);

		throw Exception(ERR_INVALID_PARAM, szError);
	} 

	m_sDebugInfo = "Database home: " + sHomeDir + " Database file: " + sFile;
	m_sFileName = sFile;


	// Create environment
	m_pEnv = createEnv(sHomeDir);

	// Create info
	m_pInfoDB = createDB(m_pEnv, sFile, DB_NAME_INFO);
	writeProperty(PROP_LM_ORDER, nOrder);
	
	// Create vocabulary
	m_pVocabDB = createDB(m_pEnv, sFile, DB_NAME_VOCAB);

	// Create n-gram information
	for (unsigned int i = 1; i <= nOrder; i++) {
		Db* db = createDB(m_pEnv, sFile, getDBNameForOrder(i));
		m_dbVector.push_back(db);
	}
}

DbEnv* LangModelBDB::openEnv(const std::string& sHomeDir, u_int32_t nFlags)
{
	nFlags |= DB_CREATE | DB_THREAD | DB_INIT_MPOOL | DB_PRIVATE;


	DbEnv* pEnv = new DbEnv(0);
//	pEnv->set_alloc(malloc, realloc, free);
	

	string sOperation = string("Opening enviroment: ") + sHomeDir;
	try {
		pEnv->open(sHomeDir.c_str(), nFlags, 0);
	}
	catch(DbException& e) {
		delete pEnv;
		RETHROW_EXCEPTION(sOperation.c_str(), e);
	}
	catch(std::exception& e) {
		delete pEnv;
		RETHROW_EXCEPTION(sOperation.c_str(), e);
	}

	return pEnv;
}

DbEnv* LangModelBDB::createEnv(const std::string& sHomeDir)
{
	return openEnv(sHomeDir, DB_CREATE);
}

Db* LangModelBDB::openDB(DbEnv* pEnv, const std::string& sFileName, const std::string& sDBName, u_int32_t nFlags)
{
	Db* pDb = NULL;

	u_int32_t nFileMode = 0;
	nFlags |= DB_THREAD;

	string sOperation = "Opening database";
	try {
		pDb = new Db(pEnv, 0);
		pDb->open(NULL, sFileName.c_str(), sDBName.c_str(), DB_BTREE, nFlags, nFileMode);
	}
	catch(DbException& e) {
		delete pDb;
		RETHROW_EXCEPTION(sOperation.c_str(), e);
 	}
	catch(std::exception& e) {
		delete pDb;
		RETHROW_EXCEPTION(sOperation.c_str(), e);
	}

	return pDb;
}

Db* LangModelBDB::createDB(DbEnv* pEnv, const std::string& sFileName, const std::string& sDBName)
{
	return openDB(pEnv, sFileName, sDBName, DB_CREATE | DB_EXCL);

}

void LangModelBDB::readVocab()
{
	// The vocabulary DB is already open. 
	// Create a cursor to iterate
	Dbc* pCursor = NULL;

	string sOperation = "Reading vocabulary";

	try {
		m_pVocabDB->cursor(NULL, &pCursor, 0);

		Dbt key, data;
		int ret;

		while (0 == (ret = pCursor->get(&key, &data, DB_NEXT))) {
			// Get the word as string
			string sWord((char*) key.get_data(), key.get_size());

			// Get the word as Vocab ID
			LWVocab::WordID nWordID;
			LW_ASSERT(data.get_size() == sizeof(nWordID));
			memcpy(&nWordID, data.get_data(), sizeof(nWordID));

			// Put the whole thing in the vocabulary
			m_pVocab->insertWord(sWord, nWordID);
		}
	}
	catch(DbException& e) {
		if (pCursor) {
			pCursor->close();
		}
		RETHROW_EXCEPTION(sOperation.c_str(), e);
	}
	catch(std::exception& e) {
		if (pCursor) {
			pCursor->close();
		}
		RETHROW_EXCEPTION(sOperation.c_str(), e);
	}

	if (pCursor) {
		pCursor->close();
	}
}

void LangModelBDB::writeVocab() 
{
	VocabIterator vocabIterator(m_pVocab);

	LWVocab::WordID nWordID;
	bool bFound;
	// Use a vocabulary iterator to get all the words
	while (LWVocab::INVALID_WORD != (nWordID = vocabIterator.next())) {
		const string& sWord = m_pVocab->getWord(nWordID, bFound);
		if (!bFound) {
			// This should never happen, but test anyway
			string sError = string("") + "Internal error. Word not found in Vocabulary. Language Model: " + getDebugInfo();
			throw Exception(ERR_INTERNAL_ERROR, sError);
		}

		// Write to the database
		Dbt key((void*) sWord.data(), sWord.length());
		Dbt data(&nWordID, sizeof(nWordID));

		m_pVocabDB->put(NULL, &key, &data, 0);
	}	
}

string LangModelBDB::getDBNameForOrder(unsigned int nOrder)
{
	char buffer[100];
	lw_snprintf(buffer, sizeof(buffer), DB_NAME_NGRAM, nOrder);

	return buffer;
}

bool LangModelBDB::writeProperty(const std::string& sPropName, unsigned int nValue)
{
	Dbt key((void*) sPropName.data(), sPropName.length());
	Dbt data(&nValue, sizeof(nValue));

	int nResult = m_pInfoDB->put(NULL, &key, &data, 0);

	return (nResult == 0);
}

bool LangModelBDB::readProperty(const std::string& sPropName, unsigned int& nValue)
{
	Dbt key((void*) sPropName.data(), sPropName.length());
	Dbt data;
	data.set_data(&nValue);
	data.set_ulen(sizeof(nValue));
	data.set_flags(DB_DBT_USERMEM);

	int nResult = 0;
	string sOperation = "Reading Language Model property";
	try {
		nResult = m_pInfoDB->get(NULL, &key, &data, 0);
	}
	catch(DbException& e) {
		RETHROW_EXCEPTION(sOperation.c_str(), e);
	}
	catch(std::exception& e) {
		RETHROW_EXCEPTION(sOperation.c_str(), e);
	}

	return (nResult == 0);
}

void LangModelBDB::convertFromTextLM(std::istream& in)
{
	// Current order of ngram we are reading
	unsigned int nOrder = 0;
	// Input buffer
	char line[1024 * 10];
	// The number of counts, by order
	unsigned int counts[MAX_SUPPORTED_ORDER + 1];
	// The current order we are processing
	unsigned int nCurrentOrder;
	// The current status of the parse
	ParseStatus nParseStatus = PS_BEFORE_DATA;

	// Initialize counts with 0
	for (unsigned int i = 0; i <= MAX_SUPPORTED_ORDER; i++) {
		counts[i] = 0;
	}

	// We read the max order from the file
	unsigned int nMaxOrder = 0;

	LangModelBDBEntry lmEntry;

	Db* currentNGramDB = NULL;

	int nLineNumber = 0;
	while (!in.fail() && (nParseStatus != PS_DONE)) {
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
							char szError[4096];
							lw_snprintf(
								szError, 
								sizeof(szError), 
								"Bad text Language Model: expected 'ngram x-count' at line %d", 
								nLineNumber);
							throw Exception(ERR_IO, szError);
						}
						counts[nReadOrder] = nReadCount;
					}
				}
				break;
			case PS_BEFORE_READING_NGRAM:
				{
					unsigned int nReadOrder = 0;
					// If we did not find the \ keep going. The "x-ngrams:" line not reached yet
					if (0 == strncmp(line, "\\end\\", 5)) {
						// We are done.
						nParseStatus = PS_DONE;
					}
					else if (0 == strncmp(line, "\\", 1)) {
						sscanf_s(line, "\\%d-grams:", &nReadOrder);
						if (0 == nReadOrder || nReadOrder > MAX_SUPPORTED_ORDER) {
							// TODO: Throw "Invalid line: expected '\\x-grams:'
							char szError[4096];
							lw_snprintf(szError, sizeof(szError), "Bad text Language Model: expected 'ngram x-grams:' at line %d", nLineNumber);
							throw Exception(ERR_IO, szError);
						}
						else {
							// The following lines are going to be the probabilities for order nReadOrder
							nCurrentOrder = nReadOrder;
							// Increase max order if necessary, as we find it in the file
							if (nMaxOrder < nReadOrder) {
								nMaxOrder = nReadOrder;
							}

							// Create the n-gram database for this order
							Db* db = createDB(m_pEnv, m_sFileName, getDBNameForOrder(nReadOrder));
							m_dbVector.push_back(db);

							currentNGramDB = db;

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
						LWVocab::WordID words[MAX_SUPPORTED_ORDER];
						// Get the probability
						char* context;
						char* szProb = strtok_s(line, " \t", &context);
						// Now get the n-gram words
						for (unsigned int i = 0; i < nCurrentOrder; i++) {
							char* szWord = strtok_s(NULL, " \t", &context);
							if (NULL == szWord) {
								char szError[4096];
								lw_snprintf(szError, sizeof(szError), "Bad text Language Model: expected 'n-gram' at line %d", nLineNumber);
								throw Exception(ERR_IO, szError);
							}
							// Make sure every word is in the vocabulary and construct the key to this n-gram
							words[i] = m_pVocab->insertWord(szWord);
						}
						// Do we have a BOW?
						char* szBOW = strtok_s(NULL, " \t", &context);

						if (NULL == szProb) {
							szProb = "";
						}
						if (NULL == szBOW) {
							szBOW = "";
						}

						// Convert 0 probabilities (written -99, the same SRI does)
						lmEntry.setWords(words, nCurrentOrder);
						lmEntry.setProb(ReadProb(szProb));
						lmEntry.setBOW(ReadProb(szBOW));

						Dbt& key = lmEntry.getKeyForWriting();
						Dbt& data = lmEntry.getDataForWriting();
						
						int result = currentNGramDB->put(NULL, &key, &data, 0);

						//LangModelBDBEntry lmEntry2;
						//lmEntry2.setWords(words, nCurrentOrder);
						//lmEntry2.setProb(0);
						//lmEntry2.setBOW(0);
						//Dbt& key2 = lmEntry2.getKeyForReading();
						//Dbt& data2 = lmEntry2.getDataForReading();

						//result = currentNGramDB->get(0, &key2, &data2, 0);
						//lmEntry2.afterRead();

						//int c = 2;
					}
				}
				break;
		}
	}

	// Check consistency
	if (PS_DONE != nParseStatus) {
		throw Exception(ERR_IO, "Text language model not properly terminated by '\\end\\'.");
	}
	
 	m_nMaxOrder = nCurrentOrder;

	if (m_dbVector.size() != m_nMaxOrder) {
		char szError[4096];
		lw_snprintf(szError, sizeof(szError),
			"Invalid language model. The number of n-gram probability tables (%d) does not match the order declared in the database (%d).",
			(unsigned int) m_dbVector.size(),
			(unsigned int) nOrder);
		throw Exception(ERR_IO, szError);
	}

	// The words were collected into the vocabulary. Write it to the database.
	writeVocab();

	// Write max n-gram order
	writeLMInfo();
}

void LangModelBDB::writeLMInfo() 
{
	this->writeProperty(PROP_LM_ORDER, m_nMaxOrder);
}

std::string getNGramDBName(unsigned int nOrder)
{
	char buffer[100];
	lw_snprintf(buffer, sizeof(buffer),
		DB_NAME_NGRAM, nOrder);

	return buffer;
}


void LangModelBDB::dump(std::ostream& out) 
{
	LW_ASSERT(m_pInfoDB);
	LW_ASSERT(m_dbVector.size() > 0);

	// "info" database
	unsigned int nMaxOrder;
	readProperty(PROP_LM_ORDER, nMaxOrder);
	out << "Language Model Order: " << nMaxOrder << endl;

	for (unsigned int nOrder = 1; nOrder <= nMaxOrder; nOrder++) {
		Db* pDB = m_dbVector[nOrder - 1];
		assert(pDB);

		Dbc* pCursor = NULL;

		string sOperation = "Dumping n-grams.";

		out << endl << "Order: " << nOrder << endl;

		try {
			pDB->cursor(NULL, &pCursor, 0);

			LangModelBDBEntry lmEntry(nOrder);
			Dbt& key = lmEntry.getKeyForReading();
			Dbt& data = lmEntry.getDataForReading();

			int ret;
			while (0 == (ret = pCursor->get(&key, &data, DB_NEXT))) {
				lmEntry.afterRead();

				LWVocab::WordID words[MAX_SUPPORTED_ORDER];
				lmEntry.getWords(words, nOrder);

				for (unsigned int i = 0; i < nOrder; i++) {
					cout << m_pVocab->getWord(words[i]);
					cout << "\t";
				}
				out << lmEntry.getProb() << "\t" << lmEntry.getBOW() << endl; 

				key = lmEntry.getKeyForReading(); 
				data = lmEntry.getDataForReading();
			}
		}
		catch(DbException& e) {
			if (pCursor) {
				pCursor->close();
			}
			RETHROW_EXCEPTION(sOperation.c_str(), e);
		}
		catch(std::exception& e) {
			if (pCursor) {
				pCursor->close();
			}
			RETHROW_EXCEPTION(sOperation.c_str(), e);
		}
	}
}

LangModel::ProbLog LangModelBDB::getContextProb(LWVocab::WordID* pNGram, unsigned int nOrder)
{
	// We only support up to getMaxOrder() n-grams
	if (nOrder > getMaxOrder()) {
		unsigned int nOldOrder = nOrder;
		nOrder = getMaxOrder();
		// If the size of the context is reduced, move the starting pointer appropriately
		pNGram = pNGram + (nOldOrder - nOrder);
	}

	LangModel::ProbLog dReturn;
	LangModelBDBEntry probNode;
	bool bFound = findProbNode(pNGram, nOrder, &probNode);
	if (bFound) {
		// Found the probability for n-gram
		dReturn = probNode.getProb();
	}
	else if (nOrder > 1) {
		// Prob not found
		// Find Back Off Weight
		bFound = findProbNode(pNGram, nOrder - 1, &probNode);
		if (bFound) {
			// Recurse to a shorter n-gram
			dReturn = probNode.getBOW() + getContextProb(pNGram + 1, nOrder - 1);
		}
		else {
			// No BOW. Just use the probablity of the shorter context
			dReturn = getContextProb(pNGram + 1, nOrder - 1);
		}
	}
	else {
		// No probability found. None of the words have been seen during training, or they have been discarded
		dReturn = PROB_LOG_ZERO; // Minus infinity
	}

	return dReturn;
}

bool LangModelBDB::findProbNode(LWVocab::WordID* pNGram, unsigned int nOrder, LangModelBDBEntry* pProbNode)
{
	assert(nOrder > 0);
	assert(nOrder <= m_dbVector.size());

	// The database for each n-gram order is in a vector
	Db* pDB = m_dbVector[nOrder - 1];

	// Prepare the key and data for BDB
	pProbNode->setWords(pNGram, nOrder);
	Dbt& key = pProbNode->getKeyForReading();
	Dbt& data = pProbNode->getDataForReading();

	int nResult = pDB->get(NULL, &key, &data, 0);	
	if (0 == nResult) {
		// Found
		// Perform marshalling
		pProbNode->afterRead();
		
		return true;
	}
	else {
		// Node not found
		return false;
	}

	// This line is never reached
	return false;
}

void LangModelBDB::breakFilePath(const std::string& sPath, std::string& sDir, std::string& sFileName)
{
	QString qsPath = QString::fromUtf8(sPath.c_str());

	QFileInfo fileInfo(qsPath);

	QString qsDir = fileInfo.absolutePath();
	QString qsFileName = fileInfo.fileName();

	sDir = qsDir.toUtf8();
	sFileName = qsFileName.toUtf8();
}

} // namespace

// LM_NO_BDB_LIB
#endif
