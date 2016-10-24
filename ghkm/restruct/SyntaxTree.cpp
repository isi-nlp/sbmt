#include "SyntaxTree.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "boost/regex.hpp"

using namespace std;
using namespace boost;

bool print(const ::Tree& tr, ::Tree::pre_order_iterator it, bool latexHead)
{
  if (!tr.is_valid(it)) return false;
	
  int numChildren = it.number_of_children();	
  if (numChildren == 0)
  {
    if (it->isState())
    {
		assert(0);
      cout << 'x' << it->sibling_index;
      if (it->getLabel() != "")
      {
	cout << ':' << it->getLabel();
      }
    }
    else
    {
      cout << it->getLabel();
    }
    return true;
  }

  if(latexHead){
	  if(tr.isHead(it)){
		  cout << "(\\bf{*" << it->getLabel()<<"}";
	  } else {
		  cout << "(" << it->getLabel();
	  }
  } else {
	  cout << "(" << it->getLabel();
  }
  Tree::sibling_iterator si = tr.child(it, 0);
  if (it.number_of_children() > 1 || (it.number_of_children() > 0 && si.number_of_children() > 0))
  {
	  if(it->getHeadPosition() <= it.number_of_children())  {
		cout << "~" << it.number_of_children() << "~"<<it->getHeadPosition()<<" "<<it->getInsideProb();
	  } else {
			cout << "~" << it.number_of_children() << "~"<<0<<" "<<it->getInsideProb();
	  }
  }
  cout << " ";
  int i = 0;
  bool isLast = false;
  while (si != si.end())
  {
    isLast = print(tr, si, latexHead);
    if (i < numChildren - 1) cout << " ";	// only print middle spaces, not final
    si++;
    i++;
  }
  if (!isLast) cout << " ";
  cout << ")";

  return false;
}

int Tree::read(const string raw) {
	if(!readRadu(raw)) {return -1;}
	  // perform postprocessing if tree exists
	  if (this->size() > 0 )
	  {
		fixIndices();
		propagateHeadInfo(this->begin());
		if (lowercasing) lowercaseLeaves();
	  }
	markHeads(begin());
	return 0;
}

bool Tree::readRadu(std::string raw)
{
  int num_children, head_position;
  int tmp_head_position;
  // clear tree
  this->clear();

  if (raw == "0" || raw == "") return true;  // if Radu parser does not parse, it outputs 0

  // store tokens from tree
  std::stringstream ss(raw);
  std::vector<std::string> tokens;
  std::string buffer;
  int lrbCount = 0;  // ( count
  int rrbCount = 0;  // ) count
  while (ss >> buffer)
  {
    if (buffer[0] == '(' && buffer != "()") {
	//cout<<"AAAAAAA: "<<buffer<<endl;
	lrbCount++;   // ignore special case of (-LRB- ()
    }
    if (buffer == ")" || buffer[buffer.length() - 1] == ')') {
	//cout<<"BBBB: "<<buffer<<endl;
	rrbCount++;
    }
    tokens.push_back(buffer);
  }
  if (lrbCount != rrbCount)
  {
 // cout<<"EEEEE"<<lrbCount<<" " <<rrbCount<<endl;
    std::cerr << "Error in treelib::Tree_<T>::readRadu(): incomplete tree" << std::endl;
    return false;
  }

  //cout<<"EEEEE"<<lrbCount<<" " <<rrbCount<<endl;
  
  // create tree representation
  std::string token;
  pre_order_iterator it = this->begin();

  std::string strippedToken;
  double prob = 0.0;
  std::string prevSeen = "";      // previously seen token

  // in Radu's format, there are 4 token types to handle:
  //   1. (TOP~1~1
  //   2. )
  //   3. risk)
  //   4. -115.21142
  for (unsigned int i = 0; i < tokens.size(); i++)
  {
    token = tokens[i];

    if (token[0] == '(' && token != "()")        // type 1: first char is open paren; ignore special case of (-LRB- ()
    {
      if (prevSeen != "") return false;
      prevSeen = token;
    }
    else if (token == ")")                       // type 2: close paren
    {
      if (prevSeen != "") return false;
      it = parent(it);                           // set it to its parent

      prevSeen = "";
    }
    else if (token[token.length() - 1] == ')')   // type 3: last char is close paren
    {
      if (prevSeen == "") return false;
      it = append_child(it, SynTreeNode(prevSeen, format));        // append previously seen as parent

      strippedToken = token.substr(0, token.length() - 1);
      append_child(it, SynTreeNode(strippedToken, format));        // append child but don't move it down
      it = parent(it);                           // set it to its parent

      prevSeen = "";
    }
    else
    {
      if (prevSeen != "")                        // type 4: number
      {
	std::stringstream ss(token);
	ss >> prob;                              // save prob
		  string str = prevSeen;
	prevSeen = prevSeen.substr(1);
	prevSeen = prevSeen.substr(0, prevSeen.find("~"));

	boost::regex re("~(\\d+)~(\\d+)");
	boost::cmatch what;
	string s1;
	if(regex_search(str.c_str(), what, re)){
		s1.assign(what[1].first, what[1].second);
		num_children = atoi(s1.c_str());
		s1.assign(what[2].first, what[2].second);
		head_position = atoi(s1.c_str());
	//	cout<<num_children<<": "<<head_position<<endl;
	} else {
  	    ++tmp_head_position;
	}

	SynTreeNode tn(prevSeen);
	tn.setInsideProb(prob);

	if (this->empty())
	{
	  it = set_head(tn);         // set head if tree is empty
	  it->setHeadPosition(head_position);
	}
	else
	{
	  it = append_child(it, tn); // append child and move it down
	  it->setHeadPosition(head_position);
	}
	prob = 0;

	prevSeen = "";
      }
      else
      {
	std::cerr << "Error in treelib::Tree_<T>::readRadu(): unexpected token `" << token << "' encountered" << std::endl;
	return false;
      }
    }
  }

  return true;
}

