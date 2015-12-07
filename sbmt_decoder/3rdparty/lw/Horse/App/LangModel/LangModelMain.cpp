// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

// NOTE: This is here because it is the first thing that includes Qt
// If we change the ordering, _CRTDBG_MAP_ALLOC's realloc macro collides
// with Qt's local member function realloc used for QByteArray, etc.
#include "LangModel/LangModelFactory.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>

#ifdef _WIN32 
#include <crtdbg.h>
#endif

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <map>
//#include <hash_map>
#ifdef __GCC2__
#include <stdio.h>
#endif

#include "Common/Util.h"
#include "Common/ParamParser.h"
#include "Common/FileOp.h"
#include "LangModel/CountsMerger.h"
#include "LangModel/CountsTrainer.h"
#include "LangModel/LangModelHistory.h"
#include "LangModel/LangModelHistoryFactory.h"
#include "LangModel/CustomizerLMTrainer.h"
#include "LangModel/impl/LangModelBDB.h"

using namespace std;
using namespace LW;

// When we split the count files, how many sentences we put in each file.
#define SENT_PER_COUNT_FILE 500000
#define COUNTS_LST "counts.lst"


LangModelParams langModelParams; //FIXME: pass as argument to helpers

class TempTokenizer 
{
public:
	void tokenize(const char* szSentence, char** pWords, unsigned int* pnWordCount, unsigned int nMaxWordCount);
};

/**
A bdb file name is prefixed by "bdb:"
e.g. "bdb:c:\lm\eng.lm"
*/
bool isBDBFileName(const std::string& sFileName) {
	return (sFileName.substr(0, 4) == "bdb:");
}

/**
This function removes "bdb:" prefix if present on a file name
*/
string getBDBFileName(const std::string& sFileName) {
	if (isBDBFileName(sFileName)) {
		return sFileName.substr(4);
	}
	else {
		return sFileName;
	}
}

//class ParamParser
//{
//private:
//	struct ParamEntry {
//		string  m_sValue;
//		bool	m_bHasValue;
//		bool	m_bFound;
//	};
//public:
//	typedef map<string, ParamEntry> ParamMap;
//	typedef vector<string> StringVector;
//public:
//	ParamParser();
//	void registerParam(const char* szParamName, bool bHasValue);
//	void parse(int argc, char** argv);
//	bool getNamedParam(const char* szParamName, string& sValue) const;
//	bool getNamedParam(const char* szParamName, int& nValue) const;
//	bool getNamedParam(const char* szParamName) const;
//	string getUnnamedParam(int unsigned nIndex) const;
//private:
//	ParamMap m_namedParams;
//	StringVector	m_unnamedParams;
//};
//
//ParamParser::ParamParser()
//{
//}
//
//void ParamParser::registerParam(const char* szParamName, bool bHasValue)
//{
//	ParamEntry paramEntry;
//	paramEntry.m_bFound = false;
//	paramEntry.m_bHasValue = bHasValue;
//	m_namedParams.insert(ParamMap::value_type(szParamName, paramEntry));
//}
//
//void ParamParser::parse(int argc, char** argv)
//{
//	for (int i = 1; i < argc; i++) {
//		string sParam(argv[i]);
//		// Is it a named param?
//		if (sParam[0] == '-') {
//			string sName = sParam.substr(1);
//			// Has this param been registered?
//			ParamMap::iterator it = m_namedParams.find(sName);
//			if (m_namedParams.end() == it) {
//				throw string("Parameter not found: ") + sName;
//			}
//			// Is it supposed to have a value?
//			if (it->second.m_bHasValue) {
//				if (i >= argc - 1) {
//					throw string("Parameter ") + sName + " has no value.";
//				}
//				it->second.m_sValue = argv[i + 1];
//				// Skip next parameter, as it was a value
//				i++;
//			}
//
//			it->second.m_bFound = true;
//		}
//		else {
//			string sValue(argv[i]);
//			// Is it an unnamed param
//			m_unnamedParams.push_back(sValue);
//		}
//	}
//}
//
//bool ParamParser::getNamedParam(const char* szParamName, string& sValue) const
//{
//	ParamMap::const_iterator it = m_namedParams.find(string(szParamName));
//	bool bReturn = false;
//	if (it != m_namedParams.end()) {
//		if (it->second.m_bFound) {
//			sValue = it->second.m_sValue;
//			bReturn = true;
//		}
//	}
//	return bReturn;
//}
//
//bool ParamParser::getNamedParam(const char* szParamName, int& nValue) const
//{
//	string sParam;
//	if (!getNamedParam(szParamName, sParam)) {
//		// Not found
//		return false;
//	}
//	else {
//		// Found
//		nValue = atoi(sParam.c_str());
//		return true;
//	}
//}
//
//
//bool ParamParser::getNamedParam(const char* szParamName) const {
//	string sDummy;
//	return getNamedParam(szParamName, sDummy);
//}
//
//
//string ParamParser::getUnnamedParam(unsigned int nIndex) const
//{
//	if (nIndex >= 0 && nIndex < m_unnamedParams.size()) {
//		return m_unnamedParams[nIndex];
//	}
//	else {
//		return "";
//	}
//}
//
//

void convertBytesToSentence(char* inBuffer, size_t nInBufferSize, char** sentence, char* sentenceWords)
{
	// Make words out of bytes e.g.
	// 0x04 0x08 0x07
	// "1 2 7"
	char szWordBuffer[20];
	char* szLastWord = sentenceWords;
	for (size_t i = 0; i < nInBufferSize; i++) {
		sprintf(szWordBuffer, "%d ", (unsigned int) (unsigned char) inBuffer[i]);
		unsigned int nWordSize = static_cast<unsigned int>(strlen(szWordBuffer));
		memcpy(szLastWord, szWordBuffer, nWordSize);
		// Add a NULL after each word
		szLastWord[nWordSize] = 0;

		// Add the pointer to this word
		sentence[i] = szLastWord;

		// Move the pointer after the NULL
		szLastWord += nWordSize + 1;
	}
}

void TempTokenizer::tokenize(const char* szSentence, char** pWords, unsigned int* pnWordCount, unsigned int nMaxWordCount)
{
	static char const* separators = "\r\n\t ";

	char* pWord = strtok((char*) szSentence, separators);
	unsigned int i = 0;
	for (i = 0; (i < nMaxWordCount) && (NULL != pWord); i++) {
		pWords[i] = pWord;
		pWord = strtok(NULL, separators);
	}
	*pnWordCount = i;
}

bool getNextSentence(istream& input, char** pWord, unsigned int* pnWordCount, unsigned int nMaxWordCount)
{
	// This buffer must be static, as it returns references inside it.
	// Obviously this is NOT inteded to work with multiple threads
	static char buffer[MAX_WORDS_IN_SENTENCE * 16];

	// Truncate to a reasonable size if necessary
	// MAX_WORDS_IN_SENTENCE is a avery high value. If we exceeded it, something was wrong.
	if (nMaxWordCount > MAX_WORDS_IN_SENTENCE) {
		nMaxWordCount = MAX_WORDS_IN_SENTENCE;
	}

	safeGetLine(input, buffer, sizeof(buffer));

	TempTokenizer tokenizer;
	tokenizer.tokenize(buffer, pWord, pnWordCount, nMaxWordCount);

	return !input.fail();
}

class TestClass {
public: 
	int m;
};

