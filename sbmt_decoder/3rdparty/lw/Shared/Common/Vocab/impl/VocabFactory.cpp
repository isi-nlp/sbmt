// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include "../VocabFactory.h"

#include "../Vocab.h"

namespace LW {

//Vocab* VocabFactory::m_pInstance = NULL;

LWVocab* VocabFactory::createInstance()
{
	LWVocab* pInstance = new LWVocab();
	return pInstance;
}

} // namespace LW
