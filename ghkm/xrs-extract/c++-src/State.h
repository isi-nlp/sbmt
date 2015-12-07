#ifndef __STATE_H__
#define __STATE_H__

/***************************************************************************
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: State.h,v 1.15 2009/11/09 21:38:56 pust Exp $
 ***************************************************************************/

#include <iostream>
#include <set>
#include <vector>
#include <string>

#include "TreeNode.h"
#include "RuleNode.h"
#include "RuleRHS.h"
#include "hashutils.h"
#include "RuleDescriptor.h"

namespace mtrule {

class State; 

typedef std::vector<State*> States;
typedef States::iterator States_it;
typedef States::const_iterator States_cit;

//! This class is used to represent a search state during rule extraction.
//! Basically, it is just an incomplete rule (no RHS).
class State {

 public:
  int generation_time; 
  static int idd; // number of State objects that have been created.

  //! Structure that defines an operator to compare the size of two 
  //! search states.
  /*! That operator is needed to manage a priority queue during rule 
	* acquisition. To make sure that we visit all states (LHS's) under a give
	* threshold, we need to have states sorted by size in the queue. 
	* This is_smaller needs to guarantee that the minimal rule will be
	* at the beginning of a set.
	*
	* NOTE, very important: the is_smaller assumes that there are no 
	* duplicate states added into the queue (via is_duplicate_state)
	* function. is_smaller should break ties, otherwise, there will
	* be only one element in the tie will be stored in the queue.
	* the ideal feature for tie breaking would be the serialized string
	* of the State, but that will slow down the speed a lot. Since 
	* we have already made the queue no duplicate, we add 'generation_time'
	* into a State, and use this generation_time to break tie. The
	* generation_time must be reset for each new sentence tuple to 
	* guarantee the determinism of different runs.
	*/
  struct is_smaller {
	 bool operator()(const State* s1, const State* s2) const {
	 	if(print_all) {
		  // return (s1->get_nb_expansions() <= s2->get_nb_expansions());
	  	  // We first compare the # expansions (effective, w/ auxiliary
		  // nodes like NPB-BAR) ignored.
		  if(s1->get_nb_expansions() < s2->get_nb_expansions()) { return true; }
	   	  else if(s1->get_nb_expansions() == s2->get_nb_expansions()) {
			// We use the #lexicalized item to order the states in
			// the same bucket (defined by # expansion).
			if(s1->get_nb_lexicalized() < s2->get_nb_lexicalized()){ 
				return true;
			} else if(s1->get_nb_lexicalized() == s2->get_nb_lexicalized()){
				// We use the # ignored expansions to break the bucket
				// of states having the same # expansions and the same
				// # lexicalized items.
				if(s1->get_nb_ignored_expansions() <
						s2->get_nb_ignored_expansions()){ return true;}
				else if(s1->get_nb_ignored_expansions() ==  
										s2->get_nb_ignored_expansions()){
					// the OR node expansions.
					if(s1->get_nb_OR_expansions() <
							s2->get_nb_OR_expansions()){ return true;}
					else if(s1->get_nb_OR_expansions() == s2->get_nb_OR_expansions()){
					   // break the tie using the generation time.
					   // we have already ensured that no duplicate
					   // states will be in the queue.
                       //cout<<s1->generation_time<< ":"<< s2->generation_time<<endl;
					  // return s1 < s2;
					   //return s1->get_lhs_str(s1->get_rootref()) 
						//   < s2->get_lhs_str(s2->get_rootref());
                       return s1->generation_time < s2->generation_time;
					} else { 
						return false; 
					}
				}
			   	else {
					return false;
				}
			}
		   	else { return false; }
		 } else {
			return false;
		 }
		/*
		 * NOTE: the above ordering must guarantee that a state that
		 * is isomorphic smaller must go before a larger node.
		 *
		 * otherwise, there will be duplicate rules generated.
		 */
		} else {
			return (s1->get_nb_expansions()+s1->get_nb_lexicalized() <= 
					  s2->get_nb_expansions()+s2->get_nb_lexicalized());
		}
	 }
  };
  
