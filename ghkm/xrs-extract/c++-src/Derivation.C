#include <algorithm>
#include <sstream>
#include <cfloat>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>

#include "Derivation.h"
#include "MyErr.h"
#include "db_access.h"

namespace mtrule {

/***************************************************************************
 * Initialization of static members:
 **************************************************************************/

int Derivation::_max_rule_expansions = MAX_RULE_EXPANSIONS;
int Derivation::_max_to_extract      = MAX_TO_EXTRACT;
bool Derivation::_verbose = false;
bool Derivation::_no_multi = false;
int Derivation::max_rules_per_node = -1;

/***************************************************************************
 * Destructor: make sure that all derivation nodes are deleted:
 **************************************************************************/

Derivation::~Derivation() {


  // Delete all derivation nodes and links to other derivation nodes:
  for(TreeNodeToDerivationNode_it it = _treeNodeToDerivationNode.begin(),
          it_end = _treeNodeToDerivationNode.end(); it != it_end; ++it) {
    DerivationNode *dn = it->second;
    if(dn != NULL) {
      DerivationOrNode& children = dn->get_children();
      for(DerivationOrNode_it it=children.begin(), it_end=children.end();
          it != it_end; ++it){
        // std::cout << "DEBUG: free node " << it->first << std::endl;
        delete it->second;
      }
      delete dn;
    }
  }

  // Cleanup rules used in derivation:

  for(int i=0, size=_derivationRules.size(); i<size; ++i) {
    delete _derivationRules[i]->get_desc();
    delete _derivationRules[i];
  }
  _derivationRules.clear();

}

/***************************************************************************
 * Perplexity/probability functions:
 **************************************************************************/

void
Derivation::one_best(DerivationNode *dn) {
  // If run without argument, assume dn is the root of the derivation forest.
  if(dn == NULL)
    dn = _derivationRoot;
  // If logp of dn is not default value (-DBL_MAX), we can leave (since the
  // most probable derivation has already been computed for the derivation
  // below dn):
  if(dn->get_logp() > -DBL_MAX || dn->has_logp())
    return;
  // Get all children of all rules just below current OR-nodes:
  DerivationOrNode& children = dn->get_children();
  // If we reached a leaf node, set logp to 0 and leave:
  if(children.empty()) {
    dn->set_logp(0.0);
    dn->set_backptr(-1);
    return;
  }
  // Run one_best recursively, and extract most probable branch under OR
  // node:
  double _best_p = -DBL_MAX;
  int _best_dn = -1;
  for(DerivationOrNode_it it=children.begin(), it_end=children.end();
      it != it_end; ++it) {
    // We now process a new branch under the OR node:
    assert(it->first < static_cast<int>(_derivationRuleIDs.size()));
    int localRuleID = it->first;
    int ruleID = _derivationRuleIDs[localRuleID]; // rule R under the OR node
    DerivationNodes* c = it->second; // DerivationNodes reachable through R
    assert(ruleID < _rule_set->get_size());
    double cur_p = _rule_set->get_logprob(ruleID);
    //*myerr << dn << "(" << c << "): ";
    //*myerr << "curp=" << cur_p;
    //*myerr << " ruleID=" << ruleID << std::endl;
    for(size_t i=0; i<c->size(); ++i) {
      one_best((*c)[i]);
      cur_p += (*c)[i]->get_logp();
      //*myerr << dn << "(" << c << "): ";
      //*myerr << "child-curp=" << (*c)[i]->get_logp() << std::endl;
    }
    //*myerr << dn << "(" << c << "): ";
    //*myerr << "cur vs best: " << cur_p << " " << _best_p << std::endl;
    if(cur_p > _best_p || -DBL_MAX >= _best_p) {
      _best_p = cur_p;
      _best_dn = it->first;
    }
  }
  dn->set_logp(_best_p);
  dn->set_backptr(_best_dn);
  if(_best_dn == -1) {
    *myerr << "SIZE: " << children.size() << std::endl;
    assert(false);
  }
}

void
Derivation::keep_only_one_best(DerivationNode *dn) {
  // If run without argument, assume dn is the root of the derivation forest.
  if(dn == NULL) {
    dn = _derivationRoot;
    // _rules_in_use is used to keep track of which rules are used in
    // the most probable derivation:
    _rules_in_use.resize(_derivationRules.size());
  }
  // Get all children of all rules just below current OR-nodes:
  DerivationOrNode& children = dn->get_children();
  if(children.size() == 0)
    return;
  // Delete everything beside the most probable derivation:
  int count_not_del = 0;
  int first = -1;
  DerivationNodes* second = NULL;
  for(DerivationOrNode_it it=children.begin(), it_end=children.end();
      it != it_end; ++it) {
    int localRuleID = it->first;     // a rule (R) used in the best derivation
    DerivationNodes* c = it->second; // DerivationNodes reachable through R
    if(localRuleID != dn->get_backptr()) {
      delete it->second;
      it->second = NULL;
    } else {
      _rules_in_use[localRuleID] = true;
      for(size_t i=0; i<c->size(); ++i) {
        keep_only_one_best((*c)[i]);
        (*c)[i]->set_nb_parents(1);
      }
      first = localRuleID;
      second = c;
      ++count_not_del;
    }
  }
  children.clear();
  assert(count_not_del == 1);
  children.insert(std::make_pair(first,second));
}

/***************************************************************************
 * Search functions:
 **************************************************************************/

/////////////////////////////////////////////////////////////////////////
// Search all complex rules that can be collected in a given parse
// (sub-)tree: (n is the root of the parse (sub-)tree)
void
Derivation::extract_derivations(TreeNode* tn, int start, int end) {

  if(_verbose) {
    *myerr << "%%% extracting derivation from: " << tn->get_cat() << "";
    *myerr << "[" << start << ":" << end << "]\n";
  }

  // Remove unary rules at the top:
  if( tn->get_cat() == "ROOT"  ||
      tn->get_cat() == "TOP" ) {
    if(tn->get_nb_subtrees() > 1) {
      // can't deal with this case; go to next parse:
      *myerr << "%%% STOPPED: can't deal with parse trees that "
             << "have multiple roots" << std::endl;
      _is_good = false;
      return;
    }
    for(int i=0;i<tn->get_nb_subtrees();++i)
      extract_derivations(tn->get_subtree(i),start,end);
    return;
  }

  // Test whether or not any rule can be extracted from the current node:
  if(tn->is_extraction_node()) {

    // 'dn' corresponds to the current derivation node:
    DerivationNode* dn = NULL;

    // Create a descriptor for DerivationNode dn
    // (to find it once we will need to link a parent DerivationNode to it)
    DerivationNodeDescriptor desc(tn,false,start,end);
    TreeNodeToDerivationNode_it it = _treeNodeToDerivationNode.find(desc);

    // Get derivation node corresponding to 'desc':
    if(it != _treeNodeToDerivationNode.end())
      dn = it->second;

    // Create root derivation node if 'tn' is the root:
    if(tn->is_root()) {
      dn = _derivationRoot = new DerivationNode(tn);
      _treeNodeToDerivationNode.insert(std::make_pair(desc,dn));
    }

    // Check whether DerivationNode exists. If not, return:
    if(dn == NULL)
      return;

    // Test if the tree node is a leaf of the tree; if so, then
    // perform one last search (to extract a pre-terminal rule) and leave:
    bool is_leaf = !tn->is_internal();

    // Extract rules from a given TreeNode 'tn'. Link these rules to
    // derivation node 'dn'. Rules are extracted for the span [start:end].
    // 'is_leaf' prevents us from looping:
    extract_rules(tn, dn, start, end, is_leaf);
  } // tn->is_extraction_node()
  else {
    // Make sure this code is only reachable with leave nodes:
    assert(tn->get_nb_subtrees() == 0);
  }

}

/////////////////////////////////////////////////////////////////////////
// Search for rule(s) among all tree fragments rooted at tn.
// All explored graph fragments are maintained on a priority queue
// that lists first the graph fragments that are the smallest in
// terms of number of nodes.
// If the preterminal flag is set to true, search for a preterminal
// rule (e.g. DT(these) -> c1), then leave instead of recursing (this
// is needed in order to avoid infinite loops).
void
Derivation::extract_rules(TreeNode *tn, DerivationNode *dn,
                          int start, int end, bool preterminal) {

  static int nummin = 0;
  // Since the derivation forest is
  // a graph, we have to make sure we don't follow the same path twice:
  assert(dn != NULL);
  if(dn->is_done())
    return;

  if(_verbose) {
    *myerr << "%%% extracting rules from: " << tn->get_cat() << "";
    *myerr << "[" << start << ":" << end << "]\n";
  }

  /////////////////////////////////////////////////////////////////////////
  // Rule extraction : step 1 : create minimal rule(s)
  /////////////////////////////////////////////////////////////////////////

  State *curState;

  // LHS of the smallest admissible rule(s) (so-called 'necessary' rules)
  // Note: there can be different RHS's (because of unaligned Chinese words),
  // but the LHS is unique.
  State *necessaryState;
  // Priority queues (openQueue and closedQueue are
  // sets of rules ordered by rule size).
  // openQueue contains all states whose successors still need to be
  // constructed (e.g. given a parse tree (A(B(C D) E(F G))), the only
  // successors of state A(B E) are A(B(C D) E) and A(B E(F G)).
  // closedQueue contains all states whose successors are already
  // in openQueue (or _closequeue).
  PriorityQueue openQueue, closedQueue;

  // Insert initial state/rule into priority queue (using the
  // TreeNode that is provided), then extend the frontier until we
  // reach an admissible state/rule (a.k.a. "necessary rule"):
  necessaryState = get_smallest_admissible_state(tn);
  if(FAIL_IF_TOO_BIG) {
    if(necessaryState->get_nb_expansions() > _max_rule_expansions) {
      delete necessaryState;
      _is_good = false;
      return;
    }
  }
  openQueue.insert(necessaryState);

  /////////////////////////////////////////////////////////////////////////
  // Rule extraction : step 2 : starting for the necessary rule,
  // extract one or more rules, until we reach the threshold
  // that specifies the maximum rule size, or maximum number of rules to
  // extract from a given TreeNode.
  /////////////////////////////////////////////////////////////////////////

  // Main search loop: as long as there are elements on the priority queue,
  // select the first element (a RuleInst) and create more rules from it:
  while(openQueue.size() > 0) {

    // Get rule at the beginning of the open list:
    PriorityQueue::iterator it = openQueue.begin();
    curState = *it;

    // Check the size (nb of nodes) of the first (i.e. smallest) tree fragment.
    // If it is bigger than a given threshold, then stop and move on:
    int cur_size = closedQueue.size() + openQueue.size();
    if(cur_size >= _max_to_extract ||
       curState->get_nb_expansions() > _max_rule_expansions)
      break;

    // For each possible expansion of any leaf node of the current RuleInst,
    // build a new RuleInst and expand its frontier to the smallest admissible
    // frontier:
    // (current tree = first element of openQueue, i.e. openQueue.begin();
    // store new RuleInsts into openQueue)
    add_successor_states_to_queue(curState,&openQueue,tn,start,end);
    // Pop out the first element of openQueue, since we don't need it anymore:
    // (note: only the reference to that RuleInst is deleted, not the RuleInst
    // itself)
    openQueue.erase(it);
    closedQueue.insert(curState);

    // Check progress:
    if(_verbose && closedQueue.size() % 100 == 0)
      *myerr << "%%% LHSs in"
             << " closed-q: " << closedQueue.size()
             << " open-q: " << openQueue.size() << "\n";
  }

  // Move the minimal rule in openQueue (if it is still there) to
  // closedQueue and delete all other rules (in openQueue):
  for(PriorityQueue::iterator it = openQueue.begin(),
          it_end = openQueue.end(); it != it_end; ++it) {
    if(necessaryState == *it) {
      closedQueue.insert(*it);
    }
    else  {
      delete *it;
    }
  }
  openQueue.clear();

  /////////////////////////////////////////////////////////////////////////
  // Rule extraction : step 3 : create a set of rules from a set of states:
  /////////////////////////////////////////////////////////////////////////

  std::vector<RuleInst*> extractedRules, necessaryRules;
  for(PriorityQueue::iterator it = closedQueue.begin(),
          it_end = closedQueue.end(); it != it_end; ++it) {

    /*
     * If the rule is not a necessary rule, and the number
     * of rules extracted from one node exceeds a max limit,
     * we dont extract the rules any more.
     * Added by Wei Wang on  Sat Dec  2 13:30:27 PST 2006
     */
    if(*it != necessaryState &&
       max_rules_per_node > (int)0 && extractedRules.size() > (size_t)max_rules_per_node) {
      if(_verbose) {
        *myerr << "the number of extracted rules exceed limit "<<max_rules_per_node<<std::endl;
      }
      continue;
    }

    //cerr<<"rules extracted" <<extractedRules.size()<<endl;
    //cerr<<"SIZE OF RULE EXTRACTED: "<<extractedRules.size()<<endl;
    State *curState = *it;
    RuleInsts *rs = new RuleInsts;
    RuleInst::get_rules_from_state
        (rs, curState, _alignment, start, end, _no_multi);
    for(int i=0, size=rs->size(); i<size; ++i) {
      RuleInst* r = (*rs)[i];


      //if(it == closedQueue.begin()) {
      if(*it == necessaryState){
        // Set the is_minimal flag to all these rules.
        r->min = true;
        //r->get_desc()->set_str_attribute("min", "1");
        ++nummin;
        necessaryRules.push_back(r);
      } else {
        r->min = false;
        //r->get_desc()->set_str_attribute("min", "0");
      }
      extractedRules.push_back(r);
    }
    delete rs;
  }
  assert(extractedRules.size() > 0);
  double count = 1/(double)extractedRules.size();

  // Delete all states in closedQueue:
  for(PriorityQueue::iterator it = closedQueue.begin(),
          it_end = closedQueue.end(); it != it_end; ++it) {
    delete *it;
  }
  closedQueue.clear();

  /////////////////////////////////////////////////////////////////////////
  // Rule extraction : step 4 : once we have a set of rules, recurse through
  // the tree to find rules at other nodes:
  /////////////////////////////////////////////////////////////////////////

  // Recurse in the tree to find more rules:
  // Collect all rules and put them in _derivationRules;
  // create links between DerivationNode objects
  std::vector<RuleInst*> deletequeue;
  for(int i=0, size=extractedRules.size(); i<size; ++i) {
    RuleInst *curRule = extractedRules[i];
    int curRuleID = _derivationRules.size();
    bool skip = false;
    // Skip rule if it is already in _derivationRule:
    // Uncomment this code if you want a rule appearing twice
    // in a derivation (but with different spans for variables)
    // have a unique identifier:
    // ------ CUT HERE
    //  if(!_global_indexing)
    //    for(int j=0, size2=_derivationRules.size(); j<size2; ++j) {
    //     if(_derivationRules[j]->is_duplicate_rule(curRule)) {
    //      RuleDescriptor* d = _derivationRules[j]->get_desc();
    //      d->set_double_attribute
    //        ("count", d->get_double_attribute("count")+1.0);
    //      d->set_double_attribute
    //        ("fraccount",d->get_double_attribute("fraccount")+count);
    //      skip = true;
    //      curRuleID = j;
    //      break;
    //     }
    //    }
    // ------ CUT HERE
    // For each RuleInst, go through all RuleNodes, and identify those
    // that are at the frontier. Get DerivationNodes at that frontier,
    // and link current derivation node (dn) to successors:
    DerivationNodes *dns = new DerivationNodes();
    // Process rhs to find rulenodes to expand (NEW METHOD):
    for(int i=0, size = curRule->get_rhs()->get_size(); i<size; ++i) {
      const RuleRHS_el* rhs_el = curRule->get_rhs()->get_element(i);
      int ruleNodeIndex = rhs_el->get_rulenode_index();
      RuleNode *ruleNode = NULL;
      if(ruleNodeIndex >= 0)
        ruleNode = curRule->get_lhs()[ruleNodeIndex];
      // Go to next node if the current one if the current one
      // does not exist:
      if(ruleNode == NULL)
        continue;
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
        derNode = new DerivationNode(treeNode);
        _treeNodeToDerivationNode.insert
            (std::make_pair(desc,derNode));
      }
      dns->push_back(derNode);
      derNode->inc_nb_parents();
    }
    // Make sure that the pair <curRuleID,dns> is not already
    // present in dn:
    bool dup = false;
    std::pair<DerivationOrNode_cit, DerivationOrNode_cit> p
        = dn->get_children().equal_range(curRuleID);
    assert(dn->get_children().count(curRuleID) <= 1);
    for(DerivationOrNode_cit it = p.first, it_end = p.second;
        it != it_end; ++it) {
      DerivationNodes* dns2 = it->second;
      if(dns->size() != dns2->size())
        continue;
      for(int i=0, size=dns->size(); i<size; ++i) {
        if((*dns)[i] != (*dns2)[i])
          continue;
      }
      break;
    }
    // Now that we have the target derivation nodes, create a new
    // link in the derivation (i.e. link dn to dns):
    if(!dup)
      dn->add_child(curRuleID,dns);
    if(!skip) {
      RuleDescriptor *d = new RuleDescriptor;
      d->set_str_attribute("algo","GHKM");
      d->set_double_attribute("count",1.0);
      d->set_double_attribute("fraccount",count);
      assert(curRuleID == static_cast<int>(_derivationRules.size()));
      curRule->set_desc(d);
      curRule->set_attr();
      _derivationRules.push_back(curRule);
    } else
      deletequeue.push_back(curRule);
  }
  // Recursive search on all nodes in the RHS:
  for(int i=0, size=necessaryRules.size(); i<size; ++i) {
    RuleInst *curRule = necessaryRules[i];
    for(int eli = 0, elsize = curRule->get_rhs()->get_size();
        eli < elsize; ++eli) {
      const RuleRHS_el* el = curRule->get_rhs()->get_element(eli);
      int index = el->get_rulenode_index();
      // Make sure curNode exists:
      if(index < 0)
        continue;
      const RuleNode *curNode = curRule->get_lhs()[index];
      const span& sp = el->get_span();
      // If not, then recurse from newRoot:
      TreeNode* newRoot = curNode->get_treenode_ref();
      if(newRoot->is_internal() || !preterminal) {
        extract_derivations(newRoot,sp.first,sp.second);
      }
    }
  }
  // Delete duplicate rules:
  for(int i=0, size=deletequeue.size(); i<size; ++i)
    delete deletequeue[i];
  // "Remember" that dn has already been processed:
  dn->set_done();
  //std::cerr << "nummin=" << nummin << '\n';
}