void printUsage() 
{
	cerr << "Usage:" << endl;
	cerr << "1. Training:" << endl;
	cerr << " LangModel -train [-lm <lm-file-output>] [-in <train-file-input>] [-counts <cout-file-output>] [-bin] [-nopad] [-order <max_ngram_order>]" << endl;
	cerr << " -bin:    binary output. The default is text." << endl;
	cerr << " -bin-trie: same as -bin (trie format)" << endl;
	cerr << " -bin-sa: new, more efficient format: sorted array" << endl;
	cerr << " -nopad:  the Language Model will assume that the sentences are already padded with <s> and </s>." << endl;
	cerr << " If -in is missing it defaults to standard input" << endl;
	cerr << " At least one of -lm or -counts must be specified." << endl;
	cerr << endl;
	cerr << "2. Tranining (partial counts)" << endl;
	cerr << " LangModel -train-counts -in<train-file-input> [-sent-per-count-file <sentence-count>] [-nopad]" << endl;
	cerr << " -nopad:  the Language Model will assume that the sentences are already padded with <s> and </s>." << endl;
	cerr << "   NOTE: The list of the files that contain counts will be created in counts.lst file in the current directory." << endl;
	cerr << endl;
	cerr << "3. Merge counts computed by -train-counts -out <merged-file-name>" << endl;
	cerr << " LangModel -merge-counts -out <counts-file> [-cutoffs <1-gram>,<2-gram>,<3-gram>,<4-gram>,<5-gram>]" << endl;
	cerr << "  <c1>,<c2>, etc are the cutoffs for unigrams, bigrams, etc. (separated by comma, no space)." << endl;
	cerr << "  NOTE: The list of the files that contain counts must be supplied in counts.lst file in the current directory." << endl;
	cerr << endl;
	cerr << "4. Convert counts (generated by -train-counts or -merge-counts) to a language model." << endl;
	cerr << " LangModel -convert-counts -in <count-file> -lm <lm-file-output> [-bin]" << endl;
	cerr << endl;
	cerr << "5. Calculate perplexity:" << endl;
	cerr << " LangModel -ppl -in <lm-file-input> -in <test-file-input>" << endl;
	cerr << "6. Convert Language Model from text to binary format:" << endl;
	cerr << " LangModel -bin2text -lm-in <lm-file-input> -lm-out <lm-file-out>" << endl;
	cerr << "7. Convert Language Model from binary to text:" << endl;
	cerr << " LangModel -text2bin -lm-in <lm-file-input> -lm-out <lm-file-out>" << endl;
	cerr << "8. Convert Language Model from sorted array (binary) to text:" << endl;
	cerr << " LangModel -sa2text -lm-in <lm-file-input> -lm-out <lm-file-out>" << endl;
	cerr << endl;
	cerr << "9. Calculate the probability of each sentence in a file" << endl;
	cerr << " LangModel -prob -lm <lm-file> -in <input-file> " << endl;
	cerr << "  NOTE: Start Sentence <s> and End Sentence </s> words are NOT automatically inserted";
	cerr << " so use them if you want to compute the probability of the whole sentence." << endl;
	cerr << endl;
	cerr << "10. Train Customizer" << endl;
	cerr << " LangModel -train-customize -in <domain-corpus> -lm-gen <generic-lm> -counts-gen <generic-counts> -lm-dom <domain-lm> -counts-comb <combined-counts>" << endl;
	cerr << "  Trains the customizer." << endl;
	cerr << "  Input:" << endl;
	cerr << "       <domain-corpus> - Domain corpus (text file)" << endl;
	cerr << "       <generic-lm> - Generic Language Model (created by regular training)" << endl;
	cerr << "       <generic-counts> - Generic counts (created by regular training)" << endl;
	cerr << "  Output:" << endl;
	cerr << "        <domain-lm> - Lang Model to train for the domain data" << endl;
	cerr << "        <combined-counts> - Counts for both Generic/Domain" << endl;
	cerr << endl;
	cerr << "11. Dump vocabulary for a given Language Model" << endl;
	cerr << " LangModel -dump-vocab -lm <lm-file>" << endl;
	cerr << endl;
}

/**
* @param lm Language model to dump the counts for
* @param szCountsFilePattern describes how the counts file name is created (e.g. "file%s.lmc")
* @param nCurrentCountFile this number must be unique and it is used to create the counts file name, using sprintf and the pattern supplied
* @param listing stream to write the output file name to. The stream must be opened by the caller
*/
bool dumpPartialCounts(LangModel& lm, const char* szCountFilePattern, int nCurrentCountFile, ostream& listing)
{
	// Time to dump the counts to a new file
	char szCountFileName[100];
	sprintf(szCountFileName, szCountFilePattern, nCurrentCountFile);

	cerr << "Writing partial counts to file: " << szCountFileName << " ..." << endl;

	// Open counts file
	ofstream counts(szCountFileName, ios::out | ios::binary);
	if (counts.fail()) {
		cerr << "Cannot open counts file: " << szCountFileName << endl;
		return false;
	}

	// Dump the counts
	lm.writeCounts(counts);

	counts.close();

	// Add the file to the listing file
	listing << szCountFileName << endl;
	listing.flush();

	return true;
}


bool trainCounts(const string& sTrainFile, const LangModelParams& params, int nSentPerCountFile, bool bPadWithStartEnd)
{
	char* sentence[MAX_WORDS_IN_SENTENCE];
	char const* listingFileName = COUNTS_LST;
	char const* countFilePattern = "lm%d.count";

	int nSentCount = nSentPerCountFile;

	LangModel* lm = LangModelFactory::getInstance()->create(params, LangModelFactory::LANG_MODEL_TRIE);

	// Create the "master" file that holds the lists of all count files produced
	ofstream listing(listingFileName, ios::out | ios::binary);
	if (listing.fail()) {
		cerr << "Cannot open count file listing file: " << listingFileName << endl;
		return false;
	}
	

	// Open the input file
	ifstream trainFile(sTrainFile.c_str(), ios::in | ios::binary);
	if (trainFile.fail()) {
		cerr << "Cannot open training file " << sTrainFile << endl;
		return false;
	}

	cerr << "Training counts for language model. " << endl;
	cerr << "Input file: " << sTrainFile << endl;
	cerr << "Max n-gram order: " << params.m_nMaxOrder << endl;
	unsigned int nWordCount;
	unsigned int nTotalWordCount = 0;
	int nCurrentCountFile = 0;

	cerr << "Counting..." << endl;
	while (getNextSentence(trainFile, sentence, &nWordCount, MAX_WORDS_IN_SENTENCE)) {
		lm->learnSentence(sentence, nWordCount, bPadWithStartEnd);
		nTotalWordCount += nWordCount;

		nSentCount--;
		if (nSentCount <= 0) {
			nSentCount = nSentPerCountFile;
			
			dumpPartialCounts(*lm, countFilePattern, nCurrentCountFile, listing);
			nCurrentCountFile++;

			// Clear counts
			lm->clear();

			cerr << "Counting..." << endl;
		}

	}
	// Dump the balance of counts the the last file
	dumpPartialCounts(*lm, countFilePattern, nCurrentCountFile, listing);
	trainFile.close();
	listing.close();

	delete lm;
	return true;
}

bool mergeCounts(const string& sOutFileName, LangModelParams& params) 
{

	cerr << "Merging counts." << endl;
	string sListingFileName = COUNTS_LST;

	// Open the listing file
	ifstream listing(sListingFileName.c_str(), ios::in | ios::binary);
	if (listing.fail()) {
		cerr << "Cannot open listing file " << sListingFileName << endl;
		return false;
	}

	// Create the merger class
	CountsMerger merger(params);

	// Get all input file names from the listing file
	vector<string> inputFileNames;
	while(!listing.fail()) {
		char szInputFileName[1000];
		safeGetLine(listing, szInputFileName, sizeof(szInputFileName));

		if (strlen(szInputFileName) > 0) {
			// Put the file name in a vector
			inputFileNames.push_back(string(szInputFileName));
		};
	}

	// Perform the merge
	if (!merger.merge(cerr, inputFileNames, sOutFileName)) {
		cerr << "Merge failed. See previous error messages." << endl;
		return false;
	}

	// At this point, the counts are sorted by ngrams and placed in one file per n-gram order
	// The file name is sOutFileName + "." + N where N is the n-gram order
	// Alter the counts for Knesser Ney
	for (unsigned int nOrder = 1; nOrder <= params.m_nMaxOrder; nOrder++) {
		ifstream in;
		ofstream out;
		char szInFileName[1000];
		char szOutFileName[1000];
		sprintf(szInFileName, "%s.%d", sOutFileName.c_str(), nOrder);
		sprintf(szOutFileName, "%s.extra.%d", sOutFileName.c_str(), nOrder);
		in.open(szInFileName, ios::in | ios::binary);
		out.open(szOutFileName, ios::out | ios::binary);

		CountsTrainer::processKNCounts(nOrder, in, out);

		in.close();
		out.close();
	}

	// Now we have to sort the counts again, as the previous operation introduced new records
	// for counts
	return true;
}