 /***************************************************************************
  * Constructors and descructors:
  **************************************************************************/
 
  //static std::map<int, bool> __map;

  //! Construct a state from a given TreeNode.
  State(RuleNode* r) : _lhs(), 
    _nb_expansions(0), 
    _nb_ignored_expansions(0), 
    _nb_OR_expansions(0), 
    _nb_nodes(0), 
    _nb_lex(0), 
    _nb_lex_zero(0),
    _is_minimal(false) { 
  _lhs.push_back(r);
  ++nb_of_states; 
	++idd;
	generation_time = idd;

}
  //! Create an empty state (no RuleNode).
  //State() : _lhs(), _nb_expansions(-1), _nb_lex(0) {}
  //! Copy constructor.
  State(const State& s);
  //! Default destructor.
  ~State();

 /***************************************************************************
  * Accessors/mutators:
  **************************************************************************/

  //! Returns a vector containing all nodes of the LHS.
  const RuleNodes& get_lhs()  const { return _lhs; }
  //! Increase number of CFG productions in the rule:
  void inc_nb_expansions()          { ++_nb_expansions; }
  //! Increase number of nodes in the rule:
  void inc_nb_nodes()               { ++_nb_nodes; }
  //! Get number of CFG productions in LHS (i.e. number of internal
  //! nodes, excluding pre-terminals, i.e. POS tags). Not including
  //! the ignored expansions.
  int get_nb_expansions()     const { return _nb_expansions; }

  //! Get the number of auxiliary node expansions.
  int get_nb_ignored_expansions()     const 
  { return _nb_ignored_expansions; }

  //! Get the number of OR node expansions.
  int get_nb_OR_expansions()     const 
  { return _nb_OR_expansions; }

  //! Get number of nodes:
  int get_nb_nodes()          const { return _nb_nodes; }
  //! Increase number of lexicalized items on the frontier of the LHS.
  void inc_nb_lexicalized()         { ++_nb_lex; }
  //! Return number of lexicalized items on the frontier of the LHS.
  int get_nb_lexicalized()    const { return _nb_lex; }

  //! Increase number of lexicalized items on the frontier of the LHS
  // that are not linked to any words on the RHS
  void inc_nb_lexicalized_zero() { ++_nb_lex_zero;} // Daniel
  //! Return number of lexicalized items on the frontier of the LHS
  // that are not linked to any words on the RHS
  int get_nb_lexicalized_zero() const { return _nb_lex_zero;} // Daniel

  //! Return reference to the state's/rule's root.
  RuleNode* get_rootref()     const { return _lhs[0]; }
  //! Returns true if current rule/state is minimal/necessary in its context.
  bool get_is_minimal()       const { return _is_minimal; }
  //! Get rule descriptor.
  RuleDescriptor* get_desc()        { return _desc; }
  //! Assign a descriptor to current rule.
  void set_desc(RuleDescriptor* d)  { _desc = d; }
  //! Set that current rule/state is minimal/necessary in its context
  //! or not.
  void set_is_minimal(bool m)       { _is_minimal = m; }
  //! Return number of State instances allocated in memory (just used 
  //! for debugging).
  static int get_nb_states_in_mem() { return nb_of_states; }
  //! If set to true, print all rules whose sizes are <= threshold.
  static void print_all_rules(bool p) { print_all = p; }
  // Saves in words the lexical items on the target/parse side
  void get_target_words(RuleNode* node, std::string& words); // Daniel
  //! If yes=true, we ignore aux expansion when computing big N.
  static void ignore_aux_expansion(bool yes) 
  { _ignore_aux_expansion = yes; }
  static void erase_aux_nodes_when_print(bool yes) 
  { _erase_aux_nodes_when_print = yes; }

  //! returns true is a category is auxilary.i.e., NPB-BAR.
  static bool is_aux_node(std::string cat);

 /***************************************************************************
  * Rule acquisition functions:
  **************************************************************************/

