#ifndef ROOT_HW_HPP
#define ROOT_HW_HPP

# include <string>
// constants and utilities for root_hw_count and root_hw_prob

using namespace std;

// form a string out of two, for two different mapping functions
string joinkey(string a, string b) {
  return a+"_"+b;
}

// TODO: unify with rule_head/rule_head_reducer which has matching hard code

static const std::string RULE_HEAD_TAG_DISTRIBUTION_FEATURE = "htdist" ;
static const std::string RULE_HEAD_WORD_DISTRIBUTION_FEATURE = "hwdist" ;
#endif