bool convertCountsForKM(const string& sCountsFile) 
{
	cerr << "Converting counts to language model." << endl;
	ifstream countsFile(sCountsFile.c_str(), ios::in | ios::binary);
	if (countsFile.fail()) {
		cerr << "Cannot open counts file " << sCountsFile << endl;
		return false;
	}
	return false;
}

bool convertCounts(const string& sCountsFile, const string& sLangModelFile, LangModelParams& params, LangModel::SerializeFormat nFormat) 
{
	cerr << "Converting counts to language model." << endl;
	ifstream countsFile(sCountsFile.c_str(), ios::in | ios::binary);
	if (countsFile.fail()) {
		cerr << "Cannot open counts file " << sCountsFile << endl;
		return false;
	}

	ofstream langModelFile(sLangModelFile.c_str(), ios::binary | ios::out);
	if (langModelFile.fail()) {
		cerr << "Cannot open Language Model file " << sLangModelFile << endl;
		return false;
	}

	LangModel* lm = LangModelFactory::getInstance()->create(params, LangModelFactory::LANG_MODEL_TRIE);

	cerr << "Reading counts from " << sCountsFile << " ..." << endl;
	lm->readCounts(countsFile);

	cerr << "Computing probabilities..." << endl;
	lm->finishedCounts();

	cerr << "Saving Language Model to " << sLangModelFile << endl;
	lm->write(langModelFile, nFormat);

	delete lm;

	countsFile.close();
	langModelFile.close();

	return true;
}

SmartPtr<LangModel> loadLM(const std::string& sFileName, bool bIsCustomizerLM, LangModel::ReadOptions nReadOptions = LangModel::READ_NORMAL,LangModelParams const& params=langModelParams)
{
	SmartPtr<LangModel> lm;
	if (bIsCustomizerLM) {
		// We have 3 file names, divided by commas
		// GenericLM,DomainLM,DomainCounts
		string sGenericLM;
		string sDomainLM;
		string sDomainCounts;

		char szBuffer[1000];
		strncpy(szBuffer, sFileName.c_str(), sizeof(szBuffer));
		szBuffer[sizeof(szBuffer)] = 0;

		// Tokenize into 3 strings
		sGenericLM = strtok(szBuffer, ",");
		sDomainLM = strtok(NULL, ",");
		sDomainCounts = strtok(NULL, ",");

		if (sGenericLM.empty() || sDomainLM.empty() || sDomainCounts.empty()) {
			throw Exception(ERR_IO, "Invalid customizer language model. Use GenericLM,CustomizerLM,DomainCounts (separated by comma).");
		}

		ifstream domainLMIn(sDomainLM.c_str(), ios::binary | ios::in);
		if (domainLMIn.fail()) {
			string sError =  string("") + "Cannot open domain language model input file " + sDomainLM;
			throw Exception(ERR_IO, sError);
		}

		ifstream domainCountsIn(sDomainCounts.c_str(), ios::binary | ios::in);
		if (domainCountsIn.fail()) {
			string sError = string("") + "Cannot open domain counts file file " + sDomainCounts;
			throw Exception(ERR_IO, sError);
		}

		// Read generic LM (call this function again)
		SmartPtr<LangModel> spGenericLM = loadLM(sGenericLM, false,nReadOptions,params);

		// Read domain LM
		cerr << "Reading domain LM from file " << sDomainLM << "..." << endl;
//		LangModelParams params;
		SmartPtr<LangModel> spDomainLM = LangModelFactory::getInstance()->create(params);
		spDomainLM->read(domainLMIn);
		domainLMIn.close();

		cerr << "Reading domain counts file from " << sDomainCounts << "..." << endl;

		lm = LangModelFactory::getInstance()->createCustomizerLM(spGenericLM, spDomainLM, domainCountsIn);
	}
	else if (isBDBFileName(sFileName)) {
		// Remove "bdb:" prefix
		string sBDBFileName = getBDBFileName(sFileName);

		cerr << "Reading BDB language model from file: " << sBDBFileName << endl;

		//lm = LangModelFactory::getInstance()->createBDBLangModel(sBDBFileName,params);
	}
	else {
		string sLangModelFile = sFileName;

		cerr << "Reading language model from file: " << sLangModelFile << endl;

		ifstream langModelFile(sLangModelFile.c_str(), ios::binary | ios::in);
		if (langModelFile.fail()) {
			throw Exception(ERR_IO, string("") + "Cannot open language model input file " + sLangModelFile + ".");
		}

		lm = LangModelFactory::getInstance()->create(langModelFile, nReadOptions,params);

		langModelFile.close();
	}


	return lm;
}


//bool convertCounts(const string& sCountsFile, const string& sLangModelFile, LangModelParams& params, bool bBinaryOut) 
//{
//	cerr << "Converting counts to language model." << endl;
//	ifstream countsFile(sCountsFile.c_str(), ios::in);
//	if (countsFile.fail()) {
//		cerr << "Cannot open counts file " << sCountsFile << endl;
//		return false;
//	}
//
//	ofstream langModelFile(sLangModelFile.c_str(), ios::binary | ios::out);
//	if (langModelFile.fail()) {
//		cerr << "Cannot open Language Model file " << sLangModelFile << endl;
//		return false;
//	}
//
//	LangModel* lm = LangModelFactory::getInstance()->create(params);
//
//	cerr << "Reading counts from " << sCountsFile << " ..." << endl;
//	lm->readCounts(countsFile);
//
//	cerr << "Computing probabilities..." << endl;
//	lm->finishedCounts();
//
//	cerr << "Saving Language Model to " << sLangModelFile << endl;
//	lm->write(langModelFile, bBinaryOut);
//
//	delete lm;
//
//	countsFile.close();
//	langModelFile.close();
//
//	return true;
//}

