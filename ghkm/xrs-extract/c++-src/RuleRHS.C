#include "RuleRHS.h"
#include "Variable.h"
#include "MyErr.h"

namespace mtrule {
  
  int  RuleRHS::obj_in_mem=0; // for debugging

  // Put rhs of Rule in correct order:
  void 
  RuleRHS::sort_rhs() { 
	 stable_sort(_els.begin(),_els.end(),is_before_in_rhs()); 
  }

  // Copy constructor:
  RuleRHS::RuleRHS(const RuleRHS& oldRHS) {
	 for(int i=0, size=oldRHS._els.size(); i<size; ++i) {
		RuleRHS_el* newE = new RuleRHS_el(*oldRHS._els[i]);
	   _els.push_back(newE);
	 }
	 _nb_lex = oldRHS._nb_lex;
	 ++obj_in_mem;
  }

  // Clear RHS:
  RuleRHS::~RuleRHS() {
    for(int i=0, size=_els.size(); i<size; ++i)
	   //if(_els[i] != NULL)
	     delete _els[i];
	 --obj_in_mem;
  }

  // Remove duplciate elements in the RHS, which 
  // can happen in cases where more than one source-
  // language word (english) align to one single
  // target-language word (chinese). Also, 
  // add unaligned chinese words.
  // TODO: add unaligned elements on the sides.
  // vector -> deque
  void 
  RuleRHS::set_contiguous_non_overlapping(const Alignment *a,
                                          int start, int end) {
	 std::deque<RuleRHS_el*> new_els;
	 // assert(_els.size() > 0); // Daniel
	 if(_els.size() == 0) // Daniel
	   return; // Daniel
	 new_els.push_back(_els[0]);
	 int iprev = 0;
	 // Aligned elements (with possibly elements in the middle):
	 for(int i=1, size=_els.size(); i<size; ++i) {
	   // Find overlap:
	   if(_els[iprev]->_span.first  == _els[i]->_span.first &&
	      _els[iprev]->_span.second == _els[i]->_span.second) {
		  // There is an overlap, so delete current element:
		  delete _els[i];
		} else if(_els[iprev]->_span.second > _els[i]->_span.first) {
		  *myerr << "INTERNAL ERROR: bad spans " << std::endl;
		  exit(1);
		} else {
		  // Deal with all unaligned items between rhs[i-1] and rhs[i]:
		  for(int j=_els[iprev]->_span.second+1, j_end=_els[i]->_span.first;
				j < j_end; ++j) {
			 // Add chinese at position j to the RHS:
			 const STRING& cur_w = a->get_target_word(j);
			 RuleRHS_el *newE 
				= new RuleRHS_el(Variable::UNALIGNEDC,cur_w,j,j,-1);
			 new_els.push_back(newE);
		  }
		  new_els.push_back(_els[i]);
		  iprev = i;
		}
	 }
	 // Unaligned items before:
	 int s = new_els[0]->_span.first;
	 //for(int j=start; j<s; ++j) {
	 for(int j=s-1; j>=start; --j) {
		// Add chinese at position j to the RHS:
		const STRING& cur_w = a->get_target_word(j);
		RuleRHS_el *newE 
		  = new RuleRHS_el(Variable::UNALIGNEDC,cur_w,j,j,-1);
		new_els.push_front(newE);
	 }
	 // Unaligned items after:
	 assert(new_els.size() > 0);
	 int e = new_els[new_els.size()-1]->_span.second;
	 for(int j=e+1; j<=end; ++j) {
		// Add chinese at position j to the RHS:
		const STRING& cur_w = a->get_target_word(j);
		assert(cur_w != "");
		RuleRHS_el *newE 
		  = new RuleRHS_el(Variable::UNALIGNEDC,cur_w,j,j,-1);
		new_els.push_back(newE);
	 }
	 // Done. Copy the new rhs:
	 _els = new_els;
  }

  // Daniel
  std::string RuleRHS::get_str(void)
  {
    std::string rhs_str;
    for(int i=0, size=get_size(); i<size; ++i) {
      if(i>0) rhs_str += " ";
      const RuleRHS_el* el = get_element(i);
      int var_index = el->get_var_index();
      if(Variable::is_variable_index(var_index)) {
	std::stringstream ss;
	ss << var_index;
	rhs_str += "x" + ss.str();
      } else {
	assert(el->get_lex() != "");
	rhs_str += "\"";
	rhs_str += el->get_lex().c_str();
	rhs_str += "\"";
      }
    }
    return rhs_str;
  }

  // Daniel
int RuleRHS::get_no_vars(void)
  {
    int no=0;
    for(int i=0, size=get_size(); i<size; ++i) {
      const RuleRHS_el* el = get_element(i);
      int var_index = el->get_var_index();
      if(Variable::is_variable_index(var_index)) 
	++no;
    }
    return no;
  }



}
