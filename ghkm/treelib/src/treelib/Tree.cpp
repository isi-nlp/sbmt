#include "treelib/Tree.h"

namespace treelib {

// Outputs the CFG rules that can be decomposed from the tree.
void Tree::getCFGRules(ostream& out, const string delimiter) const
{
    string ruleStr;
    pre_order_iterator it;

    for(it = begin(); it != end(); ++it){
	if(!it->getIsTerminal()){
	    ruleStr = getCRFRule(it);
	    out<<ruleStr<<delimiter;
	}
    }
}

// Returns the CFG rule corresponding to a tree node.
string Tree:: getCRFRule(const iterator_base& it) const 
{
    string ruleStr = "";
    sibling_iterator sib_it;

    if(it->getIsTerminal()){ ruleStr = "";}
    else {
	ruleStr = ruleStr + it->getLabel();
	ruleStr = ruleStr + " --->";

	for(sib_it = it.begin(); sib_it != it.end(); ++sib_it++) {
	    ruleStr = ruleStr + " " + sib_it->getLabel();
	}
    }
    return ruleStr;
}


} // END OF treelib


