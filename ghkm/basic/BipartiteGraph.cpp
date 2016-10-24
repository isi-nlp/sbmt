// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _BipartiteGraph_C_
#define _BipartiteGraph_C_
#include <assert.h>
#include "BipartiteGraph.h"

#ifdef WIN32
#pragma warning ( disable : 4267 )
#endif

/*  Add the vertexes . */
template<class VT>
void BipartiteGraph<VT>::addVertexes(const Link<LiBEVector<VT> >& vs)
{
    for(size_t d = 0; d < vs.size(); ++d){
        for(size_t i = 0; i < vs[d].size(); ++i){
	    Vertex<VT> vertex;
	    vertex.v = vs[d][i]; 
	    (*this)[VID(d, i)] = vertex;
	}
    }
}

/*  Add the edges. */
template<class VT>
void BipartiteGraph<VT> ::
addEdges(const LiBEVector<LiBEPair<POSITTYPE, POSITTYPE> >& es)
{
    LiBEVector<LiBEPair<POSITTYPE, POSITTYPE> >::const_iterator it;
    for(it = es.begin(); it != es.end(); ++it){
	EdgeType edge(VID(0, it->first), VID(1, it->second));
	addEdge(edge);
    }
}

template<class VT>
istream& BipartiteGraph<VT>::get(istream& in)
{
    this->clear();
    getVertexes(in);
    getEdges(in);
    return in;
}

/* Reads the vertexes from the input stream. 
*  The format of vertexes are:
*  VT sequence 1
*  VT sequence 2
*/
template<class VT>
istream& BipartiteGraph<VT>::getVertexes(istream& in)
{
    Link<LiBEVector<VT> > apair;
    in >> apair;
    addVertexes(apair);
    return in;
}

/* Reads edges from the input stream.
*  The edges are stored in the format of
*  1 2 3 4 5 6 (pairs of indexes to vertexes).
*/
template<class VT>
istream& BipartiteGraph<VT>::getEdges(istream& in)
{
    LiBEVector<LiBEPair<POSITTYPE, POSITTYPE> > edges;
    in >> edges;
    addEdges(edges);
    return in;
}

/* Writes vertexes into the output stream. 
*  Note that the VIDs of the vertexes are not
*  outupt.  This means that the VIDs are not 
*  preserved after outputing and reloading.
*/
template<class VT>
ostream& BipartiteGraph<VT>::putVertexes(ostream& out) const
{
    const_iterator it;
    set<pair<VID, VT> > vset;

    // add the vertexes.
    for(it = this->begin(); it != this->end(); ++it){
	vset.insert(make_pair(it->first, it->second.v));
    }

    typename set<pair<VID, VT> > :: const_iterator k;
    Link<LiBEVector<VT> > seqs;
    for(k = vset.begin(); k != vset.end(); ++k){
	seqs[k->first.first].push_back(k->second);
    }
    out<<seqs[0]<<seqs[1];
    return out;
}


/*
* Writes the edgees into the output stream.
*/
template<class VT>
ostream& BipartiteGraph<VT>::putEdges(ostream& out) const
{
    const_iterator it;
    set<LiBEPair<EID, EID> > alnset;

    // for each node in the alignment.
    for(it = this->begin(); it != this->end(); ++it){
	// for each of its adjancent nodes
	typename LiBESet<VID>::const_iterator j;
	for(j = it->second.adj.begin(); j != it->second.adj.end(); ++j){
	    // these two nodes should not in the same language.
	    assert(it->first.first != j->first);
	    LiBEPair<EID, EID> vpair;

	    if(it->first.first == 0){
		vpair.first = it->first.second;
	    } else {
		vpair.second = it->first.second;
	    }

	    if(j->first == 0){
		vpair.first = j->second;
	    } else {
		vpair.second = j->second;
	    }

	    alnset.insert(vpair);
	}
    }

    set<LiBEPair<EID, EID> > ::const_iterator k;
    for(k = alnset.begin(); k != alnset.end(); ++k){
	out<<*k<<" ";
    }

    out<<endl;

    return out;
}

/* Writes the alignment into the output stream. */
template<class VT>
ostream& BipartiteGraph<VT>::put(ostream& out)
{
    putVertexes(out);
    putEdges(out);
    return out;
}


#endif