State*
Derivation::get_smallest_admissible_state(TreeNode *n) {
  RuleNode* ref = new RuleNode(n,0);
  State *initState = new State(ref);
  expand_state_to_admissible_frontier(initState, n);
  initState->set_is_minimal(true);
  return initState;
}

// We need 'root', since it must be expanded even if not necessary:
bool
Derivation::expand_state_to_admissible_frontier(State *curState,
                                                TreeNode *root) {

  // Loop until the frontier defining a necessary rule can't be
  // expanded anymore:
  bool modified_frontier=true;
  while(modified_frontier) {

    // If 'modified_frontier' remains false, it stops looping:
    modified_frontier=false;

    // For each node in the current state:
    for(int i=0, size=curState->get_lhs().size(); i<size; ++i) {

      // Get reference to the current rule node:
      RuleNode *newNode = curState->get_lhs()[i];
      TreeNode *treenode_ref = newNode->get_treenode_ref();

      // Ignore internal nodes:
      if(newNode->is_internal())
        continue;

      // Expand the node only if it is necessary, i.e. it isn't necessary
      // to expand the node if there is nothing to expand in the tree
      // (unless treenode_ref is the root where the search started from,
      // in which case we need to create a rule that expands to at least
      // one level, to avoid sth like: x1:NP -> x1, a rule which is basically
      // doing nothing:
      if(treenode_ref->is_frontier_node() &&
         (treenode_ref != root))
        continue;

      // Check whether it is an internal node in the tree (we already
      // know that it isn't an internal node in the rule):
      if(treenode_ref->is_internal()) {

        // Case 1: yes, it is an internal node; add the successors
        // of newNode to curRule:
        curState->expand_successors(newNode);
        modified_frontier=true;

      } else {

        // Case 2: the current node is a leaf in the tree:

        if(newNode->is_lexicalized())
          continue;

        // Expand the node only if it is necessary, i.e. it isn't necessary
        // to expand the node if there is nothing to expand in the tree
        // (unless treenode_ref is the root where the search started from,
        // in which case we have to do something, e.g. get "DT(this) -> c1")
        newNode->set_lexicalized(true);
        curState->inc_nb_lexicalized();
        //curState->inc_nb_expansions();
        newNode->set_word(treenode_ref->get_word());
        modified_frontier=true;

      }
    }
  }
  return modified_frontier;
}

