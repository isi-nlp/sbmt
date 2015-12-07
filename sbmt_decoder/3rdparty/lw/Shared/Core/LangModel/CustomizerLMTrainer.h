// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _CUSTOMIZER_LM_TRAINER_H
#define _CUSTOMIZER_LM_TRAINER_H

#include "LangModel/LangModel.h"
// TODO: Move Trie.h out of impl directory
#include "LangModel/impl/Trie.h"

/**	
	Call sequence:
		init()
		trainSentence()
			...
		finalize()
*/

namespace LW {
	class CustomizerLMTrainer {
	public:
		CustomizerLMTrainer();
		virtual ~CustomizerLMTrainer();
	public:
		void init(std::istream& vocabularyLMFile);
		void trainSentence(char* szSentence);
		void trainSentence(char** pSentence, unsigned int nWordCount);
		void finalize(std::istream& genericCountsIn, std::ostream& domainLMOut, std::ostream& combinedCountsOut, const std::string& tempDomainCounts, bool bDeleteTemp = true);
		void writeLangModel(std::ostream& lmModelFile) const;
		void writeCounts(std::ostream& countsFile) const;
	private:
		/// Domain Language Model (this class will train it)
		LangModel* m_pDomainLM;
		/// Trie used to hold counts
		TrieNode<LWVocab::WordID, int> m_counts;
	};


} // namespace LW

#endif

