#ifndef __RULENODE_H__
#define __RULENODE_H__

/***************************************************************************
 * RuleNode.C/h
 * 
 * This class is used to represent nodes in rules (Rule class). 
 *
 * During rule acquisition, this structure represents a reference to a 
 * node present in a graph fragment (root), and the list of successors
 * that are also in the same graph fragment (suc).
 * (if suc is empty, this means that root is at the frontier of the
 * graph fragment, but not necessarily at the frontier of the graph)
 *
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: RuleNode.h,v 1.2 2006/09/12 21:51:22 marcu Exp $
 ***************************************************************************/

#include <iostream>
#include <set>
#include <vector>

#include "TreeNode.h"
#include "Variable.h"

namespace mtrule {

  class RuleNode {

	 public: 

		/************************************************************************
		 * Constructors and destructors:
		 ************************************************************************/
		
		// Build a RuleNode object from a TreeNode object:
		// (used during rule extraction)
		RuleNode(TreeNode *n, int pos) : 
		  _var_index(Variable::NOT_A_VAR), _lexicalized(false), _linked(false),  // DanielNotSure
		  _cat(n->get_cat()), _word(""), _suc(), _treenode_ref(n),
		  _pos(pos) {
		  ++nodes_in_mem; 
		}
		  
		RuleNode(const RuleNode& n) : 
		  _var_index(n._var_index), _lexicalized(n._lexicalized), _linked(n._linked),  // DanielNotSure
		  _cat(n._cat), _word(n._word), _suc(n._suc), 
		  _treenode_ref(n._treenode_ref), _pos(n._pos) { 
		  ++nodes_in_mem; 
		}
		// Default destructor:
		~RuleNode() { 
		  // Keep track of how many nodes were created/deleted:
		  --nodes_in_mem;
		}

		/************************************************************************
		 * Accessors:
		 ************************************************************************/

		// Determine if the current node is a leaf node in the rule
		// (!= leaf node in a given tree):
		bool is_internal()               const { return (_suc.size() > 0); }
		// Determine the number of successors of the current node in the rule:
		int get_nb_successors()          const { return _suc.size(); }
		// Get a specific successor in the list of successors:
		int get_successor_index(int i)   const { return _suc[i]; }
		// Set lexicalization of a TreeNode object; if set to true, it means
		// that the node (a leaf) appears as a source-language (e.g. English)
		// word, instead of a variable:
		void set_lexicalized(bool l)           { _lexicalized = l; }
		bool is_lexicalized()            const { return _lexicalized; }
		void set_linked(bool l) { _linked = l; } // Daniel
		bool is_linked() const { return _linked; }  // Daniel
		// Return index of the variable associated with the node (0 for x0, etc)
		// (note: if set to Variable::NOT_A_VAR or Variable::UNALIGNEDC, it 
		// means that there is no actual variable associated with the index):
		int get_var_index()              const { return _var_index; }
		// Set variable index:
		void set_var_index(int new_index)      { _var_index = new_index; }
		// Get syntactic category at given node:
		const STRING& get_cat()          const { return _cat; }
		void set_cat(const STRING& str)        { _cat = str; } 
		// Get leaf node at a given node: (returns "" if the node isn't at the
		// frontier of the rule)
		const STRING& get_word()         const { return _word; }
		void set_word(const STRING& str)       { _word = str; } 
		// Get position of the RuleNode in the rule:
		int get_pos()                    const { return _pos; }

		/************************************************************************
		 * Functions specific to rule acquisition:
		 ************************************************************************/

		// Clear successor list:
		void clear_successors()                { _suc.clear(); }
		// Add a RuleNode object as a successor of current object:
		void add_successor_index(int s)        { _suc.push_back(s); }
	 
		// Sometimes, it is helpful if a given RuleNode can be linked to a 
		// specific TreeNode, e.g. when extracting rules, so that we can 
		// expand a frontier by looking at the children of a TreeNode object
		// to add children to the correspond RuleNode object. The following 
		// functions help dealing with such links (pointers):
		TreeNode* get_treenode_ref()     const { return _treenode_ref; }
		void set_treenode_ref(TreeNode* nr)    { _treenode_ref = nr; }
		// Return number of TreeNode objects currently in memory:
	   static int get_nb_nodes_in_mem() { return nodes_in_mem; }

	 protected: 
		int _var_index;
		bool _lexicalized;
		bool _linked; // Daniel
		STRING _cat;
		STRING _word;
      //! Indices of successors RuleNodes. Note that to get a pointer to
      //! the given successor, you need to use the RuleInst/State (let it
      //! be ruleInst) the current object is part of: 
      //! ruleInst._lhs[i]
      //! where 'i' is one of the indices stored in _suc.
		std::vector<int> _suc;
		TreeNode* _treenode_ref; // used during frontier search only
		int _pos; // position of the RuleNode in the rule
      static int nodes_in_mem;
  };

  /***************************************************************************
	* Typedefs:
	***************************************************************************/

  // Vector of nodes that forms the lhs of a rule:
  typedef std::vector<RuleNode*> RuleNodes;
  typedef RuleNodes::iterator RuleNodes_it;
  typedef RuleNodes::const_iterator RuleNodes_cit;

}

#endif
