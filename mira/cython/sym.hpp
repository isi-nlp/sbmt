#ifndef SYM_HPP
#define SYM_HPP

#include <string>
#include <sstream>
#include <cstdlib>
#include <cassert>
#include <boost/functional/hash.hpp>
#include "numberizer.hpp"

struct sym {
  static unsigned const int terminal = 0, nonterminal = 1;
  unsigned int type : 1;
  unsigned int num : 21; // should use a union instead
  static unsigned const int no_index = 0;
  unsigned int index : 10;

  sym() : type(terminal), num(0), index(no_index) { }
  sym(int x) { *reinterpret_cast<int *>(this) = x; }
  operator int() const { return *reinterpret_cast<const int *>(this); }

  bool operator==(const sym& other) const { return int(*this) == int(other); }
  bool operator!=(const sym& other) const { return int(*this) != int(other); }
};

class alphabet {
public:
  numberizer<std::string> terminals, nonterminals;

  std::string sym_to_string(const sym& x) {
    if (x.type == sym::terminal)
      return terminals.index_to_word(x.num);
    else {
      std::ostringstream ss;
      if (x.index != sym::no_index)
	ss << "[" << nonterminals.index_to_word(x.num) << "," << x.index << "]";
      else
	ss << "[" << nonterminals.index_to_word(x.num) << "]";
      return ss.str();
    }
  }

  sym string_to_sym(const std::string &s) {
    // a nonterminal looks like [X,1]
    sym x;
    if (s[0] == '[' && s[s.length()-1] == ']') {
      x.type = sym::nonterminal;
      std::string::size_type pos = s.rfind(',');
      if (pos != std::string::npos && s.length()-pos-2 > 0) {
	x.num = nonterminals.word_to_index(s.substr(1,pos-1));
	x.index = std::atoi(s.substr(pos+1,s.length()-pos-2).c_str());
	assert(x.index != sym::no_index);
      } else {
	x.num = nonterminals.word_to_index(s.substr(1,s.length()-2));
	x.index = sym::no_index;
      }
    } else {
      x.type = sym::terminal;
      x.num = terminals.word_to_index(s);
    }
    return x;
  }

  sym string_to_sym(const char *cs) {
    std::string s(cs);
    return string_to_sym(s);
  }

  sym cat_to_sym(const std::string &s) {
    sym x;
    x.type = sym::nonterminal;
    x.num = nonterminals.word_to_index(s);
    x.index = 0;
    return x;
  }

  std::string sym_to_cat(const sym &x) {
    assert(x.type == sym::nonterminal);
    return nonterminals.index_to_word(x.num); 
  }
};

#endif
