#ifndef _PackedForest_H_
#define _PackedForest_H_ 
#include <list>
#include <set>
#include <string>
#include <ext/hash_map>
#include "LiBE.h"
#include "ForestNodes.h"

class ForestFormatError {
public:
	ForestFormatError() { cerr<<"Forest format error!\n";}
};

//! Every ANDNode is first stored in an ORNode, and then used by other ANDNodes.
template<class T>
class PackedForest
{
public:
	virtual ~PackedForest() ;

	//! Adds an ANDNode into the forest. This method knows which ORNode this
	//! ANDNode will be inserted into. If there is no such an ORNode, we create
	//! one.
	//ORNode<T>* addNode(const ANDNode<T>& andNode);

	//! Ands an AND node into the packed forest. Automatically add and ID,
	//! if the corresponding OR node doesnt exist.
	ORNode<T>* addNodeBySig(const ANDNode<T>& andNode);
	ORNode<T>* addNodeById(const ANDNode<T>& andNode, int Id);

	const ORNode<T>* getNode(const string sig) const;

	//! Get an ORNode pointer via an ORNode id.
	const ORNode<T>* getORNode(int id) const;

	void setAsRoot(ORNode<T>* root) { m_root = root;}
	ORNode<T>* getRoot() const { return m_root;}

	//! Input.
	virtual istream& get(istream& in); 

	//! Output.
	virtual ostream& put(ostream& out) const;

	void changeToNextRoot();

	//! computes the span of each node.
	void computeSpans()  { 
		if(m_root) { 
			int start = 0; 
			set<ANDNode<T>*> leaves; 
			m_root->computeSpan(start, leaves);
		}
	}

	//! update the information in the base class of each OR/AND node.
	void udpateBase()  {if(m_root){m_root->updateBase();}}

	//! Remove all existing leaves, and put the label of these leaves into
	//! the word field of its parent. This is needed for the usage of the
	//! forest in the rule extractor. 
	void cutLeaves();
	
	void dump() {
		for(typename list<ORNode<T> >::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it){
			for(typename list<ANDNode<T> >::iterator itt = it->begin(); itt != it->end(); ++itt){
			cout<<itt->type()<<itt->data.getSig().label<<endl;
		}
		}

	}
private:

	//! reading.
	ORNode<T>* read_an_OR_node(istream& in);
	ANDNode<T>* read_an_AND_node(istream& in);

	//! writing.
	void writeNode(ostream& o, 
			const ORNode<T>* node, set<int>& writtenOut) const;
	void writeNode(ostream& o, 
			const ANDNode<T>* node, set<int>& writtenOut) const;


	//! the list of OR nodes.
	list<ORNode<T> > m_nodes;


	//! The root of the forest --- we only have a root which 
	//! is an ORNode.
	ORNode<T>*           m_root;

	//! this index is used to efficiently retrieve the OR node via 
	//! the signature string.
	hash_map<string, ORNode<T>*> m_indexBySig;


private:
	//! Remove all existing leaves of the subforest, and put the label of 
	//! these leaves into the word field of its parent. This is needed for 
	//! the usage of the forest in the rule extractor. 
	void cutLeavesOfSubForest(mtrule::TreeNode*, set<mtrule::TreeNode*>& nodes);

};


typedef PackedForest<SimpleNodeData<LabSpanSig> >  SimplePackedForest;
typedef ANDNode<SimpleNodeData<LabSpanSig> >       SimpleANDNode;
typedef ORNode<SimpleNodeData<LabSpanSig> >        SimpleORNode;

#ifndef _PackedForest_cpp_
#define _PackedForest_cpp_
#include "PackedForest.cpp"
#endif


#endif