bool train(const string& sLangModelFile, const string& sCountsFile, const string& sTrainFile, const LangModelParams& params, LangModel::SerializeFormat nFormat, bool bPadWithStartEnd, bool bDumpWords) 
{
	char* sentence[MAX_WORDS_IN_SENTENCE];

	// We must create a KN model, the SA (sorted array) does not have the ability to train
	LangModel* lm = LangModelFactory::getInstance()->create(params, LangModelFactory::LANG_MODEL_TRIE);

	ifstream trainFile;
	istream* pTrainInput;
	if (!sTrainFile.empty()) {
		trainFile.open(sTrainFile.c_str(), ios::in | ios::binary);
		if (trainFile.fail()) {
			cerr << "Cannot open training file " << sTrainFile << endl;
			return false;
		}
		pTrainInput = &trainFile;
	}
	else {
		pTrainInput = &cin;
	}

	//cerr << "Max n-gram order: " << params.m_nMaxOrder << endl;
	//if (bBinaryOut) {
	//	cerr << "Language Model output in binary format." << endl;
	//}
	//else {
	//	cerr << "Language Model output in text format." << endl;
	//}

	unsigned int nWordCount;
	unsigned int nTotalWordCount = 0;
	while (getNextSentence(*pTrainInput, sentence, &nWordCount, MAX_WORDS_IN_SENTENCE)) {
		lm->learnSentence(sentence, nWordCount, bPadWithStartEnd);

		if (bDumpWords) {
			for (unsigned int i = 0; i < nWordCount; i++) {
				cout << sentence[i] << " ";
			}
			cout << endl;
		}

		nTotalWordCount += nWordCount;
	}
//	trainFile.close();

	cerr << "Word count: " << nTotalWordCount << endl;

	if (!sCountsFile.empty()) {
		ofstream outCounts(sCountsFile.c_str(), ios::binary | ios::out);
		if (outCounts.fail()) {
			cerr << "Cannot open counts file " << sCountsFile << endl;
			return false;
		}
		lm->writeCounts(outCounts);
		outCounts.close();
	}

	// If not LM file, no need to compute probabilities
	if (!sLangModelFile.empty()) {
		cerr << "Computing probabilities from counts" << endl;
		lm->finishedCounts();
	}

	// Write to LM file
	{
		// Give the user the option to not write out a language model, just counts
		if (!sLangModelFile.empty()) {
			cerr << "Writing out language model to file: " << sLangModelFile << endl;
			ofstream out(sLangModelFile.c_str(), ios::binary | ios::out);
			if (out.fail()) {
				cerr << "Cannot open language model output file " << sLangModelFile << endl;
				return false;
			}
			lm->write(out, nFormat);
			out.close();
		}
	}
	//{
	//	std::string sCountsFile = sLangModelFile + ".count";
	//	cerr << "Writing out counts to file: " << sCountsFile << endl;

	//	ofstream out(sCountsFile.c_str(), ios::binary | ios::out);
	//	if (out.fail()) {
	//		cerr << "Cannot open counts output file " << sCountsFile << endl;
	//		return false;
	//	}
	//	lm->writeCounts(out);
	//	out.close();
	//}

	delete lm;

	return true;
}

bool calculatePerplexity(const string& sLangModelFile, const string& sTestFile, const LangModelParams& params, bool bCustomizerLM = false)
{
	char* sentence[MAX_WORDS_IN_SENTENCE];

	cerr << "Computing perplexity. Input file: " << sTestFile << endl;;
	
	//ifstream langModelFile(sLangModelFile.c_str(), ios::binary | ios::in);
	//if (langModelFile.fail()) {
	//	cerr << "Cannot open language model input file " << sLangModelFile << endl;
	//	return false;
	//}

	ifstream in(sTestFile.c_str(), ios::in | ios::binary);
	if (in.fail()) {
		cerr << "Cannot open input file " << sTestFile << endl; 
		return false;
	}
	//

//	cerr << "Reading language model from file: " << sLangModelFile << endl;
//	SmartPtr<LangModel> lm = LangModelFactory::getInstance()->create(langModelFile);
////	lm->read(langModelFile);
//	langModelFile.close();
//
//	if (bCustomizerLM) {
//		// We have to load a domain LM and counts
//		string sDomainLM = sLangModelFile + ".dom.lm";
//		string sDomainCounts = sLangModelFile + ".dom.lmc";
//
//		ifstream domainLMIn(sDomainLM.c_str(), ios::binary | ios::in);
//		if (domainLMIn.fail()) {
//			cerr << "Cannot open domain language model input file " << sDomainLM << endl;
//			return false;
//		}
//
//		ifstream domainCountsIn(sDomainCounts.c_str(), ios::binary | ios::in);
//		if (domainCountsIn.fail()) {
//			cerr << "Cannot open domain counts file file " << sDomainCounts << endl;
//			return false;
//		}
//
//		// Read domain LM
//		cerr << "Reading domain LM from file " << sDomainLM << "..." << endl;
//		SmartPtr<LangModel> spDomainLM = LangModelFactory::getInstance()->create(params);
//		spDomainLM->read(domainLMIn);
//		domainLMIn.close();
//
//		// The orig LM is now the generic LM
//		SmartPtr<LangModel> spGenericLM = lm;
//
//		lm = LangModelFactory::getInstance()->createCustomizerLM(spGenericLM, spDomainLM, domainCountsIn);
//	}

	SmartPtr<LangModel> lm = loadLM(sLangModelFile, bCustomizerLM);

//	cerr << "Computing perplexity..." << endl;

	LangModel::ProbLog dProbLog = 0;
	unsigned int nWordCount;
	unsigned int nTotalWordCount = 0;
	while (getNextSentence(in, sentence, &nWordCount, MAX_WORDS_IN_SENTENCE)) {
		// Ignore blank lines
		if (nWordCount > 0) {
			LangModel::ProbLog dSentProb = lm->computeSentenceProbability(sentence, nWordCount);
			dProbLog += dSentProb;
			nTotalWordCount += nWordCount;
		}
	}
	in.close();

	LangModel::Perplexity ppl = LangModel::ProbLogToPPL(dProbLog / nTotalWordCount);

	cerr << "Word Count=" << nTotalWordCount << "; ";
	cerr << "log(prob)=" << dProbLog << "; ";
	cerr << " PPL=" << ppl;
	cerr << endl;
	return true;
}

bool calculatePerplexity2(const string& sLangModelFile, const string& sTestFile, const LangModelParams& params)
{
	char* sentence[MAX_WORDS_IN_SENTENCE];

	cerr << "Computing perplexity using history. Input file: " << sTestFile << endl;;
	
	ifstream langModelFile(sLangModelFile.c_str(), ios::binary | ios::in);
	if (langModelFile.fail()) {
		cerr << "Cannot open language model input file " << sLangModelFile << endl;
		return false;
	}

	ifstream in(sTestFile.c_str(), ios::in | ios::binary);
	if (in.fail()) {
		cerr << "Cannot open input file " << sTestFile << endl;
		return false;
	}

	cerr << "Reading language model from file: " << sLangModelFile << endl;
	LangModel* lm = LangModelFactory::getInstance()->create(in,LangModel::READ_NORMAL,params);
	langModelFile.close();

	LangModel::ProbLog dProbLog = 0;
	unsigned int nWordCount;
	unsigned int nTotalWordCount = 0;

	while (getNextSentence(in, sentence, &nWordCount, MAX_WORDS_IN_SENTENCE)) {
		// Ignore blank lines
		if (nWordCount > 0) {

			LangModel::ProbLog dProb = 0;
			LangModel::ProbLog dSentProb = 0;
		
			// Allocate here so it gets deallocated together with all
			// history objects when we leave the block
			LangModelHistoryFactory histFactory(3);
			LangModelHistory* pHistIn = histFactory.create();
			LangModelHistory* pHistOut = histFactory.create();

			// Do not compute the probability of the first word
			// just load it in the history, as the word is <s> and the prob is 0
			pHistIn->addWord(LWVocab::START_SENTENCE);

			for (unsigned int i = 0; i < nWordCount; i++) {
				LWVocab::WordID nWord = lm->getVocab()->getID(sentence[i]);
				dProb = lm->computeProbability(nWord, *pHistIn, *pHistOut);
				dSentProb += dProb;
				*pHistIn = *pHistOut;
			}

			// The prob of end of sentence
			dProb = lm->computeProbability(LWVocab::END_SENTENCE, *pHistIn, *pHistOut);
			dSentProb += dProb;

			dProbLog += dSentProb;
			nTotalWordCount += nWordCount;
		}
	}
	in.close();

	LangModel::Perplexity ppl = LangModel::ProbLogToPPL(dProbLog / nTotalWordCount);

	cerr << "Word Count=" << nTotalWordCount << "; ";
	cerr << "log(prob)=" << dProbLog << "; ";
	cerr << " PPL=" << ppl;
	cerr << endl;
	return true;
}