// This function can be used to lexicalize a given span
// (source-language and target-language) in a given rule.
// Note that there is no guarantee that the modified rule will be
// admissible at the end of the expansion of the frontier:
bool
Derivation::lexicalize_spans_in_rule(RuleInst *curRule,
                                     int pos_s_start, int pos_s_end,
                                     int pos_t_start, int pos_t_end) {

  // Loop until the frontier properly covers the span of source-
  // language and target-language words:
  bool modified_frontier=true;
  while(modified_frontier) {

    // If 'modified_frontier' remains false, it stops looping:
    modified_frontier=false;
    // 'newRule' might not go as deep as needed, and might not lexicalize
    // each element of the AT. Expand the frontier if needed:
    //for(RuleNodes_cit cit = curRule->get_lhs().begin(),
    //  cit_end = curRule->get_lhs().end(); cit != cit_end; ++cit) {
    for(int i=0, size=curRule->get_lhs().size(); i<size; ++i) {

      // Get reference to the current rule node:
      //RuleNode *newNode = *cit;
      RuleNode *newNode = curRule->get_lhs()[i];
      TreeNode *treenode_ref = newNode->get_treenode_ref();

      // Ignore internal nodes:
      if(newNode->is_internal())
        continue;

      // If current node has its English and Chinese spans outside
      // lexicalization boundaries, continue:
      if((treenode_ref->get_end()      < pos_s_start ||
          treenode_ref->get_start()    > pos_s_end)  &&
         (treenode_ref->get_nb_c_spans() == 0 ||
          treenode_ref->get_c_end(treenode_ref->get_nb_c_spans()-1)
          < pos_t_start ||
          treenode_ref->get_c_start(0) > pos_t_end))
        continue;

      // Check whether it is an internal node in the tree (we already
      // know that it isn't an internal node in the rule):
      if(treenode_ref->is_internal()) {

        // Case 1: yes, it is an internal node; add the successors
        // of newNode to curRule:
        curRule->expand_successors(newNode);
        modified_frontier=true;

      } else {

        // Case 2: the current node is a leaf in the tree:

        if(newNode->is_lexicalized())
          continue;

        // Expand the node only if it is necessary, i.e. it isn't necessary
        // to expand the node if there is nothing to expand in the tree
        // (unless treenode_ref is the root where the search started from,
        // in which case we have to do something, e.g. get "DT(this) -> c1")
        newNode->set_lexicalized(true);
        curRule->inc_nb_lexicalized();
        newNode->set_word(treenode_ref->get_word());
        modified_frontier=true;

      }
    }
  }
  return modified_frontier;
}

/*************************************************************************
 * Take first element of the queue (curRule); for each leaf node of
 * curNode that can be expanded with a 1-level (i.e. CFG) expansion in the
 * tree, create a new rule (newRule), a copy of curRule that incorporates
 * that expansion.
 *************************************************************************/
bool
Derivation::add_successor_states_to_queue(State* curState, PriorityQueue* pq,
                                          TreeNode *root, int start, int end) {

  // If any new state/rule is added, return true. Otherwise, false:
  bool added_new_states = false;

  // For each node in the current state, determine if it is a leaf,
  // and if it is an internal node in the corresponding Tree:
  for(RuleNodes_cit cit = curState->get_lhs().begin(),
          cit_end = curState->get_lhs().end(); cit != cit_end; ++cit) {
    // Current node of the lhs of curRule:
    const RuleNode *curNode = *cit;
    // Ignore internal nodes of the rule:
    if(curNode->is_internal())
      continue;
    // Expand the node only if it is necessary, i.e. it is possible
    // to add more stuff (CFG rule or lexicalization) to the rule from
    // the tree:
    if(curNode->get_treenode_ref()->is_frontier_node() &&
       (curNode->get_treenode_ref() != root) &&
       curNode->is_lexicalized())
      continue;
    // Conditions are met to create a new rule:
    State *newState = NULL;
    // Clone current state/rule:
    newState = new State(*curState);
    newState->set_is_minimal(false);
    // Find the reference (in the rule rule) to the node to expand:
    RuleNode *expansionNode
        = newState->find_noderef(curNode->get_treenode_ref());
    if(curNode->get_treenode_ref()->is_internal()) {
      // Case 1: the current node is an internal node of the parse tree:
      newState->expand_successors(expansionNode);
    } else {
      // Case 2: the current node in the rule is a leaf that is not
      // lexicalized; lexicalize it:
      expansionNode->set_lexicalized(true);
      newState->inc_nb_lexicalized();
      if(curNode->get_treenode_ref()->get_nb_c_spans() == 0) // Daniel
        newState->inc_nb_lexicalized_zero(); // Daniel
    }
    // Now that we have modified newRule, we need to make sure that
    // the frontier is admissible:
    expand_state_to_admissible_frontier(newState, root);

    // Make sure that the state isn't already present in the queue:
    bool duplicate = false;
    for(PriorityQueue::iterator it2 = pq->begin();
        it2 != pq->end(); ++it2) {
      State *elState = *it2;
      if(elState->is_duplicate_state(newState)) {
        duplicate = true;
        break;
      }
    }
    // Add newly created state to queue (if it is not a duplicate):
    if(!duplicate) {
      pq->insert(newState);
    }
    else {
      delete newState;
    }
  }

  // Might want to stop if the extraction of new rules didn't succeed:
  return added_new_states;
}


// Daniel begin
/******************************************************************
 * Subroutines to extract Hierarchical rules
 ******************************************************************/



/*
  TreeNode* Derivation::extract_nonSyntacticRules(nonSyntacticTrees, searchNode,
  pos_s_start, pos_s_end,
  pos_t_start, pos_t_end,
  nlv, a, lineNumber, 'a', lcATS);
*/



