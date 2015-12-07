#include <iostream>
#include <set>
#include <deque>

#include "TreeNode.h"
#include "RuleNode.h"
#include "State.h"

namespace mtrule {

/***************************************************************************
 * Definition of static members:
 **************************************************************************/

bool State::print_all = false;
int State::nb_of_states = 0; 
int State::idd = 0; 
bool State::_ignore_aux_expansion = false;
bool State::_erase_aux_nodes_when_print = false;
bool State::mark_const_head_in_rule = false;
//std::map<int, bool> State::__map = std::map<int, bool>();

/***************************************************************************
 * Constructors and destructors:
 **************************************************************************/

State::State(const State& s) {
  ++idd;
  generation_time = idd;


  // Create lhs:
  for(int i=0, size=s._lhs.size(); i<size; ++i) {
	 RuleNode *oldChild = s._lhs[i],
				 *newChild = new RuleNode(*oldChild);
	 _lhs.push_back(newChild);
  }
  // Copy scalar members:
  _nb_expansions = s._nb_expansions;
  _nb_ignored_expansions = s._nb_ignored_expansions;
  _nb_OR_expansions = s._nb_OR_expansions; // Wei
  _nb_lex = s._nb_lex;
  _is_minimal = s._is_minimal;
  ++nb_of_states;
}

State::~State() { 
  for(RuleNodes_it it=_lhs.begin(), it_end=_lhs.end(); it != it_end; ++it)
	 if(*it != NULL)
		delete *it;
  --nb_of_states;
  // cerr<<"DEL"<<this<<endl;
}

/***************************************************************************
 * Rule acquisition functions:
 **************************************************************************/

bool 
State::is_duplicate_state(State *newS) const {
  if(_lhs.size() != newS->_lhs.size()) 
	 return false;
  if(get_nb_expansions() != newS->get_nb_expansions()) 
	 return false;
  if(get_nb_OR_expansions() != newS->get_nb_OR_expansions())  {
	  return false;
	}

  return _is_duplicate_state(this,_lhs[0],newS,newS->_lhs[0]);
  return true;
}

RuleNode* 
State::get_headnode_recurse(RuleNode* rn) const
{
    if (!rn->is_internal()) return rn;
    TreeNode* tn = rn->get_treenode_ref()->get_head_tree();
    for(int i = 0; i < rn->get_nb_successors(); ++i) {
        RuleNode* rnc = _lhs[rn->get_successor_index(i)];
        if (rnc->get_treenode_ref() == tn) {
            return get_headnode_recurse(rnc);
        }
    }
    throw runtime_error("no headnode found!");
}

pair<int,bool> State::get_hw_intersect_frontier_recurse(RuleNode* rn, RuleNode* head, int pos) const
{
    if (head == rn) {
        return make_pair(pos,true);
    } else if (!rn->is_internal() && !rn->is_lexicalized()) {
        ++pos;
        return make_pair(pos,false);
    } else {
        pair<int,bool> p(pos,false);
        for(int i = 0; i < rn->get_nb_successors(); ++i) {
            RuleNode* rnc = _lhs[rn->get_successor_index(i)];
            p = get_hw_intersect_frontier_recurse(rnc,head,pos);
            if (p.second) return p;
            else pos = p.first;
        }
        return p;
    }
    throw runtime_error("headnode rule postion not found!");
}

string State::get_hw_intersect_frontier(RuleNode* rn) const
{
    try {
        RuleNode* head = get_headnode_recurse(rn);
        if (head->is_lexicalized()) return "\"" + head->get_treenode_ref()->get_word() + "\"";
        pair<int,bool> p = get_hw_intersect_frontier_recurse(rn,head,0);
        if (!p.second) throw runtime_error("no headnode in rule!");
        stringstream sstr;
        sstr << p.first;
        return sstr.str();
    } catch(runtime_error const& e) {
        return "";
    }
}

string State::get_hwt_of_frontiers(RuleNode* rn, bool getWord) const
{
    
    ostringstream ost;
    if(rn == get_rootref()){
        assert(rn->get_treenode_ref()->type() != ForestNode::OR);
    }
    TreeNode* tn = rn->get_treenode_ref();
    if(! rn->is_internal() && !rn->is_lexicalized()) {
        ost<<" ";
        // if getting words, print word sequence. if getting tags, print tag sequence
        // if node is preterminal tag is in cat
        ost << (getWord ? 
                tn->get_word() 
                : 
                (tn->is_preterm() ? 
                 tn->get_cat() 
                 :
                 tn->get_tag()));
    }
    for(int i=0;i<rn->get_nb_successors(); ++i) {
      ost << get_hwt_of_frontiers(_lhs[rn->get_successor_index(i)], getWord);
    }
    return ost.str();
    
}



bool 
State::_is_duplicate_state(const State *r1, RuleNode *n1, 
			   const State *r2, RuleNode *n2) {
  if(n1->get_treenode_ref() != n2->get_treenode_ref())
	 return false;
  if(n1->is_lexicalized() != n2->is_lexicalized())
	 return false;
  int size1 = n1->get_nb_successors();
  int size2 = n2->get_nb_successors();
  if(size1 != size2)
	 return false;
  for(int i=0; i<size1; ++i)
	 if(!_is_duplicate_state
	         (r1, r1->_lhs[n1->get_successor_index(i)],
				 r2, r2->_lhs[n2->get_successor_index(i)]))
		return false;
  return true;
}

void 
State::expand_successors(RuleNode *cur) {
  TreeNode *r = cur->get_treenode_ref();
  assert(r->type() != ForestNode::OR);
  if(cur->get_nb_successors() == 0) {
	 for(int i=0; i<r->get_nb_subtrees(); ++i) {
		TreeNode* newN = r->get_subtree(i);
		RuleNode* newNR = new RuleNode(newN,get_lhs().size());
		newNR->set_cat(newN->get_cat());
		_lhs.push_back(newNR);
		cur->add_successor_index(_lhs.size()-1);
	 }
  } 


  if(r->get_nb_subtrees()){
	  if(_ignore_aux_expansion){
		 STRING cat = r->get_cat();
		  // if the category ends with "-BAR", it is an auxiliar node.
		  if(cat.length() > 4 && cat.substr(cat.length() - 4 , 4) == "-BAR" ){
			  if(!_nb_expansions) { ++_nb_expansions; } 
			  else { _nb_ignored_expansions++; }
		  } else { 
			  // if the projected rule size is defined for this node,
			  // we increment the #expansions by that size.
			  if(r->get_projected_size() >= 0) {
				  _nb_expansions += r->get_projected_size();
			  } 
			  // otherwise, we increment by 1.
			  else {
				  ++_nb_expansions; 
			  }
		  }
	  } else {
		  // if the projected rule size is defined for this node,
		  // we increment the #expansions by that size.
		  if(r->get_projected_size() >= 0) {
				  _nb_expansions += r->get_projected_size();
		  } 
		  // otherwise, we increment by 1.
		  else {
		   ++_nb_expansions; 
		  }
	  }
  }

  _nb_nodes += r->get_nb_subtrees();


}

bool State::is_aux_node(std::string cat){
  if(cat.length() > 4 && cat.substr(cat.length() - 4 , 4) == "-BAR" ){
	  return true;
  } else {
	  return false;
  }
}

// if the corresponding tree node of 'cur' is an OR node, 'i' tells that
// we expand the i-th AND children of the OR node.
// otherwise (for AND node or UNDEF node), the 'i' is useless.
void State::expand_successors(RuleNode* cur, int i) {
  TreeNode *r = cur->get_treenode_ref();
  if(r->type() != ForestNode::OR){
	  expand_successors(cur);
  } else {
	  if(cur->get_nb_successors() == 0) {
		TreeNode* newN = r->get_subtree(i);
		RuleNode* newNR = new RuleNode(newN,get_lhs().size());
		newNR->set_cat(newN->get_cat());
		_lhs.push_back(newNR);
		cur->add_successor_index(_lhs.size()-1);
		expand_successors(newNR);
		_nb_nodes ++;
		_nb_OR_expansions ++;
	 }
  }

}

RuleNode* 
State::find_noderef(TreeNode *n) const {
  for(RuleNodes_cit it = _lhs.begin(), it_end = _lhs.end();
		it != it_end; ++it) {
	 RuleNode *nr = *it;
	 if(n == nr->get_treenode_ref())
		return nr;
  }
  return NULL;
}

bool State::is_duplicate_lhs(const State *newS) const {
  if(_lhs.size() != newS->_lhs.size()) return false;
  return _is_duplicate_lhs(this, get_rootref(), newS, newS->get_rootref());
}

bool State::_is_duplicate_lhs(const State *s1, RuleNode *lhs1, 
			      const State *s2, RuleNode *lhs2) {
  // compare labels -- there is always a label
  if (lhs1->get_cat() != lhs2->get_cat())
    return false;

  // do they have the same structure?
  if (lhs1->is_internal() != lhs2->is_internal())
    return false;
  if (lhs1->is_lexicalized() != lhs2->is_lexicalized())
    return false;

  // check if it is a parent to other nodes
  if(lhs1->is_internal()) {
    // has structure ``NP(a,b,c)'':

    int size1 = lhs1->get_nb_successors();
    int size2 = lhs2->get_nb_successors();
    if(size1 != size2)
      return false;

    // compare children
    for(int i=0; i < size1; ++i) {
      bool is_child_dup =
	_is_duplicate_lhs(s1, s1->_lhs[lhs1->get_successor_index(i)], 
			  s2, s2->_lhs[lhs2->get_successor_index(i)]);
      if (!is_child_dup)
	return false;
    }
  } else if (lhs1->is_lexicalized()) {
    if (lhs1->get_word() != lhs2->get_word())
      return false;
  } else {
    // not lexicalized, not internal -- just a variable and type
    if (lhs1->get_var_index() != lhs2->get_var_index())
      return false;
  }

  return true;
}

/***************************************************************************
 * Input/output, string representation:
 **************************************************************************/

// Get a string representation of the LHS of the rule:
std::string 
State::get_lhs_str(RuleNode* root, bool top) const {
  bool var_after_lex = false;
  return _get_lhs_str(root,top,var_after_lex);
}

// Get a string representation of the LHS of the rule:
// Dont not print OR node.
std::string 
State::_get_lhs_str(RuleNode* root, bool top, bool& var_after_lex) const {

  std::string lhs_str;

  // First, check if ref is a leaf of the rule to extract:
  if(root->is_internal()) {
	 // create ``NP(a,b,c)'':
	 bool do_not_print_this_node = false;
	 if(root->get_treenode_ref()->type() == ForestNode::OR){
		 do_not_print_this_node = true;
	 } else  if(State::is_aux_node(root->get_cat()) && root != get_rootref() && _erase_aux_nodes_when_print) {
			 do_not_print_this_node = true;
	 }

	 if(!do_not_print_this_node){
		 lhs_str += root->get_cat().c_str();
		 lhs_str += "(";
	 }
	 for(int i=0;i<root->get_nb_successors(); ++i) {
		if(i>0) lhs_str += " ";
		lhs_str += _get_lhs_str(_lhs[root->get_successor_index(i)],
										false,var_after_lex);
	 }
	 if(!do_not_print_this_node){
		 lhs_str += ")";
	 }
  } else {
	 // if it is aligned: ``x1:NP'':
	 if(!root->is_lexicalized()) {
		std::stringstream ss;
		ss << root->get_var_index();
		lhs_str += "x" + ss.str() + ":" + root->get_cat().c_str();
	 } else {
		// if it is unaligned: ``DT(the)'' or fail
		lhs_str += root->get_cat().c_str();
		lhs_str += "(\"";
		lhs_str += root->get_word().c_str();
		lhs_str += "\")";
	 }
  }
  return lhs_str;
}

