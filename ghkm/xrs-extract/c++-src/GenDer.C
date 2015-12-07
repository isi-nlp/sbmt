#include <algorithm>
#include <sstream>
#include <vector>
#include <string>
#include <cfloat>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>

#include "GenDer.h"
#include "MyErr.h"
#include "db_access.h"

using namespace std;

namespace mtrule {

 
GenDer::~GenDer() {}

/***************************************************************************
 * Search functions: 
 **************************************************************************/

/////////////////////////////////////////////////////////////////////////
// Search all complex rules that can be collected in a given parse 
// (sub-)tree: (n is the root of the parse (sub-)tree)
// Extended by Wei Wang on Tue Sep 19 18:54:51 PDT 2006 to handle 
// forest (composed AND nodes and OR nodes).
void
GenDer::extract_derivations(TreeNode* tn, int start, int end) {

	// if the TreeNode is an OR node, we use its daughters.
	if(tn->type() == ForestNode::OR){
	     DerivationNodeDescriptor desc(tn,false,start,end);
	     TreeNodeToDerivationNode_it it = _treeNodeToDerivationNode.find(desc);
		 // it is possible the derivation node is formed, but the extraction
		 // is undone. it is also possible the deriv node is not formed.
		 if(it == _treeNodeToDerivationNode.end() || !it->second->is_done()) 
		 {
			 for(int i=0;i<tn->get_nb_subtrees();++i) {
				extract_derivations(tn->get_subtree(i),start,end);
			 }
			 it = _treeNodeToDerivationNode.find(desc);
			 if(it != _treeNodeToDerivationNode.end()){
				 it->second->set_done();
			 } else { assert(0); }
		 }
		 return;
	}


  if(_verbose) {
    *myerr << "%%% extracting derivation from: " << tn->get_cat() << "";
    *myerr << "[" << start << ":" << end << "]\n";
  }

// TOP
#if 0
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
#endif

  // Test whether or not any rule can be extracted from the current node:
  if(tn->is_extraction_node()) {

	 // 'dn' corresponds to the current derivation node:
	 DerivationNode* dn = NULL;

	 // Create a descriptor for DerivationNode dn
	 // (to find it once we will need to link a parent DerivationNode to it)
	 // We use the OR node as identity so that the AND nodes of the OR node
	 // can be in the same OR node of the drivation tree.
	 TreeNode* idNode = tn;
	 if(tn && tn->type() == ForestNode::AND){ idNode = tn->imdPar(); }
	 DerivationNodeDescriptor desc(idNode,false,start,end);
	 TreeNodeToDerivationNode_it it = _treeNodeToDerivationNode.find(desc);
	 
	 // Get derivation node corresponding to 'desc': 
	 if(it != _treeNodeToDerivationNode.end()) {
		dn = it->second;
	 }

	 // Create root derivation node if 'tn' is the root:
	 //TOP if(tn->is_root()) {
	 if(tn && !tn->parent() && !dn) {
		 dn = _derivationRoot = new DerivationNode(tn);
		 _treeNodeToDerivationNode.insert(std::make_pair(desc,dn));
	 }

	 // Check whether DerivationNode exists. If not, return:
	 if(dn == NULL) {
		return;
	 }

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
GenDer::extract_rules(TreeNode *tn, DerivationNode *dn, 
								  int start, int end, bool preterminal) {

 // Since the derivation forest is
 // a graph, we have to make sure we don't follow the same path twice:
 assert(dn != NULL);
//if(dn->is_done())
//	return;


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
 //State *necessaryState;
 std::vector<State*> _necessaryStates;
 std::set<State*> necessaryStates;
 std::set<State*>::iterator it1, it2;

 // Priority queues (openQueue and closedQueue are
 // sets of rules ordered by rule size). 
 // openQueue contains all states whose successors still need to be 
 // constructed (e.g. given a parse tree (A(B(C D) E(F G))), the only 
 // successors of state A(B E) are A(B(C D) E) and A(B E(F G)).
 // closedQueue contains all states whose successors are already 
 // in openQueue (or _closequeue).
 PriorityQueue openQueue, closedQueue;
 
  if(_verbose) {
    *myerr << "%%%%% GENERATING  NECESSARY STATES\n";
  }
 // Insert initial state/rule into priority queue (using the 
 // TreeNode that is provided), then extend the frontier until we
 // reach an admissible state/rule (a.k.a. "necessary rule"):
 get_smallest_admissible_state(tn, _necessaryStates); 
  if(_verbose) {
    *myerr << "%%%%% 1\n";
  }
 necessaryStates.insert(_necessaryStates.begin(), _necessaryStates.end());
  if(_verbose) {
    *myerr << "%%%%% 2\n";
  }
 openQueue.insert(necessaryStates.begin(), necessaryStates.end());
  if(_verbose) {
    *myerr << "%%%%% 3\n";
  }
 if(FAIL_IF_TOO_BIG) {
	 it1 = necessaryStates.begin();
	 while(it1 != necessaryStates.end()){
	   if((*it1)->get_nb_expansions() > _max_rule_expansions) {
		  delete *it1;
		  it2 = it1;
		  ++it1;
		  necessaryStates.erase(it2);
	 } else { ++it1; }
	   if(!necessaryStates.size()){ _is_good= false; return;}
	}
 }


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
	PriorityQueue::iterator itt = openQueue.begin();
	curState = *itt;

  if(_verbose) {
    *myerr << "%%%%% POPPED " << curState->get_lhs_str(curState->get_rootref())<<endl;
  }
	// Check the size (nb of nodes) of the first (i.e. smallest) tree fragment.
	// If it is bigger than a given threshold, then stop and move on:
	int cur_size = closedQueue.size() + openQueue.size();
	if(cur_size >= _max_to_extract || 
	   curState->get_nb_expansions() > _max_rule_expansions) {
	  break;
	}

#if 0
    PriorityQueue::iterator it5;
    for(it5 = closedQueue.begin(); it5 != closedQueue.end();  ++it5) { 
		if((*it5)->is_duplicate_state(curState)) {
			openQueue.erase(it5);
			delete *it5;
			break;
		}
	}
	if(it5 != closedQueue.end()){ continue; }
#endif

	// For each possible expansion of any leaf node of the current RuleInst, 
	// build a new RuleInst and expand its frontier to the smallest admissible 
	// frontier:
	// (current tree = first element of openQueue, i.e. openQueue.begin(); 
	// store new RuleInsts into openQueue)
	add_successor_states_to_queue(curState,&openQueue,tn,start,end);

	// Pop out the first element of openQueue, since we don't need it anymore:
	// (note: only the reference to that RuleInst is deleted, not the RuleInst 
	// itself)
	openQueue.erase(itt);
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
	 if(necessaryStates.find(*it) != necessaryStates.end()){
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
 //cout<<closedQueue.size()<<" SIZE\n";
 for(PriorityQueue::iterator it = closedQueue.begin(), 
	  it_end = closedQueue.end(); it != it_end; ++it) { 
	 /* 
	  * If the rule is not a necessary rule, and the number
	  * of rules extracted from one node exceeds a max limit,
	  * we dont extract the rules any more.
	  * Added by Wei Wang on  Sat Dec  2 13:30:27 PST 2006
	  */
//	 cout<<"AAAAAAAAAAA\n";
//	 cout<<max_rules_per_node<<"  "<<extractedRules.size()<<" "<<necessaryStates.size()<<endl;
	 if( max_rules_per_node > (int)0 && extractedRules.size() > (size_t)max_rules_per_node) {
		 if(_verbose) {
			*myerr << "the number of extracted rules exceed limit "<<max_rules_per_node<<std::endl;
		 }
		 continue;
	 }

	State *st = *it;
	RuleInsts *rs = new RuleInsts;
	RuleInst::get_rules_from_state
	  (rs, st, _alignment, start, end, _no_multi);

	for(int i=0, size=rs->size(); i<size; ++i) {
	  RuleInst* r = (*rs)[i];

	  if(necessaryStates.find(*it) != necessaryStates.end()){
		 // Set the is_minimal flag to all these rules.
		 r->set_is_minimal(true);
		 necessaryRules.push_back(r);
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
//	if(!_global_indexing)
//	  for(int j=0, size2=_derivationRules.size(); j<size2; ++j) {
//		 if(_derivationRules[j]->is_duplicate_rule(curRule)) {
//			RuleDescriptor* d = _derivationRules[j]->get_desc();
//			d->set_double_attribute
//			  ("count", d->get_double_attribute("count")+1.0);
//			d->set_double_attribute
//			  ("fraccount",d->get_double_attribute("fraccount")+count);
//			skip = true;
//			curRuleID = j;
//			break;
//		 }
//	  }
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
	  vector<TreeNode*> nodes;
	  if(treeNode->type() == ForestNode::OR){
		  for(int k = 0; k < treeNode->get_nb_subtrees(); ++k) {
			  nodes.push_back(treeNode->get_subtree(k));
		  }
	  } else { nodes.push_back(treeNode);}

	  for(vector<TreeNode*>::iterator t = nodes.begin(); t != nodes.end(); ++t){
		  //cout<<"TRN"<<(*t)->get_cat()<<endl;

	 // We use the OR node as identity so that the AND nodes of the OR node
	 // can be in the same OR node of the drivation tree.
	 TreeNode* idNode = *t;
	 if(*t && (*t)->type() == ForestNode::AND){ idNode = (*t)->imdPar(); }
	  DerivationNodeDescriptor 
	     desc(idNode,ruleNode->is_lexicalized(),sp.first,sp.second); 
	    //desc(treeNode,ruleNode->is_lexicalized() && ruleNode->get_treenode_ref()->get_nb_c_spans() > 0,
		 //sp.first,sp.second); // Daniel
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
	  // we need to check this because we are indixing derivation
	  // nodes using OR monolingual forest node. structure 'nodes'
	  // contains AND forest nodes, when we get their immediate
	  // parent nodes, duplicate is possible.
	  if(find(dns->begin(), dns->end(), derNode) == dns->end()){
		  dns->push_back(derNode);
		  derNode->inc_nb_parents();
	  }
	  }
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
	  //cout<<_derivationRules.size()<<endl;
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
	  //if( newRoot->is_internal() || !preterminal) {
	  if(newRoot->type() != ForestNode::OR && newRoot->is_internal() ||
			  newRoot->type() == ForestNode::OR && newRoot->get_subtree(0)->get_nb_subtrees() != 0 ||
			  !preterminal){
		 extract_derivations(newRoot,sp.first,sp.second);
	  }
	}
 }
 // Delete duplicate rules:
 for(int i=0, size=deletequeue.size(); i<size; ++i) {
	 delete deletequeue[i];
 }
 // "Remember" that dn has already been processed:
 //WW dn->set_done();
}

//! This is an generalized version of the above function.
//! When we have an e-forest, instead of an e-tree, we can
//! have more than one minimal rules extracted from a forest
//! node due to the OR nodes in the forest.
void
GenDer::get_smallest_admissible_state(TreeNode *n, vector<State*>& states)
{
  RuleNode* ref = new RuleNode(n,0);
  State *initState = new State(ref);
  expand_state_to_admissible_frontier(initState, n, states);
}

// Generalized version of the above function. Generalized to the e-forest case.
void
GenDer::
expand_state_to_admissible_frontier(State *curSt, 
		                            TreeNode *root,
									vector<State*>& states) 
{

	std::deque<State*> unfinishedStates;
    bool modified_frontier = false;

	unfinishedStates.push_back(curSt);
	State* curState;

	while(unfinishedStates.size()){

		//cout<<"SIZE"<<unfinishedStates.size()<<endl;

		curState = unfinishedStates.front();
		unfinishedStates.pop_front();

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

				if(treenode_ref->type() != ForestNode::OR){
				  // Case 1: yes, it is an internal node; add the successors
				  // of newNode to curRule:
				  curState->expand_successors(newNode);
				  modified_frontier=true;
				  unfinishedStates.push_back(curState);
				  //cout<<curState->get_lhs_str(curState->get_rootref())<<endl;
				  break;
				} else  {

					if(newNode->get_treenode_ref()->get_nb_subtrees() &&
					   !newNode->get_treenode_ref()->get_subtree(0)->get_nb_subtrees()) {
				curState->expand_successors(curState->get_lhs()[i], 0);
				curState->get_lhs()[curState->get_lhs().size() - 1]->
					                                     set_lexicalized(true);
				//TreeNode* nd = curState->get_lhs()[curState->get_lhs().size() -1]->get_treenode_ref();
				curState->inc_nb_lexicalized();
				if(newNode->get_treenode_ref()->get_nb_c_spans() == 0) // Daniel
				  curState->inc_nb_lexicalized_zero(); // Daniel
				unfinishedStates.push_back(curState);
				  //cout<<curState->get_lhs_str(curState->get_rootref())<<endl;
				modified_frontier=true;
				break;
					} else {

					// expand the state curState on the frontier node 'newNode'.
					// since 'newNode' is an OR node, we expand on each of the
					// OR-node's AND node once.
					for(int k = 0; k < treenode_ref->get_nb_subtrees(); ++k){
						 //cout<<"newing 3\n";
						State* ns = new State(*curState);
						//cerr<<" %%: id: "<<ns->id<<endl;
						RuleNode* rnToExpand = ns->get_lhs()[i];
						// they must refer to the same tree/forest node.
						assert(rnToExpand->get_treenode_ref() == treenode_ref);
						// expand the OR node (rnToExpand) on the k-th AND
						// child.
						assert(treenode_ref->get_subtree(k)->type() != ForestNode::OR);
						ns->expand_successors(rnToExpand, k);
				         


						// this is used for pruning states generated due 
						// to very larger mininal rules. I set the threshold
						// to be 100. Increasing this threshold (i.e., to 
						// 1000) mainly increase the huge minimal rules, which
						// might never be used anyway.
						if(!k || k > 0 && unfinishedStates.size() < 100) {
							unfinishedStates.push_back(ns);
						} else { delete ns; }

					}
					// delete the state object because it has cloned into different
					// other states.
					//	 cout<<"del 3\n";
					 delete curState;
				    modified_frontier=true;
					break;
					}
				}

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
			  newNode->set_word(treenode_ref->get_word());
			  modified_frontier=true;
			  unfinishedStates.push_back(curState);
			  //cout<<curState->get_lhs_str(curState->get_rootref())<<endl;
			  break;
			}
		 }

		 if(!modified_frontier){
			 states.push_back(curState);
		 }
	}
}

// Generalized verion of the above to e-forest. Added by Wei Wang.
bool
GenDer::
add_successor_states_to_queue(State* curState, 
		                      PriorityQueue* pq, 
							  TreeNode *root, int start, int end) {


  // If any new state/rule is added, return true. Otherwise, false:
  bool added_new_states = false;

  // For each node in the current state, determine if it is a leaf,
  // and if it is an internal node in the corresponding Tree:
  RuleNodes::difference_type offset = 0;
  for(RuleNodes_cit cit = curState->get_lhs().begin(), 
		cit_end = curState->get_lhs().end(); cit != cit_end; 
		                                                      ++cit, ++offset) {

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

	 vector<State*> newstates; // hold the new states got by expanding curNode.

	 // Conditions are met to create a new rule:
	 State *newState = NULL;
	 // Clone current state/rule:
	 //cout<<"newing 1\n";
	 newState = new State(*curState);
	 // Find the reference (in the rule rule) to the node to expand:
	 RuleNode *expansionNode 
		= newState->find_noderef(curNode->get_treenode_ref());
	 if(curNode->get_treenode_ref()->is_internal()) {
		// Case 1: the current node is an internal node of the parse tree:
		if(curNode->get_treenode_ref()->type() != ForestNode::OR){
				  // Case 1: yes, it is an internal node; add the successors
				  // of newNode to curRule:
				  assert(expansionNode->get_treenode_ref()->type() != ForestNode::OR);
				  newState->expand_successors(expansionNode);
				  newstates.push_back(newState);
		} else  {

			// if this OR node is the parent of a leaf node, we expand to the
			// leaf AND node, and lexialize it.
			if(curNode->get_treenode_ref()->get_nb_subtrees() &&
		   !curNode->get_treenode_ref()->get_subtree(0)->get_nb_subtrees()) {
				newState->expand_successors(newState->get_lhs()[offset], 0);
				newState->get_lhs()[newState->get_lhs().size() - 1]->
					                                     set_lexicalized(true);
				newState->inc_nb_lexicalized();
				if(curNode->get_treenode_ref()->get_nb_c_spans() == 0) // Daniel
				  newState->inc_nb_lexicalized_zero(); // Daniel
				newstates.push_back(newState);
			} else {

					// expand the state curState on the frontier node 'newNode'.
					// since 'newNode' is an OR node, we expand on each of the
					// OR-node's AND node once.
					for(int k = 0; k < curNode->get_treenode_ref()->
													get_nb_subtrees(); ++k){
						 // cout<<"newing 2\n";
						State* ns = new State(*newState);
						RuleNode* rnToExpand = ns->get_lhs()[offset];
						// they must refer to the same tree/forest node.
						assert(rnToExpand->get_treenode_ref() == 
										  curNode->get_treenode_ref());

						// expand the OR node (rnToExpand) on the k-th AND
						// child.
						ns->expand_successors(rnToExpand, k);
						newstates.push_back(ns);
					}
					// delete the state object because it has cloned into different
					// other states.
					//	 cout<<"del 1\n";
					 delete newState;
				}
		}
	 } else {
		// Case 2: the current node in the rule is a leaf that is not
		// lexicalized; lexicalize it:
		expansionNode->set_lexicalized(true);
		newState->inc_nb_lexicalized();
		if(curNode->get_treenode_ref()->get_nb_c_spans() == 0) // Daniel
		  newState->inc_nb_lexicalized_zero(); // Daniel
		newstates.push_back(newState);
	 }

	 for(vector<State*>::iterator it2 = newstates.begin(); 
											 it2 != newstates.end(); ++it2) {
		 // Now that we have modified newRule, we need to make sure that
		 // the frontier is admissible:
		 vector<State*> ss;
		 vector<State*>::iterator ss_it;
		 expand_state_to_admissible_frontier(*it2, root, ss);
		 for(ss_it = ss.begin(); ss_it != ss.end(); ++ss_it){
			 newState = *ss_it;
			 // Make sure that the state isn't already present in the queue:
			 bool duplicate = false;
			 for(PriorityQueue::iterator it3 = pq->begin(); it3 != pq->end(); ++it3) {
				State *elState = *it3;
				if(elState->is_duplicate_state(newState)) {
				  duplicate = true;
				  break;
				}
			 }
			 // Add newly created state to queue (if it is not a duplicate):
			 if(!duplicate) { 
						 //cout<<"ins "<<newState<<"\n";
				 pq->insert(newState); 
			 }
			 else { 
						 //cout<<"del 2\n";
				 delete newState;
			 }
		 }
	 }
  }
 
  // Might want to stop if the extraction of new rules didn't succeed:
  return added_new_states;
}

//! Print rules after each derivation (for local indexing only). 
void
GenDer::print_derivation_rules
  (std::ostream& out, bool print_desc, bool min_desc) const 
{
	set<std::string> rules_already_printed;

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

	 ostringstream ost;
	 assert(curRule->get_rootref()->get_treenode_ref()->type() == ForestNode::AND);
	 ost<<(void*)curRule->get_rootref()->get_treenode_ref()->parent()<<"\t"<<rule;
	 if(rules_already_printed.find(ost.str()) == rules_already_printed.end()) {
		 rules_already_printed.insert(ost.str()) ;
	 } else { continue; }
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
	 out << "\n";
  }
}

} // mtrule
