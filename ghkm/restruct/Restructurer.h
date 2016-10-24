/** Abstract class of restructurer. */
#ifndef _Restructurer_H_
#define _Restructurer_H_
#include <set>
#include <deque>
#include "SyntaxTree.h"
#include "Tree.h"
#include "PackedForest.h"
#include "LiBEException.h"

using namespace std;

class Restructurer {
public:
	Restructurer()  {}
	//! Restruct the input tree.
	virtual void restruct(::Tree& tree) = 0;

};


//! Binarizer
class Binarizer : public Restructurer {
public:
	Binarizer() {}

	//! Binarize a tree
	void restruct(::Tree& tree);

	//! Binarizes the input tree and stores the binarization result into a
	//! packed forest.
	virtual void restruct(mtrule::Tree& tree, SimplePackedForest& packf) 
	{ throw NotYetImplemented("Binarizer::restruct(tree, forest)"); }

protected:
	//! Binarize a node.
	void restruct(::Tree& tree, ::Tree::iterator node);


	//! Returns the set of the root-node's chlildren to be factored  out as a
	//! new node. The second iterator is the iterator ! that the new nodes 
	//! will be inserted before.
	virtual pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator>
		factorOutNodes(::Tree&, ::Tree::iterator root) 
		{ throw NotYetImplemented("Binarizer::factorOutNodes"); }
};

//! Left-branching binarizer.
class LeftBranchBinarizer : public Binarizer {
public:
	LeftBranchBinarizer() {}

protected:
	//! Returns the set of the root-node's chlildren to be factored 
	//! out as a new node. The second iterator is the iterator
	//! that the new nodes will be inserted before.
	pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator>
	                     	factorOutNodes(::Tree&, ::Tree::iterator root);
};

//! Right-branching binarizer.
class RightBranchBinarizer : public Binarizer {
public:
	RightBranchBinarizer() {}

protected:
	//! Returns the set of the root-node's chlildren to be factored 
	//! out as a new node. The second iterator is the iterator
	//! that the new nodes will be inserted before.
	pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator>
	                     	factorOutNodes(::Tree&, ::Tree::iterator root);
};

//! Headword outwards binarizer.
class HeadOutwardsBinarizer : public Binarizer {
public:
	HeadOutwardsBinarizer() {}

protected:
	//! Returns the set of the root-node's chlildren to be factored 
	//! out as a new node. The second iterator is the iterator
	//! that the new nodes will be inserted before.
	pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator>
	                     	factorOutNodes(::Tree&, ::Tree::iterator root);
};


//! Parallel binarizer. We now allow only ONE auxiliary node to be 
//! introduced at each binarization step. That is, at each binarization
//! step, we allow only either left-branching or right-branching.
//! When we try make a branch, we first try make one children a branch;
//! if the remaining children are non-factorizable, we try to make two 
//! two or more children branches until succeed.
class ParallelBinarizer: public Binarizer {
public:
	//! Constructor.
	ParallelBinarizer(int factorLimit = 5) : m_factorLimit(factorLimit) {}

	//! Destructor.
	virtual ~ParallelBinarizer() {}

	//! Parallel binarize the input tree in and put the result into packed
	//! forest.
	void restruct(mtrule::Tree& tree, SimplePackedForest& forest);

	//! The maximum number of children that can be factored out.
	void setFactorLimit(int limit) { m_factorLimit = limit;}


	//! Generates an OR node holding all the alternative binarizations
	//! on a set of sibling nodes. The generated OR node is added inot
	//! the forest, and returned. 'parent' is the parent of the sibling
	//! nodes in the original tree.
	SimpleORNode* genOR(mtrule::TreeNode* parent, 
			            vector<mtrule::TreeNode*>& sibV, 
			            SimplePackedForest& forest,
	  					set<int>& factorableORNodes) const;


	//! Factorizes the sibling vector parallelly. Factorization is constrained
	//! by (i) word alignments; (ii) each ANDNode has only one auxiliary child.
	//! 'parent' is the parent of the sibling nodes in the original tree.
	virtual vector<SimpleANDNode> factorize(mtrule::TreeNode* parent,
								 vector<mtrule::TreeNode*>& sibV, 
								 SimplePackedForest& forest,
								 set<int>& factorableORNodes) const;


	//! Returns true if the set of sibling nodes can be factored out
	//! as a frontier node.
	virtual bool factorizable(std::vector<mtrule::TreeNode*>& siblingNodes) const;

	void setAln(const mtrule::Alignment* aln) { m_aln = aln;}

	//! Reads the list of categores for which we dont apply the limit.
	void readExceptCategs(string file);
	
	mtrule::span get_e_span(vector<mtrule::TreeNode*>& nodes) const ;

protected:

	//! returns true if the number of nodes to be factorized exceeds
	//! the limit.
    bool withinLimit(vector<mtrule::TreeNode*>& nodes) const;

	//! The word alignment object used to determine `factorizable'.
	const mtrule::Alignment* m_aln;

	//! The maximum number of tree nodes that can be factored out.
	short m_factorLimit;


private:

	set<string> m_exceptCats;
	
};

class Tree2ForestConverter : public ParallelBinarizer 
{
public:
	Tree2ForestConverter() {}
	~Tree2ForestConverter() {}

	virtual bool factorizable(std::vector<mtrule::TreeNode*>& siblingNodes) const;
};

#endif
