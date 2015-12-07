#ifndef __TREENODE_H__
#define __TREENODE_H__

/***************************************************************************
 * $Id: TreeNode.h,v 1.13 2009/11/09 21:38:56 pust Exp $
 ***************************************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <treelib/Tree.h>

#include "nstring.h"
#include "defs.h"
#include "hashutils.h"

namespace mtrule {
	using namespace std;

class TreeNode;

// Vector of nodes, and iterators to navigate them:
typedef std::vector<TreeNode*> TreeNodes;
typedef TreeNodes::iterator TreeNodes_it;
typedef TreeNodes::const_iterator TreeNodes_cit;

// Vector of leaves, and iterators to navigate them:
// (helpful in bottom-up processing, e.g. if "leaves"
// is a Leaves object, then leaves[x] returns a pointer
// to the node corresponding to the POS of the English
// word of index x.)
typedef std::vector<TreeNode*> Leaves;
typedef Leaves::iterator Leaves_it;
typedef Leaves::const_iterator Leaves_cit;

// Typedefs for handling spans (ranges in the index
// of English and Chinese strings):
typedef std::pair<int,int> span;
typedef std::vector<span> spans;
typedef spans::iterator spans_it;
typedef spans::const_iterator spans_cit;

}

#include "Alignment.h"
#include "defs.h"
#include "ForestNode.h"

namespace mtrule {

//! This class represents a node in a parse tree augmented
//! with information derived from word alignments.
/*! This includes (among other things): the English and 
 * Chinese span of each syntactic constituent; complement 
 * Chinese spans of the constituent.
 */
class TreeNode : public ForestNode
{

  friend class Tree;

 public:

  //! Struct defining the comparison operator used for sorting spans.
  struct span_sort {
    //! Determine whether span p1 is prior to p2.
	 bool operator()(const span& p1, const span& p2) const {
		  return (p1.first <= p2.first);
	 }
  };

  /////////////////////////////////////////////////////////
  // Constructors and descructor:
  /////////////////////////////////////////////////////////

  //! Default constructor which simply initializes a TreeNode
  //! object with defaults values.
  TreeNode() : _parent(NULL), 
				_cat(""), _word(""), _tag(""),
				_head_index(-1),_head_string_pos(-1),
				_e_span(std::make_pair(0,0)), _c_spans(), _c_comp_spans(), 
				_extraction_node(false), _frontier_node(false), 
				_is_virtual(false),
				  m_projected_size (-1)
   	{ 
	 // Keep track of how many nodes were created:
	 ++nodes_in_mem; 
  }
  //! Constructor initializes a TreeNode with information specified
  //! as argument. 
  /*! The information is the following: a pointer to the parent node (parT),
	*  syntactic category 
   * (cat), head word (word), head word index (oc), position of the
   * head word in the source-langauge sentence (hp), a TreeNodes instance
   * linking the current node to its children (subT), beginning (s) and
	* end (e) of source-language span.
	*/
  TreeNode(TreeNode* parT, 
				const STRING& cat,  
				const STRING& word, 
				const STRING& tag, 
				int oc, int hp, TreeNodes& subT,
				int s, int e)
				: _parent(parT), _cat(cat), _word(word), _tag(tag),
				  _head_index(oc),_head_string_pos(hp),
				  _sub_trees(subT),
				  _e_span(std::make_pair(s,e)), _c_spans(), _c_comp_spans(),
				  _extraction_node(false), _frontier_node(false),
				  _is_virtual(false),
				  m_projected_size (-1)
   	{ 
	  // Keep track of how many nodes were created:
	 ++nodes_in_mem; 
  }

  // Daniel
  TreeNode(TreeNode* parT, 
	   const STRING& cat,  
	   const STRING& word, 
	   int oc, int hp, TreeNodes& subT,
	   const STRING& tag, 
	   int s, int e,
	   bool extraction, bool frontier)
    : _parent(parT), _cat(cat), _word(word), _tag(tag),
    _head_index(oc),_head_string_pos(hp),
    _sub_trees(subT),
    _e_span(std::make_pair(s,e)), _c_spans(), _c_comp_spans(),
    _extraction_node(extraction), _frontier_node(frontier),
    _is_virtual(false),
  m_projected_size (-1)
   	{ 
    // Keep track of how many nodes were created:
    ++nodes_in_mem; 
    cerr<<"EEEE\n";
  }