bool calculatePerplexityByte(const string& sLangModelFile, const string& sInputFile, const LangModelParams& params)
{
	char*	sentence[MAX_WORDS_IN_SENTENCE];
	char    sentenceWords[MAX_WORDS_IN_SENTENCE * 20];
	char	buffer[MAX_WORDS_IN_SENTENCE];

	// We must create a KN model, the SA (sorted array) does not have the ability to train
	LangModel* lm = LangModelFactory::getInstance()->create(params);

	ifstream langModelFile(sLangModelFile.c_str(), ios::binary | ios::in);
	if (langModelFile.fail()) {
		cerr << "Cannot open language model input file " << sLangModelFile << endl;
		return false;
	}

	ifstream inputFile;
	istream* pInputFile;
	if (!sInputFile.empty()) {
		inputFile.open(sInputFile.c_str(), ios::in | ios::binary);
		if (inputFile.fail()) {
			cerr << "Cannot open input file " << sInputFile << endl;
			return false;
		}
		pInputFile = &inputFile;
	}
	else {
		pInputFile = &cin;
	}

//	cerr << "Reading language model from file: " << sLangModelFile << endl;
	lm->read(langModelFile);
	langModelFile.close();

	unsigned int nTotalWordCount = 0;
	LangModel::ProbLog dProbLog = 0;
	while (*pInputFile) { 
		// Read from file
		pInputFile->read(buffer, sizeof(buffer));
		unsigned int nWordCount = static_cast<unsigned int> (pInputFile->gcount());

		//// Transform each byte into a WordID
		//for (size_t i = 0; i < nWordCount; i++) {
		//	sentence[i] = buffer[i];
		//}
		convertBytesToSentence(buffer, nWordCount, sentence, sentenceWords);

		nTotalWordCount += nWordCount;

		if (nWordCount > 0) {
			LangModel::ProbLog dSentProb = lm->computeSentenceProbability(sentence, nWordCount);
			dProbLog += dSentProb;
		}
	}

	LangModel::Perplexity ppl = LangModel::ProbLogToPPL(dProbLog / nTotalWordCount);

//	cerr << "Word Count (byte)=" << nTotalWordCount << "; ";
//	cerr << "log(prob)=" << dProbLog << "; ";
//	cerr << " PPL=" << ppl;
//	cerr << endl;

	cout << ppl << endl;
	return true;
}



bool trainByte(const string& sLangModelFile, const string& sTrainFile, const LangModelParams& params, LangModel::SerializeFormat nFormat) 
{
//	LWVocab::WordID sentence[MAX_WORDS_IN_SENTENCE];
	char		  buffer[MAX_WORDS_IN_SENTENCE];
//	char* words[MAX_WORDS_IN_SENTENCE_BYTE * 5];
	char*	sentence[MAX_WORDS_IN_SENTENCE];
	char   sentenceWords[MAX_WORDS_IN_SENTENCE * 20];

	// We must create a KN model, the SA (sorted array) does not have the ability to train
	LangModel* lm = LangModelFactory::getInstance()->create(params, LangModelFactory::LANG_MODEL_TRIE);
//	LangModelImplBase* lm = ((LangModelImpl*) lmInterface)->getImplementation();

	ifstream trainFile;
	istream* pTrainInput;
	if (!sTrainFile.empty()) {
		trainFile.open(sTrainFile.c_str(), ios::in | ios::binary);
		if (trainFile.fail()) {
			cerr << "Cannot open training file " << sTrainFile << endl;
			return false;
		}
		pTrainInput = &trainFile;
	}
	else {
		pTrainInput = &cin;
	}

	unsigned int nTotalWordCount = 0;
	while (*pTrainInput) { 
		// Read from file
		pTrainInput->read(buffer, sizeof(buffer));
		unsigned int nWordCount = static_cast<unsigned int>(pTrainInput->gcount());

		//// Transform each byte into a WordID
		//for (size_t i = 0; i < nWordCount; i++) {
		//	sentence[i] = buffer[i];
		//}
		
		// Make words out of bytes e.g.
		// 0x04 0x08 0x07
		// "1 2 7"
		convertBytesToSentence(buffer, nWordCount, sentence, sentenceWords);

		lm->learnSentence(sentence, nWordCount, false);
		nTotalWordCount += nWordCount;
	}
//	trainFile.close();

	cerr << "Word count: " << nTotalWordCount << endl;

	cerr << "Computing probabilities from counts" << endl;
	lm->finishedCounts();

	// Write to LM file
	{
		cerr << "Writing out language model to file: " << sLangModelFile << endl;
		ofstream out(sLangModelFile.c_str(), ios::binary | ios::out);
		if (out.fail()) {
			cerr << "Cannot open language model output file " << sLangModelFile<< endl;
			return false;
		}
		lm->write(out, nFormat);
		out.close();
	}
	//{
	//	std::string sCountsFile = sLangModelFile + ".count";
	//	cerr << "Writing out counts to file: " << sCountsFile << endl;

	//	ofstream out(sCountsFile.c_str(), ios::binary | ios::out);
	//	if (out.fail()) {
	//		cerr << "Cannot open counts output file " << sCountsFile << endl;
	//		return false;
	//	}
	//	lm->writeCounts(out);
	//	out.close();
	//}

	delete lm;
	return true;
}


#include <iostream>
#include <fstream>
#include "LangModel/impl/BinHashMap.h"
#include "LangModel/impl/Trie.h"
struct Ptr
{
public:
	static int m_nInstCount;
public:
	Ptr() {
		m_nInstCount++;
	}
	~Ptr() {
		m_nInstCount--;
	}
};

int Ptr::m_nInstCount = 0;

struct TestNode
{
	int m1;
	int m2;
	Ptr* ptr;
	TestNode() {
		m1 = 0;
		m2 = 0;
		ptr = new Ptr;
	}
	TestNode(int p1, int p2) {
		m1 = p1;
		m2 = p2;
		ptr = new Ptr();
	}
	~TestNode() {
		delete ptr;
	}
};

