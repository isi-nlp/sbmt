#ifndef _SynTreeNode_H_
#define _SynTreeNode_H_

#include "Tree.h"
#include <string>
#include <treelib/tree.hh>
#include <iostream>

using namespace std;

#define UNDEF_CHIL_NUM 0 

class SynTreeNode : public treelib::TreeNode

{
	typedef treelib::TreeNode  _base;

 public:
  SynTreeNode() : head (NULL),  _base("") {
	  _base::setHeadPosition(UNDEF_CHIL_NUM);
  }
  SynTreeNode(string l) : _base(l) {
	  _base::setHeadPosition(UNDEF_CHIL_NUM);
  }
  SynTreeNode(string l, int t) : _base(l), 
							  tree_index(t), 
							  sibling_index(-1) ,
  							  head (NULL)
  {
	  _base::setHeadPosition(UNDEF_CHIL_NUM);
  }
#if 0
  SynTreeNode(string l, int t, int s) : _base(l),
									 tree_index(t), 
									 sibling_index(s),
									 head(NULL),
									 getHeadPosition()(-1)
  {}
#endif

  bool isState() { return sibling_index >= 0; }
  
  //string label;
  string relabel; // for some special purpose.
  int tree_index;     // index of node in tree
  int sibling_index;  // index of node among siblings: sibling_index(x1) = 1
  int state_link;     // index of input tree to which this state points

  //! The pointer to the head node.
  ::SynTreeNode* head;

  //! the index of the head. -1 means head undefined or obvious.
  int head_posit;

};

typedef treelib::Tree_ <SynTreeNode> TreeBase;

class Tree : public TreeBase
{
public:

	//! set 'node' as the head of its parent.
	void setAsHead(iterator node){
		// the tree root is not the head of anything.
		if(node != begin()){
			parent(node)->head = &(*node);
		}
	}

	//! Returns true if the 'node' is the head of its parent.
	bool isHead(iterator node) const{
		// the tree root is always the head.
		if(node == begin()){
			return true;
		} else {
			if(parent(node)->head == &(*node)){
				return true;
			} else {
				return false;
			}
		} 
	}


	
	int read(string line);

	//! Recursively marks the head of all nodes in the subtree rooted
	//! by 'node'.
	void markHeads(iterator node) {

		if(node.number_of_children() == 0){
			return;
		}

		sibling_iterator itsib = node.begin();
		/*
		 * Fix invalid head position.
		 */
		// if the getHeadPosition() is 0, we set it to be point to the last 
		// children. 
		///if(node->getHeadPosition() == 0){
		if(node->getHeadPosition() == 0){
				node->setHeadPosition(node.number_of_children());
		}
		if(node->getHeadPosition() > 0 && 
				     node->getHeadPosition() > node.number_of_children()) {
				node->setHeadPosition(node.number_of_children());
		}

		/*
		 * set the head of the current node.
		 */
		// if the getHeadPosition() of 'node' is defined
		if(node->getHeadPosition() > 0){
			itsib += node->getHeadPosition() - 1;
			setAsHead(itsib);
		} 
		else {
			// otherwise, we dont need to mark the head.
		}
		
		/*
		 * Set the heads for the decendents.
		 */
		for(itsib  = node.begin(); itsib != node.end(); ++itsib){
			markHeads(itsib);
		}
	}


private:
	bool readRadu(string line);

};

bool print(const ::Tree& tr, ::Tree::pre_order_iterator it, bool latexHead = false);
//int getTree(const string raw, ::Tree& tr);

#endif
