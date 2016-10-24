// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _KpartiteGraph_H_
#define _KpartiteGraph_H_
#include "LiBEDefs.h"
#include "LiBEPair.h"
#include "LiBESet.h"
#include <set>

using namespace std;
using namespace STL_HASH;

#define VID_MAX  10000
template<class VT>
class Vertex {
    public:
	/** Vertex ID type. */
	typedef LiBEPair<POSITTYPE, POSITTYPE> VID;

	/**< the content in the vertex. */
	VT  v; 

	/**< the VIDs of the adjacent vertexes. */
	LiBESet<VID> adj; 

	/* auxiliary type for DFS searching
	 * for detail, please refer to Cormen et al's
	 * book introduction to algorithms*/
	enum COLOR {WHITE, GRAY, BLACK};

	/** The color of this vertext.*/
	COLOR color; 
	/** path */
	VID  pi;       
	/** Discovered time of this vertex.*/
	size_t d;   
	/** Finishing time of this vertex.*/
	size_t f; 

	/** Constructor. Initialization. */
	Vertex() : color(WHITE), 
		   pi(VID_MAX, VID_MAX), 
		   d(0), f(0) {}

	//! Assignment.
	Vertex& operator = (const Vertex& other) {
	    v = other.v; 
	    adj = other.adj; 
	    color = other.color;
	    pi = other.pi; 
	    d = other.d; 
	    f = other.f;
	    return *this;
	}


	friend ostream& operator << (ostream& out, const Vertex& v) {
	    out<<"Val:"<< v.v<<"  "
	       <<"COLOR: "<<v.color
	       <<"pi: "<<v.pi
	       <<" "<<"d: "
	       <<v.d<<"f: "
	       <<v.f<<endl;

	   for(size_t i = 0; i < v.adj.size(); ++i){
		out<<" adj v: "<< v.adj[i]<<" ";
	    }
	    return out;
	}
};

/** A k-partite graph. VT is the type of vertex content. */
template<class VT >
class KpartiteGraph : public hash_map<typename Vertex<VT>::VID, Vertex<VT> >
{
    typedef  hash_map<typename Vertex<VT>::VID, Vertex<VT> > _base;

public:
    //! The type for the ID of each vertex. The vertex 
    //! ID is normally a pair consisting of the index
    //! to the slice and the index to the position in
    //! the slice.
    typedef typename Vertex<VT>::VID VID;

    //! EdgeType.
    typedef LiBEPair<VID, VID> EdgeType;


    typedef typename _base::iterator  iterator;
    typedef typename _base:: const_iterator const_iterator;

    iterator begin() { return _base::begin();}
    iterator end()   { return _base::end();}

    const_iterator begin() const {return _base::begin();}
    const_iterator end() const {return _base::end();}

    //! Constructor.
    KpartiteGraph(const size_t k) : m_K(k) {} 

    //! Returns K.
    size_t K() const { return m_K;}

    //! Assignment.
    KpartiteGraph& operator = (const KpartiteGraph& other);

    //! Destructor.
    virtual ~KpartiteGraph()   {}

    //! DFS. 
    void DFS();

    //! Transpose. Content will be changed.
    KpartiteGraph<VT>& T();

    //! Compute the strongly connected component.
    vector<KpartiteGraph<VT>*> SCC() ;  

    //! Add the edge from v1 to v2.
    //! This method assumes that e is bidirected.
    void addEdge(const EdgeType& e);

    //! Dump for debugging.
    void dump(ostream& out) const;

    //! Returns the size (in the number of vertexes) of the k-th slice.
    size_t sliceSize(size_t k) const;

    //! Returns the set of vertex IDs on the path starting from 
    //! the input vertex, which is also included in the returned 
    //! path.
    vector<VID> getPath(const VID& start) const;
    set<VID> getTree(const VID& start) const;

protected:

    size_t m_K;


    //! Helper function with DFS.
    void DFS_VISIT(const VID& id,  Vertex<VT>& u);

    //! tr stores the tree.
    void DFS_VISIT(const VID& id,  Vertex<VT>& u, set<VID>&tr);

private:
    //! Temporary variable used in DFS.
    size_t m_time;
};

#include "KpartiteGraph.cpp"

#endif /*_KpartiteGraph_H_*/