//void testBinHashMap() 
//{
//	typedef BinHashMap<unsigned int, TestNode> MyMap;
//	typedef hash_map<unsigned int, TestNode> StandardMap;
//
//	MyMap* pMap = new MyMap();
//	StandardMap map2;
//
//	MyMap::iterator it;
//	StandardMap::iterator it2;
//
////	TestNode node1(1, 11);
////	TestNode node2(2, 22);
////	map.insert(1, node1);
////	map.insert(2, node2);
//
//	unsigned int nCount = 100;
//
//	cerr << "inserting " << nCount << " elements ..." << endl;
//	for (unsigned int i = 0; i < nCount; i++) {
//		pMap->insert(i * 2);
//	}
//
//	//nCount = 10;
//
//	//cerr << "inserting " << nCount << " elements in std::map ..." << endl;
//	//for (unsigned int i = 0; i < nCount; i++) {
//	//	TestNode node(1, 1);
//	//	map2.insert(StandardMap::value_type(i * 2, node));
//	//}
//
//
//	nCount = 50;
//
////	pMap->m_nCollisionCount = 0;
//
//	unsigned int sum = 0;
//	cerr << "finding " << nCount << " elements..." << endl;
//	int nFoundCount = 0;
//	int nNotFoundCount = 0;
//	for (unsigned int i = 0; i < nCount; i++) {
//		// findElement is twice as fast, as it does not create an iterator
//		MyMap::Element* pElement = pMap->findElement(2 * i);
//		if (pElement) {
//			nFoundCount++;
//			// Check the key
//			if (pElement->first != 2 * i) {
//				cerr << "ERROR: Wrong key found: " << pElement->first << ". Expected  " << i * 2 << endl;
//			}
//		}
//		else {
//			nNotFoundCount++;
//		}
//
//
//		//it = pMap->find(i % 100000);
//		//if (it != pMap->end()) {
//		//	sum += (*it).first;
//		//}
//	}
//
//	cerr << "Found "<< nFoundCount << ". Failed to find " << nNotFoundCount << endl;
////	cerr << "Collision count = " << pMap->m_nCollisionCount << endl;
//
//	cerr << "finding " << nCount << " elements in std map..." << endl;
//	for (unsigned int i = 0; i < nCount; i++) {
//		it2 = map2.find(i % 100000);
//		if (it2 != map2.end()) {
//			sum += it2->first;
//		};
//	}
//
//	// Count the number of elements
//	cerr << "Counting elements..." << endl;
//	unsigned int nElementCount = 0;
//	for (MyMap::iterator it = pMap->begin(); it != pMap->end(); it++) {
////		cerr << "Key = " << (*it).first << endl;
//		nElementCount++;
//	}
//
//	cerr << "Element Count = " << nElementCount << endl;
//
//	MyMap::Element* pElement = pMap->findElement(12345);
//	if (pElement) {
//		cerr << "Key of element found " << pElement->first << endl;
//	}
//
//	cerr << "Sum = " << sum << endl;
//
//	delete pMap;
//
//	cerr << "Ptr instance count = " << Ptr::m_nInstCount << endl;
//}
//
//void testTrie()
//{
//	typedef TrieNode<unsigned int, int> MyTrieNode;
//	MyTrieNode trieNode;
//
//	unsigned int key[3];
//	key[0] = 1;
//	key[1] = 2;
//	key[2] = 3;
//
//	trieNode.insertKey(key, 3);
//
//	key[0] = 1;
//	key[1] = 2;
//	key[2] = 4;
//	trieNode.insertKey(key, 3);
//
//	key[0] = 1;
//	key[1] = 2;
//	key[2] = 5;
//	trieNode.insertKey(key, 3);
//
//	key[0] = 1;
//	key[1] = 3;
//	trieNode.insertKey(key, 2);
//
//	key[0] = 3;
//	trieNode.insertKey(key, 1);
//
//
//	unsigned int nCount = trieNode.getNodeCount();
//
//	// Test trie node iterator
//	unsigned int nKeySize;
//	MyTrieNode::Iterator it(&trieNode, key, &nKeySize);
//	MyTrieNode* pNode;
//	while (pNode = it.next()) {
//		for (int i = 0; i < nKeySize; i++) {
//			cerr << key[i];
//			if (i < nKeySize - 1) {
//				cerr << ", ";
//			}
//		}
//		cerr << ": " << pNode->m_payload;
//		cerr << endl;
//	}
//
//	malloc(100);
//	new int[10];
//}
//
//int main1(int argc, char* argv[]) 
//{
//#ifdef _WIN32
//	// Check C runtime allocated blocks as well
//	_CrtSetDbgFlag(_CRTDBG_CHECK_CRT_DF);
//#endif
//
//	testTrie();
//
//#ifdef _WIN32
//	_CrtDumpMemoryLeaks();
//#endif
//
//	return 0;
//}

bool convert(
			 const string& sLangModelFileIn, 
			 const string& sLangModelFileOut, 
			 LangModel::SerializeFormat nInFormat,
			 LangModel::SerializeFormat nOutFormat
			 ) 
{
	switch (nOutFormat) {
		case LangModel::FORMAT_TEXT:
			cerr << "Converting Language Model to text format." << endl;
			break;
		case LangModel::FORMAT_BINARY_TRIE:
			cerr << "Converting Language Model to binary format (trie)." << endl;
			break;
		case LangModel::FORMAT_BINARY_SA:
			cerr << "Converting Language Model to binary format (sorted array)" << endl;
			break;
		default:
			cerr << "Error: Unknown output format for Language Model." << endl;
			return false;
	}

	LangModelFactory::LangModelType nInLangModelType;
	switch (nInFormat) {
		case LangModel::FORMAT_TEXT:
		case LangModel::FORMAT_BINARY_TRIE:
			nInLangModelType = LangModelFactory::LANG_MODEL_TRIE;
			break;
		case LangModel::FORMAT_BINARY_SA:
			nInLangModelType = LangModelFactory::LANG_MODEL_SA;
			break;
		default:
			cerr << "Error: Unknown input format for Language Model." << endl;
			return false;
	}

	// Use the factory to create the LM
	LangModelParams params;
	// For now, we only know how to convert from LANG_MODEL_TRIE
	LangModel* lm = LangModelFactory::getInstance()->create(params, nInLangModelType);

	// Open the LM file
	ifstream langModelFileIn(sLangModelFileIn.c_str(), ios::binary | ios::in);
	if (langModelFileIn.fail()) {
		cerr << "Cannot open language model input file " << sLangModelFileIn << endl;
		return false;
	}

	ofstream langModelFileOut(sLangModelFileOut.c_str(), ios::binary | ios::out);
	if (langModelFileOut.fail()) {
		cerr << "Cannot open Language Model file " << sLangModelFileOut << endl;
		return false;
	}

	cerr << "Reading Language Model from file: " << sLangModelFileIn << endl;
	lm->read(langModelFileIn);
	langModelFileIn.close();

	cerr << "Writing Language Model to file: " << sLangModelFileOut << endl;
	lm->write(langModelFileOut, nOutFormat);
	langModelFileOut.close();

	delete lm;

	return true;
}

bool computeProb(const string& sLangModelFile, const string& sInputFile, bool bCustomizerLM) 
{
	//// Open the lang model file
	//ifstream langModelFile(sLangModelFile.c_str(), ios::binary | ios::in);
	//if (langModelFile.fail()) {
	//	cerr << "Cannot open language model file " << sLangModelFile << endl;
	//	return false;
	//}

	// Open the input file
	ifstream inputFile(sInputFile.c_str(), ios::in | ios::binary);
	if (inputFile.fail()) {
		cerr << "Cannot open input file " << sInputFile << endl;
		return false;
	}

	//cerr << "Loading language model...";

	//// Load the LM
	//LangModelParams params; // The defaults are OK
	//LangModel* lm = LangModelFactory::getInstance()->create(params);
	//lm->read(langModelFile);

	//langModelFile.close();

	//cerr << "Done. " << endl;

	SmartPtr<LangModel> lm = loadLM(sLangModelFile, bCustomizerLM);

	cerr << "Computing word sequence probabilities..." << endl;
	cerr << "Language Model file: " << sLangModelFile << endl;
	cerr << "Input file: " << sInputFile << endl;

	// We need the Vocab to convert words
	const LW::LWVocab* pVocab = lm->getVocab();
	
	char* wordSentence[MAX_WORDS_IN_SENTENCE];
	unsigned int nWordCount;
	LWVocab::WordID idSentence[MAX_WORDS_IN_SENTENCE];
	while (getNextSentence(inputFile, wordSentence, &nWordCount, MAX_WORDS_IN_SENTENCE)) {
		// Convert all words to IDs
		string ss = "";
		ss += wordSentence[0];
		cerr<<ss<<endl;
		pVocab->wordsToID(wordSentence, idSentence, nWordCount);

		LangModel::ProbLog prob = lm->computeSequenceProbability(idSentence, 0, nWordCount - 1);

		// Output the sentence
		for (unsigned int i = 0; i < nWordCount; i++) {
			cout << wordSentence[i] << " ";
		}
		cout << endl;
		// Output the probability
		cout << "(" << prob << ")" << endl; 
	}	LangModelHistoryFactory histFactory(3);


	inputFile.close();

	return true;
}

