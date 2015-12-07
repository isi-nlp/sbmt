/*!
 * \file Tree.h
 * \author Bryant Huang
 * \date 6/24/04
 */
// $Id: Tree.h,v 1.2 2005/10/10 17:37:47 wwang Exp $

#ifndef TREE_H
#define TREE_H

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

#include <treelib/TreeNode.h>
#include <treelib/tree.hh>
//#include <pcre++.h>

using namespace std;
//using namespace pcrepp;

///// INTERFACE /////

namespace treelib
{

/*!
 * \brief Basic tree
 *
 * The Tree_ class is a wrapper around <a
 * href="http://www.damtp.cam.ac.uk/user/kp229/tree/">Kasper
 * Peeters</a>' tree.hh STL-like C++ tree class containing templated
 * nodes.
 */
template<typename T>
class Tree_ : public tree<T>
{
    typedef tree<T>  _base;

 public:

    //typedef _base::iterator_base      iterator_base;
    //typedef _base::pre_order_iterator pre_order_iterator;
    //typedef _base::sibling_iterator   sibling_iterator;

  /*!
   * \brief The constructor initializes the tree with the given parser format
   * \param f parser format of tree (default = Radu)
   */
  Tree_(int f = RADU) : format(f), lowercasing(true) {}

  /*!
   * \brief Reads in a tree from raw input and builds the tree internally
   * \param raw raw tree
   * \return true if the tree was read in and built successfully
   */
  bool read(std::string raw);

  /*!
   * \brief Convert Penn Treebank tokenization to decoder tokenization
   */
  void convertPTBToMT();

  /*!
   * \brief Convert decoder tokenization to Penn Treebank tokenization
   */
  void convertMTToPTB();

  /*!
   * \brief Toggles automatic lowercasing of leaves during tree read-in
   */
  void enableLowercasing(bool b) { lowercasing = b; }

  /*!
   * \brief Returns a string representation of the leaves of the tree
   * \return string representation of leaves
   */
  std::string toString();

// protected:
  std::ostream& toStream(std::ostream& os) const;

  /*!
   * \brief Prints an internal tree object in Radu parser format: (NPB (DT this) (NN year) )
   */
  bool printRadu(std::ostream& os, typename treelib::Tree_<T>::pre_order_iterator it) const;

  /*!
   * \brief Prints an internal tree object in Collins parser format: (NPB DT/this NN/year)
   */
  void printCollins(std::ostream& os, typename treelib::Tree_<T>::pre_order_iterator it) const;

  /*!
   * \brief Reads in a tree from raw input (Radu parser format) and
   *        builds the tree internally
   * \param raw raw tree represented in Radu parser format
   * \return true if the tree was read in and built successfully
   */
  bool readRadu(std::string raw);

  /*!
   * \brief Reads in a tree from raw input (Collins parser format) and
   *        builds the tree internally
   * \param raw raw tree represented in Collins parser format
   * \return true if the tree was read in and built successfully
   */
  bool readCollins(std::string raw);

  /*!
   * \brief Recalculates head position and number of children for each relevant node
   *        since certain punctuation are skipped over in Radu and Collins parsers
   */
  void fixIndices();

  /*!
   * \brief Propagates headword and head POS tag up from leaves of Radu and Collins parser input
   * \param it iterator pointing to node in tree from which to begin
   */
  void propagateHeadInfo(typename treelib::Tree_<T>::pre_order_iterator it);

  /*!
   * \brief Lowercases leaves and headwords
   */
  void lowercaseLeaves();

  int format;        //!< parser format of tree (choose from ParserFormat enum)
  bool lowercasing;  //!< true if automatic lowercasing of leaves during tree read-in
};


/*!
 * \brief Basic tree that contains TreeNode nodes
 * Made into a class by Wei Wang.
 */
//typedef Tree_<TreeNode> Tree;
class Tree : public Tree_<TreeNode>
{
    typedef Tree_<TreeNode>  _base;
    typedef TreeNode NodeType;

public:
    //typedef _base::iterator_base      iterator_base;
    //typedef _base::pre_order_iterator pre_order_iterator;
    //typedef _base::sibling_iterator   sibling_iterator;

    Tree(int f = RADU) : _base(f) {}

    //! Outputs the CFG rules that can be decomposed from the tree.
    void getCFGRules(ostream& out, const string delimiter = "\n") const;

protected:

