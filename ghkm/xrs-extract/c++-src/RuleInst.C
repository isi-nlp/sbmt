#include <iostream>
#include <set>
#include <deque>
#include <cassert>

#include "RuleInst.h"
#include "TreeNode.h"
#include "RuleNode.h"
#include "MyErr.h"

namespace mtrule {

size_t RuleInst::max_nb_unaligned = MAX_NB_UNALIGNED;
bool   RuleInst::can_assign_unaligned_c_to_preterm = CAN_ASSIGN_UNALIGNED_C_TO_PRETERM;

/***************************************************************************
 * Rule acquisition functions:
 **************************************************************************/

// Check if the rule passed as argument is the same as the current object. 
// This compares both lhs and rhs:
bool 
RuleInst::is_duplicate_rule(RuleInst *newS) const {
  // Compare LHSs:
  if (!is_duplicate_lhs(newS))
    return false;

  // Compare RHSs:
  if(_rhs->get_size() != newS->_rhs->get_size()) return false;
  for(int i=0, size=_rhs->get_size(); i<size; ++i)
	 if(!_rhs->get_element(i)->is_equivalent(*newS->_rhs->get_element(i)))
		return false;
  // Otherwise, return true:
  return true;
}

// Compare internal representation of the rule: (pointers to TreeNode, etc)
bool 
RuleInst::is_duplicate_internal_rule(RuleInst *newS) const {
  // Compare LHSs:
  if(_lhs.size() != newS->_lhs.size()) return false;
  for(RuleNodes_cit it1=_lhs.begin(), it1_end=_lhs.end(), 
	it2=newS->_lhs.begin(), it2_end=newS->_lhs.end();
      it1 != it1_end; ++it1, ++it2) {
	 if((*it1)->get_treenode_ref() != (*it2)->get_treenode_ref()) 
		return false;
  }
  // Compare RHSs:
  if(_rhs->get_size() != newS->_rhs->get_size()) return false;
  for(int i=0, size=_rhs->get_size(); i<size; ++i)
	 if(*_rhs->get_element(i) != *newS->_rhs->get_element(i))
		return false;
  // Otherwise, return true:
  return true;
}

void 
RuleInst::get_rules_from_state(RuleInsts *rs, State *s, Alignment *a, 
                               int start, int end, bool no_multi) {
  RuleInst *r = new RuleInst(*s); 
  // Set var indexes recursively:
  int cur_var = -1;
  number_variables(r,r->_lhs[0],a,cur_var);
  //if(r->_lhs.size()){ number_variables(r,r->_lhs[0],a,cur_var);} // Wei
  // Sort elements in the RHS so that spans are in order:
  r->_rhs->sort_rhs();
  // Remove contiguous RHS elements that represent the same variable (or 
  // lexical item) and have the same span; add unaligned chinese words:
  r->_rhs->set_contiguous_non_overlapping(a,start,end);
  // Get all rules corresponding with the search state:
  if(no_multi) {
	 rs->push_back(r);
  } else {
	 handle_unaligned_chinese_words(rs,r,start,end);
	 delete r;
  }
}

  // Daniel
void 
RuleInst::get_rules_from_state_top(RuleInsts *rs, State *s, Alignment *a, 
				   int start, int end, bool no_multi) {
  RuleInst *r = new RuleInst(*s); 
  // Set var indexes recursively:
  int cur_var = -1;
  number_variables_top(r,r->_lhs[0],a,cur_var);
  // Sort elements in the RHS so that spans are in order:
  r->_rhs->sort_rhs();
  // Remove contiguous RHS elements that represent the same variable (or 
  // lexical item) and have the same span; add unaligned chinese words:
  // r->_rhs->set_contiguous_non_overlapping(a,start,end);
  // Get all rules corresponding with the search state:
  if(no_multi) {
	 rs = new RuleInsts;
	 rs->push_back(r);
  } else
    handle_unaligned_chinese_words(rs,r,start,end);
  delete r;
}


void
RuleInst::number_variables(RuleInst *r, RuleNode *ruleNode,
                           Alignment *a, int& cur_var) {
  if(ruleNode->is_internal()) {
	 // Internal node: recursive call of current function with all
	 // children:
	 for(int i=0, size=ruleNode->get_nb_successors(); i<size; ++i)
		number_variables(r,r->_lhs[ruleNode->get_successor_index(i)],a,cur_var);
  } else {
	 // Current node of the lhs of curRule:
	 TreeNode *treeNode = ruleNode->get_treenode_ref();
	 // Determine if the leaf is lexicalized:
	 if(ruleNode->is_lexicalized()) {
		// Add word to lhs:
		ruleNode->set_word(treeNode->get_word());
		// Check each Chinese word aligned to the current English 
		// word, and add it to RHS:
		int e_pos = treeNode->get_start();
		align c_pos = a->source_to_target(e_pos);
		for(align_it it = c_pos.begin(), it_end = c_pos.end(); 
			  it != it_end; ++it) {
		  int cur_pos = *it;
		  const STRING& cur_w = a->get_target_word(cur_pos);
		  RuleRHS_el *newE 
			 = new RuleRHS_el(Variable::NOT_A_VAR,
					  cur_w,cur_pos,cur_pos,
					  ruleNode->get_pos());
		  r->_rhs->add_element(newE);
		}
	 } else { // rulenode is not lexicalized:
		// Create a new variable:
		++cur_var;
		ruleNode->set_var_index(cur_var);
		assert(treeNode->get_nb_c_spans() == 1);
		RuleRHS_el *newE 
		  = new RuleRHS_el(cur_var,"",treeNode->get_c_start(0),
				   treeNode->get_c_end(0),
				   ruleNode->get_pos());
		r->_rhs->add_element(newE);
	 }
  }
}

