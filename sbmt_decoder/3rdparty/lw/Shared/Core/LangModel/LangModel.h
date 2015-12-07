// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _LANG_MODEL_H
#define _LANG_MODEL_H

#define _USE_MATH_DEFINES // We want to include constants like M_LN10
#include <math.h>
#ifdef __GCC2__
#include <limits.h>
#else
#include <limits>
#endif // WIN32

#include <vector>

#include "Common/Vocab/Vocab.h"
#include "Common/ErrorCode.h"

#ifndef M_LN10
#	define M_LN10 2.30258509299404568402
#endif

namespace LW {

class LangModelHistory;

/**
	Specific exception to be thrown when the Language Model loading fails
*/
class InvalidLangModelVersionException: public Exception {
public:
	InvalidLangModelVersionException(int nErrorCode, char const* szErrorMessage)
		:Exception(nErrorCode, szErrorMessage) {};
};


/**
This class is just used to for typedefs.
*/
class LangModelBase 
{
public:
	typedef float Prob;
	typedef float ProbLog;
	typedef float Perplexity;
};

// The order can be set at runtime, but it cannot exceed this value
#define MAX_SUPPORTED_ORDER 7
// Default max order, if not specified by the caller. Cannot be higher than MAX_SUPPORTED_ORDER
#define DEFAULT_MAX_ORDER 3
// Maximu number of words in a sentence
#define MAX_WORDS_IN_SENTENCE 510

#ifndef __GCC2__
static const LangModelBase::ProbLog PROB_LOG_ZERO = (LangModelBase::ProbLog) log(0.0);
static const LangModelBase::ProbLog PROB_LOG_INF = (LangModelBase::ProbLog) std::numeric_limits<float>::infinity();
namespace {
inline void kill_warning(LangModelBase::ProbLog lpz, LangModelBase::ProbLog lpi)
{
  lpz = PROB_LOG_ZERO;
  lpi = PROB_LOG_INF;
}
}
#else
//	static const LangModelBase::ProbLog PROB_LOG_ZERO = (LangModelBase::ProbLog) log(0.0);
//	static const LangModelBase::ProbLog PROB_LOG_INF = (LangModelBase::ProbLog) std::numeric_limits<float>::infinity();
	static const LangModelBase::ProbLog PROB_LOG_ZERO = -1.0/0.0;	
	static const LangModelBase::ProbLog PROB_LOG_INF = 1.0/0.0;	
#endif // GCC2
	static const LangModelBase::ProbLog PROB_LOG_ONE = 0.0;		

	static const LangModelBase::Prob PROB_EPSILON = (LangModelBase::ProbLog) 3e-06; // Probabilities smaller than this are considered 0

class LangModelParams
{
public:
	LangModelParams() {
		m_nMaxOrder = DEFAULT_MAX_ORDER;

		m_nCountCutoffs[0] = 0; // No such thing as 0-gram

		for (int i = 1; i <= MAX_SUPPORTED_ORDER; i++) {
			if (i <= 2) {
				m_nCountCutoffs[i] = 1;
			}
			else {
				m_nCountCutoffs[i] = 2;
			}
		}
	}
public:
	/// Maximum n-gram order
	unsigned int m_nMaxOrder;
	/// Index 0 is not used as use use m_nCountCutoffs[1] to represent the cutoffs for 1-grams
	unsigned int m_nCountCutoffs[MAX_SUPPORTED_ORDER + 1];
	/// Temporary directory for training (if necessary)
	std::string m_sTempDir;
	
};

class LangModel : public LangModelBase
{
public:
	enum SerializeFormat{FORMAT_TEXT = 0, FORMAT_BINARY_TRIE = 1, FORMAT_BINARY_SA = 2, FORMAT_BDB = 3};
	enum ReadOptions {READ_NORMAL = 0, READ_VOCAB_ONLY = 1};
public:
	static Perplexity ProbLogToPPL(ProbLog prob) {
		if (prob == PROB_LOG_ZERO) {
			return 0;
		}
		else {
			return static_cast<LangModel::Perplexity> (exp(-prob * M_LN10));
		}
	}
	static Prob ProbLogToProb(ProbLog prob) {
		return static_cast<LangModel::Prob> (exp(prob * M_LN10));
	}
	static ProbLog ProbToProbLog(Prob prob) {
		return static_cast<LangModel::ProbLog> (log10(prob));
	}

public:
	virtual ~LangModel(){};
public:
	virtual void clear() = 0;
	virtual void learnSentence(char** pSentence, unsigned int nSentenceSize, bool bPadWithStartEnd = true) = 0;
	virtual ProbLog computeSentenceProbability(char** pWords, unsigned int nWordCount) = 0;
	virtual ProbLog computeSequenceProbability(unsigned int* pWords, unsigned int nStartWord, unsigned int nEndWord) = 0;
	virtual ProbLog computeProbability(unsigned int nWord, LangModelHistory& historyIn, LangModelHistory& historyOut) = 0;
	virtual ProbLog computeProbability(unsigned int nWord, unsigned int* pnContext, unsigned int nContextLength) = 0;
	virtual void read(std::istream& in, LangModel::ReadOptions nOptions = LangModel::READ_NORMAL) = 0;
	virtual void readCounts(std::istream& in) = 0;
	virtual void write(std::ostream& out, SerializeFormat nFormat) = 0;
	virtual void writeCounts(std::ostream& out) = 0;
	virtual void dump(std::ostream& out) = 0;
	virtual const LWVocab* getVocab() const = 0; 
	virtual unsigned int getMaxOrder() const = 0;
	/// Let the LM know that counting is finished so it can compute probabilities.
	virtual void finishedCounts() = 0;
};

} // namespace LW

#endif // _LANG_MODEL_H
