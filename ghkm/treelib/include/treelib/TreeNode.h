/*!
 * \file TreeNode.h
 * \author Bryant Huang
 * \date 6/24/04
 */
// $Id: TreeNode.h,v 1.1 2005/08/04 02:31:08 wwang Exp $

#ifndef TREENODE_H
#define TREENODE_H

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>

//#include <pcre++.h>

//using namespace pcrepp;

namespace treelib
{

/*!
 * \brief Parse tree formats accepted; used in both Tree and TreeNode
 */
enum ParserFormat
{
  RADU,       //!< Radu parser format
  COLLINS     //!< Collins parser format
};

/*!
 * \brief Basic tree node
 *
 * The TreeNode object contains all relevant information for each node
 * in the Tree.  This can be inherited from to augment the tree node
 * with additional information. See the README for more information.
 */
class TreeNode
{
 public:
  /*!
   * \brief The constructor initializes the label and the parser format
   * \param s label of tree node
   * \param f parser format of tree node (default = Radu)
   */
  TreeNode(std::string s, int f = RADU);
  TreeNode(std::string s, bool isTerminal, int f=RADU);

  // accessors
  std::string getLabel() { return label; }
  std::string getHeadword() { return headword; }
  std::string getHeadPOS() { return headPOS; }
  unsigned int getHeadPosition() { return headPosition; }
  bool getHasVerb() { return hasVerb; }
  int getNumChildren() { return numChildren; }
  double getInsideProb() { return insideProb; }
  double getOutsideProb() { return outsideProb; }
  bool getIsPreterminal() { return isPreterminal; }
  bool getIsTerminal() { return isTerminal; }
  bool getIsVirtual() { return isVirtual; }

  // mutators
  void setLabel(std::string s) { label = s; }
  void setHeadword(std::string s) { headword = s; }
  void setHeadPOS(std::string s) { headPOS = s; }
  void setHeadPosition(unsigned int i) { headPosition = i; }
  void setHasVerb(bool b) { hasVerb = b; }
  void setNumChildren(int i) { numChildren = i; }
  void setInsideProb(double d) { insideProb = d; }
  void setOutsideProb(double d) { outsideProb = d; }
  void setIsPreterminal(bool b) { isPreterminal = b; }
  void setIsTerminal(bool b) { isTerminal = b; }
  void setIsVirtual(bool b) { isVirtual = b; }

  friend std::ostream& operator<<(std::ostream& os, const TreeNode &tn);

 protected:
  std::ostream& toStream(std::ostream& os) const;
  std::ostream& printRadu(std::ostream& os) const;
  std::ostream& printCollins(std::ostream& os) const;

  int format;         //!< parser format of tree node (choose from ParserFormat enum)

  std::string label;       //!< label of grammatical category or lexical item
  std::string headword;    //!< headword given by parser
  std::string headPOS;     //!< POS tag of headword

  unsigned int headPosition;   //!< index of headword among children; leftmost child = 1
  bool hasVerb;       //!< true if the contains VB* among its children

  int numChildren;    //!< number of immediate children
  double insideProb;  //!< inside log probability
  double outsideProb; //!< outside log probability

  bool isPreterminal; //!< node is a preterminal, i.e., an immediate parent of a leaf
  bool isTerminal;    //!< node is a terminal, i.e., a leaf

  bool isVirtual;     //!< internal use only: node is virtually-created

  int labelID;
  int headwordID;
  int headPOSID;
};

std::ostream& operator<<(std::ostream& os, const TreeNode &tn);

}  // namespace treelib

#endif