  // Daniel
void
RuleInst::number_variables_top(RuleInst *r, RuleNode *ruleNode,
			       Alignment *a, int& cur_var) 
{
  for(int i=0, size=ruleNode->get_nb_successors(); i<size; ++i){
    RuleNode* n = r->_lhs[ruleNode->get_successor_index(i)];
    TreeNode *treeNode = n->get_treenode_ref(); 
    // Create a new variable:
    ++cur_var;
    n->set_var_index(cur_var);
    if(treeNode->get_nb_c_spans() == 1){
      RuleRHS_el *newE 
	= new RuleRHS_el(cur_var,"",treeNode->get_c_start(0),
			 treeNode->get_c_end(0),
			 n->get_pos());
      r->_rhs->add_element(newE);
    }
  }
}

void 
RuleInst::get_lhs_stuff_old(RuleNode* node, 
			std::vector < std::string > & phrases, 
			std::vector<int>& varIndex, 
			std::vector< std::pair< int, int > >& boundaries,
			bool & is_lexical)
{
  if(node->is_internal()) {
    for(int i=0, size = node->get_nb_successors(); i < size; ++i)
      get_lhs_stuff_old(_lhs[node->get_successor_index(i)], phrases, varIndex, boundaries, is_lexical);
  }
  else{
    if(node->is_lexicalized()){
      is_lexical = true;
      phrases.push_back(node->get_word());
      varIndex.push_back(-1);
      boundaries.push_back(node->get_treenode_ref()->get_span());
    }
    else if(is_lexical){
      std::stringstream ss;
      ss << node->get_var_index();
      phrases.push_back("x" + ss.str() + ":" + node->get_cat());
      varIndex.push_back(node->get_var_index());
      boundaries.push_back(node->get_treenode_ref()->get_span());
    }
  }
}


void 
RuleInst::get_lhs_stuff(RuleNode* node, 
			std::vector < std::string > & phrases, 
			std::vector<int>& varIndex, 
			std::vector< std::pair< int, int > >& boundaries)
{
  if(node->is_internal()) {
    for(int i=0, size = node->get_nb_successors(); i < size; ++i)
      get_lhs_stuff(_lhs[node->get_successor_index(i)], phrases, varIndex, boundaries);
  }
  else{
    if(node->is_lexicalized()){
      phrases.push_back(node->get_word());
      varIndex.push_back(-1);
      boundaries.push_back(node->get_treenode_ref()->get_span());
    }
    else{
      std::stringstream ss;
      ss << node->get_var_index();
      phrases.push_back("::x" + ss.str());
      // phrases.push_back("x" + ss.str() + ":" + node->get_cat());
      varIndex.push_back(node->get_var_index());
      boundaries.push_back(node->get_treenode_ref()->get_span());
    }
  }
}






// Generalized by Wei to handle e-forest, not only e-tree.
void
RuleInst::handle_unaligned_chinese_words(RuleInsts *rs, RuleInst *r, 
                                         int start, int end) {
  std::vector< std::pair<int,RuleInst*> > queue; 
  // pairs of: <index of current lhs element,RuleInst> 
  RuleInst *curRule = new RuleInst(*r);
  queue.push_back(std::make_pair(0,curRule));
  // First make sure that there are no more than max_nb_unaligned 
  // unaligned elements:
  { 
	 int nb_unaligned = 0; 
	 RuleRHS *rhs = curRule->get_rhs();
	 for(int i=0, last_pos=rhs->get_size()-1; i<=last_pos; ++i) {
		RuleRHS_el* el = rhs->get_element(i);
		if(el->get_var_index() == Variable::UNALIGNEDC)
		  ++nb_unaligned;
	 }
	 if(nb_unaligned >= static_cast<int>(max_nb_unaligned)) {
		// Stop here:
		//*myerr 
		//  << "%%% WARNING: too many unaligned source-language words in current rule: "
		//  << nb_unaligned << " >= " << max_nb_unaligned
		//  << ". Not searching all combinations." << std::endl;
		rs->push_back(queue[0].second);
		queue.erase(queue.begin());
		return;
	 }
  }
  // Process queue:
  while(queue.size() > 0) {
	 RuleInst* cur_rule = queue[0].second;
	 const RuleNodes& lhs = cur_rule->get_lhs();
	 RuleRHS *rhs = cur_rule->get_rhs();
	 int    cur_pos  = queue[0].first,
			  last_pos = rhs->get_size()-1;
	 for(int i=cur_pos; i<=last_pos; ++i) {
		RuleRHS_el* el = rhs->get_element(i);
		bool is_aligned = 
		  (el->get_var_index() != Variable::UNALIGNEDC); 
		if(is_aligned)
		  continue;
		// Here, we have an unaligned c word:
		RuleRHS_el *elx=NULL, *ely=NULL;
		if(i>0)        elx = rhs->get_element(i-1);
		if(i<last_pos) ely = rhs->get_element(i+1);
		bool canx = false, cany = false;
		// Check if current element (el) can be moved backward:
		if(elx != NULL)
		  //if(elx->get_var_index() != Variable::UNALIGNEDC) {
		  if(Variable::is_variable_index(elx->get_var_index())) {
			 int index = elx->get_rulenode_index();
			 assert(index >= 0);
			 //if(lhs[index]->get_treenode_ref()->is_internal()) 
			 // if the treenode (ref) is an OR node, we need to get the real AND node to check
			 // whether it is an internal node or not. Notice, a preterminal AND node is always
			 // the child of an OR node.
			 if(lhs[index]->get_treenode_ref()->type() != ForestNode::OR && 
					   lhs[index]->get_treenode_ref()->is_internal()        ||
			lhs[index]->get_treenode_ref()->type() == ForestNode::OR   &&
				lhs[index]->get_treenode_ref()->get_subtree(0)->get_nb_subtrees() ) {
				canx = true;
			 }
			 if(can_assign_unaligned_c_to_preterm)
			 	canx = true;
		  }
		// Check if current element (el) can be moved forward:
		if(ely != NULL)
		  //if(ely->get_var_index() != Variable::UNALIGNEDC) {
		  if(Variable::is_variable_index(ely->get_var_index())) {
			 int index = ely->get_rulenode_index();
			 assert(index >= 0);
			 // if the treenode (ref) is an OR node, we need to get the real AND node to check
			 // whether it is an internal node or not. Notice, a preterminal AND node is always
			 // the child of an OR node.
			 if(lhs[index]->get_treenode_ref()->type() != ForestNode::OR && 
					   lhs[index]->get_treenode_ref()->is_internal()        ||
			lhs[index]->get_treenode_ref()->type() == ForestNode::OR   &&
				lhs[index]->get_treenode_ref()->get_subtree(0)->get_nb_subtrees() ) {
			 //if(lhs[index]->get_treenode_ref()->is_internal())
				cany = true;
			 }
			 if(can_assign_unaligned_c_to_preterm)
			  cany = true;
		  }
		if(canx) {
		  RuleInst* newRule = new RuleInst(*cur_rule);
		  RuleRHS* new_rhs = newRule->get_rhs();
		  RuleRHS_el* nelx = new_rhs->get_element(i-1);
		  RuleRHS_el* nely = new_rhs->get_element(i);
		  nelx->set_end(nely->get_end());
		  new_rhs->delete_element(i);
		  bool dup=false;
		  for(int j=0, size=queue.size(); j<size; ++j) {
			 if(queue[j].second->is_duplicate_internal_rule(newRule)) {
				delete newRule;
				dup=true;
				break;
			 }
		  }
		  if(!dup)
			 queue.push_back(std::make_pair(i-1,newRule));
		}
		if(cany) {
		  RuleInst* newRule = new RuleInst(*cur_rule);
		  RuleRHS* new_rhs = newRule->get_rhs();
		  RuleRHS_el* nelx = new_rhs->get_element(i);
		  RuleRHS_el* nely = new_rhs->get_element(i+1);
		  nely->set_start(nelx->get_start());
		  new_rhs->delete_element(i);
		  bool dup=false;
		  for(int j=0, size=queue.size(); j<size; ++j) {
			 if(queue[j].second->is_duplicate_internal_rule(newRule)) {
				delete newRule;
				dup=true;
				break;
			 }
		  }
		  if(!dup) {
			 int prev = (i>0) ? i-1 : 0;
			 queue.push_back(std::make_pair(prev,newRule));
		  }
		}
	 }
	 //*myerr << "y: " << rhs->get_element(0)->get_start()
	 //          << " "   << rhs->get_element(last_pos)->get_end() << std::endl;
	 //*myerr << cur_rule->get_str() << std::endl;
	 rs->push_back(queue[0].second); // queue[0].second == cur_rule
	 queue.erase(queue.begin());

  }
}

/***************************************************************************
 * Input/output, string representation:
 **************************************************************************/

// Daniel 
// Misnomer!!! this function (should be get_number_source_words)
int RuleInst::get_number_target_words(){
  int no = 0;
  for(int i=0; i < _rhs->get_size(); i++){
    const RuleRHS_el* el = _rhs->get_element(i);
    int var_index = el->get_var_index();
    if(!Variable::is_variable_index(var_index))
      ++no;
  }
  return no;
}


