
/***************************************************************************
 * RuleSet.C/h
 * 
 * This class is used to represent a set of rules and define functions
 * that require a set of rules to be put together, e.g. assigning 
 * probabilities to them.
 *
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: RuleSet.C,v 1.3 2009/10/12 21:57:36 pust Exp $
 ***************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cfloat>
#include <cmath>
#include <gzstream.h>
#include <cstdlib>
#include <cstring>
#include "RuleSet.h"
#include "WeightF.h"
#include "MyErr.h"

namespace mtrule {

RuleSet::RuleSet(bool count_mode, int size) : _size(size) {
  int reserve_size = NB_RULES;
  if(count_mode) {
	 if(size < reserve_size) {
#ifdef COLLECT_COUNTS
		*myerr << "Reserving " << reserve_size << std::endl;
		_count.reserve(reserve_size);
#endif
#ifdef COLLECT_FRAC_COUNTS
		*myerr << "Reserving " << reserve_size << std::endl;
		_fraccount.reserve(reserve_size);
#endif
	 }
	 _count.resize(size);
	 _fraccount.resize(size);
  } else {
	 if(size < reserve_size) {
	   _prob.reserve(NB_RULES);
	 }
	 _prob.resize(size);
  }
}

void 
RuleSet::print_counts(std::ostream& out) const {
  int size = std::max(_count.size(),_fraccount.size());
  *myerr << "printing counts (size=" << size << ")" << std::endl;
  for(int i=1; i<size; ++i) {
    std::stringstream ss;
#ifdef COLLECT_COUNTS
    out << std::setprecision(8) << _count[i];
#ifdef COLLECT_FRAC_COUNTS
	 out << ",";
#endif 
#endif
#ifdef COLLECT_FRAC_COUNTS
	 out << std::setprecision(8) << _fraccount[i];
#endif
	 out << std::endl;
  }
}

void
RuleSet::load_probs(const char* file_name) {
  *myerr << "%%% loading probability file: " << file_name << std::endl;
  // Read a prob file:
  //std::ifstream in(file_name);
  gz::igzstream in(file_name);
  if(in.fail()) {
	 *myerr << "Error opening file : " << file_name << std::endl;
	 exit(1);
  }

#ifdef COLLECT_COUNTS
  assert(_count.size() == 0);
  _count.push_back(0.0);
#endif 
#ifdef COLLECT_FRAC_COUNTS
  assert(_fraccount.size() == 0);
  _fraccount.push_back(0.0);
#endif 

  int line=1;
  std::string tmp;
  while(!in.eof()) {
	 getline(in,tmp); 

	 if(tmp.length() == 0) 
	   continue;
	 if(strncmp(tmp.c_str(),WeightF::header,strlen(WeightF::header)) == 0 ||
		strncmp(tmp.c_str(),WeightF::comment,strlen(WeightF::comment)) == 0) {
	   continue;
	 }
	 // Process each token: 
	 //size_t pos1=0, pos2=tmp.size()-1;
	 //unsigned int field_num = 0;
	 float c = atof(tmp.c_str());
	 //if(c == 0.0)
	 //  c = FLT_MIN;
	 // Store data in memory:
	 if(line >= static_cast<int>(_prob.size())) {
		_prob.resize(line+1);
	 }
	 _prob[line] = c;
	 ++line;
  }
  _size = _prob.size();
  in.close();
  *myerr << "%%% done." << std::endl;
}

double
RuleSet::get_prob(int ruleID) const { 
  if(ruleID == -1)
    return DBL_MIN;
  double p = _prob[ruleID];
  return (p==0 || p!=p) ? DBL_MIN : p;
}

double
RuleSet::get_logprob(int ruleID) const { 
  if(ruleID == -1)
    return -DBL_MAX;
  double p = _prob[ruleID];
  return (p==0 || p!=p) ? -DBL_MAX : log(p);
}

}
