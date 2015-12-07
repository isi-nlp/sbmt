#ifndef __DERIVATION_H__
#define __DERIVATION_H__

#include <sstream>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>

#include "TreeNode.h"
#include "RuleInst.h"
#include "RuleNode.h"
#include "RuleRHS.h"
#include "RuleDescriptor.h"
#include "RuleSet.h"
#include "DerivationNode.h"
#include "Variable.h"
#include "ATS.h"
#include "db_access.h"

namespace mtrule {

enum{NodeIncludesSpan, SpanIncludesNode, NodeOverlapsSpan, NodeExcludesSpan};

/***************************************************************************
 * $Id: Derivation.h,v 1.9 2007/09/12 01:03:00 wwang Exp $
 ***************************************************************************/

/*! This class encapsulates an algorithm to extract complex syntactic
 * rules governing the reordering of syntactic constituents.
 */
class Derivation {
  
 protected:

  //! Priority queue used to store all states that can be read from 
  //! a given node.
  typedef std::set<State*,RuleInst::is_smaller> PriorityQueue;
  
 public:

  /***************************************************************************
   * Constructors and destructors: 
   **************************************************************************/

  //! Default constructor: sets alignment, mode ('e' for extract, 
  //! 'd' for derivation, 'c' for counts), reference to DB, and reference
  //! to rule set (for counting).
  /*! The two last parameters can safely be set to NULL.
   */
  Derivation(Alignment *a, DB*& dbp, RuleSet *rs = NULL) : 
    _derivationRoot(NULL), _alignment(a), _global_indexing(false), 
    _dbp(dbp), _is_good(true), _rule_set(rs) //,_cross_ent(0.0),_cross_ent_norm(0) 
  {}


  //! Default destructor.
  virtual ~Derivation();

  /***************************************************************************
   * Extraction functions:
   **************************************************************************/
 
  //! Extract a derivation or a set of derivations from the root of
  //! a (sub)tree.
  virtual void extract_derivations(TreeNode *n,int,int);
  
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
  virtual void extract_rules(TreeNode* tn, DerivationNode* dn, int start, int end,
                     bool preterminal);
  // extract wsd like rules by saving them as part of the derivation forest built by ghkm
  void extract_wsd_rules_as_derivations(TreeNode* n, int start, int end, Leaves *lv,
			 std::ostream* hierarchical_out, DB* dbp, int maxPhraseSourceSize, Alignment *a, 
                         bool print_impossible, bool print_lexicalized_composed, 
			 double& vXRS, double& lcvXRS, double& lcXRS, double& lcATS, double& hlcXRS,
			 double& sIMPOSSIBLE, double& tIMPOSSIBLE, double& LC, int lineNumber); 
  // void add_attributes_nonlexicalized_derivation_rules(int lineNumber, double& vXRS);
  // void add_attributes_to_derivation_rules(RuleInst* , Alignment* , int);
  void add_attributes_to_any_rule(RuleInst* , Alignment* , int, 
				  double&, double&, double&, double&, bool first_pass=true);
  void add_attributes_to_any_rule_old(RuleInst* , Alignment* , int, 
				      double&, double&, double&, double&, bool first_pass=true);

  //! Extract AT rules from leaf nodes in lv (AT rule extraction method, 
  //! not GHKM).
  void extract_ATRules(Leaves *lv, ATS *at);

