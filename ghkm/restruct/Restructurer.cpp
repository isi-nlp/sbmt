#include "Restructurer.h"
#include <fstream>
#include "StringVocabulary.h"

using namespace std;


/////////////////////////////////////////////////////////////////////////////
//  Binarizer class
/////////////////////////////////////////////////////////////////////////////
//! Binarize a tree
void Binarizer::restruct(::Tree& tree)
{ restruct(tree, tree.begin()); } 

//! Binarize a node.
void Binarizer::restruct(::Tree& tree, ::Tree::iterator node)
{
	pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator> factoredSet;

	//while(::Tree::number_of_children(node) > 2) {
	while(node.number_of_children() > 2) {
		factoredSet = factorOutNodes(tree, node);
		string label = node->getLabel();
		string newlabel = label + "-BAR";

#if 0
		deque<Tree::iterator>::iterator i = factoredSet.first.end();
		for(i = factoredSet.first.begin(); i != factoredSet.first.end(); ++i){
			if(tree.isHead(*i)){
				break;
			}
		}
#endif

		::Tree::iterator newit = tree.insert(factoredSet.second, 
				                           SynTreeNode(newlabel, 0));
#if 0
		if(i != factoredSet.first.end()) {
			tree.setAsHead(*i);
		}
#endif

		// if the tree dont have head information, we 
		// pretend that the head is the node just factored.
		tree.setAsHead(newit);
		for(deque< ::Tree::iterator>:: iterator itset = factoredSet.first.begin();
				  itset != factoredSet.first.end(); ++itset){
			::Tree::sibling_iterator nextit = *itset;
			nextit++;
			tree.reparent(newit, *itset, nextit);
		}
	}

	::Tree::sibling_iterator sibit;
	for(sibit = node.begin(); sibit != node.end(); ++sibit){
		restruct(tree, sibit);
	}
}


/////////////////////////////////////////////////////////////////////////////
// Left-branching binarizer.
/////////////////////////////////////////////////////////////////////////////

