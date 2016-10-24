// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/** $Id: MultierSArray.h,v 1.1 2006/08/22 18:37:48 wang11 Exp $
*   $Header: /home/nlg-03/wang11/cvs-repository/dev/basic/src/MultierSArray.h,v 1.1 2006/08/22 18:37:48 wang11 Exp $ */
#ifndef _MultierSArray_H_
#define _MultierSArray_H_

#include <iostream>
#include <vector>
#include "LiBEFunctions.h"

using namespace std;
using namespace LiBE;

template<class _Key, class _Tp>
class MultierSArray 
{
public:

    /** Empty constructor.*/
    MultierSArray() {}

    /** Empty constructor. n is the number of tiers.*/
    MultierSArray(unsigned int n) : M_nodes(n) {}

    /** Destructor. */
    virtual ~MultierSArray();

public: 

    ////////////////////////////////////////////////////////////////
    //         IO Interfaces.
    ////////////////////////////////////////////////////////////////
    /** Reads a SORTED array from a input stream. */
    /** The func is used to process any value after it is read in.
    *   @param in  The input stream.
    *   @param func  The unary function applied to each node after
    *                the node is inserted.
    *   @param isTxt  If it is true, the input is a text stream.
    *                 Otherwise, it is binary. Only the text 
    *                 version is supported for now.
    */
    virtual istream& get(istream& in, 
	                 UnaryFunction<_Tp, void>* func = NULL, 
			 bool isTxt = true);

    /** Reads a SORTED array from a binary input stream. */
    virtual istream& binaryGet(istream& in);

    /** Reads a SORTED array from a text input stream. */
    virtual istream& textGet(istream& in);
    
    /** The func is used to process any value after it is read in.
    *   @param out The input stream.
    *   @param func  The unary predicate decides which node to be output.
    *   @param isTxt  If it is true, the output is a text stream.
    *                 Otherwise, it is binary. Only the text 
    *                 version is supported for now.
    */
    virtual ostream& put(ostream& out, 
	     Predicate<_Tp>* func = NULL, bool isTxt = true);

    /** Reads a SORTED array from a binary input stream. 
    *   \FIXME: const.
    */
    virtual ostream& binaryPut(ostream& out);

    /** Reads a SORTED array from a text input stream. */
    virtual ostream& textPut(ostream& out);

    /** 
    *   Returns the ponter to the elemented that can be located by
    *   the keyArray. If nothing found, NULL is returned.
    */
    _Tp* find(const _Key* keyArray, unsigned int order) const; 

    /// Returns the max order. N.B., the order starts from 1. I.e.,
    /// the order of unigram is 1, bigram is 2.
    unsigned int numTiers() const {return (unsigned int) M_nodes.size() ;};

protected:

    class _Element : public pair<_Key, _Tp> {
	friend class MultierSArray<_Key, _Tp>;

	    /// Offset of the first child. The last child offset is defined by
	    /// by the next sibling of this node (it's first child - 1)
	    int  firstChild;
	public:
	    bool operator < (const _Element & elem) const{
		if(this->first < elem.first) { return true;}
		else { return false;}
	}
    };

    typedef vector<_Element>  		TierTp;

    /** 
    *   The size of the vector equals the order. In other words, 
    *   M_nodes[0] is for unigram, M_nodes[1] is for bigram, ...
    */
    vector<vector<_Element>* > M_nodes;
};


#include "MultierSArray.cpp"


#endif 
