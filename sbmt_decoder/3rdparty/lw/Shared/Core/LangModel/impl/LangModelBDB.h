// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _LANG_MODEL_BDB_H
#define _LANG_MODEL_BDB_H

#ifndef LM_NO_BDB_LIB

#include <string>
#include <vector>
#include <istream>

#define HAVE_CXX_STDHEADERS 1
#include <db_cxx.h>


#include "LangModelImplBase.h"

class Db;
class DbEnv;

namespace LW {
class LWVocab;
class LangModelBDBEntry;

class LangModelBDB: public LangModelImplBase {
private:
	/**
	We store one BDB database per order in a vector
	*/
	typedef std::vector<Db*> DBVector;
public:
	/**
	Create a BDB Language Mode. The Vocab object can optionally be passed as a pointer
	If not passed, the Language Model will create its own Vocab object
	This class will not attempt to delete pVocab pointer. Deleting it is the responsibility of the caller.
	*/
	LangModelBDB(LWVocab* pVocab = NULL);
public:
	virtual ~LangModelBDB();
public: // Abstract methods we must implement
	/// Clears all counts and probabilities and frees memory associated with them
	virtual void clear(){};
	/// Performs training o a sentence
	virtual void learnSentence(LWVocab::WordID* pSentence, unsigned int nSentenceSize){};
	/// Called after learning is finishted to prepare counts, etc.
	virtual void prepare(){}
	/// Returns the probability of a sequence of words of size order
	virtual LangModel::ProbLog getContextProb(LWVocab::WordID* pNGram, unsigned int nOrder);
	///// Returns the probability of a word, given a context
	//virtual LangModel::ProbLog getContextProb(LWVocab::WordID nWord, LWVocab::WordID* pnContext, unsigned int nOrder) = 0;
	/// Loads the language model from a file (previously saved with write())
	virtual void read(std::istream& in, LangModel::ReadOptions nOptions = LangModel::READ_NORMAL){};
	/// Loads the language model counts from a file (previously saved with writeCounts())
	virtual void readCounts(std::istream& in) {};
	/// Writes the language model to a file
	virtual void write(std::ostream& out, LangModel::SerializeFormat nFormat) {};
	/// Writes the language model counts to a file
	virtual void writeCounts(std::ostream& out) {};
	/// Writes the language model in human readable format
	virtual void dump(std::ostream& out);
	/// Returns the actual max order of the LM
	virtual unsigned int getMaxOrder() const {
		return m_nMaxOrder;
	}
public:
	void convertFromTextLM(std::istream& in);
private:
	bool findProbNode(LWVocab::WordID* pNGram, unsigned int nOrder, LangModelBDBEntry* pProbNode);
public:
	/**
	Opens all databases pertaining to this Language Model (info, vocab and one database per n-gram order)
	*/
	void open(const std::string& sFileName);
	/**
	Creates all databases pertaining to this Language Model (info, vocab and one database per n-gram order)
	*/
	void create(const std::string& sFileName, unsigned int nOrder);
private:
	/**
	Opens a Berkeley DB environment
	*/
	DbEnv* openEnv(const std::string& sHomeDir, u_int32_t nFlags = 0);
	/**
	Creates a Berkeley DB environment
	*/
	DbEnv* createEnv(const std::string& sHomeDir);
	/**
	Opens a database
	@param pEnv the environment in which to open the database
	@param sFileName the file name for the database
	@param sDBName if multiple databases are hosted in the same file, the name of the database
	@param nFlags database open flags as specified in Berkeley DB docs. If this parameter is omited, 
	 the method assumes we want to open an existing database
	*/
	Db* openDB(DbEnv* pEnv, const std::string& sFileName, const std::string& sDBName, u_int32_t nFlags = 0);
	/**
	This method will attempt to create a Berkeley DB. If the database exists, an exception will be thrown
	For parameter description see openDB()
	*/
	Db* createDB(DbEnv* pEnv, const std::string& sFileName, const std::string& sDBName);
private:
	/**
	Database >> LWVocab object
	*/
	void readVocab();
	/**
	LWVocab object >> Database
	*/
	void writeVocab();
	/**
	Writes the LM "header" information (e.g. LM order)
	*/
	void writeLMInfo();
private:
	/**
	Returns debugging information in human readable format. 
	Used when throwing exceptions.
	*/
	const std::string& getDebugInfo() {
		return m_sDebugInfo;
	}
	/**
	Returns the database name for a given order
	*/
	static std::string getDBNameForOrder(unsigned int nOrder);
	/**
	Reads one property by name from "info" database
	*/
	bool readProperty(const std::string& sPropName, unsigned int &nValue);
	/**
	Reads one property by name from "info" database
	*/
	bool readProperty(const std::string& sPropName, std::string& sValue);
	/**
	Writes one property by name from "info" database
	*/
	bool writeProperty(const std::string& sPropName, unsigned int nValue);
	/**
	Writes one property by name from "info" database
	*/
	bool writeProperty(const std::string& sPropName, const std::string& sValue);
	/**
	Breaks a file path into directory and file name
	*/
	void breakFilePath(const std::string& sPath, std::string& sDir, std::string& sFileName);
private:
	/**
	The file name for this database (we have multiple databases in the same file)
	*/
	std::string m_sFileName;
	/**
	Vocabulary object
	*/
	LWVocab* m_pVocab;
	/**
	Flag that tells whether we should delete the vocabulary in the destructor
	*/
	bool m_bOwnVocab;
	/**
	LM Order
	*/
	unsigned int m_nMaxOrder;
	/**
	Lang model name. This is important as the LMInfo file, the directory where the 
	BDBs are stored and the BDB names must all be in sync.
	*/
	/**
	BDB Environment
	*/
	DbEnv* m_pEnv;
	/**
	Vector of BDB databases, one db per order
	*/
	DBVector m_dbVector;
	/**
	Small database that contains LM properties (e.g. max n-gram order)
	*/
	Db* m_pInfoDB;
	/**
	Vocabulary database
	*/
	Db* m_pVocabDB;
	/**
	Human readable information about this Language Model
	*/
	std::string m_sDebugInfo;
};


} // namespace

// LM_NO_BDB_LIB
#endif 

#endif