#if 0
int getTree(const vector<string> tokens, Tree& tr);
int getTree(const string raw, Tree& tr)
{
  if (raw == "0" || raw == "") return -1;  // if Radu parser does not parse, it outputs 0

  stringstream ss(raw);
  vector<string> tokens;
  int lrbCount = 0;  // ( count
  int rrbCount = 0;  // ) count
  string buffer;
  while (ss >> buffer)
  {
    if (buffer[0] == '(' && buffer != "()") lrbCount++;   // ignore special case of (-LRB- ()
    if (buffer == ")" || buffer[buffer.length() - 1] == ')') rrbCount++;
    tokens.push_back(buffer);
  }
  if (lrbCount != rrbCount)
  {
    cerr<<"incomplete tree"<<endl;
    return -1;
  }
  
  return getTree(tokens, tr);
}

//! 0 returned value means successfull.
//! otherwise, error.
int getTree(const vector<string> tokens, Tree& tr)
{

  int num_children, head_position;
  int tmp_head_position;

  // create tree representation
  int tree_index = -1;
  string token;
  Tree::pre_order_iterator it = tr.begin();

  string strippedToken;
  string prevSeen = "";      // previously seen token

  // in Radu's format, there are 4 token types to handle:
  //   1. (TOP~1~1
  //   2. )
  //   3. risk)
  //   4. -115.21142
  for (unsigned int i = 0; i < tokens.size(); i++)
  {

	  // if there is an empty node in the tree, like ( ), the tree reading
	  // is done. In this case, we return an error code.
	  if(i < tokens.size() -1 && tokens[i] == "(" && tokens[i+1] == ")"){
		  return -1;
	  }

    token = tokens[i];

    if (token[0] == '(' && token != "()")        // type 1: first char is open paren; ignore special case of (-LRB- ()
    {
      if (prevSeen != "") return 0;
      prevSeen = token;
    }
    else if (token == ")")                       // type 2: close paren
    {
      if (prevSeen != "") return 0;
      it = tr.parent(it);                           // set it to its parent

      prevSeen = "";
    }
    else if (token[token.length() - 1] == ')')   // type 3: last char is close paren
    {
      if (prevSeen == "") return 0;
      prevSeen = prevSeen.substr(1);
      it = tr.append_child(it, TreeNode(prevSeen, tree_index--));        // append previously seen as parent

      strippedToken = token.substr(0, token.length() - 1);
      tr.append_child(it, TreeNode(strippedToken, tree_index--));        // append child but don't move it down
      it = tr.parent(it);                           // set it to its parent

      prevSeen = "";
    }
    else
    {
      if (prevSeen != "")                        // type 4: number
      {
		  string str = prevSeen;
	prevSeen = prevSeen.substr(1);
	prevSeen = prevSeen.substr(0, prevSeen.find("~"));

	boost::regex re("~(\\d+)~(\\d+)");
	boost::cmatch what;
	string s1;
	if(regex_search(str.c_str(), what, re)){
		s1.assign(what[1].first, what[1].second);
		num_children = atoi(s1.c_str());
		s1.assign(what[2].first, what[2].second);
		head_position = atoi(s1.c_str());
	//	cout<<num_children<<": "<<head_position<<endl;
	} else {
  	    ++tmp_head_position;
	}

	if (tr.empty())
	{
	  it = tr.set_head(TreeNode(prevSeen, tree_index--));         // set head if tree is empty
	  it->head_posit = head_position;
	}
	else
	{
	  it = tr.append_child(it, TreeNode(prevSeen, tree_index--)); // append child and move it down
	  it->head_posit = head_position;
	}

	prevSeen = "";
      }
      else
      {
	cerr<<"unexpected token `"<< token <<"' encountered"<<endl;
	return -1;
      }
    }
  }

  tr.markHeads(tr.begin());



  return 0;
}
#endif