//! Returns the set of the root-node's chlildren to be factored 
//! out as a new node. The second iterator is the iterator
//! that the new nodes will be inserted before.
pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator>
LeftBranchBinarizer::factorOutNodes(::Tree& tr, ::Tree::iterator root)
{
	pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator> ret;
	ret.second = root.end();
	::Tree::sibling_iterator sibit = root.end();
	sibit--;
	ret.first.push_front(sibit);
	sibit--;
	ret.first.push_front(sibit);
	return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Right-branching binarizer.
/////////////////////////////////////////////////////////////////////////////

//! Returns the set of the root-node's chlildren to be factored 
//! out as a new node. The second iterator is the iterator
//! that the new nodes will be inserted before.
pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator>
RightBranchBinarizer::factorOutNodes( ::Tree& tr, ::Tree::iterator root)
{
	pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator> ret;
	::Tree::sibling_iterator sibit = root.begin();
	ret.first.push_back(sibit);
	sibit++;
	ret.first.push_back(sibit);
	sibit++;
	ret.second = sibit;
	return ret;
}



/////////////////////////////////////////////////////////////////////////////
// Head-outwards binarizer.
/////////////////////////////////////////////////////////////////////////////
//! Returns the set of the root-node's chlildren to be factored 
//! out as a new node. The second iterator is the iterator
//! that the new nodes will be inserted before.
pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator>
 HeadOutwardsBinarizer::factorOutNodes(::Tree& tr, ::Tree::iterator root)
{
	assert(root.number_of_children() >= 3);

    pair<deque< ::Tree::iterator>, ::Tree::sibling_iterator> ret;
	::Tree::sibling_iterator sibit, it1;
	for(sibit = root.begin(); sibit != root.end(); ++sibit){
		if(tr.isHead(sibit)){
			it1 = sibit;
		    it1	++;
			if(it1 == root.end()){
			    it1 = sibit;
				it1 --;
				ret.first.push_back(it1);
				ret.first.push_back(sibit);
				ret.second = root.end();
			} else {
				ret.first.push_front(it1);
				ret.first.push_front(sibit);
				it1 ++;
				ret.second = it1;
			}
		}
	}
	return ret;
}


//////////////////////////////////////////////////////////////////
// All binarizer.
//////////////////////////////////////////////////////////////////
void ParallelBinarizer:: 
restruct(mtrule::Tree& tree, SimplePackedForest& forest)
{
	// get the set of siblings of the top node of tree.
	mtrule::TreeNode* root = tree.get_root();
	if(root){
		vector<mtrule::TreeNode*> sibV;
		for(int i = 0; i < root->get_nb_subtrees(); ++i) {
			sibV.push_back(root->get_subtree(i));
		}
		set<int> factorableNodesIDs;
		SimpleORNode* orNode = genOR(tree.get_root(), sibV, forest, factorableNodesIDs);
		forest.setAsRoot(orNode);
	}
}


// factorOutNodes contains the OR node ids that are factorable and are
// descendant of the formed OR node.
// 'parent' is used to form new labels.
SimpleORNode* 
ParallelBinarizer::
genOR(mtrule::TreeNode* parent, 
      vector<mtrule::TreeNode*>& sibV, 
	  SimplePackedForest& forest,
	  set<int>& factorableORNodes) const
{
	LabSpanSig  lss(parent, sibV);
	ostringstream ost;
	ost<<lss;
	SimpleORNode* orNode = const_cast<SimpleORNode*>(forest.getNode(ost.str()));
	if(orNode){return orNode;}

	SimpleORNode* ret;
	//set<SimpleANDNode> 
	vector<SimpleANDNode> andNodes = factorize(parent, sibV, 
			                                forest, factorableORNodes);
	//set<SimpleANDNode> :: const_iterator itset;
	vector<SimpleANDNode> :: const_iterator itset;
	assert(andNodes.size());
	for(itset = andNodes.begin(); itset != andNodes.end(); ++itset){
		ret = forest.addNodeBySig(*itset);
	}
	return ret;
}

//! Factorizes the sibling vector parallelly. We only factorize nodes that 
//! can form frontier nodes according to word alignments, and each ANDNode 
//! has only one auxiliary child.
//set<SimpleANDNode> 
vector<SimpleANDNode> 
ParallelBinarizer::
factorize(mtrule::TreeNode* parent, vector<mtrule::TreeNode*>& sibV, 
		  SimplePackedForest& forest, set<int>& factorableORNodes ) const
{
	//if(parent) {
	//	cout<<parent->get_cat()<<endl;
	//}
	//set<SimpleANDNode> ret;
	vector<SimpleANDNode> ret;
	bool leftFactorable = false, rightFactorable = false;
	bool leftWithinLimit= false, rightWithinLimit= false;
	bool idLeft, idRight;
	// new AND nodes to be formed.
	SimpleANDNode andNodeRight, andNodeLeft; 
	set<int> leftFactorableNodesIDs, rightFactorableNodesIDs;
	vector<mtrule::TreeNode*> nodesFactored;
	set<int> tmp_set;

	int i, j;
	/*
	 * Each time we make i children as branches. Or, in other words, we
	 * factor out (sibV - i) sibling nodes, and form a new node. 
	 */

	// We dont binarize nodes having <= 2 children.
	if(sibV.size() > 2){
		/*
		 * we first perform right factorization.
		 */

		// nodes to be factored.
		for(j=1; j < sibV.size(); ++j){ nodesFactored.push_back(sibV[j]); }
		rightWithinLimit = withinLimit(nodesFactored);

		// form the signature of the new node.
		LabSpanSig lss(parent, sibV);
		andNodeRight.data.getSig() = lss;

		// the branch.
		vector<mtrule::TreeNode*> nodesV;
		for(size_t k = 0; k < sibV[0]->get_nb_subtrees(); ++k){
			nodesV.push_back(sibV[0]->get_subtree(k));
		}
		// if this is a PoS node, we apply the hack.
		if(!nodesV.size()) { nodesV.push_back(sibV[0]); }
		// finally generate an OR node for the this branch.
		andNodeRight.push_back(genOR(sibV[0], nodesV, forest, tmp_set));
		// generate an OR node for the factored-out nodes.
		andNodeRight.push_back(genOR(parent, nodesFactored, forest, 
									rightFactorableNodesIDs));

		// since these nodes are factorable, we memo-size the new formed
		// OR node id.
		if(factorizable(nodesFactored)){
		    rightFactorable = true;
			idRight = andNodeRight.back()->id;
		}

		/*
		 * we perform left factorization.
		 */
		nodesFactored.clear();

		// nodes to be factored out.
		for(j=0; j < sibV.size() - 1; ++j){ 
			nodesFactored.push_back(sibV[j]); 
		}
		leftWithinLimit = withinLimit(nodesFactored);


		LabSpanSig lss1(parent, sibV);
		andNodeLeft.data.getSig() = lss1;

		// make the new node.
		andNodeLeft.push_back(genOR(parent, nodesFactored, forest,
					leftFactorableNodesIDs));

		// make the branches.
		nodesV.clear();
		for(size_t k = 0; k < sibV[sibV.size() -1 ]->get_nb_subtrees(); ++k){
			nodesV.push_back(sibV[sibV.size() - 1 ]->get_subtree(k));
		}
		// if this node is a PoS node, we use the hack so that we
		// can also output the leaf word.
		if(!nodesV.size()) { nodesV.push_back(sibV[sibV.size() - 1 ]); }
		// make an OR node for the branch.
		andNodeLeft.push_back(genOR(sibV[sibV.size()-1], nodesV, forest,
									tmp_set));


		if(factorizable(nodesFactored)){
			leftFactorable = true;
			idLeft = andNodeLeft.front()->id;
		}

		if(leftFactorable && rightFactorable){
			//cout<<"LF && RF\n";
			if(rightWithinLimit){ ret.push_back(andNodeRight);}
			if(leftWithinLimit) {ret.push_back(andNodeLeft);}
			factorableORNodes.insert(idLeft);
			factorableORNodes.insert(idRight);
			factorableORNodes.insert(leftFactorableNodesIDs.begin(), 
									 leftFactorableNodesIDs.end());
			factorableORNodes.insert(rightFactorableNodesIDs.begin(),
									 rightFactorableNodesIDs.end());
			if(ret.size()){ return ret;}
		} else if(leftFactorable) {

			//cout<<"LF && ! RF\n";
			factorableORNodes.insert(idLeft);
			if(leftWithinLimit){ ret.push_back(andNodeLeft);}
			if(!includes(leftFactorableNodesIDs.begin(),
						 leftFactorableNodesIDs.end(),
						rightFactorableNodesIDs.begin(),
						 rightFactorableNodesIDs.end()) &&
					rightFactorableNodesIDs.size()){
				if(rightWithinLimit){ ret.push_back(andNodeRight);}
			} 
			factorableORNodes.insert(leftFactorableNodesIDs.begin(), 
									 leftFactorableNodesIDs.end());
			factorableORNodes.insert(rightFactorableNodesIDs.begin(),
									 rightFactorableNodesIDs.end());
			if(ret.size()){ return ret;}

		}else if(rightFactorable){
			//cout<<"! LF &&  RF\n";
			factorableORNodes.insert(idRight);
			if(rightWithinLimit){ ret.push_back(andNodeRight);}
			if(!includes(rightFactorableNodesIDs.begin(),
						rightFactorableNodesIDs.end(),
					   leftFactorableNodesIDs.begin(),
					    leftFactorableNodesIDs.end()) && 
						            leftFactorableNodesIDs.size()) {
				if(leftWithinLimit){ 
					//cout<<rightFactorableNodesIDs.size()<<" "
					//	<<leftFactorableNodesIDs.size()<<endl;
					//cout<<"INSERT\n";
					ret.push_back(andNodeLeft);
				}
			} 
			factorableORNodes.insert(leftFactorableNodesIDs.begin(), 
									 leftFactorableNodesIDs.end());
			factorableORNodes.insert(rightFactorableNodesIDs.begin(),
									 rightFactorableNodesIDs.end());

			if(ret.size()){ return ret;}

		} else {
			//cout<<"! LF &&  !RF\n";
			if(leftFactorableNodesIDs == rightFactorableNodesIDs){
				if(rightFactorableNodesIDs.size()){
					if(rightWithinLimit){ ret.push_back(andNodeRight);}
				}
			} else {
				if(rightFactorableNodesIDs.size()) {
					if(rightWithinLimit){ ret.push_back(andNodeRight);}
				}
				if(leftFactorableNodesIDs.size()){
					if(leftWithinLimit){ ret.push_back(andNodeLeft);}
				}
			} 
			factorableORNodes.insert(leftFactorableNodesIDs.begin(), 
									 leftFactorableNodesIDs.end());
			factorableORNodes.insert(rightFactorableNodesIDs.begin(),
									 rightFactorableNodesIDs.end());

			if(ret.size()){ return ret;}
		}
	} else {
		// clear it so that factorizableORNodes memo-ize the nodes
		// corresponding to the same production.
		factorableORNodes.clear();
	}

	/*
	 * If all the sibling nodes cannot be factorized, i.e., only have <= 2
	 * children or cannot be factorized any way when having >=3 children,
	 * we just form a new node covering all the sibling nodes passed in.
	 */
	LabSpanSig lss(parent, sibV);
	SimpleANDNode andNode;
	andNode.data.getSig() = lss;

	for(i = 0; i < sibV.size(); ++i){
		vector<mtrule::TreeNode*> nodesV;
		for(j = 0; j < sibV[i]->get_nb_subtrees(); ++j) {
			nodesV.push_back(sibV[i]->get_subtree(j));
		}

		// if the sibV[i] doesnt have any children, we have reached a leaf.
		// but since the mtrule::TreeNode holds PoS and word in the same
		// node, we need a hack to handle it.
		if(nodesV.size()) {
			factorableORNodes.clear();
			andNode.push_back(genOR(sibV[i], nodesV, forest, factorableORNodes));
		} else {
	       if(sibV.size() && *sibV.begin() == parent){ }
		   else {
				nodesV.push_back(sibV[i]);
		   }
		   andNode.push_back(genOR(sibV[i], nodesV, forest, tmp_set));
		}
	}
	ret.push_back(andNode);
	return ret;
}


bool 
ParallelBinarizer::
withinLimit(vector<mtrule::TreeNode*>& nodes) const {
	// if the node label is exceptional, we dont have the limit.
	if(nodes.size()) {
		string parentLab = nodes.front()->parent()->get_cat();
		if(m_exceptCats.find(parentLab) != m_exceptCats.end()) {
			return true;
		}
	}

	if(nodes.size() > m_factorLimit) { 
		// if the nodes contains the head, then it is fine.
		for(size_t i = 0; i < nodes.size(); ++i){
			if(nodes[i]->parent()->get_head_tree() == nodes[i]){
				return true;
			}
		}
		return false;
	} else {
		return true;
	}
}

//! Returns true if the set of sibling nodes can be factored out
//! as a new frontier node. In other words, the inside span of
//! these nodes should have no overlap with the outside span.
//! this piece of code is essentially a copy from the xrs-extract.
//! I, however, dont quite understand it.
bool 
ParallelBinarizer::
factorizable(vector<mtrule::TreeNode*>& nodes) const {
	if(!nodes.size()) { return true;}

	mtrule::spans inside_spans;
	mtrule::spans outside_spans;
	vector<mtrule::TreeNode*>::const_iterator i;
	size_t j;
	// compute the inside span.
	for(i = nodes.begin(); i != nodes.end(); ++i){
		for(j = 0; j < (*i)->get_nb_c_spans(); ++j){
			inside_spans.push_back((*i)->get_c_span(j));
			//cout<<inside_spans.back().first<<":"<<inside_spans.back().second<<endl;
		}
	}

	// comptue the outside span. first get the outside span
	// of the parent node.
	mtrule::TreeNode* par = (*nodes.begin())->parent();
	for(j = 0; j < par->get_nb_c_comp_spans(); ++j){
		outside_spans.push_back(par->get_c_comp_span(j));
	}

	// then get the inside span of the sibling nodes not in 'nodes'.
	set<mtrule::TreeNode*> nodes_set(nodes.begin(), nodes.end());
	size_t k;
	for(j = 0; j < par->get_nb_subtrees(); ++j){
		if(nodes_set.find(par->get_subtree(j)) == nodes_set.end()){
			for(k = 0; k < par->get_subtree(j)->get_nb_c_spans(); ++k){
				outside_spans.push_back(par->get_subtree(j)->get_c_span(k));
			}
		}
	}

	mtrule::TreeNode::merge_spans(inside_spans, 
			             const_cast<mtrule::Alignment*>(m_aln));
	mtrule::TreeNode::merge_spans(outside_spans, 
		   	             const_cast<mtrule::Alignment*>(m_aln));

	// BEGIN
	// the following codes decide whether the factored-out nodes
	// can form a extraction node.
    // decide if the merged span can form extraction node.
	// there is a hole in chinese, then the node cannot be factored out.
	// the span size can be 0, in which case, it cannot be factorized.
	if(inside_spans.size() != 1 ) {return false;}
	else {
		mtrule::span espan = get_e_span(nodes);
		if(!inside_spans.size()) {exit(1);}
		if(!TreeNode::clean_chinese_to_english_alignment(inside_spans[0], espan, 
				            const_cast<mtrule::Alignment*>(m_aln))) {
			return false;
		}
	}
	// END. 

	// make sure there is no overlapping.
	for(k = 0; k < outside_spans.size(); ++k){
		inside_spans.push_back(outside_spans[k]);
	}

     std::stable_sort(inside_spans.begin(),inside_spans.end(),mtrule::TreeNode::span_sort());
     // If there is any overlap, this means that it isn't a pot frontier node:
     bool ret = true;
     for(int i=1, size=inside_spans.size(); i<size; ++i) {
			//cout<<inside_spans.back().first<<":"<<inside_spans.back().second<<endl;
        if(inside_spans[i-1].second >= inside_spans[i].first) {
          ret = false;
          break;
        }
     }

    return ret;
}

//! Reads the list of categores for which we dont apply the limit.
void ParallelBinarizer::
readExceptCategs(string file)
{
	ifstream in(file.c_str());
	if(in){
		string line;
		while(getline(in, line)){
			m_exceptCats.insert(line);
		}
	}
}

mtrule::span ParallelBinarizer::
get_e_span(vector<mtrule::TreeNode*>& nodes) const 
{
	mtrule::span ret(100000, -100);
	for(size_t i = 0; i < nodes.size(); ++i){
		if(ret.first > nodes[i]->get_start()){
			ret.first = nodes[i]->get_start();
		}

		if(ret.second < nodes[i]->get_end()){
			ret.second = nodes[i]->get_end();
		}
	}
	return ret;
}

//! Factorizes the sibling vector parallelly. Factorization is constrained
//! by (i) word alignments; (ii) each ANDNode has only one auxiliary child.
//! 'parent' is the parent of the sibling nodes in the original tree.
bool 
Tree2ForestConverter::
factorizable(vector<mtrule::TreeNode*>& nodes) const 
{
	if(!nodes.size()){ return true;}


	if(nodes.size() < (*nodes.begin())->parent()->get_nb_subtrees()){
		return false;
	} else {
		return true;
	}
}

