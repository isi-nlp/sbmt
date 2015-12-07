#ifndef _GEN_DER_H_
#define _GEN_DER_H_

#include <iostream>
#include <fstream>
#include <set>
#include <vector>

#include "Derivation.h"

using namespace std;

namespace mtrule {

/***************************************************************************
 * $Id: GenDer.h,v 1.2 2006/12/01 19:51:54 wwang Exp $
 ***************************************************************************/

/*! Derivation genralized to deal with e-forst, which is composed of AND and
 * OR node.  I didnt directly modify the Derivation.[h,C] to avoid mess up
 * the original code.
 *
 * Identical rules from the same node will be counted once --- differnet
 * from the base class. 
 *
 * TOP rules are also printed out, so that multi derivation at the TOP 
 * can have a single derivation node as the root of the derivation forest!!
 */
class GenDer : public Derivation {

	typedef Derivation _base;
  
 protected:

  //! Priority queue used to store all states that can be read from 
  //! a given node.
  typedef _base::PriorityQueue PriorityQueue;
  
 public:

  /***************************************************************************
   * Constructors and destructors: 
   **************************************************************************/

  //! Default constructor: sets alignment, mode ('e' for extract, 
  //! 'd' for derivation, 'c' for counts), reference to DB, and reference
  //! to rule set (for counting).
  /*! The two last parameters can safely be set to NULL.
   */
  GenDer(Alignment *a, DB*& dbp, RuleSet *rs = NULL) : _base(a, dbp, rs)
  {}

  //! Default destructor.
  ~GenDer();

  /***************************************************************************
   * Extraction functions:
   **************************************************************************/
 
  //! Extract a derivation or a set of derivations from the root of
  //! a (sub)tree.
  void extract_derivations(TreeNode *n,int,int);
  
  //! Find rule(s) that respect the given word alignment, inspecting all 
  //! tree fragments rooted at tn.
  /*! Graph fragments are maintained on a priority queue 
   * that lists first the smallest graphs (smallest in terms of number
   *  number of internal nodes.)
   * If the preterminal flag is set to true, search for a preterminal 
   * rule (e.g. DT(these) -> c1), then leave instead of recursing (this
   * is needed in order to avoid infinite loops). If the global_index
   * flag is set to true, global indexes of the rules are used instead
   * of local indexes (function runs a bit faster).
   */
  void extract_rules(TreeNode* tn, DerivationNode* dn, int start, int end,
                     bool preterminal);

  void
  print_derivation_rules(std::ostream& out, bool print_desc, bool min_desc) const;
 protected:

  /***************************************************************************
   * Frontier search functions:
   **************************************************************************/
 
  //! Generalized version to e-forest.
  void get_smallest_admissible_state(TreeNode *n, vector<State*>& states);

  //! Expand the frontier of curState until it corresponds to an admissible
  //! frontier, i.e. Chinese spans must be contiguous and non-overlapping.
  void	expand_state_to_admissible_frontier(State *curState, 
		                            TreeNode *root,
									std::vector<State*>& states); 

  bool add_successor_states_to_queue(State* curState, 
		                      PriorityQueue* pq, 
							  TreeNode *root, int start, int end);


};

}

#endif