void Derivation::extract_wsd_rules_as_derivations(TreeNode* n, int start, int end, Leaves *lv,
                                                  std::ostream* hierarchical_out, DB* dbp, int maxSourcePhraseSize, Alignment *a,
                                                  bool print_impossible, bool print_lexicalized_composed,
                                                  double& vXRS, double& lcvXRS, double& lcXRS, double& lcATS, double& hlcXRS,
                                                  double& sIMPOSSIBLE, double& tIMPOSSIBLE, double& LC, int lineNumber)
{
  if(this->is_good()){
    // add WSD attributes to existing GHKM, nonlexicalized rules
    for(int i=0, size=_derivationRules.size(); i<size; ++i) {
      if(!_rules_in_use.empty() && !_rules_in_use[i])
        continue;
      RuleInst *curRule = _derivationRules[i];
      add_attributes_to_any_rule(curRule, a, lineNumber, vXRS, lcvXRS, lcXRS, LC, true);
    }


    // extract now the WSD-like rules

    // Create a copy of 'lv', which expands virtual nodes,
    // i.e. if the the sequence of preterminals is "the JJ investments"
    // (with JJ(short @-@ term)), we get "the short @-@ term investments"
    Leaves* nlv = new Leaves();
    // Map indexes in nlv to indexes in lv:
    std::vector<int> indexes;
    /**/
    for(int i=0, size=lv->size(); i<size; ++i) {
      TreeNode* curNode = (*lv)[i];
      if(curNode->is_virtual())
        for(int j=0; j<curNode->get_nb_subtrees(); ++j) {
          nlv->push_back(curNode->get_subtree(j));
          indexes.push_back(i);
        }
      else {
        nlv->push_back(curNode);
        indexes.push_back(i);
      }
    }
    /**/

    if(print_impossible){
      // generate all source-to-IMPOSSIBLE phrase pairs
      for(int i=0; i < a->get_source_len(); i++)
        for(int length = 1; length < maxSourcePhraseSize && i + length-1 < a->get_source_len(); length++){
          // the source phrase spans positions [i, i+length-1]
          // determine now the span of the corresponding Chinese phrase
          int start_target = a->get_target_len(),
              end_target = 0;
          for(int k=i; k <= i+length-1; k++){
            align& targetA = a->source_to_target(k);
            if(targetA.size() == 0)
              continue; // non-aligned word
            for(align_it it=targetA.begin(); it != targetA.end(); it++){
              int cur_pos_t = *it;
              if(cur_pos_t > end_target)
                end_target = cur_pos_t;
              if(cur_pos_t < start_target)
                start_target = cur_pos_t;
            }
          }
          // check out whether the phase pair is good
          // (check the alignment in the source to target direction
          bool good_AT = true;
          if(start_target == a->get_target_len() && end_target == 0){
            // non-aligned phrase
            *hierarchical_out << "SOURCE-IMPOSSIBLE ### line=" << lineNumber << " type=lcXRS"
                              << " sphrase={ @IMPOSSIBLE@ }"
                              << " tphrase={ " << a->get_source_words(i,i+length-1)
                              << " } "
                              << std::endl;
            ++sIMPOSSIBLE;
            continue;
          }
          for(int k = start_target; k <= end_target; k++){
            align& sourceA = a->target_to_source(k);
            if(sourceA.size() == 0)
              continue; // non-aligned word
            for(align_it it = sourceA.begin(); it != sourceA.end(); it++){
              int cur_pos_s = *it;
              if(cur_pos_s < i || i+length-1 < cur_pos_s){
                good_AT = false;
                break;
              }
            }
            if(!good_AT){
              // print @IMPOSSIBLE
              *hierarchical_out << "SOURCE-IMPOSSIBLE ### line=" << lineNumber << " type=lcXRS"
                                << " sphrase={ @IMPOSSIBLE@ }"
                                << " tphrase={ " << a->get_source_words(i,i+length-1)
                                << " } "
                                << std::endl;
              ++sIMPOSSIBLE;
              break;
            }
          }
        }
    }

    // generate phrase-pairs for all positions from 1 to #wordsInChinese
    // up to length  maxSourcePhraseSize
    // create a vector with all Chinese words first

    for(int i=0; i < a->get_target_len(); i++)
      for(int length = 1; length <= maxSourcePhraseSize && i+length-1 < a->get_target_len(); length++){

        // the target phrase spans positions [i, i+length-1]
        // determine now the span of the corresponding English phrase

        int start_source = a->get_source_len(),
            end_source = 0;
        for(int k=i; k <= i+length-1; k++){
          align& sourceA = a->target_to_source(k);
          if(sourceA.size() == 0)
            continue; // non-aligned word
          for(align_it it=sourceA.begin(); it != sourceA.end(); it++){
            int cur_pos_s = *it;
            if(cur_pos_s > end_source)
              end_source = cur_pos_s;
            if(cur_pos_s < start_source)
              start_source = cur_pos_s;
          }
        }
        if(print_impossible && start_source == a->get_source_len() && end_source == 0){
          // non-aligned target phase
          *hierarchical_out << "TARGET-IMPOSSIBLE ### line=" << lineNumber << " type=lcXRS"
                            << " sphrase={ " << a->get_target_words(i,i+length-1) << "}  "
                            << " tphrase={ @IMPOSSIBLE@ } "
                            << std::endl;
          ++tIMPOSSIBLE;
          continue;
        }
        // check out whether the phase pair is good
        // (check the alignment in the source to target direction
        // this was originally in the code
        bool good_AT = true;
        for(int k = start_source; k <= end_source; k++){
          align& targetA = a->source_to_target(k);
          if(targetA.size() == 0)
            continue; // non-aligned word
          for(align_it it = targetA.begin(); it != targetA.end(); it++){
            int cur_pos_t = *it;
            if(cur_pos_t < i || i+length-1 < cur_pos_t){
              good_AT = false;
              break;
            }
          }
          if(print_impossible && !good_AT){
            // print @IMPOSSIBLE
            *hierarchical_out << "TARGET-IMPOSSIBLE ### line=" << lineNumber << " type=lcXRS"
                              << " sphrase={ " << a->get_target_words(i,i+length-1) << "}  "
                              << " tphrase={ @IMPOSSIBLE@ } "
                              << std::endl;
            ++tIMPOSSIBLE;
            break;
          }
        }
        if(!good_AT)
          continue;


        if(start_source > end_source)
          continue; // don't extract rules for non-aligned target phrases

        // Try to extract rule: step 1: go high enough in the tree

        int pos_s_start = start_source, pos_s_end = end_source,
            pos_t_start = i, pos_t_end = i+length-1;

        TreeNode* searchNodeLeft = (*nlv)[pos_s_start],
            *searchNodeRight = (*nlv)[pos_s_end],
            *searchNode = NULL;


        if(searchNodeLeft == searchNodeRight)
          searchNode = searchNodeLeft;
        else{
          bool found=false;
          while(!found && searchNodeLeft != NULL){
            searchNodeLeft = searchNodeLeft->parent();
            for(TreeNode* t=searchNodeRight; t != NULL; t=t->parent())
              if(searchNodeLeft == t){
                found = true;
                break;
              }
          }
          if(searchNodeLeft == NULL){ // common parent not found
            assert(false);
            break;
          }

          searchNode = searchNodeLeft;
        }
        // searchNode = common parent; the root of the rule we are going to extract

        // ensure that if there are unaligned Chinese words to the left or right,
        // then the searchNode is not a pre-terminal
        bool has_unaligned_chinese = false;
        if(a->target_to_source(pos_t_start).size() == 0)
          has_unaligned_chinese = true;
        if(a->target_to_source(pos_t_end).size() == 0)
          has_unaligned_chinese = true;
        if(has_unaligned_chinese && searchNode->is_preterm())
          searchNode = searchNode->parent();

        std::vector < TreeNode* > nonSyntacticTrees;

        bool first_pass=true;
        int multiChild=0; // it is incremented when going up the tree/lhs has more than 1 node

        do{
          /*
           *hierarchical_out << "first_pass=" << (first_pass ? "true" : "false") << std::endl;
           for(int j=i; j <= i+length-1; j++)
           *hierarchical_out << a->get_target_word(j) << ' ';
           *hierarchical_out << " -> ";
           for(int j=start_source; j <= end_source; j++)
           *hierarchical_out << a->get_source_word(j) << ' ';
           *hierarchical_out << std::endl;
           searchNode->dump_pretty_tree(*hierarchical_out, 0);
          */

          // extract the normal rules +
          // stay in the loop if extracting composed rules (print_lexicalized_composed=true)

          // Build a RuleInst object and expand its fronties until it is admissible
          RuleNode* ref = new RuleNode(searchNode,0);
          RuleInst *newRule = new RuleInst(ref);
          // Lexicalize the part of the rule that covers the AT:
          lexicalize_spans_in_rule(newRule,pos_s_start,pos_s_end,
                                   pos_t_start,pos_t_end);
          // Make sure that we reach an admissible frontier:
          expand_state_to_admissible_frontier(newRule, searchNode);
          // Create a valid rule out of the current search state:
          RuleInsts* rs = new RuleInsts;
          RuleInst::get_rules_from_state(rs, newRule, a,
                                         pos_t_start, pos_t_end, false);
          delete newRule;
          // delete ref; don't need this; it is destroyed with the newRule??

          // Finally, insert rule(s) into derivation:
          for(int ri=0, rsize=rs->size(); ri<rsize; ++ri) { // to avoid duplicate rules
            RuleInst *curRule = (*rs)[ri];
            std::pair<int, int> pp = curRule->get_first_last_target_pos();
            // std::cout << "TRY: " << curRule->get_str() << " [" << pos_t_start << "," << pos_t_end << "] [" << pp.first << ',' << pp.second << "]" << std::endl;
            if(pos_t_start != pp.first || pos_t_end != pp.second){
              // std::cout << "DELETE TOO BIG: " << curRule->get_str() << std::endl;
              delete curRule;
              continue;
            }

            RuleDescriptor *rd = new RuleDescriptor;
            curRule->set_desc(rd);
            curRule->set_attr();
            RuleRHS* rhs = curRule->get_rhs();

            // add WSD attributes to rule if it is already part of the derivation

            bool isAlreadyDerived = false;
            for(int j=0, size2=_derivationRules.size(); j<size2; ++j)
              if(_derivationRules[j]->is_duplicate_rule(curRule)) {
                isAlreadyDerived = true;
                add_attributes_to_any_rule(_derivationRules[j],a,lineNumber,
                                           vXRS,lcvXRS,lcXRS,LC,first_pass);
                break;
              }

            if(isAlreadyDerived){
              //  std::cout << "DELETE HAS ALREADY: " << curRule->get_str() << std::endl;
              delete curRule;
              delete rd;
              continue; // go to next rule
            }
            // this is a new rule I need to save as part of the derivation
            // see if the head and all children can be hooked up into the current derivation

            bool canHookUp = true;
            DerivationNodes *dns = new DerivationNodes();

            if(rhs->get_no_vars() > 0){
              // this is a rule that has some variables. Go through the RuleNodes in its frontier.
              // Get DerivationNodes at that frontier (which should already exist) and link the current
              // derivation node dn to successors

              // Process rhs to find rulenodes that have variables
              for(int i1=0, size = curRule->get_rhs()->get_size(); i1<size; ++i1) {
                const RuleRHS_el* rhs_el = curRule->get_rhs()->get_element(i1);
                RuleNode *ruleNode = NULL;
                int var_index = rhs_el->get_var_index();
                if(Variable::is_variable_index(var_index)){
                  int ruleNodeIndex = rhs_el->get_rulenode_index();
                  if(ruleNodeIndex >= 0)
                    ruleNode = curRule->get_lhs()[ruleNodeIndex];
                  // Go to next node if the current one if the current one
                  // does not exist:
                  if(ruleNode == NULL){
                    canHookUp = false;
                    break;
                  }

                  TreeNode *treeNode = ruleNode->get_treenode_ref();
                  // Find the TreeNode corresponding to the current RuleNode:
                  const span& sp = rhs_el->get_span();
                  DerivationNodeDescriptor desc1(treeNode,ruleNode->is_lexicalized() && ruleNode->get_treenode_ref()->get_nb_c_spans() > 0, sp.first, sp.second);
                  // Find if there is a derivation node already corresponding
                  // to treeNode:
                  DerivationNode* derNode1 = NULL;
                  TreeNodeToDerivationNode_it it  = _treeNodeToDerivationNode.find(desc1);
                  if(it != _treeNodeToDerivationNode.end()){
                    derNode1 = it->second;
                    dns->push_back(derNode1);
                    derNode1->inc_nb_parents();
                  }
                  else{
                    canHookUp = false;
                    break;
                  }
                }
              }
            }
            if(!canHookUp){
              // std::cout << "DELETE CANNOT HOOKUP:" << curRule->get_str() << std::endl;
              for(unsigned int i=0; i < dns->size(); i++)
                dns->at(i)->dec_nb_parents();
              delete dns; // clean up memory
              delete curRule;
              delete rd;
              continue; // move on to the next rule
            }


            // add new rule to list of derivation rules
            int curRuleID = _derivationRules.size();
            _derivationRules.push_back(curRule);

            // add attributes to curRule
            add_attributes_to_any_rule(_derivationRules[_derivationRules.size()-1],a,lineNumber,
                                       vXRS,lcvXRS,lcXRS,LC,first_pass);

            // need to make now derNode part of the derivation forest
            std::set<DerivationNode*> searched;
            if(! _derivationRoot->insert_into_derivation_forest(_derivationRules, searchNode, dns, curRuleID, searched)){
              // std::cout << "DELETE CANNOT INSERT: " << curRule->get_str() << std::endl;
              for(unsigned int i=0; i < dns->size(); i++)
                dns->at(i)->dec_nb_parents();
              delete dns;
              delete curRule;
              delete rd;
              _derivationRules.resize(_derivationRules.size()-1);
            }
            else{
              // std::cout << "INSERTED: " << curRuleID+1 << ' ' << curRule->get_str() << " ### " << curRule->get_desc_str(true) << std::endl;
            }
          }
          // print_derivation_forest(std::cout, 0);
          // std::cout << "***********\n\n";
          delete rs;
          searchNode = searchNode->parent();
          first_pass = false;
          if( searchNode->get_nb_subtrees() > 1)
            multiChild++;
        } while (print_lexicalized_composed==true &&
                 searchNode &&
                 multiChild < 2 &&
                 // searchNode->get_nb_subtrees() == 1 &&
                 searchNode->get_cat() != "TOP" &&
                 searchNode->get_cat() != "ROOT");
      }
    delete nlv;
  }
}



