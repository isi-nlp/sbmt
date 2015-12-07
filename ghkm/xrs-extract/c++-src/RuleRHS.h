#ifndef __RULERHS_H__
#define __RULERHS_H__

#include <iostream>
#include <deque>

#include "RuleRHS_el.h"
#include "TreeNode.h"

namespace mtrule {

/***************************************************************************
 * RuleRHS.C/h
 * 
 * This class is used to represent the RHS of rule.
 *
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: RuleRHS.h,v 1.2 2006/09/12 21:51:22 marcu Exp $
 ***************************************************************************/

class RuleRHS {

 public:

  /***********************************************************************
	* Constructors and destructors:
	***********************************************************************/
 
  // Default constructor:
  RuleRHS() : _els(), _nb_lex(0) {
	 ++obj_in_mem;
  }
  // Make a fresh copy of a RuleRHS object:
  RuleRHS(const RuleRHS& rhs);
  // Delete all RuleRHS_el objects allocated in 'rhs':
  ~RuleRHS();

  /***********************************************************************
	* Accessors and mutators:
	***********************************************************************/

  std::string get_str(void); // Daniel
  int get_no_vars(void); // Daniel  
  // Get number of elements in the RHS:
  int get_size() const { return _els.size(); }
  // Get number of elements that are lexicalized:
  int get_nb_lexicalized()    const { return _nb_lex; }
  // Get a given element of the RHS:
  RuleRHS_el* get_element(int i) { return _els[i]; } 
  // Delete a given element from RHS:
  void delete_element(int i) { 
	 std::deque<RuleRHS_el*>::iterator it = _els.begin()+i;
	 delete *it;
	 _els.erase(_els.begin()+i); 
  }
  // Add a new element to the RHS:
  void add_element(RuleRHS_el *el) { 
	 if(el->is_lexicalized())
		++_nb_lex;
	 _els.push_back(el); 
  }
  // Return number of DerivationNode objects currently in memory:
  static int get_nb_obj_in_mem() { return obj_in_mem; }
 
  /***********************************************************************
	* Rule acquisition functions:
	***********************************************************************/
  
  // Put rhs of Rule in correct order:
  void sort_rhs();
  // Remove duplicates in RHS (i.e. elements that cover the same span), 
  // and add unaligned elements:
  void set_contiguous_non_overlapping(const Alignment *,int,int);
  
 protected:	
  std::deque<RuleRHS_el*> _els;
  int _nb_lex;         // number of RHS elements that are lexicalized
  static int obj_in_mem;

};

}

#endif
