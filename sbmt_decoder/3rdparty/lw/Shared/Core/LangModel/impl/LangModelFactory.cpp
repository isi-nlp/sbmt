// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include "../LangModelFactory.h"

#include <fstream>

#include "Common/handle.h"
#include "Common/Vocab/VocabFactory.h"
#include "Common/Serializer/Serializer.h"
#include "LangModel/LangModel.h"
#include "LangModelKN.h"
#include "LangModelSA.h"
#include "LangModelBDB.h"
#include "LangModelImpl.h"
#include "LangModel/Trace.h"
#include "CustomizerLM.h"
#include "LMVersion.h"

//LangModel* LangModelFactory::m_pInstance = NULL;

namespace LW {

#define C_LANG_MODEL_CAT_PATH "config/lang-models/lang-model"
#define C_LANG_MODEL_ID "lid"

/**
	This class is returned by the factory and the sole purpose is to manage
	the lifetime of the pointer returned by the factory
*/
class LangModelWrapper : public LangModel {
public:
	/// If bDelete is true, this class will assume ownership of the pointer
	LangModelWrapper(LangModel* pLangModel, bool bDelete = false) {
		m_pLangModel = pLangModel; 
		m_bDelete = bDelete;
	};
	virtual ~LangModelWrapper() {
		if (m_bDelete) {
			delete m_pLangModel;
		}
	}