  //! Determine if a given state is equivalent to another.
  bool is_duplicate_state(State *) const;
  //! Internal function (recursive part of is_duplicate_search_state()).
  static bool _is_duplicate_state(const State *, RuleNode *,
											 const State *, RuleNode *);
  //! Grow search state to include more nodes.
  /*! Assuming that cur is a reference to a leaf node in the current state, 
	* expand that leaf node to include the children that can be read from
	* the tree, e.g. if the tree is VP(V NP(NP PP)) and the rule
	* is VP(V NP), then executing expand_successors() on NP creates a new
	* rule  VP(V NP(NP PP)). */
  void expand_successors(RuleNode *cur);
  //! If 'cur' corresponds to an OR node, expand the OR node to its i-th
  //! AND children. otherwise, i is useless.
  void expand_successors(RuleNode* cur, int i); 
  //! Find among all nodes in the LHS of current State/Rule the 
  //! one that points to the same parsetree node as n.
  /*! If this doesn't exist, return NULL. */
  RuleNode* find_noderef(TreeNode *n) const;

  //! Determine if one LHS is a duplicate of another (for comparing rules).
  //! This is different than is_duplicate_state above, because it looks
  //! at all qualities of the state (the labels, etc.), that are specific
  //! to the LHS of a rule, but not the whole state in the rule search.
  bool is_duplicate_lhs(const State *) const;
  static bool _is_duplicate_lhs(const State *s1, RuleNode* lhs1,
			       const State *s2, RuleNode* lhs2);

 /***************************************************************************
  * Input and output:
  **************************************************************************/

  //! Returns a string representation of the LHS (xRs notation).
  std::string get_lhs_str(RuleNode* root, bool top=true) const;
  //! Internal function (recursive part of get_lhs_str()).
  std::string _get_lhs_str(RuleNode* root, bool top, bool& var_after_lex) const; 

  std::string get_head_tree(RuleNode* rn) const;

  string get_hwt_of_frontiers(RuleNode* rn, bool getWord) const;
  
  RuleNode* get_headnode_recurse(RuleNode* rn) const;
  
  pair<int,bool> 
  get_hw_intersect_frontier_recurse(RuleNode* rn, RuleNode* head, int pos) const;

  string get_hw_intersect_frontier(RuleNode* rn) const;
  
  static bool mark_const_head_in_rule;

 protected:

  //! Recursively set attributes of the LHS:
  bool set_lhs_attr(RuleNode* root, bool top, int& nb_lex,
                    bool& var_after_lex) const;

  
/***************************************************************************
 * Member variables:
 **************************************************************************/

  //! Vector of references to RuleNode objects representing the LHS of a 
  //! (partial) rule.
  RuleNodes _lhs;      
  //! Number of CFG productions in the state/rule (i.e. number of 
  //! internal nodes). If we decide to ignore some nodes (i.e., NPB-BAR)
  //! when counting the N (in big N), _nb_expansions will be the
  //! value after ignoring those nodes.
  int _nb_expansions;

  //! The number of ignored/auxiliary expansions.
  int _nb_ignored_expansions;

  //! Wei: The number of OR node expansions. Used in forest-based rule 
  //! extraction.
  int _nb_OR_expansions;

  //! Number of nodes in state:
  int _nb_nodes;
  //! Number of lexicalized nodes at the frontier of the LHS.
  int _nb_lex;
  //! number of lexicalized nodes at the frontier of the LHS that are
  // not linked to any words on the RHS
  int _nb_lex_zero; // Daniel
  //! Is current rule/state minimal/necessary in its context? 
  bool _is_minimal;
  //! Pointer to an (optional) rule descriptor (which will be stored 
  //! after the '###' in the rule file).
  RuleDescriptor* _desc; 


  bool m_deleted;

/***************************************************************************
 * Static members:
 **************************************************************************/

  static bool print_all;   // print all rules of size <= threshold.
  static int nb_of_states; // number of State objects that have been created.
  //! if set true, then ignore the aux spansion when computing big N.
  static bool _ignore_aux_expansion;
  static bool _erase_aux_nodes_when_print;

};

}

#endif
