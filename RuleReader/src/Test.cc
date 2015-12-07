#include <iostream>
#include <fstream>
#include <RuleReader/Rule.h>
#include <RuleReader/RuleNode.h>
#include <ext/hash_map>
#include <sstream>

using namespace std;

void printNode(ns_RuleReader::RuleNode *curr, int depth) // print a RuleNode tree
{ 
  for (int counter=0;counter<depth;counter++) { // print out a tab
    cerr << " ";
  }

  if (curr->isNonTerminal()) { 	// if this is a nonterminal node
    if(curr->isPreTerminal()){
	cerr<<" [pre]";
    }
    cerr << "NT: " << curr->getString(false) << endl; 
    

    for (vector<ns_RuleReader::RuleNode *>::iterator it=curr->getChildren()->begin();it!=curr->getChildren()->end();it++) { // recurse on children
      printNode((*it), depth+1);
    } // end for each child
  } else if (curr->isLexical()) { // is this a lexical leaf node?
    cerr << "LEX: " << curr->getString() << endl;

  } else if (curr->isPointer()) { // is this a pointer leaf node?
    cerr << "POINTER: " << curr->getRHSIndex() << " " << curr->getString() << endl;

  }
}

using namespace ns_RuleReader;

int main(int argc, char *argv[])
{
  string line;
  Rule myRule;

  // for each rule line 
  while (getline(cin, line)) { 

    // skip comment line or empty line.
    if (line.find("$$$", 0) == 0 || line == "") { continue; }


    try { 
	/*
	 * Read the rule
	 */
        istringstream ist(line);
        ist >> myRule;

	/*
	 * Get rule attribute
	 */
	string attr =  myRule.getAttributeValue("count");
	cout<<"Rule count: "<<attr<<endl;
	
	/* 
	 * print out the tree string at the LHS.
	 */
        cout<< myRule.getLHSRoot()->treeString(" ") << endl;

	myRule.clear();
    } catch (const char*s) { 

      cerr << "Caught exception: " << s << endl;
      myRule.clear();
      continue;
    }
  }
}
