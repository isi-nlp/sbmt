// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _LANG_MODEL_HISTORY_H
#define _LANG_MODEL_HISTORY_H

#include "Common/Vocab/Vocab.h"
#include "LangModel/LangModel.h"

namespace LW {

	class LangModelHistory 
	{
		friend class LangModelHistoryFactory;
	protected:
		/**
			Only allow creation by the factory, which is a friend of this class 
		*/
		 LangModelHistory(unsigned int nMaxHistorySize, LWVocab::WordID* pWordHistory);
	public:
		/**
			Adds one word to the history, shifting existing words if necessary
		*/
		void addWord(LWVocab::WordID nWord);
		/**
			Returns the probability stored in the history object
		*/
		LangModel::ProbLog getProb() const;
		/**
			Updates the internally stored probability
		*/
		void setProb(LangModel::ProbLog newProb);
		/**
			Returns a pointer to the buffer holding the words
		*/
		LWVocab::WordID* getWordBuffer();
		/**
			Returns the number of words currently in the buffer
		*/
		LWVocab::WordID getWordCount() const;
		/**
			Copy operator
		*/
		LangModelHistory& operator=(const LangModelHistory& src);
	private:
		/// Buffer for word history
		LWVocab::WordID* m_pWordHistory;
		/// The allocated size for word history
		unsigned int m_nWordHistorySize;
		/// The number of words actually stored in the word history
		unsigned int m_nWordHistoryCount;
		/// The probability associated with the words already stored in the history
		LangModel::ProbLog m_prob;
	};

} // namespace LW

#endif // _LANG_MODEL_HISTORY_H
