// ;; -*- mode: C++; fill-column: 80; comment-column: 59; -*-
// ;;

#ifndef __RR_RULE_H__
#define __RR_RULE_H__

#include <string>
#include <vector>

#if defined(_STLPORT_VERSION)
#	include <hash_map>

#	ifndef stlext_ns_alias_defined
#	define stlext_ns_alias_defined

namespace stlext = ::std;
namespace stlextp = ::std;

#	endif


#else

#	include <ext/hash_map>

#	ifndef stlext_ns_alias_defined
#		define stlext_ns_alias_defined
namespace stlext = ::__gnu_cxx;
namespace stlextp = ::std;
#	endif


#endif

#include <stdexcept>

//#ifndef __NO_GNU_NAMESPACE__
//using namespace __gnu_cxx;
//#endif
//using namespace std;

// NOTES About Dave's (dave kauchak @ ucsd) Binarized rules:
/*
The RuleReader code should still read in normal rules just fine as
well.  The main differences between the internal representation of a
normal rule and a binary rule are as follows:

- there is a field called "label" (accessed via getLabel()), which is
the constituent resulting from applying this rule (either a virtual
constituent or an actual constituent).  This is the portion before the
":VL:" in the rule and is what should be put on the chart during
parsing/decoding.

- I've added a function "is_binarized_rule" which returns true if this
rule is a binary rule (i.e. a rule that resulted from the binarization
prcess).

- xRs states (in the code pointers) that occur in the lhs tree may or
may not be instantiated for the given rule on the rhs.  If they are on
the rhs (i.e. the state, e.g. x0, occurs on the rhs) then the rhs_index
will be the index into the rhsStates vector and indicates the position
on the rhs (either 0 or 1 since it's binary).  If the variable is only
a place holder, and therefore does not occur on the rhs, then it's
index is -1.

- right hand sides of normal rules can only be an xRs variable or a
lexical item.  Binary rules can also have a virtual label on the rhs.
To accomodate this, an addition rhs vector has been added,
rhsConstituents.  As with rhsLexicalItems and rhsStates,
rhsConstituents has the same length as these other two vectors and
contains "" if there is not constituent at that index.
*/


namespace ns_RuleReader {

/// comment (ignored) lines are empty, or begin with % or $$$
  bool is_comment_line(std::string const& line);

  class stringHash {
  public:
    inline std::size_t operator()(const std::string &x) const {
      return stlext::hash<const char *>()(x.c_str());
    }
  };

  class RuleNode;

  const std::string virtualDelimiter=":VL: ";
const std::string commentDelimiter="% "; // FIXME: comments at ends of lines are no longer allowed!  too difficult to parse safely e.g. attribute={{{ % }}}
  const std::string attributeDelimiter="###";
  const std::string sideDelimiter="->";

  //! Rule: This is a representation of an xRs rule
  /** The LHS of the xRs rule is represented by a tree of RuleNodes.
    * The RHS is represented by a vector of strings containing either
    * chinese lexical items, or parts of speech describing the necessary
    * non-terminal to match in that location.
    */
  class Rule {
  public:

      struct bad_format : public std::runtime_error
      {
          std::string input;
          std::string problem;
          bad_format(const std::string &input_,const std::string &problem_) : std::runtime_error(
              std::string("RuleReader::Rule::bad_format (").append(problem_).append(") on input:  ").append(input_)
              ), input(input_),problem(problem_)
          {}
          ~bad_format() throw ()
          {
          }
      };
      struct attribute_value {
          std::string value;
          bool bracketed;
      };
	  typedef stlext::hash_map<std::string, attribute_value, stringHash>
              attribute_map;
      //! Empty constructor. You need to call the create method
      //! to create the rule.
      Rule() : lhsRoot( NULL)  {}

      //! Read a rule from the string given. Throws string exception.
      Rule(const std::string &str);

    ~Rule();

    //! Create a rule via a string. Added by Wei Wang.
      void init(const std::string &str); // recopied from constructor by Jon Graehl - also does clear() so is safe to use on constructed rules.

      void create(const std::string& str)
      {
          init(str);
      }

    //! Clears the rule content.
    void clear();

    //! Get the value of an attribute
    std::string getAttributeValue(const std::string &attr);

    // Does an attribute exist in the rule?
    bool existsAttribute(const std::string &attr)
    { return attributes.find(attr)!=attributes.end(); }

    //! Get the label of an xRs state
    std::string getStateLabel(const std::string &state);

    //! Get the comment string associated with a rule, if there is one
    std::string getComment();

    //! Get the list of key/value pairs in the attribute list
    attribute_map *getAttributes();

      //! Print (with original quotation) the original attribute list
      std::string getAttributesText() const;

    //!< Get the hash map of state->labels
    stlext::hash_map<std::string, std::string, stringHash> *getStateLabels();

    //! Get a vector of strings representing the RHS of a rule,
    //! only xRs states are non-empty
    const std::vector<std::string> *getRHSStates() const;

