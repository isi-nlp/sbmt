// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _EditeDistance_H_
#define _EditeDistance_H_

#include <vector>
#include "assert.h"
#include "LiBEDefs.h"
#include "LiBEFunctions.h"


using namespace std;
using namespace LiBE;

const SCORET M_subCost = 1;
const SCORET M_delCost = 1;
const SCORET M_insCost = 1;

template<class T>
class EditDistance 
{
public:
    EditDistance() {
	M_delCost = 3;
	M_insCost = 3;
	M_subCost = 3;
    }
    typedef enum {CORR, SUB, DEL, INS} AlignType;

    void setDelCost(SCORET c) { M_delCost = c;}
    void setInsCost(SCORET c) { M_insCost = c;}
    void setSubCost(SCORET c) { M_subCost = c;}

    size_t operator()(const vector<T>& seq1, const vector<T>& seq2,
	              vector<AlignType>& alignment,
		      BinaryPredicate<T, T>& func) const;

private:
    SCORET M_delCost;
    SCORET M_insCost;
    SCORET M_subCost;
};

/** Copy the correct from src vector to the tgt vector
*   according to the alignment. This is usefull, for
*   example, for overwriting a tgt string in lower case
*   with a src string with case information.
*/
template<class T>
void copy_correct(const vector<T>& src, vector<T>& tgt, 
	     const vector<typename EditDistance<T>::AlignType>& algingment)
{
    size_t i = 0, j = 0, k;

    for(k = 0; k < algingment.size(); ++k){
	switch (algingment[i] ){
	    case EditDistance<T>::CORR : 
		tgt[j] = src[i];
		i++;
		j++;
		break;
	    case EditDistance<T>::INS:
		j++;
		break;
	    case EditDistance<T>::SUB:
		i++;
		j++;
		break;
	    case EditDistance<T>::DEL:
		i++;
		break;
            default:
		cerr<<"Unknown edit operation!"<<endl;
		exit(1);
	}
    }
}

template<class T>
size_t EditDistance<T>::
operator()(const vector<T>& seq1, const vector<T>& seq2,
	 vector<typename EditDistance<T>::AlignType>& alignment,
	 BinaryPredicate<T,T>& func) const
{
    size_t sub, ins,  del;
    unsigned length1 = seq1.size();
    unsigned length2 = seq2.size();

    typedef struct {
	SCORET cost;          // minimal cost of partial alignment
	AlignType error;	// best predecessor
    } ChartEntry;

    /* 
     * Allocate chart statically, enlarging on demand
     */
    static unsigned maxLength1 = 0;
    static unsigned maxLength2 = 0;
    static ChartEntry **chart = 0;

    if (chart == 0 || length1 > maxLength1 || length2 > maxLength2) {
	/*
	 * Free old chart
	 */
	if (chart != 0) {
	    for (unsigned i = 0; i <= maxLength2; i ++) {
		delete [] chart[i];
	    }
	    delete [] chart;
	}

	/*
	 * Allocate new chart
	 */
	maxLength1 = length1;
	maxLength2 = length2;
    
	chart = new ChartEntry*[maxLength2 + 1];
	assert(chart != 0);

	unsigned i, j;

	for (i = 0; i <= maxLength2; i ++) {
	    chart[i] = new ChartEntry[maxLength1 + 1];
	    assert(chart[i] != 0);
	}

	/*
	 * Initialize the 0'th row and column, which never change
	 */
	chart[0][0].cost = 0;
	chart[0][0].error = CORR;

	/*
	 * Initialize the top-most row in the alignment chart
	 * (all words inserted).
	 */
	for (j = 1; j <= maxLength1; j ++) {
	    chart[0][j].cost = chart[0][j-1].cost + M_insCost;
	    chart[0][j].error = INS;
	}

	for (i = 1; i <= maxLength2; i ++) {
	    chart[i][0].cost = chart[i-1][0].cost + M_delCost;
	    chart[i][0].error = DEL;
	}
    }

    /*
     * Fill in the rest of the chart, row by row.
     */
    for (unsigned i = 1; i <= length2; i ++) {

	for (unsigned j = 1; j <= length1; j ++) {
	    SCORET minCost;
	    AlignType minError;

	    if (func(seq1[j-1], seq2[i-1])) {
		minCost = chart[i-1][j-1].cost;
		minError = CORR;
	    } else {
		minCost = chart[i-1][j-1].cost + M_subCost ;
		minError = SUB;
	    }

	    SCORET delCost = chart[i-1][j].cost + M_delCost;
	    if (delCost < minCost) {
		minCost = delCost;
		minError = DEL;
	    }

	    SCORET insCost = chart[i][j-1].cost + M_insCost;
	    if (insCost < minCost) {
		minCost = insCost;
		minError = INS;
	    }

	    chart[i][j].cost = minCost;
	    chart[i][j].error = minError;
	}
    }

    /*
     * Backtrace
     */
    unsigned totalErrors;

    {
	unsigned i = length2;
	unsigned j = length1;
	unsigned k = 0;

	sub = del = ins = 0;

	while (i > 0 || j > 0) {

	    switch (chart[i][j].error) {
	    case CORR:
		i --; j --;
		alignment.push_back(CORR);
		break;
	    case SUB:
		i --; j --;
		sub ++;
	        //  alignment[k] = SUB;
	        alignment.push_back(SUB);
		break;
	    case DEL:
		i --;
		del ++;
		alignment.push_back(DEL);
		//alignment[k] = DEL;
		break;
	    case INS:
		j --;
		ins ++;
		alignment.push_back(INS);
		//alignment[k] = INS;
		break;
	    }

	    k ++;
	}

	/*
	 * Now reverse the alignment to make the order correspond to words
	 */
	int k1, k2;	/* k2 can get negative ! */

	for (k1 = 0, k2 = k - 1; k1 < k2; k1++, k2--) {
	    AlignType x = alignment[k1];
	    alignment[k1] = alignment[k2];
	    alignment[k2] = x;
	}
	
	totalErrors = sub + del + ins;
    }

    return totalErrors;

}
	        
#endif