 std::pair<int, int> RuleInst::get_first_last_target_pos()
 {
  int start = 10000000, end = 0;
  for(int i=0; i < _rhs->get_size(); i++){
    RuleRHS_el* el = _rhs->get_element(i);
    if(el->is_lexicalized()){
      if(el->get_start() < start)
	start = el->get_start();
      if(el->get_end() > end)
	end = el->get_end();
    }
  }
  return std::make_pair(start, end);
 }

 

// Get a string representation of the RHS of the rule:
std::string
RuleInst::get_rhs_str() const { 
  std::string rhs_str;
  for(int i=0, size=_rhs->get_size(); i<size; ++i) {
	 if(i>0) rhs_str += " ";
	 const RuleRHS_el* el = _rhs->get_element(i);
	 int var_index = el->get_var_index();
	 if(Variable::is_variable_index(var_index)) {
		std::stringstream ss;
		ss << var_index;
		rhs_str += "x" + ss.str();
	 } else {
		assert(el->get_lex() != "");
		rhs_str += "\"";
		rhs_str += el->get_lex().c_str();
		rhs_str += "\"";
	 }
  }
  return rhs_str;
}

std::string
RuleInst::get_str() const { 
  return get_lhs_str(get_rootref()) + " -> " + get_rhs_str();
}

// Daniel
std::string
RuleInst::get_str_vars() { 
  std::string rhs_str =  get_lhs_str(get_rootref()) + " -> ";
  for(int i=0, size=_rhs->get_size(); i<size; ++i) {
     const RuleRHS_el* el = _rhs->get_element(i);
    int var_index = el->get_var_index();
    if(Variable::is_variable_index(var_index)) {
      std::stringstream ss;
      ss << var_index;
      if(i>0) rhs_str += " ";
      rhs_str += "x" + ss.str();
    } 
  }

  return rhs_str;
}

// get only the words on the source side (no variables)
// Daniel
std::string
RuleInst::get_source_words(){
  std::string rhs_str;
  for(int i=0, size=_rhs->get_size(); i<size; ++i) {
    const RuleRHS_el* el = _rhs->get_element(i);
    if(el->is_lexicalized()) {
      rhs_str += el->get_lex() + " ";
    } 
  }
  return rhs_str;
}




//! Set various attributes describing the shape of a rule.
//! Return value is true the RHS is complex.
bool
RuleInst::set_rhs_attr(int& nb_lex) { 
  nb_lex = 0;
  bool is_complex = false;
  bool var_after_lex = false;
  for(int i=0, size=_rhs->get_size(); i<size; ++i) {
	 const RuleRHS_el* el = _rhs->get_element(i);
	 int var_index = el->get_var_index();
	 if(Variable::is_variable_index(var_index)) {
		if(nb_lex > 0)
		  var_after_lex = true;
	 } else {
		// Detect complex:
		if(var_after_lex)
		  is_complex = true;
		assert(el->get_lex() != "");
		++nb_lex;
	 }
  }
  _desc->set_int_attribute("nRHS",_rhs->get_size());
  _desc->set_int_attribute("nlRHS",nb_lex);
  return is_complex;
}

void
RuleInst::set_attr() { 
  int num_lex_lhs=-1; int num_lex_rhs=-1;
  bool var_after_lex=false;
  bool complex_lhs = set_lhs_attr(get_rootref(),true,num_lex_lhs,var_after_lex);
  bool complex_rhs = set_rhs_attr(num_lex_rhs);
  std::string type;
  if(num_lex_lhs == 0) { 
	 if(num_lex_rhs == 0)
		type = "unlex";
	 else 
		type = "tlex";
  } else if(num_lex_rhs == 0) {
	 type = "slex";
  } else {
	 type = (complex_lhs || complex_rhs) ?
			  "complex" : "AT";
  }
  _desc->set_str_attribute("type",type);
}

// Rule reading code:
RuleInst::RuleInst(const std::string& str) : State(NULL)  {
	// not implemented:
	assert(false);
}



}