    //! Get a vector of strings representing the RHS of a rule,
    //! only lexical items are non-empty
    std::vector<std::string> *getRHSLexicalItems();

    //! Get a vector of strings representing the RHS of a rule,
    //! only virtual constituents are non-empty
    std::vector<std::string> *getRHSConstituents();

    //! Get the root of the tree on the LHS of the xRs rule.
    RuleNode *getLHSRoot();

    bool is_lexical(std::size_t index);

    //! Is this a rule that was created by the binarization process.
      ///FIXME: current brf files start with NP:VL: even though NP isn't virtual.  therefore this always returns 'true'.  but is_virtual_label() is correct, if that helps.  I see plenty of code that assumes virtual label <=> binarized (not syntax) rule.
    bool is_binarized_rule();

    //! Get the label associated with this rule.  Must be a binarized
    //! rule.
    std::string get_label();

    //! Set the label associated with this rule.  Must be a binarized
    //! rule.
    void set_label(std::string new_label);

    //! Is this label a virtual constituent or an actual constituent.  you may only ask this about a binarized rule (the binarizer provides a virtual_label=yes|no attr.)
    bool is_virtual_label();

    //! return the pos associate with this variable state, e.g. x0
    std::string pos_lookup(std::string state) const;

    //! returns a string representation of this rule.  If vl_format
    //! is true, then a string representation similar to virtual
    //! labelling is returned (i.e. with only pos tags) and no ->
    std::string toString(bool vl_format=false);

    //! The rule string.
    std::string entireString();

    //! returns the string representation of rhs element i
    //! if addQuote is true, the lexical words will be quoted.
    std::string rhsElement(int i, bool pos_only=true, bool addQuote=true) const;

    //! @param pos_only  if true, then only the PoS of the variable is
    //!                  displayed.
    //! @param addQuote  If true, the lexical words will be quoted.
    //! @param strvec    The string vector obtained by tokenizing
    //!                  the returned string.
    //! N.B. : be sure not to change the default values.
    std::string rhs_string(bool pos_only=false, bool addQuote=true,
	                       std::vector<std::string>* strvec = NULL) const;

    //! Returns the yeild of this tree. For example,
    //!  for LHS NP-C(NPB(NN("zhang"))), the yield will be zhang.
    //! If there is variable, the yield will look like : zhang x1 .
    //! No quote for lexical items.
    //! @param strvec    The string vector obtained by tokenizing
    //!                  the returned string.
    std::string lhs_yield_string(std::vector<std::string>* strvec = NULL);

    /**
     * @brief  Convert the tree into the a certain parse format.
     *
     * This method essentially did the same as the treeString(...)
     * method except for the formatting.
     */
    std::string parseTreeString(const char* format = "radu");

    //! Constructs an alignment from this rule. This alignment
    //! should be the format required by the rule extractor.
    std::string constructAlignmentString();

    //! Return the number of internal nodes (excluding preterminals).
    //! \TODO make this method const.
    std::size_t numInternalNodes() ;

    //! How many nodes (terminal + nonterminal)
    std::size_t numNodes();

    //! input the rule from a stream.
    friend std::istream& operator>>(std::istream& in, Rule& r);

    //! Marks the head nodes according to the head-marker string.
    void mark_head(const std::string headmarker);

    //! Input the var index that is normalized on the LHS side,
    //! convert it into an index normalized on the RHS side.
    //! Added by Wei Wang.
    int rhsVarIndex(int i) const;

    //! setHeads: mark a node as head if the label of the node is
    //! NT-H. otherwise, set the right-most child as head.
    void setHeads(RuleNode* root);

    //! convert the leaf nodes labeled by "-RRB-" or "-LRB-"
    //! into real round brackets.
    void restoreRoundBrackets(RuleNode* root);

    //! Outputs the deplm string (must have headinfo before calling
    //! this fuction.
    void dumpDeplmString(RuleNode*, std::ostream&) ;

  private:

    //! Auxiliary function for mark_head.
    //! Mark head by reading chars off the istream.
    void mark_head(std::istream& s, RuleNode* node);


    //! The label of this rule, i.e. portion before :VL: for binarized
    //! rule.  "" if this rule is not a binarized rule..
    std::string label;

    //! Comment string as given in the rule
    std::string comment;

    //! true if this is a rule generated by the binarization process.
    bool binarized_rule;

      std::string attributeStr;

    //! maps key->value
     attribute_map attributes;

    //! maps state->label (eg: x1->NN)
    stlext::hash_map<std::string, std::string, stringHash> stateLabels;

    //! Vector of strings representing the RHS of a rule, only xRs
    //! states are non-empty.
    std::vector<std::string> rhsStates;

    //! Vector of strings representing the RHS of a rule, only lexical
    //! items are non-empty. These three vectors have the same number of
    //! items
    std::vector<std::string> rhsLexicalItems;

    //! Vector of strings representing the RHS of a rule, only virtual
    //!	constituents are non-empty
    std::vector<std::string> rhsConstituents;

    //! Root of the tree on the LHS of the xRs rule
    RuleNode *lhsRoot;

  };
}

#endif