  TreeNode(TreeNode* node); // Daniel

  //! Default destructor.
  ~TreeNode();

  /////////////////////////////////////////////////////////
  // Accessors/mutators:
  /////////////////////////////////////////////////////////
 
  // Direct accessors for the member variables of the TreeNode object:
  // English span: (it is always contiguous, so there is only one span)
  //! Get source-language span corresponding to the current constituent.
  span get_span()              const { return _e_span; }
  //! Get start position of source-language span.
  int get_start()              const { return _e_span.first; }
  //! Get end position of source-language span.
  /*!
	* Note: if the span has a length of one, then get_start() and
	* get_end() return the same value (note the difference with the 
	* convention used in decoding).
	*/
  int get_end()                const { return _e_span.second; }
  //! Set start position of source-language span.
  void set_start(int s)              { _e_span.first = s; }
  //! Set end position of source-language span.
  void set_end(int e)                { _e_span.second = e; }
  //! Get length of source-language span.
  int get_length()             const { return (_e_span.second-_e_span.first); }
  //! Get number of Chinese span(s)
  /*!
	* Note: there might be more than one span since target-language spans
	* are not always contiguous.
	*/
  int get_nb_c_spans()         const { return _c_spans.size(); }
  //! Get number of Chinese complement span(s). 
  /*! Note: the complement span of a node is the union of spans of nodes
   * that are neither successors nor descendants of current node. 
   */ 
  int get_nb_c_comp_spans()    const { return _c_comp_spans.size(); }
  //! Get i-th target-language span.
  span get_c_span(int i)       const { return _c_spans[i]; }

  //! Get the i-th TL complement span.
  span get_c_comp_span(int i)       const { return _c_comp_spans[i]; }

  //! Get start position of i-th target-language span.
  int get_c_start(int i)       const { return _c_spans[i].first; }
  //! Get end position of i-th target-language span.
  int get_c_end(int i)         const { return _c_spans[i].second; }
  //! Get length of i-th target-language span.
  int get_c_length(int i)      const 
  { return _c_spans[i].second-_c_spans[i].first; }
  //! Set start position of i-th target-language span.
  void set_c_start(int i,int s)       { _c_spans[i].first = s; }
  //! Set end position of i-th target-language span.
  void set_c_end(int i, int e)        { _c_spans[i].second = e; }

  spans& get_c_spans() {return _c_spans;} // Daniel
  spans& get_c_comp_spans() {return _c_comp_spans;} // Daniel
  void set_c_spans(int s, int e) { _c_spans.resize(0); _c_spans.push_back(std::make_pair(s, e));}; // Daniel
  void set_c_spans(spans cs); // Daniel


  //! Get pointer to the parent TreeNode.
  TreeNode*& parent()                { return _parent; }
  virtual TreeNode* imdPar() { return parent(); }

  //! Returns true if this is a head node. top node is always a head node.
  bool isHead() {
      // top node is always a head node.
      if(!parent()){return true;}
      else {
          if(this == parent()->get_head_tree()){
              return true;
          } else {
              return false;
          }
      }
  }

  //! Get number of TreeNode children.
  int get_nb_subtrees()        const { return _sub_trees.size(); }
  //! Get vector of pointers to TreeNode children.
  TreeNodes& get_subtrees()          { return _sub_trees; }
  //! Get pointer to i-th TreeNode child.
  TreeNode*& get_subtree(int i)      { return _sub_trees[i]; }
  //! Get position of the headword of current TreeNode in the source-
  //! language string.
  int get_head_string_pos()    const { return _head_string_pos; }
  //! Get pointer to the TreeNode child that contains the headword.
  TreeNode*& get_head_tree()         { return _sub_trees[_head_index]; }
  //! Get the index of the child that contains the headword 
  //! (among the children of the current TreeNode object)
  int get_head_index()         const { return _head_index; }

  // Wei
  void set_head_index(int index) { _head_index = index; }