bool trainCustomizer(
				  const string& sDomainCorpus, 
				  const string& sGenericLM, 
				  const string& sGenericCounts,
				  const string& sDomainLM,
				  const string& sCombinedCounts
				  )
{
	char* sentence[MAX_WORDS_IN_SENTENCE];

	cerr << "Training Customizer..." << endl;

	CustomizerLMTrainer customizerTrainer;

	cerr << "Loading vocabulary from Generic LM " << sGenericLM << " ..." << endl;

	// Open generic LM stream
	ifstream genericLMIn(sGenericLM.c_str(), ios::in | ios::binary);
	if (genericLMIn.fail()) {
		cerr << "Could not open generic Langauge Model " << sGenericLM << endl;
		return false;
	}

	// Open generic counts
	ifstream genericCountsIn(sGenericCounts.c_str(), ios::in | ios::binary);
	if (genericCountsIn.fail()) {
		cerr << "Cannot open generic counts file " << sGenericCounts << endl;
		return false;
	}

	ofstream domainLMOut(sDomainLM.c_str(), ios::out | ios::binary);
	if (domainLMOut.fail()) {
		cerr << "Cannot open domain LM file for output: " << sDomainLM << endl;
		return false;
	}

	ofstream combinedCountsOut(sCombinedCounts.c_str(), ios::out | ios::binary);
	if (combinedCountsOut.fail()) {
		cerr << "Cannot open combined counts file for output: " << sCombinedCounts << endl;
		return false;
	}

	// Initialize trainer
	customizerTrainer.init(genericLMIn);
	
	// Open training file (domain)
	ifstream trainFile;
	istream* pTrainInput;
	if (!sDomainCorpus.empty()) {
		trainFile.open(sDomainCorpus.c_str(), ios::in | ios::binary);
		if (trainFile.fail()) {
			cerr << "Cannot open training file " << sDomainCorpus << endl;
			return false;
		}
		pTrainInput = &trainFile;
	}
	else {
		pTrainInput = &cin;
	}

	cerr << "Training domain corpus... " << endl;

	// Parse sentences and train
	unsigned int nWordCount;
	unsigned int nTotalWordCount = 0;
	while (getNextSentence(*pTrainInput, sentence, &nWordCount, MAX_WORDS_IN_SENTENCE)) {
		customizerTrainer.trainSentence(sentence, nWordCount);

		nTotalWordCount += nWordCount;
	}

	cerr << "Training done. Writing Lang Model and counts..." << endl;

	customizerTrainer.finalize(genericCountsIn, domainLMOut, combinedCountsOut, "_tempDomainCounts.lmc");

	cerr << "Done" << endl;
	return true;
}

bool dumpVocab(string& sLangModel) {
	SmartPtr<LangModel> lm = loadLM(sLangModel, false, LangModel::READ_VOCAB_ONLY);

	const LWVocab* pVocab = lm->getVocab();
	VocabIterator it(pVocab);

	LWVocab::WordID nWord;
	while (LWVocab::INVALID_WORD != (nWord = it.next())) {
		const string& sWord = pVocab->getWord(nWord);
		cout << sWord << endl;
	}

	return true;
}

////--------------------
////-------------------
//bool createBDBLM() 
//{
//	XMLConfig config;
//	LangModelFactory::initInstance(&config);
//
//	string sFileName = "c:\\tmp\\bdb\\lm";
//	{
//		unlink(sFileName.c_str());
//
//		LangModelBDB trainLM;
//		trainLM.create(sFileName, 0);
//
//		ifstream in("c:\\tmp\\lm\\test.lm");
//		trainLM.convertFromTextLM(in);
//	}
//	{
//		SmartPtr<LangModel> lm = LangModelFactory::getInstance()->createBDBLangModel(sFileName);
//
//		lm->dump(cout);
//	}
//
//	return true;
//}

				 

