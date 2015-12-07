/***************************************************************************
 * DerivationNode.C/h
 * 
 * This class is used to represent nodes in packed derivation 
 * forests (Derivation class). Each node represents an "OR"-node
 * that maps rules to successor DerivationNodes.
 *
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: DerivationNode.C,v 1.5 2006/12/18 21:07:00 wwang Exp $
 ***************************************************************************/

#include "DerivationNode.h"
 
namespace mtrule {

int  DerivationNode::nodes_in_mem=0; // for debugging

bool DerivationNode::insert_into_derivation_forest(RuleInsts& rules, TreeNode* searchNode, 
						   DerivationNodes* dns, int curRuleID,
						   std::set<DerivationNode*>& searched )
{
	// Added by Wei to avoid duplicate search.
	if(searched.find(this) != searched.end()){
		return false;
	}

  DerivationNode* dn = this;
  TreeNode* tn = this->get_treenode();
  DerivationOrNode& children = dn->get_children();
  if(tn != searchNode && children.empty()) {
	  searched.insert(this);
    return false;
  }

  // determine current rule span
  RuleInst* curRule = rules[curRuleID];
  RuleRHS* curRHS = curRule->get_rhs();
  int start_rule = curRHS->get_element(0)->get_start(),
    end_rule = curRHS->get_element(curRHS->get_size()-1)->get_end();

  

  // determine current rule span of immediate child
  int childRuleID = children.begin()->first;
  RuleInst *r = rules[childRuleID];
  RuleRHS* rhs = r->get_rhs();
  int start_rule_child = rhs->get_element(0)->get_start(),
    end_rule_child = rhs->get_element(rhs->get_size()-1)->get_end();
  
  if(tn != searchNode){
    if(start_rule_child > start_rule || end_rule_child < end_rule) {
      // searchNode c-span outside the scope of tn c-span
	  searched.insert(this);
      return false;
	}
    bool res = false;
    for(DerivationOrNode_it it = children.begin(), it_end = children.end(); it != it_end; ++it){
      DerivationNodes* c = it->second;
      for(size_t i = 0; i < c->size(); i++)
	res |= (*c)[i]->insert_into_derivation_forest(rules, searchNode, dns, curRuleID, searched);
    }
	searched.insert(this);
    return res;
  }
  else{
    // ensure the rule was not already added to this derivation node
    bool dup = false;
    if(!children.empty()){
      for(DerivationOrNode_it it = children.begin(), it_end = children.end(); it != it_end; ++it){
	int ruleID = it->first;
	if(ruleID == curRuleID){
	  dup = true;
	  break;
	}
      }
    }
    // ensure that all children in an OR node span the same spans as the curRule
    if(!dup && children.size() >= 1){
      if(start_rule != start_rule_child || end_rule != end_rule_child)
	dup = true;
      //      std::cout << "DEBUG**: " << curRule->get_str() << "\n";
      //      std::cout << "DEBUG**: curRule[" << start_rule << "," << end_rule << "] child_rule[" << start_rule_child << "," << end_rule_child << "]\n\n";
    }

    if(!dup){
      dn->add_child(curRuleID,dns);
      dn->inc_nb_parents();
      // derNode->inc_nb_parents();
	  searched.insert(this);
      return true;
    }
    // derNode->inc_nb_parents();
	searched.insert(this);
    return false;
  }
	searched.insert(this);
}



}
