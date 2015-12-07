#include <iostream>
#include "PackedForest.h"
#include "TreeNode.h"
#include "LabelSpan.h"
#include <string>
//#include "LiBE.h"

using namespace std;
using namespace __gnu_cxx;
using namespace mtrule;

////////////////////////////////////////////////////////////////
// Packed Forest.
////////////////////////////////////////////////////////////////

template<class T> 
PackedForest<T>:: ~PackedForest() 
{
}

#if 0
template<class T> 
ORNode<T>* PackedForest<T>:: 
addNode(const ANDNode<T> & andNode) 
{
	typename T::Signature sig = andNode.data.getSig(); //
	// andNode sholuld have assignment.
	ORNode<T>* pnode;
	if(pnode = m_index[sig]){
		pnode->push_back(andNode);
	} else {
		ORNode<T> orNode;
		orNode.id=m_nodes.size();
		m_nodes.push_back(orNode);
		m_nodes.back().push_back(andNode);
		m_index[sig] = &m_nodes.back();
		pnode = & m_nodes.back();
	}

	return pnode;
}
#endif

//! Ands an AND node into the packed forest. Automatically add and ID,
//! if the corresponding OR node doesnt exist.
template<class T> 
ORNode<T>* PackedForest<T>:: 
addNodeBySig(const ANDNode<T> & andNode) 
{
	//cerr<<"Adding and node:"<<andNode.data.getSig()<<endl;
	ostringstream ost;
	ost<<andNode.data.getSig();

	typename hash_map<string, ORNode<T>*>::const_iterator it1;
	it1 = m_indexBySig.find(ost.str());

	if(it1 != m_indexBySig.end()){
		typename list<ANDNode<T> >::const_iterator it2;
		for(it2 = it1->second->begin(); it2 != it1->second->end(); ++it2){
			if(*it2 == andNode){ break; }
		}
		if(it2 == it1->second->end()){
			it1->second->push_back(andNode);
			it1->second->back().setType(ForestNode::AND);
		}
		return it1->second;
	} else {
		// create a new OR node, and then put this and node in.
		ORNode<T> n;
		m_nodes.push_back(n);
		m_nodes.back().id = m_nodes.size() +  1;
		m_nodes.back().push_back(andNode);
		m_indexBySig[ost.str()] = &m_nodes.back();
		m_nodes.back().setType(ForestNode::OR);
		return &(m_nodes.back());
	}
}

//! Ands an AND node into the packed forest. Automatically add and ID,
//! if the corresponding OR node doesnt exist.
template<class T> 
ORNode<T>* PackedForest<T>:: 
addNodeById(const ANDNode<T> & andNode, int id) 
{
	//cerr<<"Adding and node:"<<andNode.data.getSig()<<endl;
	ostringstream ost;
	ost<<andNode.data.getSig();

	ORNode<T>* pOR = const_cast<ORNode<T>*>(getORNode(id));

	if(pOR){
		typename list<ANDNode<T> >::const_iterator it2;
		pOR->push_back(andNode);
		pOR->back().setType(ForestNode::AND);
		return pOR;
	} else {
		// create a new OR node, and then put this and node in.
		ORNode<T> n;
		m_nodes.push_back(n);
		m_nodes.back().id = id;
		m_nodes.back().push_back(andNode);
		m_nodes.back().setType(ForestNode::OR);
		m_nodes.back().back().setType(ForestNode::AND);
		return &(m_nodes.back());
	}
}

template<class T>
const ORNode<T>* PackedForest<T>::
getNode(const string sig) const
{
	typename hash_map<string, ORNode<T>*>::const_iterator it1;
	it1 = m_indexBySig.find(sig);

	if(it1 != m_indexBySig.end()){
		return it1->second;
	} else { return NULL; }
}

//! Get an ORNode pointer via an ORNode id.
template<class T>
const ORNode<T>* PackedForest<T>:: getORNode(int id) const { 
	typename list<ORNode<T> >::const_iterator itlist;
   	for(itlist = m_nodes.begin(); itlist != m_nodes.end(); ++itlist){
		if((*itlist).id == id){return &(*itlist);}
	}
	return NULL;
}

