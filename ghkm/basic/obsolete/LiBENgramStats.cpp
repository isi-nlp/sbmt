// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _LiBENgramStats_C_
#define _LiBENgramStats_C_

#include "LiBE.h"
#include "LiBENgramStats.h"
#include "strmanip.h"
#include "Vocabulary.h"
#include "LiBEVector.h"

using namespace std;

#ifndef DONT_USE_SRILM

/*------------------------------------------------------------
* SRINgramCountsWrapper
* ------------------------------------------------------------*/

template<class CountT>
void SRINgramCountsWrapper<CountT>::
putNode(ostream& out, 
          Predicate<CountT>* func,
	  NgramNode* node,
	  char *buffer, /* output buffer */
	  char *bptr,	/* pointer into output buffer */
	  unsigned int level, 
	  unsigned int order, 
	  bool sorted)
{
    NgramNode *child;
    VocabIndex wid;

    TrieIter<VocabIndex,CountT> iter(*node, sorted ? vocab.compareIndex() : 0);

    /*
     * Iterate over the child nodes at the current level,
     * appending their word strings to the buffer
     */
    while ((child = iter.next(wid))) {
	string str;
	char* word;

	ostringstream ost(str);
	ost <<wid;
	word = (char*)ost.str().c_str();

	unsigned wordLen = strlen(word);

	if (bptr + wordLen + 1 > buffer + maxLineLength) {
	   *bptr = '0';
	   cerr << "ngram ["<< buffer << word
		<< "] exceeds write buffer\n";
	   continue;
	}
        
	strcpy(bptr, word);

	/*
	 * If this is the final level, print out the ngram and the count.
	 * Otherwise set up another level of recursion.
	 */
	if (order == 0 || level == order) {
	    if(func){
		if((*func)(child->value())){
		   out<<buffer<<"\t"<<child->value()<<endl;
		}
	    } else {
	       out<<buffer<<"\t"<<child->value()<<endl;
	    }
	} 

	
	if (order == 0 || level < order) {
	   *(bptr + wordLen) = ' ';
	   putNode(out, func, child, buffer, bptr + wordLen + 1, level + 1,
			order, sorted);
	}
    }
}

template<class CountT>
ostream& SRINgramCountsWrapper<CountT>:: 
put(ostream& out, Predicate<CountT>* func, bool isTxt)
{
    assert(isTxt);
    static char buffer[maxLineLength];
    putNode(out, func, &counts, buffer, buffer, 1, 0);
    return out;
}

template<class CountT>
istream& SRINgramCountsWrapper<CountT>::
get(istream& in, UnaryFunction<CountT,void>* func, bool isTxt)
{
    assert(isTxt);

    size_t i;
    string line;
    while(getline(in, line, '\n')) {
	vector<string> strs = split(line, "\t");
	if(strs.size() != 2){
	    cerr<<"Warning: Skipping one line"<<endl;
	    continue;
	}

	istringstream ist1(strs[0]);
	LiBEVector<VocabularyIndex> ngram;
	ist1 >> ngram;

	istringstream ist2(strs[1]);
	CountT count;
	ist2 >>  count;

	VocabIndex wids[ngram.size() + 1];

	for(i = 0;  i < ngram.size(); ++i){
	    wids[i] = ngram[i];
	}
	wids[ngram.size()] = Vocab_None;

	//*counts.insert(wids) += count;
	CountT* c = counts.insert(wids);
       	*c = count;

	if(func){ (*func)(*c);}
    }


    return in;
}
#endif /// USE_SRILM

#endif