/*************************************************************************
 * Subroutines to extract AT rules:
 *************************************************************************/

// Extract AT rules from leaf nodes in lv:
// (currently, this can't be integrated with derivations)
void
Derivation::extract_ATRules(Leaves *lv, ATS *at) {

  // Create a copy of 'lv', which expands virtual nodes,
  // i.e. if the the sequence of preterminals is "the JJ investments"
  // (with JJ(short @-@ term)), we get "the short @-@ term investments"
  Leaves* nlv = new Leaves();
  // Map indexes in nlv to indexes in lv:
  std::vector<int> indexes;
  for(int i=0, size=lv->size(); i<size; ++i) {
    TreeNode* curNode = (*lv)[i];
    if(curNode->is_virtual())
      for(int j=0; j<curNode->get_nb_subtrees(); ++j) {
        nlv->push_back(curNode->get_subtree(j));
        indexes.push_back(i);
      }
    else {
      nlv->push_back(curNode);
      indexes.push_back(i);
    }
  }

  // Find all phrases that start at position i:
  for(int si=0, ssize=nlv->size(); si<ssize; ++si) {
    TreeNode* curNode = (*nlv)[si];
    const STRING& sw = curNode->get_word();

    // Find all AT entries that match word 'sw':
    const ATentries& entries = at->get_entries();
    std::pair<ATentries_cit,ATentries_cit> p = entries.equal_range(sw.c_str());

    for(ATentries_cit cit = p.first; cit != p.second; ++cit) {

      bool good_AT_s = true;
      // For each AT, try to match leaves with elements of the AT:
      const PhrasePair *pp = cit->second;
      const Phrase     *sp = pp->_s, *tp = pp->_t;

      // Check source language string:
      for(int j=0, last=sp->size()-1; j<=last; ++j) {
        if(si+j >= static_cast<int>(nlv->size()) ||
           (*nlv)[si+j]->get_word() != sp->el(j)) {
          good_AT_s = false;
          break;
        }
      }
      if(!good_AT_s) continue;
      // Start and end indexes in lv not (nlv):
      int s_mapped_si = indexes[si];
      int e_mapped_si = indexes[si+sp->size()-1];

      // Check target language string: (it can start at any position,
      // but must be consistent with the alignment):
      for(int ti=0, tsize=_alignment->get_target_len(); ti<tsize; ++ti) {
        bool good_AT_t = true;
        if(_alignment->get_target_word(ti) == tp->el(0)) {
          // The beginning of the phrase is a match:
          for(int k=0, last=tp->size()-1; k<=last; ++k) {
            if(ti+k >= _alignment->get_target_len() ||
               _alignment->get_target_word(ti+k) != tp->el(k)) {
              good_AT_t = false;
              break;
            }
          }
          if(!good_AT_t) continue;
          // Check alignment (source -> target):
          bool at_least_one = false;
          for(int asi=s_mapped_si; asi<=e_mapped_si; ++asi) {
            align& al = _alignment->source_to_target(asi);
            if(al.size() == 0)
              continue;
            at_least_one = true;
            for(align_it it = al.begin(), it_end = al.end();
                it != it_end; ++it) {
              int cur_pos_t = *it;
              if(cur_pos_t < ti || ti+tp->size() <= cur_pos_t) {
                good_AT_t = false;
                break;
              }
            }
          }
          if(!good_AT_t) continue;
          // Check alignment (target -> source):
          for(int ati=ti, ati_last=ti+tp->size()-1; ati<=ati_last; ++ati) {
            align& al = _alignment->target_to_source(ati);
            if(al.size() == 0)
              continue;
            at_least_one = true;
            for(align_it it = al.begin(), it_end = al.end();
                it != it_end; ++it) {
              int cur_pos_s = *it;
              if(cur_pos_s<s_mapped_si || e_mapped_si<cur_pos_s) {
                good_AT_t = false;
                break;
              }
            }
          }
          if(!good_AT_t) continue;
          if(!at_least_one) continue;
          // We know here that we can extract an alignment template rule:
          *myerr << "%%% can extract AT rule: "
                 << sp->get_str() << " # "
                 << tp->get_str() << " (ATSline="
                 << pp->_i
                 << ")" << std::endl;
          // Extract rule: step 1: go high enough in the tree:
          int pos_s_start = s_mapped_si, pos_s_end = e_mapped_si,
              pos_t_start = ti,          pos_t_end = ti+tp->size()-1;
          TreeNode* searchNode = curNode;
          int sespan=-1, eespan=-1, scspan=-1, ecspan=-1;
          while(true) {
            if(searchNode == NULL) {
              assert(false);
              break;
            }
            if(!searchNode->is_extraction_node()) {
              searchNode = searchNode->parent();
              continue;
            }
            // Collect some information about english and chinese spans:
            sespan = searchNode->get_start();
            eespan = searchNode->get_end();
            assert(searchNode->get_nb_c_spans() == 1);
            scspan = searchNode->get_c_start(0);
            ecspan = searchNode->get_c_end(0);
            // Take into account unaligned words: (if they help
            // covering the span)
            while(pos_t_start < scspan && scspan > 0 &&
                  _alignment->target_to_source(scspan-1).size() == 0)
              --scspan;
            while(pos_t_end > ecspan &&
                  ecspan+1 < _alignment->get_target_alignment_len() &&
                  _alignment->target_to_source(ecspan+1).size() == 0)
              ++ecspan;
            // Stop going up the tree if the spans are cover
            // the e- and c- phrases:
            if((!searchNode->parent() ||
                !searchNode->parent()->is_virtual()) &&
               pos_s_start  >= sespan && pos_s_end <= eespan &&
               (searchNode->get_nb_c_spans() == 0 ||
                (pos_t_start >= scspan && pos_t_end <= ecspan))) {
              break;
            }
            // Not a good node: go higher in the tree:
            searchNode = searchNode->parent();
          }
          // Check if we reached a position where the rule can be extracted:
          if(searchNode == NULL)
            continue;
          // Build a RuleInst object and expand its frontier until it is
          // admissible:
          RuleNode* ref = new RuleNode(searchNode,0);
          RuleInst *newRule = new RuleInst(ref);
          // Lexicalize the part of the rule that covers the AT:
          lexicalize_spans_in_rule
              (newRule,pos_s_start,pos_s_end,pos_t_start,pos_t_end);
          // Make sure that we reach an admissible frontier:
          expand_state_to_admissible_frontier(newRule, searchNode);
          // Create a valide rule out of the current search state:
          RuleInsts* rs = new RuleInsts;
          RuleInst::get_rules_from_state
              (rs, newRule, _alignment, scspan, ecspan, false);
          delete newRule;
          // Finally, print rule(s):
          for(int ri=0, rsize=rs->size(); ri<rsize; ++ri) {
            RuleInst *curRule = (*rs)[ri];
            RuleDescriptor *rd = new RuleDescriptor;
            curRule->set_desc(rd);
            curRule->set_attr();
            rd->set_str_attribute("algo","AT");
            rd->set_double_attribute("probAT",pp->_p);
            rd->set_int_attribute("ATSline",pp->_i);
            rd->set_str_attribute("sphrase","{{{"+sp->get_str()+"}}}");
            rd->set_str_attribute("tphrase","{{{"+tp->get_str()+"}}}");
            std::string rule = curRule->get_str();
            std::string desc = curRule->get_desc_str(false);
            //if(_dbp != NULL)
            //  db_insert(_dbp,const_cast<char*>(rule.c_str()),"-1");
            //else
              std::cout << rule << " ### " << desc << std::endl;
            delete curRule;

            delete rd;
          }
          delete rs;
        }
      }
    }
  }
  delete nlv;
}