  //! Extract Hierarchical rules (derivations from the root of a tree +
  //! AT-like rules from leaf nodes in lv
  /*
  // Daniel
  void extract_HierarchicalRules(TreeNode *n, int start, int end, Leaves *lv, std::ostream* hierarchical_out, DB* dbp, int maxPhraseSourceSize, Alignment* a, bool print_impossible, bool print_lexicalized_composed, double&, double&, double&, double&, double&, double&, double&, double&, int lineNumber); 
  //! Extract modified hierarchical rules that use pseudo-nonterminals
  // Daniel
  void extract_modified_rules(TreeNode *searchNode, 
			    int pos_s_start, int pos_s_end, 
			    int pos_t_start, int pos_t_end,
			    Leaves *leaves, Alignment *a,
			    int lineNumber, 
			    std::ostream *hierarchical_out, char type, double& count);
  // Daniel
  void extract_modified_rules_SL(TreeNode *searchNode, 
			       int pos_s_start, int pos_s_end, 
			       int pos_t_start, int pos_t_end,
			       Leaves *leaves, Alignment *a,
			       int lineNumber, 
			       std::ostream *hierarchical_out, char type, double& count);
  */
  //! Set global indexing to true: each rule has its global index.
  void set_global_indexing() { _global_indexing = true; }
  //! Set various limits on number of rules and rule shapes.
  static void set_limits(int max_rule_expansions,
                         int max_to_extract) {
    _max_rule_expansions = max_rule_expansions;
    _max_to_extract      = max_to_extract;
  }

  static void set_max_rules_per_node (int i) {max_rules_per_node = i;}

  /***************************************************************************
   * I/O functions:
   **************************************************************************/
  
  //! Print all rules used in derivation. The second argument
  //! determines if rule descriptors should be printed too.
  virtual void print_derivation_rules(std::ostream& out,bool=true,bool=false) const;
  //! Print all non-lexicalized (on the target side/English) rules used  
  //! in derivation. The second argument
  //! determines if rule descriptors should be printed too.
  void print_nonlexicalized_derivation_rules(std::ostream& out, int, double&, bool) const; // Daniel
  //! Print dforest to output stream 'out'.
  void print_derivation_forest(std::ostream& out,
                               int derivation_index=-1) const;
  //! Returns a string representing the derivation forest.
  std::string get_derivation_string(DerivationNode *,
                                    DerivationNodeIndex *, 
                                    int&, bool) const;



  /***************************************************************************
   * Functions to extract Viterbi derivations:
   **************************************************************************/
  
  //! Returns true if a probability file was loaded and the Viterbi
  //! derivation has probability more than 0 (no unseen rule).
  bool non_zero_prob() const { return _derivationRoot->get_logp() > -DBL_MAX; }
  //! Viterbi 'decoding' to find the most probable derivation in the derivation
  //! forest. 
  void one_best(DerivationNode *dn=NULL);
  //! Everything in the forest is deleted beside the best-scoring derivation
  //! (this assumes that one_best() has been run before).
  void keep_only_one_best(DerivationNode *dn=NULL);
  void compute_cross_entropy(DerivationNode *dn);

  /***************************************************************************
   * Misc functions:
   **************************************************************************/

  //! Return true if the given rule is present in the rule set of 
  //! the current sentence.
  bool has_rule(const std::string& r, bool lhs_only=false) const;
  //! Collect counts of rules used in derivation forest.
  void add_derivation_counts() const;
  //! Get global rule indices (fill _derivationRuleIDs).
  void retrieve_global_rule_indices();
  //! Returns true if the derivation was successful (it should
  //! always be successful, unless FAIL_IF_TOO_BIG is set to true).
  bool is_good() const { return _is_good; }
  //! Activate verbose mode.
  static void set_verbose(bool v=true) { _verbose=v; }
  //! Do not search all possible ways of attaching foreign words to 
  //! tree constituents.
  static void set_no_multi_attach(bool n) { _no_multi = n; }


  /***************************************************************************
   * Old functions (ignore):
   **************************************************************************/

  //! Add rules to DB (deprecated: all accesses to the DB in 
  //! extract are now read-only)
  void add_rules_to_db() const;

 protected:

  /***************************************************************************
   * Frontier search functions:
   **************************************************************************/
 
