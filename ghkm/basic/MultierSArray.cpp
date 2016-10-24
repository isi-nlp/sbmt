// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _MultierSArray_C_
#define _MultierSArray_C_

#include "MultierSArray.h"
#include "LiBE.h"
#include <cassert>
using namespace std;

template<class _Key, class _Tp>
MultierSArray<_Key, _Tp>:: ~MultierSArray()
{
    for(size_t i = 1; i <= numTiers(); ++i){
	if(M_nodes[i - 1]) { delete M_nodes[i - 1];}
    }
}

// The values below are for reading 
//	[N-Gram Order] [N-Gram Count]
//      [WORD ID] [Prob] [BOW] [FirstChildOffset]
//	...
//	[0] // 0 means end
template<class _Key, class _Tp>
istream& MultierSArray<_Key, _Tp>:: 
binaryGet(istream& in)
{
    unsigned int nGramOrder;
    unsigned int nGramCount;
    bool bDone = false;

    while (!bDone) {
	in.read((char*)&nGramOrder, sizeof(unsigned int));
	in.read((char*)&nGramCount, sizeof(unsigned int));

	// the termination conditon.
	if (0 == nGramOrder) {
		bDone = true;
	}
	else {
	    // Allocate a tier.
	    TierTp* tier = new TierTp(nGramCount);
	    // Transfer all entries here
	    in.read((char*)&((*tier)[0]), nGramCount * sizeof(_Element));
	    if(nGramOrder > M_nodes.size()){
		// Add this tier.
		M_nodes.push_back(tier);
	    } else {
		M_nodes[nGramOrder - 1] = tier;
	    }
	}
    }

    return in;
}

// The values below are for reading 
//	[N-Gram Order] [N-Gram Count]
//      [WORD ID] _Tp  [FirstChildOffset]
//	...
//	[0] [0] // 0 means end
template<class _Key, class _Tp>
istream& MultierSArray<_Key, _Tp>:: 
textGet(istream& in)
{
    unsigned int i, nGramOrder, nGramCount;
    bool bDone = false;
    string line;

    while (!bDone) {

	getline(in, line, '\n');
	istringstream ist(line);

	ist >> nGramOrder;
	ist >> nGramCount;
	if (0 == nGramOrder) {
		bDone = true;
	}
	else {
	    // Allocate a tier.
	    TierTp* tier = new TierTp(nGramCount);
	    for(i = 0; i < nGramCount; ++i){
		getline(in, line, '\n');
		istringstream ist(line);
		ist >> (*tier)[i].first 
		   >> (*tier)[i].second >> (*tier)[i].firstChild;
	    }
	    if(nGramOrder > M_nodes.size()){
		// Add this tier.
		M_nodes.push_back(tier);
	    } else {
		M_nodes[nGramOrder - 1] = tier;
	    }
	}
    }


	return in;
}

#if 0
// The values below are for reading 
//	[N-Gram Order] [N-Gram Count]
//      [WORD ID] _Tp  [FirstChildOffset]
//	...
//	[0] [0] // 0 means end
template<class _Key, class _Tp>
iFile & MultierSArray<_Key, _Tp>:: 
get(iFile& in)
{
    if(in.isText()){
	textGet(in);
    } else {
	binaryGet(in);
    }
    return in;
}
#endif

// The values below are for reading 
//	[N-Gram Order] [N-Gram Count]
//      [WORD ID] [Prob] [BOW] [FirstChildOffset]
//	...
//	[0] // 0 means end
template<class _Key, class _Tp>
ostream& MultierSArray<_Key, _Tp>:: 
binaryPut(ostream& out)
{
    unsigned int nGramOrder;
    unsigned int nGramCount;

    for(nGramOrder = 1; nGramOrder <= numTiers(); ++nGramOrder){
	nGramCount = M_nodes[nGramOrder - 1]->size();
	out.write((char*)&nGramOrder, sizeof(unsigned int));
	out.write((char*)&nGramCount, sizeof(unsigned int));
	out.write((char*)&(*M_nodes[nGramOrder - 1])[0], 
		  nGramCount* sizeof(_Element));
    }

    // the termination.
    nGramOrder = nGramCount = 0;
    out.write((char*)&nGramOrder, sizeof(unsigned int));
    out.write((char*)&nGramCount, sizeof(unsigned int));

    return out;
}