/***************************************************************************
 * RuleInst printing functions:
 **************************************************************************/

void
Derivation::add_derivation_counts() const {
  for(int i=0, size=_derivationRules.size(); i<size; ++i) {
    RuleInst *curRule = _derivationRules[i];
    RuleDescriptor *d = curRule->get_desc();
#ifdef COLLECT_COUNTS
    float count = d->get_double_attribute("count");
    _rule_set->add_count(_derivationRuleIDs[i], count);
#endif
#ifdef COLLECT_FRAC_COUNTS
    float fraccount = d->get_double_attribute("fraccount");
    _rule_set->add_fraccount(_derivationRuleIDs[i], fraccount);
#endif
  }
}

  void Derivation::retrieve_global_rule_indices() {std::cerr << "commented out 2016-05-02\n"; }
  /*

void
Derivation::retrieve_global_rule_indices() {
  int size = _derivationRules.size();
  assert(_derivationRuleIDs.size() == 0);
  assert(_dbp != NULL);
  _derivationRuleIDs.resize(size);
  for(int i=0; i<size; ++i) {
    //std::string rulestr = _derivationRules[i]->get_str();
    std::stringstream rulestr;
    rulestr << _derivationRules[i]->get_str() << " ### "<<SIZEID<<"=" << _derivationRules[i]->get_nb_expansions();
    // const char* indstr = db_lookup(_dbp,const_cast<char*>(rulestr.str().c_str()));
    int* indstr = db_lookup_s2i(_dbp,const_cast<char*>(rulestr.str().c_str()));
    //cerr<<rulestr.str()<<" ||||  "<<indstr<<endl;
    if(indstr == NULL) {
      *myerr << "Missing rule: " << rulestr << "|" << std::endl;
      *myerr<<_derivationRules[i]->get_str() << " ### "<<SIZEID<<"=" << _derivationRules[i]->get_nb_expansions()<<endl;
      *myerr << "Missing rule: " << rulestr << "|" << std::endl;
      _derivationRuleIDs[i] = -1;
      continue;
    }

    int ind = *indstr;
    _derivationRuleIDs[i] = ind;
  }
}
  */

//! Print rules after each derivation (for local indexing only).
void
Derivation::print_derivation_rules
(std::ostream& out, bool print_desc, bool min_desc) const {
  // WW
  //set<std::string> rules_already_printed;
  for(int i=0, size=_derivationRules.size(); i<size; ++i) {
    if(!_rules_in_use.empty() && !_rules_in_use[i])
      continue;
    RuleInst *curRule = _derivationRules[i];
    RuleDescriptor *d = curRule->get_desc();
    const TreeNode* par = curRule->get_lhs()[0]->get_treenode_ref();
    if(print_desc && !min_desc) {
      int id=-1;
      if(_derivationRuleIDs.empty() || _rule_set == NULL) {
        id = i+1;
      } else {
        id = _derivationRuleIDs[i];
        assert(id < _rule_set->get_size());
        d->set_double_attribute("logp",_rule_set->get_logprob(id));
        d->set_double_attribute("p",_rule_set->get_prob(id));
      }
      d->set_int_attribute("id",id);
      d->set_int_attribute("start",par->get_start());
      d->set_int_attribute("end",par->get_end()+1);
    }
    std::string rule = curRule->get_str();
#if 0
    // ww
    ostringstream ost;
    // assert(curRule->get_rootref()->get_treenode_ref()->type() == ForestNode::AND);
    ost<<(void*)curRule->get_rootref()->get_treenode_ref()->parent()<<"\t"<<rule;
    if(rules_already_printed.find(ost.str()) == rules_already_printed.end()) {
      rules_already_printed.insert(ost.str()) ;
    } else { continue; }
    // ww
#endif
    std::string desc = curRule->get_desc_str(true);
    out << rule;
    if(print_desc) {
      out << " ###" ;
      // print the head of the lhs tree.
      if(State::mark_const_head_in_rule){
        out<<" headmarker={{{ "<<curRule->get_head_tree(curRule->get_rootref())<<" }}} ";
      }
      out <<  desc;
    }

    out << " hwpos={{{" << curRule->get_hw_intersect_frontier(curRule->get_rootref())<<"}}}";

    out << " hwf={{{" << curRule->get_hwt_of_frontiers(curRule->get_rootref(), true) << "}}}";
    out << " htf={{{" << curRule->get_hwt_of_frontiers(curRule->get_rootref(), false) << "}}}";

    TreeNode* tn = curRule->get_rootref()->get_treenode_ref();
    out<<" ht={{{";
    ;
    if(tn->is_preterm()){
      out<<tn->get_cat();
    } else {
      out<<tn->get_tag();
    }
    out<<"}}} hw={{{";
    out<<tn->get_word();
    out<<"}}}\n";
  }
}


//! Print rules after each derivation (for local indexing only).
// Daniel
void Derivation::print_nonlexicalized_derivation_rules(std::ostream& out,
                                                       int lineNumber,
                                                       double& vXRS,
                                                       bool print_desc=true) const {
  for(int i=0, size=_derivationRules.size(); i<size; ++i) {
    if(!_rules_in_use.empty() && !_rules_in_use[i])
      continue;
    RuleInst *curRule = _derivationRules[i];
    if(curRule->get_nb_lexicalized() == 0 ||
       curRule->get_desc()->get_str_attribute("type") == string("unlex")){
      RuleDescriptor *d = curRule->get_desc();
      // const TreeNode* par = curRule->get_lhs()[0]->get_treenode_ref();
      std::string rule = curRule->get_str();
      std::string desc = curRule->get_desc_str(false);
      out << rule << " ### line=" << lineNumber << " type=vXRS fc=" << d->get_double_attribute("fraccount");
      std::string sourcePhrase = curRule->get_source_words();
      out << " sphrase={{{ " << ((sourcePhrase != "") ? sourcePhrase : "NIL") << " }}}";
      std::string targetPhrase;
      curRule->get_target_words(curRule->get_rootref(),targetPhrase);
      out << " tphrase={{{ " << ((targetPhrase != "") ? targetPhrase : "NIL") << " }}}";
      out << std::endl;
      ++vXRS;
    }
  }
}




