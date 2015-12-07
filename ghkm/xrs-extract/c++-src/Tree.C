#include <vector>
//#include <pcre++.h>

#include "Tree.h"
#include <boost/regex.hpp>
//using namespace pcrepp;

namespace mtrule {

  // Static init:
  int Tree::_nb_parses=0;

  Tree::Tree() { _root = new TreeNode(); _leaves = NULL; }

  Tree::Tree(TreeNode *root) : _root(root), _leaves(NULL) {}

  Tree::Tree(std::string& treestr, const std::string& format) {
	 //*myerr << "%%% tree format: " << format << std::endl; 
	 _root = new TreeNode();
	 _leaves = _root->read_parse(treestr,format);
	 ++_nb_parses;

	 // get the projected node size.
	 get_projected_size(_root);
  }

  Tree::~Tree() { 
	 if(_root != NULL)
		delete _root; 
	 if(_leaves != NULL)
		delete _leaves;
  }

  /***************************************************************************
	* Misc functions for tree editing:
	***************************************************************************/

  // Raise punctuation in the Collins parser output (new version):
  void
  Tree::fix_collins_punc() {

    // For each leaf node, fix the tree recursively:
	 Leaves_it it = _leaves->begin(), it_end = _leaves->end();
	 for(; it != it_end; ++it) {
		
		TreeNode* curNode = *it;
		TreeNode* parentNode = curNode->_parent;

		// Correct tree structure if needed:
		while
		  // the last child must be a punctuation:
		  (curNode->_cat.find("PUNC") != std::string::npos &&
		  // the parent must exist:
		  parentNode != NULL && 
		  // it must not be the root of the tree:
		  !parentNode->is_root() &&
		  // the parent NT must neither be S nor FRAG:
		  parentNode->_cat != "S" && parentNode->_cat != "FRAG" &&
		  // make sure that the punctuation symbol is the last of its
		  // parents:
		  *parentNode->_sub_trees.rbegin() == curNode) {
		  	
			 // Get link to grandparent:
			 TreeNode* grandparentNode = parentNode->_parent;
			 // Make sure that the grandparent exisits:
			 if(grandparentNode == NULL) 
			   break;
			 // Expand the subtree rooted at the parent:
			 TreeNodes_it it2 = grandparentNode->_sub_trees.begin(), 
						 it2_end = grandparentNode->_sub_trees.end();
			 for(int i=0; it2 != it2_end; ++i, ++it2) {
				if(*it2 == parentNode) {
				  // Check if head index should be modified:
				  if(grandparentNode->_head_index > i) 
				    ++grandparentNode->_head_index;
				  // Copy punctuation in parent's subtrees:
				  grandparentNode->_sub_trees.insert(it2+1,curNode);
				  // Erase punc at the level below:
				  parentNode->_sub_trees.pop_back();
				  break; // done
				}
			 }
			 // Make sure that English spans are updated accordingly:
			 parentNode->_e_span.first  = 
			   (*parentNode->_sub_trees.begin())->_e_span.first;
			 parentNode->_e_span.second = 
			   (*parentNode->_sub_trees.rbegin())->_e_span.second;
			 // Go up the tree:
			 parentNode = grandparentNode;

		}
	 }
  }

  // get the project size, used for dealing with projected trees. the size are
  // big N size projectd from available parse trees.
  void Tree::get_projected_size(TreeNode* root){
	  if(!root) {return;}

	  std::string pattern = "^(\\S+)\\^([0-9]+)$";  // NP^2
	  std::string cat = root->get_cat();
	  //cerr<<cat<<" AAA\n";
      //Pcre re(pattern);
      boost::regex re(pattern);
      boost::smatch rm;
      if (boost::regex_search(cat,rm,re))   {
          root->set_cat(rm.str(1));
		  //root->set_cat(re[0]);
          root->set_projected_size(atoi(rm.str(2).c_str()));
		  //root->set_projected_size(atoi(re[1].c_str()));
      }

	  for(size_t  i = 0; i < root->get_nb_subtrees(); ++i){
		  get_projected_size(root->get_subtree(i));
	  }
  }
}