	virtual void clear() 
		{m_pLangModel->clear();};
	virtual void learnSentence(char** pSentence, unsigned int nSentenceSize, bool bPadWithStartEnd = true) 
		{m_pLangModel->learnSentence(pSentence, nSentenceSize, bPadWithStartEnd);};
	virtual LangModel::ProbLog computeSentenceProbability(char** pWords, unsigned int nWordCount)
		{return m_pLangModel->computeSentenceProbability(pWords, nWordCount);};
	virtual LangModel::ProbLog computeSequenceProbability(unsigned int* pWords, unsigned int nStartWord, unsigned int nEndWord)
		{return m_pLangModel->computeSequenceProbability(pWords, nStartWord, nEndWord);};
	virtual ProbLog computeProbability(unsigned int nWord, LangModelHistory& historyIn, LangModelHistory& historyOut)
		{return m_pLangModel->computeProbability(nWord, historyIn, historyOut);};
	virtual ProbLog computeProbability(unsigned int nWord, unsigned int* pnContext, unsigned int nContextLength) 
		{return m_pLangModel->computeProbability(nWord, pnContext, nContextLength);};
	virtual void read(std::istream& in, LangModel::ReadOptions nOptions = LangModel::READ_NORMAL) 
		{m_pLangModel->read(in, nOptions);};
	virtual void readCounts(std::istream& in)
		{m_pLangModel->readCounts(in);};
	virtual void write(std::ostream& out, LangModel::SerializeFormat nFormat)
		{m_pLangModel->write(out, nFormat);};
	virtual void writeCounts(std::ostream& out)
		{m_pLangModel->writeCounts(out);}
	virtual void dump(std::ostream& out)
		{m_pLangModel->dump(out);};
	virtual const LWVocab* getVocab() const 
		{return m_pLangModel->getVocab();};
	virtual unsigned int getMaxOrder() const
		{return m_pLangModel->getMaxOrder();};
	virtual void finishedCounts()
		{m_pLangModel->finishedCounts();};
private:
	LangModel* m_pLangModel;
	bool m_bDelete;
};

LangModelFactory* LangModelFactory::m_pInstance;

LangModelFactory::LangModelFactory(XMLConfig* pConfig) 
{
	//assert(pConfig);

	m_pConfig = pConfig;
}

LangModelFactory::~LangModelFactory() {
	cleanup();
}

void LangModelFactory::initInstance(XMLConfig* pConfig, bool bCreateAllPreload) 
{
	//assert(pConfig);

	if(m_pInstance) delete m_pInstance;
	m_pInstance = new LangModelFactory(pConfig);
	// Create all LM that have the preload attribute set
#ifndef LM_NO_COMMON_LIB
	m_pInstance->createInternal("", bCreateAllPreload);
#endif
}

LangModelFactory* LangModelFactory::getInstance() 
{
	// If this is NULL, initInstance has not been called yet, which is the the caller's responsibility
	//assert(m_pInstance);
	if (NULL == m_pInstance) {
		throw Exception(ERR_XML_PARSE, "LanguageModelFactory::initInstance() not called before getInstance()");
	}

	return m_pInstance;
}

LangModel* LangModelFactory::create(const LangModelParams& params, LangModelType nLangModelType)
{
	//assert(params.m_nMaxOrder > 0);
	//assert(params.m_nMaxOrder <= MAX_SUPPORTED_ORDER);

	LangModelImplBase* pImpl = NULL;
	LWVocab* pVocab = VocabFactory::createInstance();

	//assert(pVocab);

	switch (nLangModelType) {
	case LANG_MODEL_TRIE:
		pImpl = new LangModelKN(pVocab, params);
		break;
#ifndef LM_NO_BDB_LIB
	case LANG_MODEL_BDB:
		pImpl = new LangModelBDB(pVocab);
		break;
#endif
	default:
            pImpl = new LangModelSA(pVocab, params);
	}

	//assert(pImpl);

	LangModel* pInstance = new LangModelImpl(pImpl, pVocab);	
	//assert(pInstance);

	// Don't return a wrapper in this case, just a plain pointer
	// it is the caller responsibility to delete it
	return pInstance;
}

#ifndef LM_NO_COMMON_LIB
LangModel* LangModelFactory::create(const std::string& sLangModelID) 
{
	SmartPtr<LangModel> spInstance;
	// Attempt to get it from the cache first
	LangModelCache::iterator it = m_langModelCache.find(sLangModelID);
	if (m_langModelCache.end() != it) {
		// Found it
		spInstance = it->second;
	}
	else {
		// Not found. Create it. This method will also store it in the cache.
		spInstance = createInternal(sLangModelID);
	}

	// The instance count is maintained in the cache
	// so spInstance destructor does not actually delete the pointer
	return new LangModelWrapper(spInstance.getPtr());
}
// LM_NO_COMMON_LIB
#endif

SmartPtr<LangModel> LangModelFactory::createCustomizerLM(
		SmartPtr<LangModel> spGenericLM, 
		SmartPtr<LangModel> spDomainLM, 
		std::istream& combinedCounts, 
		unsigned int nDomainReplicationFactor) 
{
	CustomizerLM* pCustomizerLM = new CustomizerLM(spGenericLM, spDomainLM, combinedCounts, nDomainReplicationFactor);

	// We return NULL for now
	// The vocabulary is the one of Domain LM which is a superset of the Generic LM vocab.
	return new LangModelImpl(pCustomizerLM, const_cast<LWVocab*> (pCustomizerLM->getMasterVocab()), false);
}

#ifndef LM_NO_COMMON_LIB
SmartPtr<LangModel> LangModelFactory::createInternal(const std::string& sLangModelID, bool bCreateAllPreload)
{
	//assert(m_pConfig);

	SmartPtr<LangModel> spLangModel;

	// Create all nodes LMs that have the preload attribute set
	ConfigNode::ConfigNodeList nodeList;
	m_pConfig->getRootNode()->getNodes("lang-models/lang-model", nodeList);
	for(size_t i = 0; i < nodeList.size(); i++){
		std::string sID;
		int nPreload = 0;
		// lid attribute must be present
		if (!nodeList[i]->getString("#lid", sID)) {
			throw Exception(ERR_XML_PARSE, "lang-model.lid attribute not found in configuration.");
		}
		// preload attribute might be missing, that's ok
		nodeList[i]->getInt("#preload", nPreload);

		if (sLangModelID == sID || ((nPreload == 1) && bCreateAllPreload)) {
			// Is it already in the cache? This should never happen, but check, just in case
			LangModelCache::iterator it = m_langModelCache.find(sID);
			if (it == m_langModelCache.end()) {
				// Get the file name for the language model
				std::string sLangModelFile;
				if (!nodeList[i]->getString("lm-file", sLangModelFile)) {
					throw Exception(ERR_XML_PARSE, "lang-model.lang-model-file node not found in configuration.");
				}
				// Fix the name
				sLangModelFile = m_pConfig->makePath(sLangModelFile.c_str());

				// Just use the default value for params, it does not matter in this case
				LangModelParams params;
				spLangModel = create(params);

				TRACE(lib_lang_model.factory, ("Opening language model file %s.", sLangModelFile.c_str()));

				// Read the language model from the file 
				// Open the stream
				std::ifstream in(sLangModelFile.c_str(), std::ios::in | std::ios::binary);
				if (in.fail()) {
					throw Exception(ERR_IO, std::string("Language model file ") + sLangModelFile + " not found.");
				}

				TRACE(lib_lang_model.factory, ("Reading language model from file %s.", sLangModelFile.c_str()));

				spLangModel->read(in);

				// Cache the object, once created
				m_langModelCache[sID] = spLangModel;

				TRACE_CALL(lib_lang_model.factory, ("Created Language Model from file."));
			}
			else {
				// For debug we stop here. For release we can continue, but we probably have 2 LMs with the same ID. Pretty bad.
				//assert(false);
				spLangModel = it->second;
			}
		}
	}
	
	// The reference count will be maintained by the map
	return spLangModel;
}
// LM_NO_COMMON_LIB
#endif

void LangModelFactory::cleanup()
{
	//LangModelCache::iterator it = m_langModelCache.begin();
	//while (it != m_langModelCache.end()) {
	//	delete it->second;
	//}
	m_langModelCache.clear();
}

LangModel* LangModelFactory::create(std::istream& in, LangModel::ReadOptions nReadOptions,LangModelParams const& params)
{
	LangModel* pLangModel;// = create(params, LANG_MODEL_SA);

	// Guess the language model file type by looking at the magic number
	ISerializer ser(&in);
	unsigned int nMagic;
	ser.read(nMagic);
	switch (nMagic) {
		case VERSION_MAGIC_NUMBER:
			pLangModel = create(params, LANG_MODEL_TRIE);
			break;
		case VERSION_MAGIC_NUMBER2:
			pLangModel = create(params, LANG_MODEL_SA);
			break;
		default:
			pLangModel = create(params, LANG_MODEL_TRIE);
			break;
	}

	in.seekg(0);
	pLangModel->read(in);

	return pLangModel;
}

#ifndef LM_NO_BDB_LIB
LangModel* LangModelFactory::createBDBLangModel(const std::string& sFileName) 
{
	LWVocab* pVocab = VocabFactory::createInstance();

	// Create the BDB language model
	LangModelParams params;
	LangModelBDB* pImpl = new LangModelBDB(pVocab);
	pImpl->open(sFileName);

	// Wrap it
	LangModel* pWrapper = new LangModelImpl(pImpl, pVocab);

	return pWrapper;
}
#endif


} // namespace LW
