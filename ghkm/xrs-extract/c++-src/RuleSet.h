#ifndef __RULESET_H__
#define __RULESET_H__

/***************************************************************************
 * RuleSet.C/h
 * 
 * This class is used to represent a set of rules and define functions
 * that require a set of rules to be put together, e.g. assigning 
 * probabilities to them.
 *
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: RuleSet.h,v 1.1.1.1 2006/03/05 09:20:26 mgalley Exp $
 ***************************************************************************/

#include <iostream>
#include <vector>
#include <cassert>
#include <set>
#include <new>

#include "defs.h"

namespace mtrule {

class RuleSet {

 public:
  
 /***************************************************************************
  * Constructors and descructors:
  **************************************************************************/
 
  // Construct a Rule set, initially empty:
  RuleSet(bool count_mode, int size=0);
  ~RuleSet();

  void add_count(int ruleID, float c) { 
    check_resize(ruleID); 
	 _count[ruleID] += c;
  }
  void add_fraccount(int ruleID, float c) { 
    check_resize(ruleID); 
    _fraccount[ruleID] += c;
  }
  float get_count(int ruleID) const { return _count[ruleID]; }
  float get_fraccount(int ruleID) const { return _fraccount[ruleID]; }
  double get_prob(int ruleID) const;
  double get_logprob(int ruleID) const;
  int   get_size() const { return _size; }
  void print_counts(std::ostream& out) const;
  void load_probs(const char* file_name);

 protected:
 
  void check_resize(int ruleID) {
    assert(ruleID != 0);
	 int old_size = _size;
    if(ruleID >= _size) {
	   _size = ruleID+1;
		std::cerr << "Resizing rule count tables from size " 
		          << old_size << " to size " 
					 << _size << " (capacity=" << _fraccount.capacity() 
					 << ")" << "." << std::endl;
#ifdef COLLECT_COUNTS
		_count.resize(_size);
#endif
#ifdef COLLECT_FRAC_COUNTS
		_fraccount.resize(_size);
#endif
	 }
  }
 
 /***************************************************************************
  * Member variables:
  **************************************************************************/

 std::vector<float> _count;
 std::vector<float> _fraccount;
 std::vector<float> _prob;
 int _size;

 /***************************************************************************
  * Static members:
  **************************************************************************/

private:
 // Make sure this is never used by accident:
 RuleSet(const RuleSet& s);

};

}

#endif
