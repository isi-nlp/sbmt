#ifndef _RegexRestructurer_H_
#define _RegexRestructurer_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <getopt.h>
#include "SyntaxTree.h"
#include "pcre++.h"

using namespace std;
using namespace pcrepp;


class RegexRestructurer
{
public:

    RegexRestructurer(string regex, string newroot);
    virtual ~RegexRestructurer() {}
  
    //! Restructure the children of this node according to the
    //! regular expresssion.
    void restructure(Tree::pre_order_iterator it, Tree& tree);

private:

    string getIDString(Tree::pre_order_iterator it, Tree& tree) const;
    Pcre m_regex;
    string m_newroot;

};


#endif
