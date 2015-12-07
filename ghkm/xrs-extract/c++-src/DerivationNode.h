#ifndef __DERIVATIONNODE_H__
#define __DERIVATIONNODE_H__

/***************************************************************************
 * DerivationNode.C/h
 * 
 * This class is used to represent nodes in packed derivation 
 * forests (Derivation class). Each node represents an "OR"-node
 * that maps rules to successor DerivationNodes.
 *
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: DerivationNode.h,v 1.4 2006/12/18 21:07:00 wwang Exp $
 ***************************************************************************/

#include <iostream>
#include <cfloat>
#include <set>
#include <map>
#include <vector>

#include "TreeNode.h"
#include "DerivationNodeDescriptor.h"
#include "RuleInst.h"
#include "hashutils.h"

namespace mtrule {

  class DerivationNode;

  // Vector of DerivationNodes, to represent successors of
  // a DerivationNode:
  typedef std::vector<DerivationNode*> DerivationNodes;
  typedef DerivationNodes::iterator DerivationNodes_it;
  typedef DerivationNodes::const_iterator DerivationNodes_cit;

  // Each Derivation node is similar to an OR-node, from which 
  // different rules can lead to different sets (vectors) 
  // of other DerivationNodes:
  typedef std::multimap<int,DerivationNodes*> DerivationOrNode;
  typedef DerivationOrNode::iterator DerivationOrNode_it;
  typedef DerivationOrNode::const_iterator DerivationOrNode_cit;

  // The following defines hashtables that map TreeNode memory addresses
  // to DerivationNode addresses. This is used in Derivation, in which we
  // need to know at a given TreeNode if there is a corresponding DerivationNode
  typedef hash_map<DerivationNodeDescriptor,DerivationNode*,
                   DerivationNodeDescriptor::hash_desc> 
    TreeNodeToDerivationNode;
  typedef TreeNodeToDerivationNode::iterator TreeNodeToDerivationNode_it;
  typedef TreeNodeToDerivationNode::const_iterator TreeNodeToDerivationNode_cit;

  // Used to index derivation nodes, e.g. for keeping track of pointer indexes
  // when we want to output ((1 #1(2 3)) (4 #1)) instead of 
  // ((1 (2 3)) (4 (2 3))):
  typedef hash_map<DerivationNode*,int,hash_ptr> DerivationNodeIndex;
  typedef DerivationNodeIndex::iterator DerivationNodeIndex_it;
  typedef DerivationNodeIndex::const_iterator DerivationNodeIndex_cit;

  //! This class is best thought of as a representation of an OR node in the derivation.
  //! The difference with usual representations of derivation forests is that
  //! derivations defined by the current set of classes don't involve other types 
  //! of nodes (e.g. rule nodes). DerivationNode objects encapsulate hash tables
  //! that associate rule IDs (key) to vectors of pointers to DerivationNode
  //! objects (data). No class for rule nodes are needed with this representation.
  class DerivationNode {

    // TODO: hash_multimap --> hash_map

	 public: 

		/************************************************************************
		 * Constructors and destructors:
		 ************************************************************************/
		
		//! Build a DerivationNode object from a TreeNode object.
		DerivationNode(TreeNode *n) : _refTreeNode(n), _parent_counter(0), 
		  _done(false), _logp(-DBL_MAX), _logp_computed(false), _backptr(-1)  {
		  // Keep track of how many nodes were created/deleted:
		  ++nodes_in_mem; 
		}
		//! Default destructor.
		~DerivationNode() { 
		  // Keep track of how many nodes were created/deleted:
		  --nodes_in_mem;
		}
		  
		/************************************************************************
		 * Accessors/mutators:
		 ************************************************************************/

      //! Get TreeNode corresponding to the current DerivationNode.
		TreeNode* get_treenode() const { return _refTreeNode; }
      //! Add element to the OR-node that connects the current derivation
		//! node to its children.
		void add_child(int ruleID, DerivationNodes* dns) {
		  _children.insert(std::make_pair(ruleID,dns));
		}
		//! Returns all children of current node.
		DerivationOrNode& get_children() { return _children; } 
		//! Increment nb of parents.
		void inc_nb_parents() { ++_parent_counter; }
		// decrement nb of parents.
		void dec_nb_parents(){ --_parent_counter; }
		//! Set number of parents in derivation.
		void set_nb_parents(int n) { _parent_counter = n; }
		//! Get number of parents in derivation.
		int get_nb_parents() const { return _parent_counter; }
		//! Set this to true if the derivation below this node is already complete.
		void set_done() { _done = true; }
		//! Returns true if the derivation below this node is already complete.
		bool is_done() const { return _done; }
		//! Returns the logp of DerivationNode.
		double get_logp() const { return _logp; }
		//! Sets logp of DerivationNode.
		void set_logp(double lp) { _logp = lp; _logp_computed=true; }
		//! Returns true if the logp has already been computed for the given
		//! derivation node.
		bool has_logp() const { return _logp_computed; }
		//! Returns the (local) index of the rule below the current OR node
		//! that corresponds to the most probable (Viterbi) derivation.
	   int get_backptr() const { return _backptr; }
		//! Sets the (local) index of the rule below the current OR node
		//! that corresponds to the most probable (Viterbi) derivation.
		void set_backptr(int i) { _backptr = i; }
		//! Return number of DerivationNode objects currently in memory.
	   static int get_nb_nodes_in_mem() { return nodes_in_mem; }
	   
	   bool insert_into_derivation_forest(RuleInsts& rules, // DerivationNode* derNode, 
					      TreeNode* searchNode, 
					      DerivationNodes* dns, int curRuleID, std::set<DerivationNode*>& searched);
					      

	 protected: 
	   //! Reference to parse tree node associated with derivation node.
		TreeNode* _refTreeNode;
		//! Count the number of nodes in the derivation
	   //! that have the current node as a child 
	   //! (to create pointers like #1)
		int _parent_counter; 
		//! This determines that this is a node in a derivation
		//! that has already been completed, so no need to continue.
		bool _done; 
		//! Log-probability associated with given node.
		float _logp;
		//! True if logp (Viterbi probability) is already computed.
		bool _logp_computed;
		//! Back pointer to the rule along the most probable derivation.
		/*! (this stores the local index of that rule != global DB index; important
		 * distinction, since the same rule can have two different local indices
		 * for cases where variables in the RHS correspond to different spans 
		 * due to unaligned chinese words. */
		int _backptr;
		//! Children of current derivation node.
		DerivationOrNode _children;
		//! Number of DerivationNode objects in memory.
      static int nodes_in_mem;
    private:
	   //! This prevents accidental copy constructions.
	   DerivationNode(const DerivationNode&);
  };

}

#endif
