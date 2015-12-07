#ifndef _ForestNodes_H_
#define _ForestNodes_H_ 

#include <list>
#include <set>
#include <string>
#include <ext/hash_map>
//#include "LiBE.h"
#include "LabelSpan.h"
#include "TreeNode.h"

using namespace mtrule;
using namespace std;
using namespace __gnu_cxx;

///////////////////////////////////////////////////
template<class SIG>
class SimpleNodeData {
public:
	typedef SIG Signature;
	const SIG getSig() const { return sig;}
	SIG& getSig() { return sig;}
	SimpleNodeData& operator = (const SimpleNodeData& other) {
		sig = other.getSig();
		return *this;
	}
private:
	SIG sig;
};
///////////////////////////////////////////////////

template<class T>
class ORNode;

typedef mtrule::TreeNode  AA;

//! ANDNode contains a list of ORNodes. T must have T::Signature.
template<class T>
class ANDNode : public list<ORNode<T>* >, 
				public AA
//                public mtrule::TreeNode
{
	//typedef typename mtrule::TreeNode _base;
	typedef mtrule::TreeNode _base;
	typedef list<ORNode<T>*> _base2;

public:
    typedef typename T::Signature Signature;
	typedef typename mtrule::ForestNode::NodeType NodeType;

	/*********************************************
	  For my own stuffs.
	 *********************************************/
	ANDNode() { m_imdPar = NULL; _base::setType(AND); }

	// to avoid the TreeNode to do the cleaning up.
	~ANDNode() { _sub_trees.clear(); }

	T  data;

	//! Assignment.
	ANDNode<T>& operator=(const ANDNode<T>& other);

	//! computes the span of the forest nodes. 
	//! \param wdIndex  is the index to the leftmost word covered by
	//! this node.
	void computeSpan(int& wdIndex, set<ANDNode<T>*>& leavesComputed );

	//! returns the span covered by this node.
	pair<int, int> getSpan() const;
	
	/*********************************************
	  For the good of the base tree node class. 
	 *********************************************/
	//! Copy all my information upwards to the base TreeNode.
	void copyUp();


	TreeNode* imdPar() {  return m_imdPar; }
	void setImdPar(TreeNode* n) {  m_imdPar= n; }
protected:

	//! The imediate parent of this node.
	TreeNode* m_imdPar;
};


///////////////////////////////////////////////////

//! ORNode is a list of ANDNodes, corresponding to parsing alternatives. 
//! It also has an ID for sharing.
template<class T>
class ORNode : public list<ANDNode<T> >, public mtrule::TreeNode
{
	typedef list<ANDNode<T> > _base;
	typedef mtrule::TreeNode _baseTreeNode;
public:
	typedef typename ANDNode<T>::Signature Signature;
	typedef typename mtrule::ForestNode::NodeType NodeType;

	//! Constructor.
	ORNode() : id(-1) { _baseTreeNode::setType(OR); }

	// to avoid the TreeNode to do the cleaning up.
	~ORNode() { _sub_trees.clear(); }

	//! The id of the OR node.
	int id;

	//! Assignment.
	ORNode<T>& operator=(const ORNode<T>& other);

	//! Get the span covered by this node.
	pair<int, int> getSpan() const;

	//! computes the span of the forest nodes. 
	//! \param wdIndex  is the index to the leftmost word covered by
	//! 	this node.
	void computeSpan(int& wdIndex, set<ANDNode<T>*>& leavesComputed);


	/*********************************************
	  For the good of the base tree node class. 
	 *********************************************/
	//! Copy all my information upwards to the base TreeNode.
	void copyUp();

	TreeNode* imdPar() {  return parent(); }
protected:

	//! The grand parent of this ORNode.
	//TreeNode* m_parent;
};


#ifndef _ForestNodes_cpp_
#define _ForestNodes_cpp_
#include "ForestNodes.C"
#endif

#endif