    //! Returns the CFG rule corresponding to a tree node.
    string getCRFRule(const iterator_base& it) const;
};



///// IMPLEMENTATION /////

template<typename T>
bool treelib::Tree_<T>::read(std::string raw)
{
  bool success;

  switch (format)
  {
  case COLLINS:
    success = readCollins(raw);
    break;
  default:
    success = readRadu(raw);
  }

  // perform postprocessing if tree exists
  if (this->size() > 0 )
  {
    fixIndices();
    propagateHeadInfo(this->begin());
    if (lowercasing) lowercaseLeaves();
  }

  return success;
}

template<typename T>
bool treelib::Tree_<T>::readRadu(std::string raw)
{
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
    /*if (buffer[0] == '(' && buffer!= "()" && buffer!= "(__LW_AT__)" &&
			                     buffer!= "__LW_AT__()"  && 
								buffer!= "__LW_AT_(__LW_AT__)"  ) {
    */
    if (buffer[0] == '(' && buffer!= "()" && buffer[buffer.length() - 1] != ')') {
    //if (buffer[0] == '(' && buffer != "()" && buffer != "(__LW_)" ) {
      //cout<<"AAAAAAA: "<<buffer<<endl;
	lrbCount++;   // ignore special case of (-LRB- ()
    }
    //if (buffer == ")" || buffer[buffer.length() - 1] == ')') {
    if (buffer == ")" || buffer[buffer.length() - 1] == ')') {
      //cout<<"BBBB: "<<buffer<<endl;
	rrbCount++;
    }
    tokens.push_back(buffer);
  }
  if (lrbCount != rrbCount)
  {
  //cout<<"EEEEE"<<lrbCount<<" " <<rrbCount<<endl;
    std::cerr << "Error in treelib::Tree_<T>::readRadu(): incomplete tree" << std::endl;
    return false;
  }

  //cout<<"EEEEE"<<lrbCount<<" " <<rrbCount<<endl;
  
  // create tree representation
  std::string token;
  typename treelib::Tree_<T>::pre_order_iterator it = this->begin();

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

