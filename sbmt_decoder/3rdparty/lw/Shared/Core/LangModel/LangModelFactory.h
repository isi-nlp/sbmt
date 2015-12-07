// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _LANG_MODEL_FACTORY_H
#define _LANG_MODEL_FACTORY_H

#include "LangModel.h"
#include "Common/SmartPtr.h"

#include <map>
#include <istream>

#ifdef LM_NO_COMMON_LIB
namespace LW {
	class XMLConfig {};
}
#else
#include "Common/XMLConfig.h"
#endif

namespace LW {

class LangModelFactory {
public:
	enum LangModelType {LANG_MODEL_ANY = 0, LANG_MODEL_TRIE = 1, LANG_MODEL_SA = 2, LANG_MODEL_BDB = 3};
private:
	/// Map that caches instances created
	typedef std::map<std::string, SmartPtr<LangModel> > LangModelCache;
protected:
	/// Prohibits creation from outside
	LangModelFactory(XMLConfig* pConfig);
	/// Virtual destructor
	virtual ~LangModelFactory();
public:
	/// Method called by the Controller to initialize the factory. This method must be the first one to be called
	static void initInstance(XMLConfig* pConfig, bool bCreateAllPreload = true);
public:
	/// Singleton method
	static LangModelFactory* getInstance();
#ifndef LM_NO_COMMON_LIB
	/// Returns a pointer to a shared, cached Language Model
	LangModel* create(const std::string& sLangModelID);
#endif
	/// Creates a new (uninitialized) Language Model. The caller has to select learning or loading before using it.
	LangModel* create(const LangModelParams& params, LangModelType nLangModelType = LANG_MODEL_SA);
	/// Creates a LM and loads it from a stream
	LangModel* create(std::istream& in, LangModel::ReadOptions nReadOptions = LangModel::READ_NORMAL,LangModelParams const& params=LangModelParams());
	/// Creates an Customizer LM
	SmartPtr<LangModel> createCustomizerLM(
		SmartPtr<LangModel> spGenericLM, 
		SmartPtr<LangModel> spDomainLM, 
		std::istream& combinedCounts, 
		unsigned int nDomainReplicatorFactor = 10);
#ifndef LM_NO_BDB_LIB
	LangModel* createBDBLangModel(const std::string& sFileName);
#endif
private:
#ifndef LM_NO_COMMON_LIB
	/// Creates a new Language Model from configuration file. If bCreateAllPreload is set
	/// it will attempt to create all objects that have the preload attribute set
	SmartPtr<LangModel> createInternal(const std::string& sLangModelID, bool bCreateAllPreload = false);
#endif 
	/// Performs allocated object cleanup
	void cleanup();
private:
	/// Singleton instance
	static LangModelFactory* m_pInstance;
	/// The instances are cached in a map
	LangModelCache m_langModelCache;
	/// 
	XMLConfig* m_pConfig;
};

} // namespace LW


#endif // _LANG_MODEL_FACTORY_H
