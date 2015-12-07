// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _VOCAB_FACTORY_H
#define _VOCAB_FACTORY_H

#include "Vocab.h"

namespace LW {

class VocabFactory {
//private:
//	static Vocab* m_pInstance;
public:
	static LWVocab* createInstance();
};

} // namespace LW

#endif // _VOCAB_FACTORY_H
