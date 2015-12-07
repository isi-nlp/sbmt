// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _PROB_H
#define _PROB_H

#include "Trie.h"
#include "../LangModel.h"

namespace LW {

struct ProbNode 
{
	/// n-gram probability
	LangModel::ProbLog m_prob;
	/// Back off weight 
	LangModel::ProbLog   m_bow;
public:
	ProbNode(LangModel::ProbLog prob = 0) {
		m_prob = prob;
		m_bow = 0;
	}
};

/*
/// This function is required by the tries
inline void clear(ProbNode& ref)
{
	// No need to to anything, the constructor will initialize the node
}
*/

//class Prob
//{
//public:
//	typedef double Prob;
//	typedef double ProbLog;
//public:
//	static ProbLogToPPL(ProbLog prob) {
//		return exp(-prob);
//	}
//};

} // namespace LW


#endif // _PROB_H
