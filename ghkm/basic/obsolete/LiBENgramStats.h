// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _LiBENgramStats_H_
#define _LiBENgramStats_H_

#include <functional>
#include <iostream>
#include "LiBEFunctions.h"
#include "MultierSArray.h"
#include "LiBEDefs.h"
#include "StringVocabulary.h"

using namespace std;
using namespace LiBE;

#ifndef DONT_USE_SRILM

#include "NgramStats.h"
#include "NgramStats.cc"

/** A wrapper class for SRILM NgramCounts class. This class will
*   be replaced by my own trie data structure.*/
template<class CountT> 
class SRINgramCountsWrapper : public NgramCounts<CountT>
{
    typedef NgramCounts<CountT>  _base;

public:
    //! Constructor.
    SRINgramCountsWrapper(unsigned int order) : _base(M_dummyVcb, order) {}
    virtual ~SRINgramCountsWrapper() {}

    /** The func is used to process any value after it is read in.
    *   @param in  The input stream.
    *   @param func  The unary function applied to each node after
    *                the node is inserted.  This argument is 
    *                currently ignored.
    *   @param isTxt  If it is true, the input is a text stream.
    *                 Otherwise, it is binary. Only the text 
    *                 version is supported for now.
    */
    virtual istream& get(istream& in, 
	     UnaryFunction<CountT, void>* func = NULL, bool isTxt = true);

    /** The func is used to process any value after it is read in.
    *   @param out The input stream.
    *   @param func  The unary predicate decides which node to be output.
    *                This argument is currently ignored.
    *   @param isTxt  If it is true, the output is a text stream.
    *                 Otherwise, it is binary. Only the text 
    *                 version is supported for now.
    */
    virtual ostream& put(ostream& out, 
	         Predicate<CountT>* func = NULL, bool isTxt = true);

protected:
    void putNode(ostream& out, 
	           Predicate<CountT>* func,
	           NgramNode *node, 
		   char* buffer,
		   char* bptr,
		   unsigned int level, 
	           unsigned int order, bool sorted = false);

    /** This is a dummy vcb used to initialize the base.*/
    Vocab  M_dummyVcb;
};
#endif // USE_SRILM

//! An adaptor of the MultierSArray.
template<class CountT>
class MultierSArrayAdaptor : public MultierSArray<EID, CountT> {
    typedef MultierSArray<EID, CountT>  _base;
public:

    MultierSArrayAdaptor(unsigned int order) : _base(order) {}

    virtual ~MultierSArrayAdaptor()  {}

    CountT*  findCount(const EID* ngram) {
	size_t l = StringVocab::ngramLength(ngram);
	return _base::find(ngram, l);
    }

    CountT*  insertCount(const EID* ngram) {
	LW_ASSERT(0);
	return NULL;
    }

    unsigned int getorder() { return _base::numTiers();}
};

template<class CountT>
class AbstractNgramStats
{
public:
    virtual ~AbstractNgramStats() {}
    /** The func is used to process any value after it is read in.
    *   @param in  The input stream.
    *   @param func  The unary function applied to each node after
    *                the node is inserted.
    *   @param isTxt  If it is true, the input is a text stream.
    *                 Otherwise, it is binary.
    */
    virtual istream& get(istream& in, 
	     UnaryFunction<CountT, void>* func = NULL, bool isTxt = true) = 0;

    /** The func is used to process any value after it is read in.
    *   @param in  The input stream.
    *   @param func  The unary predicate decides which node to be output.
    *   @param isTxt  If it is true, the output is a text stream.
    *                 Otherwise, it is binary. Only the text 
    *                 version is supported for now.
    */
    virtual ostream& put(ostream& out, 
	         Predicate<CountT>* func = NULL, bool isTxt = true) = 0;

    virtual CountT*  findCount(const EID* ngram) = 0;
    virtual CountT*  insertCount(const EID* ngram) = 0;

    virtual unsigned int getorder() = 0;
};


#ifndef DONT_USE_SRILM
template<class CountT,  
         template<class T> class ContainerTp = SRINgramCountsWrapper>
#else 
template<class CountT,  template<class T> class ContainerTp >
#endif
class LiBENgramStats : public ContainerTp<CountT>, 
                       public AbstractNgramStats<CountT>
{
    typedef ContainerTp<CountT> _base;
public:
    //! Constructor.
    LiBENgramStats(unsigned int order) : _base(order) {}
    virtual ~LiBENgramStats()  {}



    /** The func is used to process any value after it is read in.
    *   @param in  The input stream.
    *   @param func  The unary function applied to each node after
    *                the node is inserted.
    *   @param isTxt  If it is true, the input is a text stream.
    *                 Otherwise, it is binary.
    */
    istream& get(istream& in, 
	     UnaryFunction<CountT, void>* func = NULL, bool isTxt = true)
    { return _base::get(in, func, isTxt);}


    /** The func is used to process any value after it is read in.
    *   @param in  The input stream.
    *   @param func  The unary predicate decides which node to be output.
    *   @param isTxt  If it is true, the output is a text stream.
    *                 Otherwise, it is binary. Only the text 
    *                 version is supported for now.
    */
    ostream& put(ostream& out, 
	         Predicate<CountT>* func = NULL, bool isTxt = true)
    { return _base::put(out, func, isTxt);}

    CountT*  findCount(const EID* ngram)  { return _base::findCount(ngram);}
    CountT*  insertCount(const EID* ngram) { return _base::insertCount(ngram);}

    unsigned int getorder()  { return _base::getorder();}
};

#include "LiBENgramStats.cpp"

#endif
