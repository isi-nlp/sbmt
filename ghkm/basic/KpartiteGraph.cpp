// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _KpartiteGraph_C_
#define _KpartiteGraph_C_

#include <set>
#include "LiBEPair.h"
#include "KpartiteGraph.h"

using namespace std;

template<class VT> struct cmp {
    bool operator()(const typename KpartiteGraph<VT>::iterator v1, 
		    const typename KpartiteGraph<VT>::iterator v2) const
    {
	if(v1->second.f > v2->second.f) { return true;}
	else {return false;}
    }
};



template<class VT>
void KpartiteGraph<VT>::DFS()
{
    vector<iterator> iters;
    iterator itG;
    for(itG = begin(); itG != end(); ++itG){
	itG->second.color = Vertex<VT>::WHITE;
	itG->second.pi = VID(VID_MAX, VID_MAX);
	iters.push_back(itG);
    }

    m_time = 0; 

    // sort the vertexes according to the f.
    sort(iters.begin(), iters.end(), cmp<VT>());

    typename vector<iterator> :: iterator itt;
    for(itt = iters.begin(); itt !=  iters.end(); ++itt){
	if((*itt)->second.color == Vertex<VT>::WHITE){
	    DFS_VISIT((*itt)->first, (*itt)->second);
	}
    }
}

template<class VT>
void KpartiteGraph<VT>::DFS_VISIT(const VID& id,  Vertex<VT>& u)
{
    u.color = Vertex<VT>::GRAY;
    m_time++;
    u.d = m_time;

    typename LiBESet<VID>::iterator it;
    for(it = u.adj.begin(); it != u.adj.end(); ++it){
	Vertex<VT>& adjV = (*this)[*it];
	if(adjV.color == Vertex<VT>::WHITE){
	    adjV.pi = id;
	    DFS_VISIT(*it, adjV);
	}
    }
    u.color = Vertex<VT>::BLACK;
    m_time++;
    u.f = m_time;
}
template<class VT>
void KpartiteGraph<VT>::DFS_VISIT(const VID& id,  Vertex<VT>& u, set<VID>& tr)
{
    tr.insert(id);

    u.color = Vertex<VT>::GRAY;
    m_time++;
    u.d = m_time;

    typename LiBESet<VID>::iterator it;
    for(it = u.adj.begin(); it != u.adj.end(); ++it){
	Vertex<VT>& adjV = (*this)[*it];
	if(adjV.color == Vertex<VT>::WHITE){
	    adjV.pi = id;
	    DFS_VISIT(*it, adjV, tr);
	}
    }
    u.color = Vertex<VT>::BLACK;
    m_time++;
    u.f = m_time;
}


//! Assignment.
template<class VT>
KpartiteGraph<VT>& 
KpartiteGraph<VT>::operator = (const KpartiteGraph<VT>& other)
{
    _base::operator= (other);
    return *this;
}


//! Transpose. Content will be changed.
template<class VT>
KpartiteGraph<VT>& KpartiteGraph<VT>::T()
{
    // construct E^T.
    set<EdgeType> trE;

    iterator itG; 
    for(itG = begin(); itG != end(); ++itG){
	typename LiBESet<VID>::iterator itAdj;
	for(itAdj = itG->second.adj.begin();
			    itAdj != itG->second.adj.end(); ++itAdj){
	    trE.insert(EdgeType(*itAdj, itG->first));
	}
	itG->second.adj.clear();
    }

    // construct the E^T based on the trE.
    typename set<EdgeType>::iterator itE;
    for(itE = trE.begin(); itE != trE.end(); ++itE){
	(*this)[itE->first].adj.insert(itE->second);
    }
    return *this;
}

template<class VT>
vector<KpartiteGraph<VT>*> KpartiteGraph<VT>::SCC()  
{
    // do the initialiation.
    iterator itG;
    for(itG = begin(); itG != end(); ++itG){
	itG->second.color = Vertex<VT>::WHITE;
	itG->second.pi = VID(VID_MAX, VID_MAX);
	itG->second.d = 0;
	itG->second.f = 0;
    }

    m_time = 0;


    DFS();
    KpartiteGraph trG = *this;
    trG.T();
    trG.DFS();

    // sort the vertexes in the descending order in f[u].
    vector<iterator> iters;

    for(itG = trG.begin(); itG != trG.end(); ++itG){
	itG->second.color = Vertex<VT>::WHITE;
	iters.push_back(itG);
    }

    // sort the vertexes according to the f.
    sort(iters.begin(), iters.end(), cmp<VT>());


    vector<KpartiteGraph*> sccVec;

    typename vector<iterator> ::iterator itit;
    for(itit = iters.begin(); itit != iters.end(); ++itit){
	if((*itit)->second.color == Vertex<VT>::WHITE){
	    // extract the depth-first paths.
	    sccVec.push_back(new KpartiteGraph(K()));
	    set<VID> tr;
	    trG.DFS_VISIT((*itit)->first, (*itit)->second, tr);

	    typename set<VID>::iterator itId;
	    for(itId = tr.begin(); itId != tr.end(); ++itId){
		(*sccVec.back())[*itId] = trG[*itId];
	    }
	}
    }

    return sccVec;
}


//! Returns the set of vertex IDs on the path starting from 
//! the input vertex, which is also included in the returned 
//! path.
template<class VT>
vector<typename KpartiteGraph<VT>::VID> 
KpartiteGraph<VT>::
getPath(const VID& start) const
{
    vector<VID>  ret;

    VID st = start;
    while(1){
        const_iterator itG;
	if((itG = find(st)) != end()){
	    ret.push_back(st);
	    st = itG->second.pi;
	    if(itG->second.pi.first == VID_MAX || 
		    itG->second.pi.second == VID_MAX){
		break;
	    }
	} else {
	    cerr<<"Can not find vertex "<<st<<" in k-partite graph."
		<<endl;
	    exit(1);
	}
    }
    return ret;
}

template<class VT>
set<typename KpartiteGraph<VT>::VID> 
KpartiteGraph<VT>::
getTree(const VID& start) const
{
    set<typename KpartiteGraph<VT>::VID> ret; 
    return ret;
}


//! Set the edge from v1 to v2.
template<class VT>
void
KpartiteGraph<VT>::
addEdge(const EdgeType& e)
{
    (*this)[e.first].adj.insert(e.second);
    (*this)[e.second].adj.insert(e.first);
}

//! Dump for debugging.
template<class VT>
void KpartiteGraph<VT>:: dump(ostream& out) const
{
    const_iterator it;
    for(it = begin(); it != end(); ++it){
	cout<<"Current Vertex: "<<it->first<<endl;
	cout<<it->second;
	cout<<endl;
    }
}


//! Returns the size (in the number of vertexes) of the k-th slice.
template<class VT>
size_t KpartiteGraph<VT>:: sliceSize(size_t k) const
{
    size_t num = 0; 
    const_iterator it;
    for(it = begin(); it != end(); ++it){
	if((size_t)it->first.first == k){ num++; }
    }
    return num;
}

#endif
