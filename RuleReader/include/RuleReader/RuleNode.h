// ;; -*- mode: C++; fill-column: 80; comment-column: 59; -*-
// ;;

#ifndef __RR_RULENODE_H__
#define __RR_RULENODE_H__

//using namespace std;

#include <string>
#include <vector>

namespace ns_RuleReader {



  typedef enum {
    RULENODE_UNDEFINED,					   // not used
    RULENODE_NONTERMINAL,				   // its a non-terminal, ie: this is a label of a syntactic constituent
    RULENODE_LEXICAL,					   // its a leaf node, this is an english lexical item
    RULENODE_POINTER,					   // its a pointer, indicating the position on the rhs of the rule
    RULENODE_MAX
  } RULENODE_TYPE;

  const int UNDEFINED_RHSINDEX=-1;
  const std::string UNDEFINED_STR="__UNDEFINED__";

  //! RuleNode: This represents the root of the LHS of an xRs rule
  /*! Each RuleNode contains:
    - either:
    Non-terminals (strings) (these are internal nodes)
    Lexical Items (strings) (these are leaves)
    Pointers (ints) (these are leaves) - these you which rhs spot this leaf corresponds to

    - a pointer to children of this node
   */

  class RuleNode {
  public:
    RuleNode(std::string _str, RuleNode* _parent, RULENODE_TYPE t);               //!< Create a rule node with this string, and these children. if children is empty, then this is RULENODE_LEXICAL, else RULENODE_NONTERMINAL
    RuleNode(int _rhsIndex, std::string _str, RuleNode* _parent);
    ~RuleNode();

    bool isLeaf();				   //!< Does this node have any children?

    //! Preterminal or nonterminal.
    bool isNonTerminal();				   //!< Is this a nonterminal node?

    //! preterminal. Returns true if this node is a nonterminal, has
    //! only one child, and the child is lexial (isLexical returns true;
    //! otherwise, it returns false.
    bool isPreTerminal();

    bool isLexical();					   //!< Is this a lexicalized leaf?
    bool is_lexical_subtree();                             //!< Does this subtree contain only lexical components (i.e. no xRs states)?
    bool isPointer();					   //!< Is this an xRs state (points to an input on the RHS of the xRs rule?)

    //! returns the string at this node - entireLexicalSubtree prints
    //! children nodes if they're leixcal items also.
    //! @param addQuote  true means that lexical items will be quoted using
    //!                  "". By default, it is true.
    std::string getString(bool entireLexicalSubtree=false, bool addQuote=true);

    //! if only_pos=true then only the parts of speech are returned for
    //! variables and not the entire variable with pos.
    //! @param addQuote  true means that lexical items will be quoted using
    //!                  "". By default, it is true.
    std::string getString(bool entireLexicalSubtree, std::string delimiter,
	             bool only_pos, bool addQuote=true);

    //! Returns the yeild of this tree. For example,
    //!  for LHS NP-C(NPB(NN("zhang"))), the yield will be zhang.
    //! If there is variable, the yield will look like : zhang x1 .
    //! No quote for lexical items.
    //! @param strvec    The string vector obtained by tokenizing
    //!                  the returned string.
    std::string yield(std::vector<std::string>* strvec);

    std::vector<std::string>* get_lex_vector();
    int getRHSIndex(); 					   //!< returns rhsIndex at this node

    RuleNode* getParent();                                 //!< returns the parent of this node, NULL if this node is the root

      typedef std::vector<RuleNode *> children_type;
      
    children_type *getChildren(); 			   //!< get a handle on the children

    int get_child_index();                                 //!< returns the index of this child in its parents children list
    bool is_child(ns_RuleReader::RuleNode* child);         //!< returns true if child is a child of this node

    std::string get_state();                                    //!< get the state associated with this variable, i.e x0.  Must be a pointer/variable node.
    std::string get_pos();                                      //!< get the pos portion associated with thie variable, i.e. NP.  Must be a pointer/variable node.

    std::string pos_lookup(std::string search_state);                //!< get the pos associated with this variable state, i.e. x0
   
    //! get the number of time the pos associated with this variables
    //! state occures before reaching this variable (bottom up manner).
    int pos_count(std::string search_state);

    std::string treeString(std::string delimiter=" ");                                     //!< returns a string representation of this node
    std::string treeString(std::string delimiter, bool only_pos);

    /**
     * @brief  Convert the tree into the a certain parse format.
     *
     * This method essentially did the same as the treeString(...)
     * method except for the formatting.
     */
    std::string parseTreeString(const char* format = "radu");


    std::string get_lex_string();                               //!< returns the lexical leaves in order of a lexical subtree

    // a few misc. things for binarization
    bool is_in_rule();  // has this node already been included in a binary rule
    void set_in_rule(bool val);  // set this node as being included or not included in a binary rule
    std::string rule_tag;
    std::string new_rhs_state;  // new rhs state associated with this node

    //! How many nonterminals (excluding preterminals) are in
    //! the subtree rooted at this node.
    std::size_t numInternalNodes();

    //! How many nodes (terminal + nonterminal) 
    std::size_t numNodes();

    //! Set this node to be the head.
    void set_as_head();
    void set_as_non_head();

    bool isHeadNode()  const { return is_head;}

    std::string getLabel() const { return str;}
    void setLabel(const std::string lab) { str=lab;}

  private:
    RULENODE_TYPE type;					   //!< Type of this node. Determines what information is valid at this node.

    RuleNode * parent;                                     //!< Parent of this node, null if this is the root

    std::string str;						   //!< String at this node. only valid if type RULENODE_NONTERMINAL or RULENODE_LEXICAL
    int rhsIndex;					   //!< The RHS index of this leaf in the xRs tree. Only valid if type RULENODE_POINTER
                                                           // For binary rules, if the variable occurs on the rhs of this node then the index
                                                           //  will be the index on the rhs (either 0 or 1).  If this node does not occur on the rhs
                                                           //  and is only acting as a place-holder on the lhs, then the index is -1.

    std::string state;
    std::string pos;

    children_type children;			   //!< Children of this node

    // various things for binarization
    bool in_rule;

    //! \brief count the number of times a PoS appears in the subtree 
    //! rooted as this node. 
    //! @param search_state  is the xRs state we are checking its
    //!    existentce.
    //! @param search_pos  is the PoS that we are counting is 
    //!    appearence.
    //! @param count  is the count of the search_pos.  (\TODO, change
    //!   it to a reference.)
    //! @return  true if the state is in the tree. (This doesnt work
    //!   because there is a bug. So DONT check the stat using the 
    //!   returned value of this method.
    bool pos_count(std::string search_state, std::string search_pos, int* count);

    bool is_head;

  }; }

#endif