  //! Get syntactic category (string) of current node. 
  const STRING& get_cat()      const { return _cat; }

  void set_cat(std::string s) {_cat = s;} // Daniel

  //! Get the headword (string) of current node. 
  /*!
	* Note: with terminal nodes, this function simply gives you
	* the the terminal string.
	*/
  const STRING& get_word()     const { return _word; }  
  //! Set the headword of current node.
  void set_word(const STRING& s)     { _word = s; }

  const STRING& get_tag()     const { return _tag; }  
  //! Returns true if the current node is internal to the tree, i.e.
  //! if it is not a POS/terminal node.
  bool is_internal()           const { return (_sub_trees.size() > 0); }
  //! Returns true if the current node is a POS/terminal node.
  /*! 
	*/
  bool is_preterm()            const { return (_sub_trees.size() == 0); }
  bool is_virtual()            const { return _is_virtual; }
  bool is_root()               const { 
	 if(_parent == NULL) 
		 return true;
	 return (_parent->_cat == "ROOT" || 
				_parent->_cat == "MULTI" || 
				_parent->_cat == "TOP" ); 
  }

  //! Returns the height of this node from the root.
  int height() {
      if(is_root()) { return 0;}
      else { return parent()->height() + 1;}
  }
  
  /////////////////////////////////////////////////////////
  // Rule extraction:
  /////////////////////////////////////////////////////////
  
  // Is the current node a potential frontier node?
  // (you must run assign_alignment_spans() and 
  // assign_complement_alignment_spans() on the root of the aligned tree for 
  // this to be set correctly):
  bool is_frontier_node() const      { return _frontier_node; }
  // Whether or not a rule can be extracted from the current node
  // (Chinese span must be contiguous):
  bool is_extraction_node() const    { return _extraction_node; }

  // Wei Wang.
  int get_projected_size() const { return m_projected_size;}
  void set_projected_size(int s) { m_projected_size = s;}

 
  /////////////////////////////////////////////////////////
  // I/O:
  /////////////////////////////////////////////////////////
 
  // Read a Collins's parse tree from a given stream, and root that tree
  // at the current node (note: it will erase the current node if it is 
  // already initialized):
  // Use treelib to load a parse from a stream:
  Leaves* read_parse( std::string&, const std::string& format = "collins");
  // Dump tree in Collins's format:
  void dump_collins_tree( std::ostream& os, bool root = true) const;
  void dump_radu_tree( std::ostream& os, bool root ) const;
  // Print the tree rooted at the current node that is supposed to be easy 
  // to read by humans, indenting nodes and printing the following: 
  // word, English span, (Chinese) span, complement span, and whether or not 
  // a given node is part of a valid frontier graph fragment:
  void dump_pretty_tree( std::ostream&, int ) const;
  void dump_pretty_tree_old( std::ostream&, int ) const;
  void dumpPOS( std::ostream&, Leaves* ) const;
  std::string get_string( Leaves* ) const;
  std::string lhs_str() const;
  std::string rhs_str() const;

  /////////////////////////////////////////////////////////
  // Spans:
  /////////////////////////////////////////////////////////

  // Merge a given list of spans, e.g. two spans are put together
  // if they cover contiguous, non-overlapping spans (or if any
  // target language word between the two spans is unaligned):
  static void merge_spans(spans& s, Alignment* a);
  // Simple span printing function:
  static void print_spans(spans& s);
  // Assign spans, complement, and set the _frontier_node bool variable
  // to each node in the tree:
  static void assign_alignment_spans(TreeNode* n, Alignment* a);
  static void assign_alignment_spans(TreeNode* n, Alignment *a, 
		                         set<TreeNode*> & memoize);
  static void assign_complement_alignment_spans(TreeNode* n, Alignment* a);
  static void assign_complement_alignment_spans(TreeNode* n, Alignment* a,
		  set<TreeNode*> & memoize);
  static void assign_frontier_node(TreeNode* n);
  static void assign_frontier_node(TreeNode* n, set<TreeNode*> & memoize);
  // Correct english spans (after punctuation has been fixed):
  void check_alignment_spans();
  // Determine the source-language constituent (English)
  // is cleanly aligned to a single target-language span (Chinese):
  static bool clean_english_to_chinese_alignment(TreeNode* n, Alignment* a);
  static bool clean_chinese_to_english_alignment(TreeNode* n, Alignment* a);
  static bool clean_chinese_to_english_alignment(const span cspan, 
		                                      const span espan , Alignment* a);

