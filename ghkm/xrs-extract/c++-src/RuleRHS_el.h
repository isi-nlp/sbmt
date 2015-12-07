#ifndef __RULERHS_EL_H__
#define __RULERHS_EL_H__

/***************************************************************************
 * RuleRHS_el.C/h
 * 
 * This class is used to represent elements of the RHS of rule.
 * TODO: add ptr btw variable and and RuleNode in LHS
 *
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: RuleRHS_el.h,v 1.1.1.1 2006/03/05 09:20:26 mgalley Exp $
 ***************************************************************************/

#include <iostream>

#include "RuleNode.h"
#include "defs.h"
#include "nstring.h"

namespace mtrule {

  class RuleRHS_el {

    friend class RuleRHS;

	public:
	 // Constructors and destructor: 
	 RuleRHS_el(int i, const STRING& l, int start, int end, int rni): 
	   _var_index(i),_lex(l),_span(start,end),_rn_index(rni) {
      ++els_in_mem;
	 }
    RuleRHS_el(const RuleRHS_el& el):
	   _var_index(el._var_index),_lex(el._lex),_span(el._span),
		_rn_index(el._rn_index) {
      ++els_in_mem;
	 }
	 ~RuleRHS_el() {
      --els_in_mem;
    }
	  
    // Comparison operator: (internal)
	 bool operator==(const RuleRHS_el& e) const { 
	   return (_var_index == e._var_index && 
		        _span == e._span &&
		        _lex == e._lex);
	 }
	 bool operator!=(const RuleRHS_el& e) const { 
	   return !operator==(e);
	 }

    // Comparison operator:
	 bool is_equivalent(const RuleRHS_el& e) const { 
	   return (_var_index == e._var_index && 
		        _lex == e._lex);
	 }

	 // Accessors/mutators:
	 bool is_lexicalized()    const { return _lex != ""; } 
	 const span& get_span()   const { return _span; }
	 int  get_start()         const { return _span.first; }
	 int  get_end()           const { return _span.second; }
	 void set_start(int s)          { _span.first = s; }
	 void set_end(int s)            { _span.second = s; }
	 int get_var_index()      const { return _var_index; }
	 STRING get_lex()         const { return _lex; }
	 int get_rulenode_index() const { return _rn_index; }
	 // Return number of DerivationNode objects currently in memory:
	 static int get_nb_els_in_mem() { return els_in_mem; }

   protected:	
	 int _var_index;
	 STRING _lex;
	 // This member is only relevant during rule acquistion, 
	 // and should be set to default values in other contexts (e.g. EM 
	 // training or perplex eval):
	 span _span;
	 int _rn_index; // rule-node index
    static int els_in_mem;
  };

  struct is_before_in_rhs {
	 bool operator()(const RuleRHS_el* e1, const RuleRHS_el* e2) const {
		return (e1->get_span().first <= e2->get_span().first);
	 }
  };

}

#endif
