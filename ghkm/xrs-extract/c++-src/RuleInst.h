#ifndef __RULEINST_H__
#define __RULEINST_H__

/***************************************************************************
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: RuleInst.h,v 1.7 2009/09/24 23:16:17 pust Exp $
 ***************************************************************************/

#include <iostream>
#include <set>
#include <vector>

#include "State.h"
#include "TreeNode.h"
#include "RuleNode.h"
#include "RuleRHS.h"
#include "hashutils.h"

namespace mtrule {

class RuleInst;

//! A vector of pointers to rule instances.
typedef std::vector<RuleInst*> RuleInsts;
//! Iterator for RuleInst. 
typedef RuleInsts::iterator RuleInsts_it;
//! Constant iterator for RuleInst. 
typedef RuleInsts::const_iterator RuleInsts_cit;

//! This class is used to represent a rule instance as in the "What's in a 
//! translation rule?" paper. 
/*! It contains information specific to the instance of the rule (e.g. 
 *  to nodes of a parse tree), so two instances of the same rule are 
 *  generally not equal.
 */
class RuleInst : public State {

 public:
     bool min;

  //! Create a rule from a given TreeNode object.
  RuleInst(RuleNode* r) : State(r) {
	 _rhs = new RuleRHS();
     min = false;
	 // std::cout << "create rule: " << get_str() << std::endl;
  }
  //! Create a copy of the LHS of rule s, and create
  // an empty RHS.
  RuleInst(const State& s) : State(s) {
	 _rhs = new RuleRHS();
     min = false;
	 // std::cout << "create rule: " << get_str() << std::endl;
	 
  }
  //! Create a copy of rule r.
  RuleInst(const RuleInst& r) : State(r) {
	 _rhs = new RuleRHS(*r._rhs);
     min = false;
	 // std::cout << "create rule: " << get_str() << std::endl;
  }
  //! Create rule instance from string.
  RuleInst(const std::string& str);
  //! Default destructor.
  ~RuleInst() { 
    // std::cout << "      delete rule: " << get_str() << std::endl;
	 delete _rhs;
  }


 /***************************************************************************
  * Accessors/mutators:
  **************************************************************************/

  //! Get the RHS of the current rule.
  RuleRHS* get_rhs() { return _rhs; }

 /***************************************************************************
  * Rule acquisition functions:
  **************************************************************************/

  //! Determine if a given rule is equivalent to another.
  bool is_duplicate_internal_rule(RuleInst *newS) const;
  bool is_duplicate_rule(RuleInst *) const;

  //! Get all valid rules that can be extracted from search state s.
  /*! There is only one rule extracted if there is no unaligned chinese, 
	*  and more than one if such words exist. */
  static void get_rules_from_state(RuleInsts* rs, State *s, Alignment *a, 
                                   int, int, bool);
  // Daniel
  // same as above but only for the top rule
  static void get_rules_from_state_top(RuleInsts* rs, State *s, Alignment *a, 
                                   int, int, bool);


 /***************************************************************************
  * Input and output:
  **************************************************************************/

  //! Return a string representing the rule descriptor:
  std::string get_desc_str(bool short_desc) const {
    assert(_desc != NULL);
    return _desc->get_str(short_desc);
  }

  //! Set attributes of the rule (calls set_lhs_attr, set_rhs_attr).
  void set_attr();
  //! Get a string representation of the rule in xRs format and 
  //! additional information regarding its rule type.
  std::string get_str() const;
  //! Get a string representation of the rule in xRs format
  //! Ignode all Chinese words on the RHS
  std::string get_str_vars();  // Daniel
  std::string get_source_words(); // Daniel
  //! Get # words on target side (ignoring the variables x0, x1, ...)
  int get_number_target_words(); // Daniel
  std::pair<int, int> get_first_last_target_pos(); // Daniel
  //! Set maximum number of foreign unaligned words per search state.
  //! If there is more, all get attached to the highest possible 
  //! constituent in the tree.
  static void set_max_nb_unaligned(int max) { max_nb_unaligned = max; }
  static void set_can_assign_unaligned_c_to_preterm()
  { can_assign_unaligned_c_to_preterm = true; }

  void get_lhs_stuff(RuleNode* root, 
		     std::vector < std::string > & phrases, 
		     std::vector<int>& varIndex, 
		     std::vector< std::pair< int, int > >& boundaries);
  

  void get_lhs_stuff_old(RuleNode* root, 
		     std::vector < std::string > & phrases, 
		     std::vector<int>& varIndex, 
		     std::vector< std::pair< int, int > >& boundaries,
		     bool & is_lexical);
 

 protected:

  //! Set attributes of the RHS : number of elements, number of lex elements.
  bool set_rhs_attr(int&);
  //! Get a string representation of the RHS (xRs format).
  std::string get_rhs_str() const;
  //! Number variables and add them to the RHS.
  /*! DFS traversal of the LHS that assigns numbers to variables in the
	*  order they are seen. */
  static void number_variables(RuleInst*, RuleNode*, Alignment*, int&);
  // Same as above but just for the top rule
  static void number_variables_top(RuleInst*, RuleNode*, Alignment*, int&); // Daniel
  //! Get all rules that can be extracted from a given search state, 
  //! considering all possible ways of assigning unaligned Chinese words
  //! to syntactic constituents. 
  /*! rs is the vector where these rules are to be stored. r is the 
	*  original rule in which all unaligned Chinese words are assumed to 
	*  be assigned to the top constituent, and from which other rules are 
	*  derived (changing the assignment of syntactic constituents). 
	*  Start and end define the span from which rules have to be extracted;
	*  unaligned Chinese words are ignored if their indexes are outside
	*  that span.
   */
  static void handle_unaligned_chinese_words(RuleInsts *rs, RuleInst *r,
                                             int start ,int end);
 
/***************************************************************************
 * Member variables:
 **************************************************************************/

  RuleRHS* _rhs;         //!< RHS of the rule.
  static size_t max_nb_unaligned;
  static bool can_assign_unaligned_c_to_preterm;
 
};

}

#endif