int main(int argc, char* argv[])
{
	try
	{
		SETBINARYCIN;
		SETBINARYCOUT;

		XMLConfig config;
		LangModelFactory::initInstance(&config);


		ParamParser params;
		
		params.registerParam("dump-words", false);

		params.registerParam("help", false);
		params.registerParam("train", false);
		params.registerParam("train-customize", false);
		params.registerParam("ppl", false);
		params.registerParam("ppl2", false);
		params.registerParam("train-counts", false);
		params.registerParam("merge-counts", false);
		params.registerParam("convert-counts", false);
		params.registerParam("bin2text", false);
		params.registerParam("text2bin", false);
		params.registerParam("trie2sa", false);
		params.registerParam("sa2text", false);
		params.registerParam("prob", false);
		params.registerParam("dump-vocab", false);

		params.registerParam("lm", true);
		params.registerParam("lm-customize", true);
		params.registerParam("counts", true);
		params.registerParam("lm-in", true);
		params.registerParam("lm-out", true);
		params.registerParam("in", true);
		params.registerParam("out", true);
		params.registerParam("order", true);
		params.registerParam("sent-per-count-file", true);
		params.registerParam("bin", false); // Generic binary = bin-trie
		params.registerParam("bin-trie", false); // Old binary format (trie)
		params.registerParam("bin-sa", false); // New binary format - sorted array
		// Do not pad the sentence with <s> </s>
		params.registerParam("nopad", false);
		params.registerParam("byte", false);
		params.registerParam("dbyte", false);

		params.registerParam("cutoffs", true);

		params.registerParam("lm-gen", true);
		params.registerParam("counts-gen", true);
		params.registerParam("lm-dom", true);
		params.registerParam("counts-comb", true);

		params.parse(argc, argv);
		ParamParser::StringVector invalidParams = params.getInvalidParams();
		if (invalidParams.size() > 0) {
			for (size_t i = 0; i < invalidParams.size(); i++) {
				cerr << "Error: Invalid command line switch: -" << invalidParams[i] << endl;
			}
			cerr << endl;
			printUsage();
			return 1;
		}

		if (params.getNamedParam("help")) {
			printUsage();
			return 0;
		}

		bool bDumpWords = params.getNamedParam("dump-words");

		bool bTrain = params.getNamedParam("train");
		bool bTrainCustomizer = params.getNamedParam("train-customize");
		bool bPerplexity = params.getNamedParam("ppl");
		bool bPerplexity2 = params.getNamedParam("ppl2");
		bool bTrainCounts = params.getNamedParam("train-counts");
		bool bMergeCounts = params.getNamedParam("merge-counts");
		bool bConvertCounts = params.getNamedParam("convert-counts");
		bool bBinToText = params.getNamedParam("bin2text");
		bool bTextToBin = params.getNamedParam("text2bin");
		bool bTrieToSA = params.getNamedParam("trie2sa");
		bool bSAToText = params.getNamedParam("sa2text");
		bool bComputeProb = params.getNamedParam("prob");
		bool bPadWithStartEnd = !params.getNamedParam("nopad");
		bool bTrainOnBytes = params.getNamedParam("byte");
		bool bTrainOnDoubleBytes = params.getNamedParam("dbyte");
		bool bDumpVocab = params.getNamedParam("dump-vocab");

		LangModel::SerializeFormat nFormat = LangModel::FORMAT_TEXT;
		// Preserve the default binary format
		if (params.getNamedParam("bin")) {
			nFormat = LangModel::FORMAT_BINARY_TRIE;
		}
		// This format was also the default for text to binary conversion
		if (bTextToBin) {
			nFormat = LangModel::FORMAT_BINARY_TRIE;
		}

		if (params.getNamedParam("bin-trie")) {
			nFormat = LangModel::FORMAT_BINARY_TRIE;
		}
		if (params.getNamedParam("bin-sa")) {
			nFormat = LangModel::FORMAT_BINARY_SA;
		}


		// If max order is not specified, we are happy with 0. 
		// That will cause LangModelParams to pick up the default value (3)
		int nParamMaxOrder = 0;
		if (params.getNamedParam("order", nParamMaxOrder)) {
			if (nParamMaxOrder < 1 || nParamMaxOrder > MAX_SUPPORTED_ORDER) {
				cerr << "-order must be between 1 and " << MAX_SUPPORTED_ORDER << endl;
				return 1;
			}

			langModelParams.m_nMaxOrder = nParamMaxOrder;
		}

		// Read cutoffs
		string sCutoffs;
		if (params.getNamedParam("cutoffs", sCutoffs)) {
			// Casting the string is ok, we don't modify it
			char* szCutoff = strtok((char*) sCutoffs.c_str(), ",");
			unsigned int i = 1;
			while (szCutoff) {
				unsigned int nCutoff = atoi(szCutoff);

				if (nCutoff == 0) {
					cerr << "Warning: Cutoff 0 specified for " << i << "-grams" << endl;
				}

				langModelParams.m_nCountCutoffs[i] = nCutoff;

				szCutoff = strtok(NULL, ",");		
				i++;
			}
		}

		if (bTrain && bPerplexity) {
			printUsage();
			return 1;
		}
		else if (bTrain) {
			string sLangModelFile;
			string sTrainFile;
			string sCountsFile;
			params.getNamedParam("lm", sLangModelFile);
			params.getNamedParam("in", sTrainFile);
			params.getNamedParam("counts", sCountsFile);
			string sCountFile = string(sLangModelFile) + ".count";

			// At least one of the counts of lang model file must be passed on the command line
			if (sLangModelFile.empty() && sCountsFile.empty()) {
				cerr << "ERROR: When you select -train you must use either -lm or -counts or both" << endl;
				cerr << "Type LangModel -help for available options" << endl;
				return 1;
			}

			if (bTrainOnBytes) {
				if (!trainByte(sLangModelFile, sTrainFile, langModelParams, nFormat)) {
					return 1;
				}
			}
			else if (bTrainOnDoubleBytes) {
				if (!trainByte(sLangModelFile, sTrainFile, langModelParams, nFormat)) {
					return 1;
				}
			}
			else if (!train(sLangModelFile, sCountsFile, sTrainFile, langModelParams, nFormat, bPadWithStartEnd, bDumpWords)) {
				return 1;
			}
		}
		else if (bTrainCustomizer) {
			string sDomainCorpus;
			string sGenericLM;
			string sGenericCounts;
			string sDomainLM;
			string sCombinedCounts;

			// This is optional. If empty, we read from stdin
			params.getNamedParam("in", sDomainCorpus);
			params.getNamedParam("lm-gen", sGenericLM);
			params.getNamedParam("counts-gen", sGenericCounts);
			params.getNamedParam("lm-dom", sDomainLM);
			params.getNamedParam("counts-comb", sCombinedCounts);

			if (sGenericLM.empty() || sGenericCounts.empty() || sDomainLM.empty() || sCombinedCounts.empty()) {
				printUsage();
				return 1;
			}

			if (!trainCustomizer(sDomainCorpus, sGenericLM, sGenericCounts, sDomainLM, sCombinedCounts)) {
				return 1;
			}

		}
		else if (bTrainCounts) {
			string sTrainFile;
			int nSentPerCountFile = SENT_PER_COUNT_FILE;
			params.getNamedParam("in", sTrainFile);
			params.getNamedParam("sent-per-count-file", nSentPerCountFile);
			if ((sTrainFile.length() == 0)) {
				printUsage();
				return 1;
			}
			if (!trainCounts(sTrainFile, langModelParams, nSentPerCountFile, bPadWithStartEnd)) {
				return 1;
			}
		}
		else if (bMergeCounts) {
			string sOutFileName;
			if (!params.getNamedParam("out", sOutFileName)) {
				cerr << "Output file name for merging counts must be supplied. Use -out." << endl;
				return 1;
			}
			if (!mergeCounts(sOutFileName, langModelParams)) {
				return 1;
			}
		}
		else if (bConvertCounts) {
			string sCountsFile;
			string sLangModelFile;
			params.getNamedParam("in", sCountsFile);
			params.getNamedParam("lm", sLangModelFile);

			if (nParamMaxOrder > 0) {
				cerr << "Warning: Specifiying order on the command line has no effect. The order is read from the Language Model file." << endl;
			}

			if (sCountsFile.length() == 0 || sLangModelFile.length() == 0) {
				printUsage();
				return 1;
			}

			if (!convertCounts(sCountsFile, sLangModelFile, langModelParams, nFormat)) {
				return 1;
			}

		}
		else if (bPerplexity || bPerplexity2) {
			string sLangModelFile;
			string sTestFile;
			params.getNamedParam("lm", sLangModelFile);
			params.getNamedParam("in", sTestFile);
			bool bCustomizerLM = false;
			if (params.getNamedParam("lm-customize")) {
				params.getNamedParam("lm-customize", sLangModelFile);
				bCustomizerLM = true;
			}

			if (nParamMaxOrder > 0) {
				cerr << "Warning: Specifiying order on the command line has no effect. The order is read from the Language Model file." << endl;
			}

			if ((sLangModelFile.length() == 0) || (sTestFile.length() == 0)) {
				printUsage();
				return 1;
			}

			if (bPerplexity) {
				if (bTrainOnBytes) {
					if (!calculatePerplexityByte(sLangModelFile, sTestFile, langModelParams)) {
						return 1;
					}
				}
				else if (!calculatePerplexity(sLangModelFile, sTestFile, langModelParams, bCustomizerLM)) {
					return 1;
				}
			}
			else {
				if (bTrainOnBytes) {
					cerr << "-ppl2 not valid with byte training. Please use -ppl switch." << endl;
					return 1;
				}
				else if (!calculatePerplexity2(sLangModelFile, sTestFile, langModelParams)) {
					return 1;
				}
			}
		}
		else if (bBinToText || bTextToBin) {
			if (bBinToText && bTextToBin) {
				printUsage();
				return 1;
			}

			string sLangModelFileIn;
			params.getNamedParam("lm-in", sLangModelFileIn);
			if (sLangModelFileIn.length() == 0) {
				printUsage();
				return 1;
			}
		
			string sLangModelFileOut;
			params.getNamedParam("lm-out", sLangModelFileOut);
			if (sLangModelFileOut.length() == 0) {
				printUsage();
				return 1;
			}

			if (!convert(sLangModelFileIn, sLangModelFileOut, LangModel::FORMAT_BINARY_TRIE, nFormat)) {
				return 1;
			}
		}
		else if (bTrieToSA) {
			string sLangModelFileIn;
			params.getNamedParam("lm-in", sLangModelFileIn);
			if (sLangModelFileIn.length() == 0) {
				printUsage();
				return 1;
			}
		
			string sLangModelFileOut;
			params.getNamedParam("lm-out", sLangModelFileOut);
			if (sLangModelFileOut.length() == 0) {
				printUsage();
				return 1;
			}

			if (!convert(sLangModelFileIn, sLangModelFileOut, LangModel::FORMAT_BINARY_TRIE, LangModel::FORMAT_BINARY_SA)) {
				return 1;
			}
		}
		else if (bSAToText) {
			string sLangModelFileIn;
			params.getNamedParam("lm-in", sLangModelFileIn);
			if (sLangModelFileIn.length() == 0) {
				printUsage();
				return 1;
			}
		
			string sLangModelFileOut;
			params.getNamedParam("lm-out", sLangModelFileOut);
			if (sLangModelFileOut.length() == 0) {
				printUsage();
				return 1;
			}

			if (!convert(sLangModelFileIn, sLangModelFileOut, LangModel::FORMAT_BINARY_SA, LangModel::FORMAT_TEXT)) {
				return 1;
			}
		}
		else if (bComputeProb) {
			string sLangModelFile;
			string sInputFile;
			params.getNamedParam("lm", sLangModelFile);
			params.getNamedParam("in", sInputFile);

			bool bCustomizerLM = false;
			if (params.getNamedParam("lm-customize")) {
				params.getNamedParam("lm-customize", sLangModelFile);
				bCustomizerLM = true;
			}

			if ((sLangModelFile.length() == 0) || (sInputFile.length() == 0)) {
				printUsage();
				return 1;
			}
			if (!computeProb(sLangModelFile, sInputFile, bCustomizerLM)) {
				return 1;
			}
		}
		else if (bDumpVocab) {
			string sLangModelFile;
			params.getNamedParam("lm", sLangModelFile);
			if (sLangModelFile.empty()) {
				printUsage();
				return 1;
			}
			if (!dumpVocab(sLangModelFile)) {
				return 1;
			}
		}
		else {
			printUsage();
			return 1;
		}
	}
	catch (Exception& e) {
		cerr << "Exception caught: " << e.getErrorMessage() << endl;
		return 1;
	}
	catch (...) {
		cerr << "Unknown exception. Aborting." << endl;
		return 1;
	}

#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}