// adds WSDtype, align, lineNumber, sPhrase, and tPhrase attributes to a rule
void
Derivation::add_attributes_to_any_rule(RuleInst* curRule, Alignment* a, int lineNumber,
                                       double& vXRS, double& lcvXRS, double& lcXRS, double& LC,
                                       bool first_pass) {

  std::vector<string> sourcePhrase, targetPhrase;
  std::vector<int> sourceVarIndex, targetVarIndex;
  std::vector< std::pair< int, int > > sourceBoundaries, targetBoundaries, adjustedTargetBoundaries;
  RuleRHS* rhs = curRule->get_rhs();
  // determine source phrase (may contain internal variables)
  for(int i=0, size=rhs->get_size(); i < size; ++i){
    const RuleRHS_el* el = rhs->get_element(i);
    int var_index = el->get_var_index();
    if(Variable::is_variable_index(var_index)){
      std::stringstream ss;
      ss << var_index;
      sourceVarIndex.push_back(var_index);
      sourcePhrase.push_back("::x" + ss.str());
    }
    else{
      sourceVarIndex.push_back(-1);
      sourcePhrase.push_back(el->get_lex());
    }
    sourceBoundaries.push_back(std::make_pair( el->get_start(), el->get_end()));
  }
  // determine corresponding target phrase
  RuleNode* n = curRule->get_lhs()[0];
  curRule->get_lhs_stuff(n, targetPhrase, targetVarIndex, targetBoundaries);

  bool has_vars_on_target = false;
  for(unsigned i=0; i < targetVarIndex.size(); i++)
    if(targetVarIndex[i] != -1){
      has_vars_on_target = true;
      break;
    }


  // compute beginning and end of target phrase

  int start_target = 100000, end_target = 0;
  for(unsigned int i=0; i < targetBoundaries.size(); i++){
    if(targetBoundaries[i].first < start_target)
      start_target = targetBoundaries[i].first;
    if(targetBoundaries[i].second > end_target)
      end_target = targetBoundaries[i].second;
  }

  // compute adjusted target boundaries

  int start = 0, gap = 0;
  if(targetBoundaries.size() > 0)
    start = targetBoundaries[0].first;
  for(unsigned int i=0; i < targetBoundaries.size(); i++){
    int b = targetBoundaries[i].first - start - gap;
    // int e = targetBoundaries[i].second - start - gap;
    gap += targetBoundaries[i].second - targetBoundaries[i].first;
    adjustedTargetBoundaries.push_back(std::make_pair( b, b));
  }


  /*
    std::cout << "src={{{ ";
    for(unsigned int i=0; i < sourcePhrase.size(); i++)
    std::cout << sourcePhrase[i] << ' ';
    std::cout << "}}} ";
    std::cout << "tgt: ";
    for(unsigned int i=0; i < targetPhrase.size(); i++)
    std::cout << targetPhrase[i] << ' ';
  */
  std::stringstream srcBsstr;

  for(unsigned int i=0; i < sourceBoundaries.size(); i++)
    srcBsstr << '[' << sourceBoundaries[i].first << ',' << sourceBoundaries[i].second + 1 << ']' << ' ';

  std::stringstream tgtBsstr;

  for(unsigned int i=0; i < targetBoundaries.size(); ++i)
    tgtBsstr << '[' << targetBoundaries[i].first << ',' << targetBoundaries[i].second + 1 << ']' << ' ';

  //std::cout << "tgtB={{{ ";
  //for(unsigned int i=0; i < targetBoundaries.size(); i++)
  //    std::cout << targetBoundaries[i].first << ':' << targetBoundaries[i].second << ' ';
  //std::cout << "}}}";
  /*
    std::cout << "atgtB: ";
    for(unsigned int i=0; i < adjustedTargetBoundaries.size(); i++)
    std::cout << adjustedTargetBoundaries[i].first << ':' << adjustedTargetBoundaries[i].second << ' ';
    std::cout << std::endl << "=====================" << std::endl;

  */

  // determine alignment attribute
  string a_attr;
  char *buffer = (char *)malloc(300);
  sprintf(buffer,"#s=%d #t=%d ",(int)sourcePhrase.size(), (int)targetPhrase.size());
  a_attr += string(buffer);

  for(unsigned int i = 0; i < sourcePhrase.size(); i++){
    string::size_type loc = sourcePhrase[i].find("::x", 0);
    if(loc != string::npos){
      // it is a variable
      unsigned int k=0;
      while(targetPhrase[k] != sourcePhrase[i]) ++k;
      sprintf(buffer,"%d,%d ", i, k);
      a_attr += string(buffer);
    }
    else{
      // it is a word; find alignment
      align& sourceA = a->target_to_source(sourceBoundaries[i].first);
      for(align_it it=sourceA.begin(); it != sourceA.end(); it++){
        int absolute_pos = *it, relative_pos = -1;
        for(unsigned int j=0; j < targetBoundaries.size(); j++)
          if(targetBoundaries[j].first == absolute_pos){
            relative_pos = adjustedTargetBoundaries[j].first;
            break;
          }
        assert(relative_pos != -1);
        sprintf(buffer,"%d,%d ", i, relative_pos);
        a_attr += string(buffer);
      }
    }
  }

  delete buffer;
  string s_attr, t_attr;
  for(unsigned int i=0; i < sourcePhrase.size(); i++)
    s_attr += sourcePhrase[i] + ' ';
  for(unsigned int i=0; i < targetPhrase.size(); i++)
    t_attr += targetPhrase[i] + ' ';

  RuleDescriptor *d = curRule->get_desc();

  d->set_str_attribute("min",curRule->min ? "{{{1}}}" : "{{{0}}}");
  d->set_str_attribute("tgtb","{{{ " + tgtBsstr.str() + " }}}");
  d->set_str_attribute("srcb","{{{ " + srcBsstr.str() + " }}}");
  // determine WSD type
  if(first_pass){
    if(rhs->get_no_vars() > 0){
      if(s_attr == ""){
        ++vXRS;
        d->set_str_attribute("WSD","vXRS");
      }
      else{
        ++lcvXRS;
        d->set_str_attribute("WSD","lcvXRS");
      }
    }
    else{
      if(has_vars_on_target == false){
        ++lcXRS;
        d->set_str_attribute("WSD","lcXRS");
      }
      else{
        ++lcvXRS;
        d->set_str_attribute("WSD","lcvXRS");
      }
    }
  }
  else{
    ++LC;
    d->set_str_attribute("WSD",string("lcvXRS-C"));
  }

  if(a_attr != "")
    d->set_str_attribute("align", "{{{[ " + a_attr + "]}}}");
  if(s_attr != "")
    d->set_str_attribute("sphrase", "{{{ " + s_attr + "}}}");
  if(t_attr != "")
    d->set_str_attribute("tphrase", "{{{ " + t_attr + "}}}");
  d->set_int_attribute("lineNumber",lineNumber);

  return;

}



