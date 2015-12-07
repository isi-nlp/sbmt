#ifndef __ATS_H__
#define __ATS_H__

/***************************************************************************
 * ATS.C/h
 * 
 * This class represents a set of alignment templates (constructed by
 * reading ATS files).
 *
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: ATS.h,v 1.1.1.1 2006/03/05 09:20:26 mgalley Exp $
 ***************************************************************************/

#include <string>
#include <fstream>

#include "hashutils.h"

namespace mtrule {

  using std::string;

  // TODO: to make these representations more efficient, use integers
  // (in combination with DerivationNode.h, TreeNode.h, and RuleNode.h)

  // Simple structure to represent a phrase:
  struct Phrase {
    // Member vars:
    std::vector<std::string*> _ws;
	 // Member functions:
	 Phrase(int size) { _ws.reserve(size); } 
	 ~Phrase() { for(int i=0,s=_ws.size();i<s;++i) if(_ws[i]) delete _ws[i]; }
	 void add(const string& w) { _ws.push_back(new string(w)); }
	 int size() const { return _ws.size(); }
	 const string& el(int i) const { return *_ws[i]; }
	 std::string get_str() const { 
	   string str;
	   for(int i=0,s=_ws.size();i<s;++i) {
		  if(i>0) str += " "; 
		  str += *_ws[i];
		}
		return str;
	 }
  };
  
  // Phrase pair in an alignment template:
  struct PhrasePair {
    // Member vars:
	 Phrase *_s, *_t;
	 double _p;
	 int _i;
	 PhrasePair(Phrase *s,Phrase *t,double p,int i) : _s(s),_t(t),_p(p),_i(i) {}
    ~PhrasePair() { delete _s; delete _t; } 
	 int ssize() const { return _s->size(); }
	 int tsize() const { return _t->size(); }
  };
  
  // Type used to represent ATs:
  typedef hash_multimap<std::string,PhrasePair*,hash_str> ATentries;
  typedef ATentries::iterator ATentries_it;
  typedef ATentries::const_iterator ATentries_cit;

  class ATS {

    public:
     void read_ATS_file(std::ifstream& is);
	  const ATentries& get_entries() { return _entries; }
	  ~ATS();

    protected:
     ATentries _entries;
  };

}

#endif