  /////////////////////////////////////////////////////////
  // Span sets and crossing sets:
  /////////////////////////////////////////////////////////

  alignp_it span_set_begin() { return _span_set.begin(); }
  alignp_it span_set_end() { return _span_set.end(); }
  alignp_it comp_span_set_begin() { return _comp_span_set.begin(); }
  alignp_it comp_span_set_end() { return _comp_span_set.end(); }

  
  size_t size_cross() const { return _cross_set.size(); }

  alignp_it cross_set_begin() { return _cross_set.begin(); }
  alignp_it cross_set_end() { return _cross_set.end(); }
  alignp_it comp_cross_set_begin() { return _comp_cross_set.begin(); }
  alignp_it comp_cross_set_end() { return _comp_cross_set.end(); }

  /////////////////////////////////////////////////////////
  // Misc functions:
  /////////////////////////////////////////////////////////
  
  // Some debugging tools:
  static int get_nb_nodes_in_mem() { return nodes_in_mem; }

 protected:
  
  /////////////////////////////////////////////////////////
  // Some functions used internally only: 
  /////////////////////////////////////////////////////////
 
  TreeNode* new_child( treelib::Tree*, treelib::Tree::iterator, int&, 
							  TreeNode*, Leaves* );
 
  /////////////////////////////////////////////////////////
  // Member variables:
  /////////////////////////////////////////////////////////
 
  TreeNode* _parent;      //!< pointer to parent node
  STRING _cat;            //!< syntactic category (string) of the node
  STRING _word;           //!< head word (string)
  STRING _tag;
  int _head_index;        //!< index of the subtree containing the headword
  int _head_string_pos;   //!< position of the headword (in the c-string)
  TreeNodes _sub_trees;   //!< pointers to subtree nodes

  span _e_span;           //!< source-language span (typically English)
  spans _c_spans;         //!< target-language span(s)
  spans _c_comp_spans;    //!< target-language complement span(s)
  bool _extraction_node;  // Flag indicating whether a rule can be extracted
								  // from the current node
								  // (i.e. contiguous span, and no chinese word of the
								  //  span maps outside of the english span)
  bool _frontier_node;    // Flag indicating that the current node is a
								  // potential frontier node in a valid rule.
  bool _is_virtual;       // is this a virtual leaf-node, as in 
								  // JJ(JJ(short),JJ(@-@),JJ(term))
  alignp_set _span_set;       //!< set of alignments in current span
  alignp_set _comp_span_set;  //!< set of alignments in current complement span
  alignp_set _cross_set;      //!< set of crossings in current span
  alignp_set _comp_cross_set; //!< set of crossings in current complement span

  /////////////////////////////////////////////////////////
  // Static member variables: (make sure you run init() for 
  // these to be set correctly)
  /////////////////////////////////////////////////////////

  //! Value used to keep track how many TreeNodes
  //! objects are in memory.
  static int nodes_in_mem; 

 public:
  static bool find_bad_align;
  //! if true, the tree leaves will be turned into lower-case.
  static bool lowercasing;

 protected: 
  //! This undefined constructor is declared private, to prevent 
  //! accidental use of the (compiler-generated) default copy 
  //! constructor. 
  TreeNode(const TreeNode& p) : _parent(NULL), 
				_cat(""), _word(""), _tag(""),
				_head_index(-1),_head_string_pos(-1),
				_e_span(std::make_pair(0,0)), _c_spans(), _c_comp_spans(), 
				_extraction_node(false), _frontier_node(false), 
				_is_virtual(false),
				  m_projected_size (-1)
   	{ 
	 // Keep track of how many nodes were created:
	 ++nodes_in_mem; 
	  //cerr<<"TreeNode(TreeNode) undefined"<<endl;
  }

 private:

  int m_projected_size ;


};

}

#endif