// adds WSDtype, align, lineNumber, sPhrase, and tPhrase attributes to a rule
void
Derivation::add_attributes_to_any_rule_old(RuleInst* curRule, Alignment* a, int lineNumber,
                                           double& vXRS, double& lcvXRS, double& lcXRS, double& LC,
                                           bool first_pass) {
  std::vector<string> sourcePhrase, targetPhrase;
  std::vector<int> sourceVarIndex, targetVarIndex;
  std::vector< std::pair< int, int > > sourceBoundaries, targetBoundaries, adjustedTargetBoundaries;
  RuleRHS* rhs = curRule->get_rhs();
  // determine source phrase (may contain internal variables)
  bool is_lexical = false;
  for(int i=0, size=rhs->get_size(); i < size; ++i){
    const RuleRHS_el* el = rhs->get_element(i);
    int var_index = el->get_var_index();
    if(Variable::is_variable_index(var_index)){
      if(!is_lexical)
        continue;
      is_lexical = true;
      std::stringstream ss;
      ss << var_index;
      sourceVarIndex.push_back(var_index);
      sourcePhrase.push_back("::x" + ss.str());
    }
    else{
      is_lexical = true;
      sourceVarIndex.push_back(-1);
      sourcePhrase.push_back(el->get_lex());
    }
    sourceBoundaries.push_back(std::make_pair( el->get_start(), el->get_end()));
  }
  int size = sourceVarIndex.size()-1;
  while( size > 0 && sourceVarIndex[size] != -1) --size;
  size++;
  if(size >= 0){
    sourceVarIndex.resize(size);
    sourcePhrase.resize(size);
    sourceBoundaries.resize(size);
  }
  // determine corresponding target phrase
  is_lexical = false;
  RuleNode* n = curRule->get_lhs()[0];
  curRule->get_lhs_stuff_old(n, targetPhrase, targetVarIndex, targetBoundaries, is_lexical);
  // delete from target phrase every variable that is not part of source phrase
  unsigned int i=0;
  while(i < targetVarIndex.size()){
    if(targetVarIndex[i] == -1){
      ++i;
      continue;
    }
    else if(find(sourceVarIndex.begin(), sourceVarIndex.end(), targetVarIndex[i]) == sourceVarIndex.end()){
      // should delete this variable
      for(unsigned int j = i; j < targetVarIndex.size()-1; j++){
        targetVarIndex[j] = targetVarIndex[j+1];
        targetPhrase[j] = targetPhrase[j+1];
        targetBoundaries[j] = targetBoundaries[j+1];
      }
      size = targetVarIndex.size()-1;
      if(size >=0){
        targetVarIndex.resize(size);
        targetPhrase.resize(size);
        targetBoundaries.resize(size);
      }
    }
    else
      ++i;
  }


  /*
    size = targetVarIndex.size()-1;
    while(size > 0 && targetVarIndex[size] != -1 ) --size;
    size++;
    if(size >= 0){
    targetVarIndex.resize(size);
    targetPhrase.resize(size);
    targetBoundaries.resize(size);
    }
  */

  bool has_vars_on_target = false;
  for(unsigned i=0; i < targetVarIndex.size(); i++)
    if(targetVarIndex[i] != -1){
      has_vars_on_target = true;
      break;
    }


  // compute beginning and end of target phrase

  int start_target = 100000, end_target = 0;
  for(unsigned int i=0; i < targetBoundaries.size(); i++){
    if(targetBoundaries[i].first < start_target)
      start_target = targetBoundaries[i].first;
    if(targetBoundaries[i].second > end_target)
      end_target = targetBoundaries[i].second;
  }

  // compute adjusted target boundaries

  int start = 0, gap = 0;
  if(targetBoundaries.size() > 0)
    start = targetBoundaries[0].first;
  for(unsigned int i=0; i < targetBoundaries.size(); i++){
    int b = targetBoundaries[i].first - start - gap;
    // int e = targetBoundaries[i].second - start - gap;
    gap += targetBoundaries[i].second - targetBoundaries[i].first;
    adjustedTargetBoundaries.push_back(std::make_pair( b, b));
  }

  /*

    std::cout << "src: ";
    for(unsigned int i=0; i < sourcePhrase.size(); i++)
    std::cout << sourcePhrase[i] << ' ';
    std::cout << "tgt: ";
    for(unsigned int i=0; i < targetPhrase.size(); i++)
    std::cout << targetPhrase[i] << ' ';
    std::cout << "srcB: ";
    for(unsigned int i=0; i < sourceBoundaries.size(); i++)
    std::cout << sourceBoundaries[i].first << ':' << sourceBoundaries[i].second << ' ';
    std::cout << "tgtB: ";
    for(unsigned int i=0; i < targetBoundaries.size(); i++)
    std::cout << targetBoundaries[i].first << ':' << targetBoundaries[i].second << ' ';
    std::cout << "atgtB: ";
    for(unsigned int i=0; i < adjustedTargetBoundaries.size(); i++)
    std::cout << adjustedTargetBoundaries[i].first << ':' << adjustedTargetBoundaries[i].second << ' ';
    std::cout << std::endl;

  */

  string a_attr;
  // determine alignment attribute
  for(unsigned int i = 0; i < sourcePhrase.size(); i++){
    string::size_type loc = sourcePhrase[i].find("::x", 0);
    if(loc != string::npos)
      // it is a variable
      a_attr += sourcePhrase[i] + " ";
    else{
      // it is a word; find alignment
      align& sourceA = a->target_to_source(sourceBoundaries[i].first);
      if(sourceA.size() == 0)
        a_attr += string("x ");
      char *buffer = (char *)malloc(100);
      for(align_it it=sourceA.begin(); it != sourceA.end(); it++){
        int absolute_pos = *it, relative_pos = -1;
        for(unsigned int j=0; j < targetBoundaries.size(); j++)
          if(targetBoundaries[j].first == absolute_pos){
            relative_pos = adjustedTargetBoundaries[j].first;
            break;
          }
        assert(relative_pos != -1);
        sprintf(buffer,"%d,",relative_pos);
        a_attr += string(buffer);
      }
      delete buffer;
      a_attr.resize(a_attr.size()-1);
      a_attr += string(" ");
    }
  }

  string s_attr, t_attr;
  for(unsigned int i=0; i < sourcePhrase.size(); i++)
    s_attr += sourcePhrase[i] + ' ';
  for(unsigned int i=0; i < targetPhrase.size(); i++)
    t_attr += targetPhrase[i] + ' ';

  RuleDescriptor *d = curRule->get_desc();



  // determine WSD type
  if(first_pass){
    if(rhs->get_no_vars() > 0){
      if(s_attr == ""){
        ++vXRS;
        d->set_str_attribute("WSD","vXRS");
      }
      else{
        ++lcvXRS;
        d->set_str_attribute("WSD","lcvXRS");
      }
    }
    else{
      if(has_vars_on_target == false){
        ++lcXRS;
        d->set_str_attribute("WSD","lcXRS");
      }
      else{
        ++lcvXRS;
        d->set_str_attribute("WSD","lcvXRS");
      }
    }
  }
  else{
    ++LC;
    d->set_str_attribute("WSD",string("lcvXRS-C"));
  }

  if(a_attr != "")
    d->set_str_attribute("align", "{{{ " + a_attr + "}}}");
  if(s_attr != "")
    d->set_str_attribute("sphrase", "{{{ " + s_attr + "}}}");
  if(t_attr != "")
    d->set_str_attribute("tphrase", "{{{ " + t_attr + "}}}");
  d->set_int_attribute("lineNumber",lineNumber);

  return;

}






//! Return true if the given rule is present in the rule set of
//! the current sentence.
bool
Derivation::has_rule(const std::string& r, bool lhs_only) const {
  for(int i=0, size=_derivationRules.size(); i<size; ++i) {
    if(!_rules_in_use.empty() && !_rules_in_use[i])
      continue;
    RuleInst *curRule = _derivationRules[i];
    //RuleDescriptor *d = curRule->get_desc();
    //const TreeNode* par = curRule->get_lhs()[0]->get_treenode_ref();
    std::string rule = (lhs_only) ?
        curRule->get_lhs_str(curRule->get_rootref()) :
        curRule->get_str();
    if(rule == r)
      return true;
  }
  return false;
}

  void Derivation::add_rules_to_db() const {std::cerr << "commented out 2016-05-02\n"; }
  /*
//! Save rules in database:
void
Derivation::add_rules_to_db() const {
  for(int i=0, size=_derivationRules.size(); i<size; ++i) {
    RuleInst *curRule = _derivationRules[i];
    std::string rule = curRule->get_str();
    int* index = db_lookup_s2i(_dbp,const_cast<char*>(rule.c_str()));
    if(index == NULL) {
      db_insert(_dbp,const_cast<char*>(rule.c_str()),-1);
    }
  }
}
  */

void
Derivation::print_derivation_forest(std::ostream& out,
                                    int derivation_index) const {
  // Print header:
  out << "@@@ ";
  // Print derivation forest:
  out << "forest={{{";
  DerivationNodeIndex* ind = new DerivationNodeIndex;
  int first_index = 1;
  out <<
      get_derivation_string(_derivationRoot,ind,first_index,true);
  delete ind;
  out << "}}}";
  if(_derivationRoot->get_logp() > -DBL_MAX) {
    out << " logp=" << _derivationRoot->get_logp();
    //out << " ppl="  << TODO;
  }
  if(derivation_index != -1)
    out << " line=" << derivation_index;
  out << " fstring={{{" << _alignment->str_target() << "}}}";
  out << " estring={{{" << _alignment->str_source() << "}}}";
  out << "\n";
  // Print rules:
  if(!_global_indexing)
    print_derivation_rules(out,true);

}


std::string
Derivation::get_derivation_string(DerivationNode *dn,
                                  DerivationNodeIndex *ind,
                                  int& last_index,
                                  bool firstIter) const {
  std::string tmp;
  // Current OR-node:
  const DerivationOrNode& or_node = dn->get_children();
  // Write a pointer if needed (note: a pointer starts with "#", e.g. #1):
  // (if the pointer was already defined, then set 'skip' to true, so
  // that we skip the recursive part)
  bool skip = false;
  // There can only be pointers with DerivationNodes that have more
  // than one parent. The next block is ignored if there is only one parent:
  if(dn->get_nb_parents() > 1) {
    bool set_pointer = false;
    if(or_node.size() > 1)
      set_pointer = true;
    else if(or_node.size() == 0)
      set_pointer = false;
    else {
      const DerivationNodes* dns = or_node.begin()->second;
      for(size_t i=0; i<dns->size(); ++i) {
        if((*dns)[i]->get_children().size() > 0) {
          set_pointer = true;
          break;
        }
      }
    }
    if(set_pointer) {
      int index = -1;
      std::stringstream ss;
      tmp += "#";
      DerivationNodeIndex_cit cit = ind->find(dn);
      if(cit != ind->end()) {
        index = cit->second;
        skip = true;
        ss << index << " ";
      } else {
        index = last_index++;
        ind->insert(std::make_pair(dn,index));
        ss << index;
      }
      tmp += ss.str();
    }
  }
  if(!skip) {
    // Check whether the OR has more than one child.
    // if yes, write "(OR ...)":
    if(or_node.size() > 1)
      tmp += "(OR ";
    // Recursive call:
    for(DerivationOrNode_cit cit=or_node.begin(), cit_end=or_node.end();
        cit != cit_end; ++cit) {
      int ruleID = cit->first;
      const DerivationNodes* dns = cit->second;
      //const RuleInst* rule = _derivationRules[ruleID];
      bool is_leaf = true;
      for(int i=0, size=dns->size(); i<size; ++i) {
        if((*dns)[i]->get_children().size() > 0) {
          is_leaf = false;
        }
      }
      if(!is_leaf)
        tmp += "(";
      if(_global_indexing) {
        std::stringstream ruleIDstr;
        ruleIDstr << _derivationRuleIDs[ruleID];
        tmp += ruleIDstr.str() + " ";
      } else {
        std::stringstream ruleIDstr;
        ruleIDstr << ruleID+1;
        tmp += ruleIDstr.str() + " ";
      }
      for(int i=0, size=dns->size(); i<size; ++i) {
        tmp += get_derivation_string
            ((*dns)[i],ind,last_index,false);
      }
      if(!is_leaf) {
        tmp.erase(tmp.length()-1,1);
        tmp += ") ";
      }
    }
    if(or_node.size() > 1) {
      tmp.erase(tmp.length()-1,1);
      tmp += ") ";
    }
  }
  if(firstIter)
    tmp.erase(tmp.length()-1,1);
  return tmp;
}

}
