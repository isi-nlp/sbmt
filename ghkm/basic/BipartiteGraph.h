// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _BipartiteGraph_
#define _BipartiteGraph_

#include "KpartiteGraph.h"
#include "LiBEVector.h"


template<class VT>
class BipartiteGraph : public KpartiteGraph<VT>
{
    typedef KpartiteGraph<VT> _base;

public:
    //! Vertex ID type.
    typedef typename _base::VID VID;

    //! EdgeType.
    typedef typename _base::EdgeType EdgeType;

    /** Iterators to iterate the graph.*/
    typedef typename _base::iterator  iterator;
    typedef typename _base:: const_iterator const_iterator;

    /** Empty constructor.*/
    BipartiteGraph()  : _base(2)  {}


    /** Add the vertexes. */
    void addVertexes(const Link<LiBEVector<VT> >& vs);

    /** Add the edges. */
    void addEdges(const LiBEVector<LiBEPair<POSITTYPE, POSITTYPE> >& es);

    /** Reads the graph from the input stream. */
    virtual istream& get(istream& in);

    /** Reads the vertexes from the input stream. */
    virtual istream& getVertexes(istream& in);

    /** Reads the edges from the input stream. */
    virtual istream& getEdges(istream& in);

    /** Writes the graph into the output stream. */
    virtual ostream& put(ostream& out);

    /** Writes vertexes into the output stream. 
    *  Note that the VIDs of the vertexes are not
    *  outupt.  This means that the VIDs are not 
    *  preserved after outputing and then reloading.
    */
    virtual ostream& putVertexes(ostream& out) const;

    /** Writes the edges into the output stream. */
    virtual ostream& putEdges(ostream& out) const;
};

#include "BipartiteGraph.cpp"

#endif