    /*
    if (token[0] == '(' && token != "()" && token != "(__LW_AT__)" && 
			          token != "__LW_AT__()"  && 
				  token!= "__LW_AT_(__LW_AT__)" ) */       // type 1: first char is open paren; ignore special case of (-LRB- ()
    if(token[0] == '(' && token != "()" && token[token.length()-1]!=')') {
      if (prevSeen != "") return false;
      prevSeen = token;
    }
    else if (token == ")")                       // type 2: close paren
    {
      if (prevSeen != "") return false;
      it = _base::parent(it);                           // set it to its parent

      prevSeen = "";
    }
    else if (token[token.length() - 1] == ')')   // type 3: last char is close paren
    {
      if (prevSeen == "") return false;
      it = _base::append_child(it, T(prevSeen, format));        // append previously seen as parent

      strippedToken = token.substr(0, token.length() - 1);
      // we need to pass the terminal information because
      // there is no way to guarantee the 100% correctness
      // by looking the token itself.
      _base::append_child(it, T(strippedToken, true, format));        // append child but don't move it down
      it = _base::parent(it);                           // set it to its parent

      prevSeen = "";
    }
    else
    {
      if (prevSeen != "")                        // type 4: number
      {
	std::stringstream ss(token);
	ss >> prob;                              // save prob

	T tn(prevSeen);
	tn.setInsideProb(prob);

	if (this->empty())
	{
	  it = _base::set_head(tn);         // set head if tree is empty
	}
	else
	{
	  it = _base::append_child(it, tn); // append child and move it down
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

// original code by Ignacio Thayer
template<typename T>
bool treelib::Tree_<T>::readCollins(std::string raw)
{
  this->clear();
  
  if (raw == "") return true;  // TODO: detect bad Collins parse

  // store tokens from tree  
  std::stringstream ss(raw);
  std::vector<std::string> tokens;
  std::string buffer;
  int lrbCount = 0; // ( count
  int rrbCount = 0; // ) count
  while (ss >> buffer)
  { 
    if (buffer[0] == '(') lrbCount++;
    if (buffer == ")" || buffer[buffer.length() - 1] == ')') rrbCount++;
    tokens.push_back(buffer);
  }
  if (lrbCount != rrbCount)
  { 
    std::cerr << "Error in treelib::Tree_<T>::readCollins(): mismatched parens: " << __FILE__ << ":" << __LINE__ << std::endl;
    return false;
  }

  // create tree representation  
  std::string token;
  typename treelib::Tree_<T>::pre_order_iterator treeIt = this->begin();
  
  std::string strippedToken;
  
  // In Collins' format, there are 3 token types to handle:
  // 1. (S~denies~2~2
  // 2. Spain/NNP
  // 3. )
  
  for (std::vector<std::string>::iterator tokIt = tokens.begin(); tokIt != tokens.end(); tokIt++)
  {
    token = (*tokIt);

    if (token[0] == '(') 	// type 1
    {
      if (this->empty()) treeIt = _base::set_head(T(token, format));
      else treeIt = _base::append_child(treeIt, T(token, format));
    }
    else if (token == ")")      // type 3
    {
      treeIt = _base::parent(treeIt);
    }
    else 			// type 2
    {
      int slashIndex = token.find_last_of("/");
      std::string lexeme = token.substr(0, slashIndex);
      std::string tag = token.substr(slashIndex + 1, token.length());
      typename treelib::Tree_<T>::pre_order_iterator preTerminal = _base::append_child(treeIt, T(tag, format));
      
      _base::append_child(preTerminal, T(lexeme, format));
    }
  }

  return true;
}

template<typename T>
void treelib::Tree_<T>::fixIndices()
{
  typename treelib::Tree_<T>::pre_order_iterator it = this->begin();
  int skipPosition;
  int truePosition;
  int headPosition;
  std::string label;
  int assumedNumChildren;
  while (it != this->end())
  {
    assumedNumChildren = it->getNumChildren();
    it->setNumChildren(it.number_of_children()); // set the number of children

    headPosition = it->getHeadPosition();
    if ((headPosition > 0 or not it->getIsPreterminal()) and assumedNumChildren != it->getNumChildren())     // contains headword
    {
      skipPosition = 0;
      truePosition = 1;
      bool headset = false;

      // find true headword position
      for (typename treelib::Tree_<T>::sibling_iterator si = _base::child(it, 0); si != si.end(); si++)
      {
          label = si->getLabel();
          if (label != "``" && label != "''" && label != "." &&
              label != "," && label != ":" && label.find("PUNC") == std::string::npos) { 
                  skipPosition++;
          }
          if (skipPosition == headPosition and not headset) {
              it->setHeadPosition(truePosition);       // reassign headword position
              headset = true;
          }
          truePosition++;
      }
      if (truePosition - 1 != it->getNumChildren()) throw std::runtime_error("bad parse -- headword/size markers nonsensical");
    }
    it++;
  }
}

template<typename T>
void treelib::Tree_<T>::propagateHeadInfo(typename treelib::Tree_<T>::pre_order_iterator it)
{
  if (it.number_of_children() == 0)  // leaf
  {
    typename treelib::Tree_<T>::pre_order_iterator _parent = _base::parent(it);
    typename treelib::Tree_<T>::pre_order_iterator grandparent = _base::parent(_parent);
    if (grandparent->getHeadPosition() - 1 == _base::index(_parent))   // leaf is head child of grandparent
    {
      grandparent->setHeadword(it->getLabel());   // propagate up from leaf to grandparent
      grandparent->setHeadPOS(_parent->getLabel());
    }
  }

  // recurse through children
  typename treelib::Tree_<T>::sibling_iterator si = _base::child(it, 0);
  while (si != si.end())
  {
    propagateHeadInfo(si);

    // determine if VB* is among children
    if (si->getHasVerb() || si->getLabel().substr(0, 2) == "VB") it->setHasVerb(true);

    si++;
  }

  // assign headword if none assigned yet
  if (it->getHeadPosition() > 0 && it->getHeadword() == "")
  {
    if (it->getHeadPosition() > it.number_of_children())
    {
      std::cerr << "Error in treelib::Tree_<T>::propagateHeadInfo(): head position (" << it->getHeadPosition() << ") > number of children (" << it.number_of_children() << ')' << std::endl;
      return;
    }

    T thisHead = *_base::child(it, it->getHeadPosition() - 1);  // retrieve head child
    it->setHeadword(thisHead.getHeadword());  // propagate up from head child
    it->setHeadPOS(thisHead.getHeadPOS());
  }
}

template<typename T>
void treelib::Tree_<T>::lowercaseLeaves()
{
  typename treelib::Tree_<T>::pre_order_iterator it = this->begin();
  std::string s;
  for (; it != this->end(); it++)
  {
    if (it.number_of_children() == 0)  // lowercase leaves
    {
      s = it->getLabel();
      transform(s.begin(), s.end(), s.begin(), (int(*)(int))std::tolower);
      it->setLabel(s);
    }
    s = it->getHeadword();             // lowercase headwords
    if (s != "")
    {
      transform(s.begin(), s.end(), s.begin(), (int(*)(int))std::tolower);
      it->setHeadword(s);
    }
  }
}

template<typename T>
void treelib::Tree_<T>::convertPTBToMT()
{
  typename treelib::Tree_<T>::pre_order_iterator it = this->begin();
  std::string thisLabel;
  std::string parentLabel;
  int dashIndex;
  int numLeaves = 0;
  for (; it != this->end(); it++)
  {
    if (it.number_of_children() == 0)  // leaf
    {
      numLeaves++;

      thisLabel = it->getLabel();
      parentLabel = _base::parent(it)->getLabel();

      // convert initial -- to *
      if (numLeaves == 1 && thisLabel == "--")
      {
	it->setLabel("*");
      }

      // convert tokens
      if (thisLabel == "``")
      {
	it->setLabel("\"");
      }
      else if (thisLabel == "''")
      {
	it->setLabel("\"");
      }
      else if (thisLabel == "-lrb-" || thisLabel == "-LRB-")
      {
	it->setLabel("(");
      }
      else if (thisLabel == "-rrb-" || thisLabel == "-RRB-")
      {
	it->setLabel(")");
      }
      else if (thisLabel == "-lcb-" || thisLabel == "-LCB-")
      {
	it->setLabel("{");
      }
      else if (thisLabel == "-rcb-" || thisLabel == "-RCB-")
      {
	it->setLabel("}");
      }

      // break up hyphened words into individual words and change - to @-@
      // (JJ never-before-seen) -> (JJ 0.00000 (JJ never) (JJ @-@) (JJ before) (JJ @-@) (JJ seen) )
      // NOTE: @-@ is split into @, @-@, @
      dashIndex = thisLabel.find("-", 1);        // TODO: ignore cases of "9-11" (digits surrounding)
      if (dashIndex >= 1 && dashIndex < thisLabel.length() - 1)   // must be an internal dash
      {
 	it->setLabel(parentLabel);             // create parent node
	it->setIsVirtual(true);
 	it = _base::append_child(it, T("-DUMMY-", format));  // append dummy child

	while (true)
 	{
 	  it->setLabel(thisLabel.substr(0, dashIndex));           // set this label to pre-dash data
 	  thisLabel = thisLabel.substr(dashIndex + 1);            // save post-dash data
 	  it = _base::insert_after(_base::parent(it), T(parentLabel, format));  // create uncle to the right
	  it->setIsVirtual(true);
 	  _base::append_child(it, T("@-@", format));                     // to uncle, add @-@ child
 	  it = _base::insert_after(it, T(parentLabel, format));          // create uncle to the right
	  it->setIsVirtual(true);
 	  it = _base::append_child(it, T(thisLabel, format));            // to uncle, add new label

 	  // check if more hyphens to process
 	  dashIndex = thisLabel.find("-", 1);        // TODO: ignore cases of "9-11" (digits surrounding)
 	  if (dashIndex < 1 || dashIndex >= thisLabel.length() - 1) break;  // must be an internal dash
 	}
      }
    }
  }
}

template<typename T>
void treelib::Tree_<T>::convertMTToPTB()
{
  typename treelib::Tree_<T>::pre_order_iterator it = this->begin();
  std::string thisLabel;
  std::string parentLabel;
  int dashIndex;
  int numLeaves = 0;
  bool matchingUncles;
  std::string rightLabel;
  bool justMerged;
  typename treelib::Tree_<T>::sibling_iterator sit;
  for (; it != this->end(); it++)
  {
    if (it.number_of_children() == 0)  // leaf
    {
      numLeaves++;

      thisLabel = it->getLabel();
      parentLabel = _base::parent(it)->getLabel();

      // convert initial * to --
      if (numLeaves == 1 && thisLabel == "*")
      {
	it->setLabel("--");
      }

      // convert tokens
      if (thisLabel == "\"")
      {
	if (format == RADU)
	{
	  if (parentLabel == "``") it->setLabel("``");
	  if (parentLabel == "''") it->setLabel("''");
	}
	else if (format == COLLINS)
	{
	  if (parentLabel == "PUNC``") it->setLabel("``");
	  if (parentLabel == "PUNC''") it->setLabel("''");
	}
      }
      else if (thisLabel == "(")
      {
	if (lowercasing) it->setLabel("-lrb-");
	else it->setLabel("-LRB-");
      }
      else if (thisLabel == ")")
      {
	if (lowercasing) it->setLabel("-rrb-");
	else it->setLabel("-RRB-");
      }
      else if (thisLabel == "{")
      {
	if (lowercasing) it->setLabel("-lcb-");
	else it->setLabel("-LCB-");
      }
      else if (thisLabel == "}")
      {
	if (lowercasing) it->setLabel("-rcb-");
	else it->setLabel("-RCB-");
      }

      // join hyphen-separated words into a single word
      // (JJ 0.00000 (JJ never) (JJ @-@) (JJ before) (JJ @-@) (JJ seen) ) -> (JJ never-before-seen)
      justMerged = false;
      if (thisLabel == "@-@")
      {
	// have same parent label
	sit = _base::parent(it);
	if (index(sit) >= 1 && index(sit) < _base::parent(sit).number_of_children() - 1)  // look if left uncle and right uncle exist
	{
	  matchingUncles = true;
	  sit--;   // move to left uncle
	  if (sit->getLabel() != parentLabel) matchingUncles = false;
	  sit++;   // move to right uncle
	  sit++;
	  if (sit->getLabel() != parentLabel) matchingUncles = false;
	  if (matchingUncles)  // both uncles have same label as parent
	  {
	    rightLabel = _base::child(sit, 0)->getLabel();  // get right cousin's label
	    sit = erase(sit);  // erase right uncle
	    sit--;
	    sit = erase(sit);  // erase this @-@
	    sit--;
	    it = _base::child(sit, 0);                               // reassign it
	    it->setLabel(it->getLabel() + "-" + rightLabel);  // append "-" and right cousin's label to left cousin's label

	    justMerged = true;
	  }
	}
      }
      if (justMerged && index(_base::parent(it)) >= _base::parent(_base::parent(it)).number_of_children() - 1)  // no more @-@ to merge
      {
	if (_base::parent(_base::parent(it)).number_of_children() > 1) std::cerr << "Error in treelib::Tree_<T>::convertMTToPTB(): @-@ merging process failed" << std::endl;

	// move node up
	it = _base::parent(it);
	it->setLabel(_base::child(it, 0)->getLabel());
	erase_children(it);
      }
    }
  }
}

template<typename T>
std::string treelib::Tree_<T>::toString()
{
  std::string s = "";
  typename treelib::Tree_<T>::pre_order_iterator it = this->begin();
  for (; it != this->end(); it++)
  {
    if (it.number_of_children() == 0)  // leaf
    {
      if (s != "") s += " ";
      s += it->getLabel();
    }
  }
  return s;
}

template<typename T>
bool treelib::Tree_<T>::printRadu(std::ostream& os, typename treelib::Tree_<T>::pre_order_iterator it) const
{
  if (this->empty())
  {
    std::cout << 0;    // default Radu parser behavior
    return true;
  }

  if (!this->is_valid(it)) return false;
  
  if (it.number_of_children() == 0)
  {
    os << *it;
    return true;
  }
  
  os << '(' << *it;

  typename treelib::Tree_<T>::sibling_iterator si = _base::child(it, 0);
  int i = 0;
  bool isLexical;
  while (si != si.end())
  {
    if (si != si.begin()) os << ' ';   // only print middle spaces, not final
    isLexical = printRadu(os, si);
    si++;
    i++;
  }

  if (!isLexical) os << ' ';
  os << ')'; 

  return false;
}

template<typename T>
void treelib::Tree_<T>::printCollins(std::ostream& os, typename treelib::Tree_<T>::pre_order_iterator it) const
{
  if (this->empty())
  {
    std::cout << std::endl;   // print blank line
    return;
  }

  if (!this->is_valid(it)) return;
  
  if (it.number_of_children() != 0)
  {
    if (it->getHeadword() == "" && it->getLabel() != "TOP")   // preterminal; TOP w/o headword = Collins failed parse
    {
      os << *_base::child(it, 0) << '/' << *it;  // print 'terminal/preterminal'
      return;
    }
    else
    {
      os << '(' << *it;
    }
  }

  typename treelib::Tree_<T>::sibling_iterator si = _base::child(it, 0);
  int i = 0;
  while (si != si.end())
  {
    if (si != si.begin()) os << ' ';   // only print middle spaces, not final
    printCollins(os, si);
    si++;
    i++;
  }

  os << " )";
}

template<typename T>
std::ostream& treelib::Tree_<T>::toStream(std::ostream& os) const
{
  switch (format)
  {
  case COLLINS:
    printCollins(os, this->begin());
    break;
  default:
    printRadu(os, this->begin());
  }
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const treelib::Tree_<T> &tr)
{
  return tr.toStream(os);
}

}  // namespace treelib

#endif