template<class T> 
istream& PackedForest<T>::get(istream& in)
{
	if(in.peek() == '0') { m_root = NULL; return in; }

	ORNode<T>* orNode;
	try{
		orNode = read_an_OR_node(in, 0);
		setAsRoot(orNode);
	} catch (ForestFormatError& e) {
		setAsRoot(NULL);
		return in;
	}

	computeSpans();
	orNode->copyUp();

	// also create the signature map here.
	return in;
}

// or-node is of form
// form1: (#id (NN ...) ... (NN ...), or
// form2: (#id)
// form3  (#id leaf)
// the latter node refers to anther node in the shared forest.
template<class T> 
ORNode<T>*PackedForest<T>::read_an_OR_node(istream& in, int height)
{
	char ch;
	while(in.peek() == ' '){ in>>ws;}
	if(in.eof()){ return NULL; }

	// we come to ( 
	in>>ch;
	if(ch != '('){ 
		cerr<<"Expected (, but got "<<ch<<endl; 
		throw ForestFormatError();
		return NULL;
	}
	// we come to #
	in>>ch;
	if(ch != '#'){ 
		cerr<<"Expected #, but got "<<ch<<endl; 
		throw ForestFormatError();
		return NULL;
	}

	// or node id.
	int nodeID;
	in >> nodeID;

	while(in.peek() == ' '){ in>>ws;}

	// if this or node stores just a id ..., i.e., (#4)
	if(in.peek() == ')'){
		in>>ch;
		return const_cast<ORNode<T>*>(getORNode(nodeID));
	} else {
		ORNode<T>* porNode = NULL;
		// (#4 (NN...) (NN ...))
		// form3  (#id leaf)
		while(in.peek() != ')'){
			ANDNode<T>* pandNode=read_an_AND_node(in, height);
			porNode = addNodeById(*pandNode, nodeID);
			porNode->id = nodeID;
			delete pandNode;
			while(in.peek() == ' '){ in>>ws; }
		}
		// read the )
		in>>ch;
		return porNode;
	} 
}

// an and node is of form
// (label #1(children)).  
template<class T> 
ANDNode<T>*PackedForest<T>::read_an_AND_node(istream& in, int height)
{
	char ch;
	ANDNode<T>* andNode = new ANDNode<T>;


	while(in.peek() == ' '){in>>ws;}
	if(in.eof()){ return NULL; }

	// not a leaf
	if(in.peek() == '('){
		in >> ch;
		// read the label.
		in >> andNode->data.getSig().label;

		while(in.peek() == ' '){in>>ws;}
		while(in.peek() != ')'){
			try {
				andNode->push_back(read_an_OR_node(in, height+1));
			} catch (ForestFormatError& e){
				throw e;
				return NULL;
			}

			while(in.peek() == ' '){in>>ws;}
		}
		while(in.peek() == ' '){in>>ws;}
		in >> ch;
	} else {
		// leaf
		string label = "";
		while(in.peek() != ')'){
			in>>ch;
			label += ch;
		}
		//in >>ch;

	
		// if the label is '-RRB-' or '-LRB-', we change it into ')' or '('
		// sliently. that is, internally, it is always the round bracketes
		// themselves. on disk, it will be -RRB- or -LRB-.
        // Wei Wang  Wed Feb 21 12:22:02 PST 2007
		//if(label == "-RRB-") { label = ")";}
		//if(label == "-LRB-") { label = "(";}
        string::size_type pos;
        while((pos=label.find("-RRB-")) != string::npos){
            label.replace(pos, 5, ")");
        }
        while((pos=label.find("-LRB-")) != string::npos){
            label.replace(pos, 5, "(");
        }
		
		andNode->data.getSig().label = label;
		andNode->data.getSig().isInternal = false;
	}

	return andNode;
}


template<class T>
void PackedForest<T>::
writeNode(ostream& o, const ORNode<T>* node, set<int>& writtenOut) const
{
	//cout<<"OR: "<<node->is_extraction_node()<<endl;
	if(writtenOut.find(node->id) != writtenOut.end()){
		o<<" (#"<<node->id<<")";
	} else {
		o<<" (#"<<node->id<<"";
//		o<<" " <<node->type()<<" ";
		typename list<ANDNode<T> > :: const_iterator it;
		for(it = node->begin();  it != node->end(); ++it){
			writeNode(o, &(*it), writtenOut);
		}
		o<<")";
		writtenOut.insert(node->id);
	}
}

