#include <sstream>
#include "TreeProjector.h"

using namespace std;

namespace mtrule {
//! the projector.
/**
 * \param eTree   the English tree. It is represented using mtrule::Tree
 *          so that we can utilize its functionalities.
 */
string TreeProjector::
project(mtrule::Tree& eTree) 
{
    if(!is_good()){
        return "0";
    }

	m_leaves = eTree.get_leaves();

	// Determine the target language span for each syntactic constituent
	// (called on the root, and the rest is done recursively):
    TreeNode *eRoot = eTree.get_root();
 //   TreeNode::assign_alignment_spans(eRoot,_alignment);
  //  TreeNode::assign_complement_alignment_spans(eRoot,_alignment);
   // TreeNode::assign_frontier_node(eRoot);

	extract_derivations(eRoot,0, _alignment->get_target_len()-1);


	string ret = project(_derivationRoot);

	if(m_projRuleSize) {
		ret = "(TOP^1~0~0 -0 " + ret + ")";
	} else {
		ret = "(TOP~0~0 -0 " + ret + ")";
	}

	return  ret;
}


//! Project an e-tree node into an f-tree node.
string
TreeProjector::
project(DerivationNode* node) {

	// get the RuleInst from this node.
	DerivationOrNode& ornode = node->get_children();
	assert(ornode.size() == 0 || ornode.size() == 1);

	if(ornode.size() == 0) { 
		return ""; 
	}

	RuleInst* rule = _derivationRules[ornode.begin()->first];

	size_t nExpand = rule->get_nb_expansions();
	ostringstream ost; ost<<nExpand;
	string sExpand =  ost.str();

	vector<std::string> projected_segments;
	vector<bool> is_lex; // if the correspoding element in projected_segments is f lex.
	vector<int> wordinx; // the f word index if it is f lex ( the above is true)
	map<int, string> inx2pos; // word index 2 part of speech tag.

	  //cout<<rule->get_str()<<endl;
	for(int i=0; i< rule->get_rhs()->get_size(); ++i) {
	  const RuleRHS_el* rhs_el = rule->get_rhs()->get_element(i);
	  int ruleNodeIndex = rhs_el->get_rulenode_index();

	  // if this is an f lexical item, we print it out.
	  if( rhs_el->is_lexicalized()) {
		  projected_segments.push_back(rhs_el->get_lex());
		  is_lex.push_back(true);
		  int posit = rhs_el->get_start();
		  wordinx.push_back(posit);
		  align& aln = _alignment->target_to_source( posit) ;
		  // if this f-lex word is aligned only to 
		  // one e-word, we borrow the POS from the
		  // e-word.
		  if(aln.size() == 1) { 
			  int ii = *(aln.begin());
			  TreeNode* ptn = (*m_leaves)[ii];
			  inx2pos[posit]= ptn->get_cat();
		  }

		  continue;
	  } else  {
		  is_lex.push_back(false);
		  wordinx.push_back(-1);
	  } 
				  
	  RuleNode* ruleNode = rule->get_lhs()[ruleNodeIndex];
	  // just to play safe.
	  if(ruleNode == NULL) {continue;}

	  TreeNode *treeNode = ruleNode->get_treenode_ref();
	  // Find the TreeNode correspond to the current RuleNode:
	  const span& sp = rhs_el->get_span();
	  DerivationNodeDescriptor 
	    // desc(treeNode,ruleNode->is_lexicalized(),sp.first,sp.second); Daniel
	    desc(treeNode,ruleNode->is_lexicalized() && ruleNode->get_treenode_ref()->get_nb_c_spans() > 0,
		 sp.first,sp.second); // Daniel
	  // Find if there is a derivation node already corresponding
	  // to treeNode:
	  DerivationNode* derNode = NULL;
	  TreeNodeToDerivationNode_it it 
		 = _treeNodeToDerivationNode.find(desc);
	  if(it != _treeNodeToDerivationNode.end()) {
		 derNode = it->second;
	  } else {
		  assert(0);
	  }

	  projected_segments.push_back(project(derNode));
	  //cerr<<tmp_size_str<<endl;
	}

	/*
	 * assemble the segments, and label the f-lex items.
	 */
	string rule_root_label = rule->get_rootref()->get_treenode_ref()->get_cat();
	string ret = "";

	// the root label of this rule.
	if(projected_segments.size()) {
		ret += "(" + rule_root_label;

		if(projected_segments.size() == 1 && is_lex[0]){
			ret += " ";
		} else {
			if(m_projRuleSize) {
				ret += "^" + sExpand;
			}
			ret += "~0~0 -0 "; 
		}
	}

	//cout<<is_lex.size()<<" " <<projected_segments.size()<<" sssss"<<endl;
	for(size_t ss = 0; ss < projected_segments.size(); ++ss){
		// print f-lex items.
		if(is_lex[ss] && projected_segments.size() == 1){
			ret += projected_segments[ss];
		} else if(is_lex[ss]) {

			if(wordinx[ss] >= 0){
				if(inx2pos.find(wordinx[ss]) != inx2pos.end()){
					ret += "(" + inx2pos[wordinx[ss]] + " " + projected_segments[ss] + ") ";
				} else {
					ret += "(@" + rule_root_label  + " " + projected_segments[ss] + ") ";
				}
			} else {
				ret += "(@" + rule_root_label + " " + projected_segments[ss] + ") ";
			}
		} 
		// vars.
		else {
			ret += " " + projected_segments[ss];
		}
	}

	ret += ") ";
	return ret;

}


} // namespace mtrule
