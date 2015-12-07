#ifndef __TREE_H__
#define __TREE_H__

#include "TreeNode.h"

namespace mtrule {

/************************************************************************
 * This is the representation of a tree structure, e.g. 
 * a parse tree. It just contains a link to the root of the tree 
 * (see the TreeNode.h class), and a vector of leaves (Leaves typedef).
 * 
 * $Id: Tree.h,v 1.2 2006/12/07 20:27:46 wwang Exp $
 ************************************************************************/

class Tree {
  public:

	 ///////////////////////////////////////////////////////////////////
	 // Constructors and desctructors:
	 ///////////////////////////////////////////////////////////////////
	 
	 Tree();
	 Tree(TreeNode *root);
	 Tree(std::string&, const std::string&);
	 ~Tree();
	 
	 ///////////////////////////////////////////////////////////////////
	 // Accessors:
	 ///////////////////////////////////////////////////////////////////

	 TreeNode* get_root() const { return _root; }
	 Leaves* get_leaves() const { return _leaves; }
	 TreeNode* get_leaf(int i)  { return (*_leaves)[i]; }
	 static int get_nb_parses() { return _nb_parses; }

	 ///////////////////////////////////////////////////////////////////
	 // Other functions:
	 ///////////////////////////////////////////////////////////////////

	 // Return the string corresponding to the parse tree:
	 std::string get_string() const
	 {	return _root->get_string(_leaves); }
  
	 // Move punctuation up the tree:
	 void fix_collins_punc();

  protected:
	 TreeNode* _root;
	 Leaves* _leaves;

	 static int _nb_parses;

  private:
  // get the project size, used for dealing with projected trees. the size are
  // big N size projectd from available parse trees.
  void get_projected_size(TreeNode* root);
};

}

#endif