  // Daniel
void
State::get_target_words(RuleNode* node, std::string& words)
{

  // First, check if ref is a leaf of the rule to extract:
  if(node->is_internal()) {
    for(int i=0;i<node->get_nb_successors(); ++i) 
      get_target_words(_lhs[node->get_successor_index(i)], words);
  } 
  else if(node->is_lexicalized())
    words += node->get_word() + " ";
}


// Set attributes of LHS.
bool
State::set_lhs_attr(RuleNode* root, bool top, int& nb_lex,
                    bool& var_after_lex) const {

  std::string lhs_str;
  bool is_complex=false;

  // First, check if ref is a leaf of the rule to extract:
  if(root->is_internal()) {
	 // create ``NP(a,b,c)'':
	 for(int i=0;i<root->get_nb_successors(); ++i) {
		if(i>0) lhs_str += " ";
		is_complex = set_lhs_attr(_lhs[root->get_successor_index(i)],
						              false,nb_lex,var_after_lex);
	 }
  } else {
	 // if it is aligned: ``x1:NP'':
	 if(!root->is_lexicalized()) {
		std::stringstream ss;
		ss << root->get_var_index();
		if(nb_lex > 0)
		  var_after_lex = true;
	 } else {
		// if it is unaligned: ``DT(the)'' or fail
		if(var_after_lex)
		  is_complex = true;
		++nb_lex;
	 }
  }
  // Set attributes before exiting:
  if(top) {

	 _desc->set_int_attribute(SIZEID, _nb_expansions);
	 //_desc->set_int_attribute("nlRHS", nb_lex);
  }
  return is_complex;
}

string State::get_head_tree(RuleNode* rn) const 
{
    if(rn == get_rootref()){
        assert(rn->get_treenode_ref()->type() != ForestNode::OR);
    }
    string ret = "";
    TreeNode* tn = rn->get_treenode_ref();
    bool do_print = true;
    if(State::is_aux_node(rn->get_cat()) && rn != get_rootref() &&  _erase_aux_nodes_when_print && rn->is_internal()){
        do_print = false;
    }

    if(rn->is_internal() && (rn->get_treenode_ref()->type() == ForestNode::OR)){
        do_print = false;
    }

    if(tn){
        //ret += "(";
        if(rn == get_rootref()){
            if(do_print) ret += "R";
        } else {
            if(tn->isHead()) {
                if(do_print){ ret += "H";}
            } else {
                if(do_print){ ret += "D";}
            }
        }

        if(rn->get_nb_successors()) {
            if(do_print){ ret += "(";}
        }
        for(int i=0;i<rn->get_nb_successors(); ++i) {
            //if(do_print){ if(i>0) ret += " ";}
             ret += get_head_tree(_lhs[rn->get_successor_index(i)]);
         }
        if(rn->get_nb_successors()) {
            if(do_print){ ret += ")";}
        }

        // ret += ")";
    }

    return ret;

}

}