// The values below are for reading 
//	[N-Gram Order] [N-Gram Count]
//      [WORD ID] [Prob] [BOW] [FirstChildOffset]
//	...
//	[0] // 0 means end
template<class _Key, class _Tp>
ostream& MultierSArray<_Key, _Tp>:: 
textPut(ostream& out)
{
    unsigned int nGramOrder, nGramCount, i;

    for(nGramOrder = 1; nGramOrder <= numTiers(); ++nGramOrder){
	nGramCount = M_nodes[nGramOrder - 1]->size();

	out << nGramOrder << " "<<nGramCount<<endl;
	for(i = 0; i < nGramCount; ++i){
	    out << (*M_nodes[nGramOrder - 1])[i].first<<" "
		<< (*M_nodes[nGramOrder - 1])[i].second<<" "
		<< (*M_nodes[nGramOrder - 1])[i].firstChild<<endl;

	}
    }

    // the termination.
    out<<"0"<<" "<<"0"<<endl;
    return out;
}

#if 0
// The values below are for reading 
//	[N-Gram Order] [N-Gram Count]
//      [WORD ID] _Tp  [FirstChildOffset]
//	...
//	[0] [0] // 0 means end
template<class _Key, class _Tp>
oFile & MultierSArray<_Key, _Tp>:: 
put(oFile& out)
{
    if(out.isText()){
	textPut(out);
    } else {
	binaryPut(out);
    }
    return out;
}
#endif




template<class _Key, class _Tp>
_Tp* MultierSArray<_Key, _Tp>:: 
find(const _Key* pWords, unsigned int nOrder) const
{
    // We can't find anything if the order is higher than the one this 
    // LM was created for
    if (nOrder > numTiers()) {
	return NULL;
    }

    //cerr<<"BBB: ";
    ///for(size_t ll = 0; ll < nOrder; ++ll){
//	cerr<<pWords[ll]<<" ";
 //   }
  //  cerr<<endl;

    unsigned int nStartIndex = 0;
    unsigned int nEndIndex = M_nodes[0]->size();
	
    // Set up the indexes we perform binary search between
    // For the root node we do a bin search over all entries
    typename TierTp :: const_iterator sI, eI, iter;
    for (unsigned int i = 1; i <= numTiers(); i++) {
	TierTp* tier = M_nodes[i - 1];

	sI = tier->begin() + nStartIndex;
	eI = tier->begin() + nEndIndex;

	// Do a binary search for the entry
        _Element dummyElem;  dummyElem.first = pWords[i - 1];
	iter = lower_bound(sI, eI, dummyElem);
	//cerr<<"U"<<sI->first<<" "<<nStartIndex<<" "<<eI->first<<" "<<nEndIndex<<endl;

	// not found.
	if(iter == eI || pWords[i - 1] < iter->first){
	    return NULL;
	}

	///cerr<<pWords[i-1]<<" "<<iter->first<<"V"<<endl;

	// The entry was found
	if (i == nOrder ) {
	    return (_Tp*)&iter->second;
	}

	//cerr<<"X"<<endl;
	typename TierTp::const_iterator itt = iter;
	itt++;
	if(itt == tier->end()){
	    nEndIndex = M_nodes[i]->size();
	} else {
	    nEndIndex = itt->firstChild;
	}
	// Adjust the interval we search on
	nStartIndex = iter->firstChild;
	//cerr<<"Y"<<endl;
    }

    // This line should never be reached
    assert(0);
    return NULL;
}

/* Reads a SORTED array from a input stream. */
/* The func is used to process any value after it is read in.
*   @param in  The input stream.
*   @param func  The unary function applied to each node after
*                the node is inserted.
*   @param isTxt  If it is true, the input is a text stream.
*                 Otherwise, it is binary. Only the text 
*                 version is supported for now.
*/
template<class _Key, class _Tp>
istream& MultierSArray<_Key, _Tp>:: 
get(istream& in, UnaryFunction<_Tp, void>* func, bool isTxt)
{
    if(isTxt){ return textGet(in); }
    else { return binaryGet(in); }
}

/** The func is used to process any value after it is read in.
*   @param out The input stream.
*   @param func  The unary predicate decides which node to be output.
*                This argument is currently ignored.
*   @param isTxt  If it is true, the output is a text stream.
*                 Otherwise, it is binary. Only the text 
*                 version is supported for now.
*/
template<class _Key, class _Tp>
ostream& MultierSArray<_Key, _Tp>:: 
put(ostream& out, Predicate<_Tp>* func, bool isTxt)
{
    if(isTxt){ return textPut(out); }
    else { return binaryPut(out); }
}

#endif