  //! Determine if a state 'newS' is already present in the queue.
  bool is_duplicate(RuleInst *newS) const;
  //! Add initial search state to the priority queue (LHS of all 'minimal
  //! rules' that can be extracted from current node).
  virtual State* get_smallest_admissible_state(TreeNode *n);
  //! Expand the frontier of curState until it corresponds to an admissible
  //! frontier, i.e. Chinese spans must be contiguous and non-overlapping.
  virtual bool expand_state_to_admissible_frontier(State *curState, TreeNode *root);
  //! This function can be used to lexicalize a given span  
  //! (source-language and target-language) in a given rule. 
  /*! Note that there is no guarantee that the modified rule will be 
   *  admissible at the end of the expansion of the frontier:
   *  This is used by the ATS rule extractor, who needs to make sure
   *  that the the ephrase ([pos_s_start:pos_s_end]) and cphrase 
   *  are fully lexicalized (i.e. the phrases must appear in the rule).
   */
  bool lexicalize_spans_in_rule(RuleInst *curRule,
                                int pos_s_start, int pos_s_end, 
                                int pos_t_start, int pos_t_end);
  //! Add successors states (of current state) to the priority queue. 
  //! The set of successor states of state S is defined as the composition 
  //! of S with a minimal rule (the particular etree/cstring/a tells you if
  //! a given rule is minimal or not).
  /*! Example: a possible successor state of A(x0:B x1:C) is A(B(x0:D) x1:C) */
  virtual bool add_successor_states_to_queue(State*,PriorityQueue*,TreeNode*,int,int);
  //! (Pretty-)print information about the search state (LHS).
  void dump_all_states(std::ostream& os, PriorityQueue *pq) const;
  
  /***************************************************************************
   * Member variables:
   **************************************************************************/

  //! Root of a packed derivation forest.
  DerivationNode* _derivationRoot;
  //! Pointer to the alignment between the e-tree and c-string.
  Alignment* _alignment;
  //! Set this to true if you want to use a global indexing scheme:
  bool _global_indexing;
  // Handle to an open Berkeley database from while rules can be
  // read and to which rules have to be written:
  mutable DB* _dbp;
  //! Set to true when the derivation isn't failed (a derivation 
  //! can be failed e.g. if there is a necessary rule that is bigger 
  //! that a given threshold on size):
  bool _is_good;
  //! Reference to a RuleSet used to store rule counts.
  RuleSet* _rule_set;
  //! Map TreeNodes to Derivation nodes.
  /*! This hashmap is used to determine, once we reach a given TreeNode 
   * object, whether a DerivationNode object has already been created
   * for that TreeNode (and for a specified span). This situation happens
   * in cases whether the packed derivation forest is a graph (not a mere 
   * tree), i.e. a DerivationNode object can have more than one parent
   * in the derivation. */
  TreeNodeToDerivationNode _treeNodeToDerivationNode;
  //! Set (vector) of rules created or used to match an etree/cstring pair. 
  /*! The index in that vector works as a local index. 
   *  Functions in this class provide facilities to get global indexes of 
   *  all rules in the set and print rules (both internal and external 
   *  representation). */
 public:
  RuleInsts _derivationRules;
  //! Table that maps local indices to global ones.
  /*! Note: local indices are the ones in _derivationRules, i.e. once
   *  _derivationRuleIDs has been filled, the n-th element of 
   *  _derivationRules correspond to the n-th element of _derivationRuleIDs.
   */
  std::vector<int> _derivationRuleIDs;
  //! The following vector is used to keep track which rules are used in 
  //! the derivation forest (when keep_only_one_best() is not used, the
  //! matter is simple: all rules in _derivationRules are used; however, 
  //! when that function is called, we need to keep track which rules are
  //! still used).
  std::vector<bool> _rules_in_use;
  //! Maximum number of rule expansions:
  static int _max_rule_expansions, _max_to_extract;
  //! If this is set to true, Derivation prints various debug messages.
  static bool _verbose;
  //! If true, do not search all possible ways of assigning foreign words
  //! in the tree.
  static bool _no_multi;
  static int max_rules_per_node;

  // Wei Wang.
  Derivation() {}
};

}

#endif
