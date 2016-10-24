#ifndef _Relabeler_H_
#define _Relabeler_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ext/hash_set>
#include <getopt.h>
#include "SyntaxTree.h"

using namespace std;
using namespace __gnu_cxx;
namespace __gnu_cxx{
    template<> struct hash<string> {
	size_t operator()(const string x) const{
	  return hash<const char*>()(x.c_str());
	}
    };

}


class Relabeler
{
public:

    Relabeler(string config);
    virtual ~Relabeler() {}
  
    void relabel(string parseFile);
private:
    void relabel(Tree& tree);

    hash_set<string> m_labels;
};



#endif