template<class T>
void PackedForest<T>::
writeNode(ostream& o, const ANDNode<T>* node, set<int>& writtenOut) const
{
//	cout<<"AND: "<<node->is_extraction_node()<<endl;
	if(node->size()){
		// internal node.
		o<<" ("<<node->data.getSig().label<<" ";
//o<<"--" <<node->get_span().first<<"-"<< node->get_span().second<<" ";
	//	o<<"::"<<node->is_extraction_node()<<" ";
		typename list<ORNode<T>*>::const_iterator it;
		for(it = node->begin(); it != node->end(); ++it){
			writeNode(o, *it, writtenOut);
		}
		o<<")";
	} else {
		// leaf
		string label = node->data.getSig().label;

        // Wei Wang Wed Feb 21 12:06:16 PST 2007
		//if(label == ")") { label = "-RRB-"; } 
		//else if(label == "("){ label = "-LRB-"; }
        // replace every ')' (or '(') occurence with '-RRB-' or '-LRB-'.
        string::size_type pos;
        while((pos=label.find(")")) != string::npos){
            label.replace(pos, 1, "-RRB-");
        }
        while((pos=label.find("(")) != string::npos){
            label.replace(pos, 1, "-LRB-");
        }
        
		o<<" "<<label;
	}
}

template<class T>
ostream& PackedForest<T>:: put(ostream& out) const
{
	set<int> orNodeIDs;
	if(m_root){
		writeNode(out, m_root, orNodeIDs); 
	} else {
		out<<"0"<<endl;
	}
	return out;
}

template<class T>
void PackedForest<T>:: changeToNextRoot()
{
	if(getRoot()){
		if(getRoot()->size()){
			setAsRoot(*(getRoot()->begin()->begin()));
		}
	}
}


template<class T>
void PackedForest<T>::cutLeaves()
{
	if(!m_root) { return;}
	set<mtrule::TreeNode*> nodes;
	set<mtrule::TreeNode*> doneNodes;
	cutLeavesOfSubForest(getRoot(), nodes, doneNodes);
	for(typename set<mtrule::TreeNode*>::iterator it = nodes.begin(); 
												it != nodes.end(); ++it) {
		ANDNode<T>* andNode = static_cast<ANDNode<T>*>(*it);
				string word = (*andNode->begin())->begin()->data.getSig().label;
				andNode->set_word(word);
				andNode->get_subtrees().clear();
				andNode->clear();
	}
}

template<class T>
void PackedForest<T>::cutLeavesOfSubForest(mtrule::TreeNode* node, set<mtrule::TreeNode*>& nodes, set<mtrule::TreeNode*> & doneNodes)
{
	if(doneNodes.find(node) != doneNodes.end()){
		return;
	}
	if(node) {
		if(node->type() == ForestNode::AND){
			ANDNode<T>* andNode = dynamic_cast<ANDNode<T>*>(node);
			assert(andNode);
			if(andNode->size() == 1 && (*andNode->begin())->size() == 1 &&
					(*andNode->begin())->begin()->size() == 0){
				nodes.insert(andNode);
#if 0
				string word = (*andNode->begin())->begin()->data.getSig().label;
				andNode->set_word(word);
				andNode->get_subtrees().clear();
				andNode->clear();
#endif
//				return;
			}
			for(typename list<ORNode<T>*>::iterator it = andNode->begin(); it != andNode->end();
					++it){
				cutLeavesOfSubForest(*it, nodes, doneNodes);
			}
		} else if(node->type() == ForestNode::OR){
			ORNode<T>* orNode = dynamic_cast<ORNode<T>*>(node);
			assert(orNode);
			for(typename list<ANDNode<T> >::iterator it = orNode->begin(); it != orNode->end();
					++it){
				cutLeavesOfSubForest(&(*it), nodes, doneNodes);
			}
		} else {
//			cout<<"ERROR: "<<node->get_cat()<<" "<<node->type()<<" span: "<<node->get_span().first<<"-"<<node->get_span().second<<endl;

		}
	}

	doneNodes.insert(node);
}


