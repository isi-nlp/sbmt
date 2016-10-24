#ifndef _Transformer_H_
#define _Transformer_H_

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ext/hash_map>
#include <vector>
#include "getopt.h"
#include "tree.h"
#include "RegexRestructurer.h"
#include "SyntaxTree.h"

using namespace std;
using namespace __gnu_cxx;

namespace __gnu_cxx
{
  template<> struct hash<string>
  {
    size_t operator()(const string& x) const
    {
      return hash<const char*>()(x.c_str());
    }
  };
}


class Rule
{
 public:
  Rule() {}
  Rule(Tree l, Tree r) : lhs(l), rhs(r) {}

  Tree lhs;
  Tree rhs;
};

class Transformer
{
 public:
  Transformer(string configFilename, bool showRules = false);
  ~Transformer();

  void loadConfig(string configFilename);
  void loadConfigProperty(string property, string value);
  
  void process(string inputFilename);
  void loadRule(const string raw, int mode);

  void transduceTree(Tree& tr);
  bool checkMatch(Tree& trInput, Tree::pre_order_iterator itInput, Tree& trRule, Tree::pre_order_iterator itRule);
  void applyRule(Tree& tr, Tree::pre_order_iterator it, Rule r);
  string getID(const Tree& tr, Tree::pre_order_iterator it) const;

  Tree getTree(const string raw);
  Tree getTree(const vector<string> tokens);
  Tree getTTETree(const vector<string> tokens);

  void printRule(Rule& r);
  void printRules(int mode);
  bool printPennTree(const Tree& tr, Tree::pre_order_iterator it);
  bool printTree(const Tree& tr, Tree::pre_order_iterator it, Tree::pre_order_iterator end);

  void printWarning(const string message);
  void printError(const string message);

  int verbosity;
  bool showTransformed;
  bool showLineNos;
  bool showRules;

 protected:
  hash_map<string, vector<Rule> > rules3plus;
  hash_map<string, vector<Rule> > rules2;
  int ruleCount;

  string baseDir;
  string rules3plusFilename;
  string rules2Filename;

  bool useRules2;
  bool useRules3plus;

  string inputFilename;


  vector<RegexRestructurer*> m_regexRestr;
};
#endif
