// ;; -*- mode: C++; fill-column: 80; comment-column: 59; -*-
// ;;

#include <string>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <RuleReader/Rule.h>
#include <RuleReader/RuleNode.h>
#include <map>
#include <graehl/shared/string_match.hpp>

#ifdef TEST
#include "test.hpp"
#endif

using namespace std;
using namespace graehl;

namespace ns_RuleReader {

bool is_comment_line(std::string const& line) 
{
    return line.empty() || line[0]=='%' || graehl::starts_with(line,"$$$");
}

#ifdef ENABLE_RRDBP
#define RRDBP(a) cerr<<#a<<"="<<a<<"\n"
#else
#define RRDBP(a) 
#endif
// delim1 (optional, return true if found) must occur before delim2 before delim3; ignores quoted "asdf" or """, exception on "", stops looking after delim3
static bool find_rule_delims_in_order(const string &rule,const string &delim1,string::size_type &pos1,const string &delim2,string::size_type &pos2,const string &delim3,string::size_type &pos3) 
{
    pos1=pos2=pos3=string::npos; // avoid uninit. warn
    enum {
        rule_outside,
        rule_onequote,
        rule_twoquote,
        rule_inside_longquote,
        rule_attributes
    } state = rule_outside;
    char c;
    // replace in ret all the characters inside "..." before the ### delimeter
    unsigned n_delim=0;
    string::size_type len=rule.length();
    bool found_delim1=false;
    for (string::size_type i = 0;i<len;++i) {
        c=rule[i];
        switch(state) {
        case rule_outside:
            if (c=='"')
                state=rule_onequote;
            else {
                string::const_iterator rule_b=rule.begin()+i,rule_e=rule.end();
                if (match_begin(rule_b,rule_e,delim1)) {
                    found_delim1=true;
                    if (++n_delim!=1)
                        goto bad_delim;
                    pos1=i;
                    i+=delim1.length()-1;
                } else if (match_begin(rule_b,rule_e,delim2)) {
                    if (!found_delim1) { // delim1 is optional
                        pos1=-delim1.length(); //fixme what is going on
                        n_delim=2;
                    } else if (++n_delim!=2)
                        goto bad_delim;
                    pos2=i;
                    i+=delim2.length()-1;
                } else if (match_begin(rule_b,rule_e,delim3)) {
                    if (++n_delim!=3)
                        goto bad_delim;
                    pos3=i;
                    return found_delim1;
                }
            }
            break;
        case rule_onequote:
            if (c=='"')
                if (rule[i+1] == '@')
                    state=rule_inside_longquote;
                else
                    state=rule_twoquote;
            else
                state=rule_inside_longquote;
            break;
        case rule_twoquote:
            if (c=='"')
                state=rule_outside;
            else
                goto empty_quotes;
            break;
        case rule_inside_longquote:
            if (c=='"')
                if ((rule[i-1] == '@' && (rule[i+1] == '@' or rule[i+1] == '"')) or rule[i+1] == '@')
                    state=rule_inside_longquote;
                else
                    state=rule_outside;
            break;
        default:
            assert(0=="impossible state in find_rule_delims_in_order");
        }
    }
bad_delim:
    throw Rule::bad_format(rule,"rule delimiters not all found (in proper order)");
empty_quotes:
    throw Rule::bad_format(rule,"empty quoted string not allowed"); 
}

static inline void assign_substring_interval(string &to,const string &from,string::size_type a,string::size_type b) 
{
    to.assign(from,a,b-a);
}


// return true if :VL:
static inline bool segment_rule_string(const string &rule,string &virtual_label,string &lhs,string &rhs,string &attributes,string &comment) 
{
    using namespace ns_RuleReader;
    string::size_type vl,side,attr;
    bool is_virtual=find_rule_delims_in_order(rule,virtualDelimiter,vl,sideDelimiter,side,attributeDelimiter,attr);
    if (is_virtual) {
        assign_substring_interval(virtual_label,rule,0,vl);
        RRDBP(virtual_label);
    } else {
        virtual_label="";
    }
    assign_substring_interval(lhs,rule,vl+virtualDelimiter.length(),side);
    RRDBP(lhs);
    assign_substring_interval(rhs,rule,side+sideDelimiter.length(),attr);
    RRDBP(rhs);
    assign_substring_interval(attributes,rule,attr+attributeDelimiter.length(),rule.length());
    RRDBP(attributes);

/* FIXME:
  //filterQuotes doesn't parse attributes {{{ % }}} so it would be unsafe to do this
  commentIndex = str.find(commentDelimiter, attributeIndex); // find where the comments start
  if (commentIndex == string::npos) {                      // if no comments
    commentIndex = str.length();                           // call them at the end
  } // end if no comments
*/
    comment="";//FIXME
    
    return is_virtual;
}

#ifdef TEST
const char *good_rr_rule[] = {
    "NP(\"### :VL: \",\"->\") -> \"\"\" ### asdf",
    "",
    "NP(\"### :VL: \",\"->\") ",
    " \"\"\" ",
    " asdf",

    "->###\"\"",
    "",
    "",
    "",
    "\"\"",
    
    "bob:VL: ->###",
    "bob",
    "",
    "",
    "",
    0
};

const char *bad_rr_rule[] = {
    "",
    ":VL: ",
    ":VL: ###",
    ":VL: ### ->",
    "\":VL: \"->\" ###",
    "->\"\"###",
    "\"->###",
    0
};
    
    
    
BOOST_AUTO_UNIT_TEST( TEST_rulereader_rule )
{
    string s[5];
    for (const char ** g=good_rr_rule;*g;g+=5) {
        RRDBP(g[0]);
        segment_rule_string(g[0],s[1],s[2],s[3],s[4],s[0]);
        for (unsigned j=1;j<=4;++j)
            BOOST_CHECK_EQUAL(s[j],g[j]);
    }
    for (const char ** b=bad_rr_rule;*b;++b) {
        string bad=*b;
        RRDBP(bad);
        BOOST_CHECK_THROW(segment_rule_string(bad,s[1],s[2],s[3],s[4],s[0]),Rule::bad_format);
    }    
}

#else
// not test ...
    

// find the next occurrence of character c in string str
// starting at index, ignoring any occurrences of c that
// fall in a virtual label
static int findNext(string str, char c, int index)
{
  int found_index = -1;  // index of a valid found occurrence of c
  int virtual_count = 0;  // how many V[ have we nested into
  bool in_quote = false;  // are we within a quoted region?

  // iterate through all of the characters in the string starting at
  // index until we either find the end of the string or find a valid
  // (i.e. not in virtual label) occurrence of c in str
  for( int i = index; i < (int)str.length() && found_index == -1; i++ )
  {
    // see if the current character is the one we're looking for
    if( str[i] == c )
    {
      // see if we're in a virtual label
      if( virtual_count == 0 )
      {
	found_index = i;
      }
      // otherwise, we're in a virtual label
    }

    // see if this is the beginning of a virtual
    // label, i.e. starts with V[ and is not
    // in a quoted region
    if( !in_quote &&
	str[i] == 'V' &&
	i+1 != (int)str.length() &&
	(str[i+1] == '[' || str[i+1] == '<') )
    {
      virtual_count++;
    }

    if( str[i] == '\"' )
    {
      // we need to check for the actual quote character
      // which occurs as three quotes in a row.  If this happens,
      // then we shouldn't swap the value on the first quote
      bool found_three_quotes = (i+2 < (int)str.length()) && str[i+1] == '\"' && str[i+2] == '\"';

      if( !found_three_quotes )
      {
	// swap the value of in_quote
	in_quote = !in_quote;
      }
    }


    // see if this is the end of a virtual label
    // i.e. a ] not in a quoted region
    if( !in_quote &&
	(str[i] == ']' || str[i] == '>') )
    {
      assert(virtual_count > 0);
      virtual_count--;
    }
  }

  return found_index;
}

static int findNextDelimiter(const string &str)		   // find the next space, open, or close paren
{
    typedef std::string::size_type SP;
  SP nextSpace;
  SP nextOpen;
  SP nextClose;
  SP openQuote=string::npos;
  SP closeQuote=string::npos;


  openQuote = str.find("\"", 0);

  if (openQuote!=string::npos) {
    closeQuote = str.find("\"", openQuote+1);
    if(closeQuote != string::npos && closeQuote == openQuote + 1){
	closeQuote = str.find("\"", openQuote+2);
    }
  }
  SP start = (SP)-1;

  do {
    nextSpace = str.find(' ', start+1);
    start = nextSpace;
  } while (start!=string::npos && openQuote!=string::npos && start>=openQuote && start<=closeQuote);

  start = (SP)-1;
  do {
    //nextOpen = str.find('(', start+1);
    nextOpen = findNext(str, '(', start+1);
    start = nextOpen;
  } while (start!=string::npos && openQuote!=string::npos && start>=openQuote && start<=closeQuote);

  start = (SP)-1;
  do {
    //nextClose = str.find(')', start+1);
    nextClose = findNext(str, ')', start+1);
    start = nextClose;
  } while (start!=string::npos && openQuote!=string::npos && start>=openQuote && start<=closeQuote);

  // return the minimum

  if (nextSpace != string::npos && nextSpace < nextOpen && nextSpace < nextClose) {
    return nextSpace;
  }

  if (nextOpen != string::npos && nextOpen < nextSpace && nextOpen < nextClose) {
    return nextOpen;
  }

  if (nextClose != string::npos && nextClose < nextSpace && nextClose < nextOpen) {
    return nextClose;
  }

  return -1;
}

template <class Stack>
static inline RuleNode *push_rule_node(Stack &s, const string &a, RuleNode* b, RULENODE_TYPE c) {
    s.push_back(0); // if you allocate first, then you risk a memory leak on exception
    return s.back()=new RuleNode(a,b,c);
}

template <class Stack>
static inline RuleNode *push_rule_node(Stack &s,int a, const string &b, RuleNode* c)
{
    s.push_back(0); // if you allocate first, then you risk a memory leak on exception
    return s.back()=new RuleNode(a,b,c);
}


Rule::Rule(const string &str) :
    lhsRoot(NULL)
{
    init(str);
}

void Rule::init(const string &str) 
{
    clear();
    lhsRoot=NULL;
    
    typedef std::string::size_type SP;

    string lhsStr,rhsStr;
    
    binarized_rule=segment_rule_string(str,label,lhsStr,rhsStr,attributeStr,comment);
    bool is_virtual_rule=binarized_rule; //FIXME: why redundant name?

  /*cerr << "label: " << label << endl;
  cerr << "lhs: " << lhsStr << endl;
  cerr << "rhs: " << rhsStr << endl;
  cerr << "attrib: " << attributeStr << endl;*/

  int nOpenParens=0;

  // read the RHS
  vector<string> rhsUnknown;

  istringstream is(rhsStr);
  string token;

  while (is >> token) {					// for each token on rhs
      if (token[0]=='"' && token[token.length()-1]=='"') {   // if quoted
          rhsLexicalItems.push_back(token.substr(1, token.length()-2)); // push back lexical item
          rhsUnknown.push_back("");
      } else { 						   // not quoted
          rhsLexicalItems.push_back("");			   // null lex item

          // its either a state or a rhs constituent
          // for now, we'll just push it on the unKnownRhs vector and
          // then figure it out after parsing the lhs
          rhsUnknown.push_back(token);
      } // end if not quoted

      // just add blanks for now for both rhs states
      // and rhs constituents
      rhsStates.push_back("");
      rhsConstituents.push_back("");
  } // end for each token on rhs

  // read the LHS

  vector<RuleNode *> stack;				   // current stack

  while (!lhsStr.empty()) { 				   // while there's stuff left to eat
      int next = findNextDelimiter(lhsStr);		   // get next space or paren
      if (next==-1)
          throw Rule::bad_format(lhsStr,"unparsed stuff at the end of lhs");
      token = lhsStr.substr(0, next);			   // get the token
      //    cerr << "DBG: Got token: " << token << endl;
      char delim = lhsStr[next];
      lhsStr = lhsStr.substr(next+1, lhsStr.length()); 	   // shift the lhs

      if (delim == '(') { 				   // opening a new constituent
          nOpenParens++;
          if (!stack.empty()) {
              // if the stack is not empty, we add this new RN onto the children list of the previous node on top of the stack
              vector<RuleNode *> &rns=*stack.back()->getChildren();
              rns.push_back(0);
              rns.back()=push_rule_node(stack,token, stack.back(), RULENODE_NONTERMINAL);
          } else {  					   // stack is empty, this is our first node
              lhsRoot = push_rule_node(stack,token, NULL, RULENODE_NONTERMINAL);
          }	// end if this is our first node
      } else if (delim == ' ') { 				   // space
          if (token.length()>0) { 				   // then this should be an xRs state
              //	cerr << token << endl;
              //SP colonIndex = token.find(':',0);
              int colonIndex = findNext(token, ':', 0);

              if (colonIndex == (int)string::npos)		   // there should be a :, like x1:NN
                  throw Rule::bad_format(str,"no :NT following variable "+token);

              string xrsState = token.substr(0, colonIndex);	   // eg: x1
              string xrsLabel = token.substr(colonIndex+1, token.length()); // eg: NN

			  stateLabels.insert(stlextp::pair<std::string, std::string>(xrsState, xrsLabel));

              unsigned int counter=0;

              for( counter = 0; counter < rhsUnknown.size(); counter++){   // we need to find the index on the rhs of this state
                  if (rhsUnknown.at(counter)==xrsState) {		   // if a match found

                      if( !stack.empty() )
                          push_rule_node(*stack.back()->getChildren(),counter, xrsState + ":" + xrsLabel, stack.back());  // add to children of node on top of stack
                      else
                          lhsRoot = new RuleNode(counter, xrsState + ":" + xrsLabel, NULL);

                      // copy the unknown item to rhs states
                      rhsStates.at(counter) = rhsUnknown.at(counter);

                      // and remove it from the rhsUnknown
                      rhsUnknown.at(counter) = "";

                      break;
                  } // end if match found
              } // end for each rhs member

              if (counter==rhsUnknown.size()) {		   // if not found
                  if( is_virtual_rule )
                  {
                      // it's ok that we didn't find it on the rhs
                      push_rule_node(*stack.back()->getChildren(),-1, xrsState + ":" + xrsLabel, stack.back());
                  }
                  else
                  {
                      throw Rule::bad_format(str,"xRs state "+xrsState+" missing from RHS of real rule");
                  }
              } // end if not found

          } // end if this was an xrsstate

      } else if (delim == ')') { 				   // if )
          nOpenParens--;
          if (token.length()==0) {
              // do nothing
          } else if (token[0]=='"' && token[token.length()-1]=='"') { // its a lexical item
              push_rule_node(*stack.back()->getChildren(),token.substr(1, token.length()-2), stack.back(), RULENODE_LEXICAL);
              //stack.back()->getChildren()->push_back(newRN);

          }	else if (token.length()>0) { // if ( and a non-lexical token behind us, then its an xrsState
              //unsigned int colonIndex = token.find(':', 0);
              int colonIndex = findNext(token, ':', 0);
              if (colonIndex == (int)string::npos)
                  throw Rule::bad_format(str,"no :NT following variable "+token);
              string xrsState = token.substr(0, colonIndex);	   // find the state name
              string xrsLabel = token.substr(colonIndex+1, token.length()); // find the label

			  stateLabels.insert(stlextp::pair<string, string>(xrsState, xrsLabel)); // insert the mapping

              unsigned int counter=0;

              for( counter = 0; counter < rhsUnknown.size(); counter++){   // we need to find the index on the rhs of this state
                  if (rhsUnknown.at(counter)==xrsState) {		   // if a match found
# define OLD_VIRTUAL_RN
                      
# ifdef OLD_VIRTUAL_RN
                                            RuleNode *newRN = new RuleNode(counter, xrsState + ":" + xrsLabel, stack.back());
                                            stack.back()->getChildren()->push_back(newRN);  // add to children of node on top of stack
# else 
                      push_rule_node(*stack.back()->getChildren(),counter, xrsState + ":" + xrsLabel, stack.back());
# endif 
                      
                      // copy the unknown item to rhs states
                      rhsStates.at(counter) = rhsUnknown.at(counter);

                      // and remove it from the rhsUnknown
                      rhsUnknown.at(counter) = "";

                      break;
                  } // end if match found
              } // end for each rhs member

              if (counter==rhsUnknown.size()) {		   // if not found
                  if( is_virtual_rule )
                  {
                      
                      // it's ok that we didn't find it on the rhs
# ifdef OLD_VIRTUAL_RN
                                            RuleNode* newRN = new RuleNode(-1, xrsState + ":" + xrsLabel, stack.back());
                                            stack.back()->getChildren()->push_back(newRN);
# else 
                      push_rule_node(*stack.back()->getChildren(),-1, xrsState + ":" + xrsLabel, stack.back());
# endif 
                  }
                  else
                  {
                      throw Rule::bad_format(str,"xRs state "+xrsState+" missing from RHS of real rule");
                  }
              } // end if not found
          } // end if xrsState

          stack.pop_back();					   // move up the tree
      } else { 						   // unhandled delimiter
          throw Rule::bad_format(str,"unexpected delimiter "+delim);
      } // end if unhandled delimiter
  } // while lhs string not empty
  if (nOpenParens!=0)
      throw Rule::bad_format(str,"unbalanced parens");

  // go through the unknown rhs element vector
  // if there is still something there then it is constituent
  // and it should be copied to the rhsConstituents vector
  bool unknown_found = false;

  for( int i = 0; i < (int)rhsUnknown.size(); i++ )
  {
    if( rhsUnknown.at(i) != "" )
    {
      rhsConstituents.at(i) = rhsUnknown.at(i);
      unknown_found = true;
      if (!is_virtual_rule)
          throw Rule::bad_format(str,"unquoted rhs item in real rule doesn't match an lhs variable: "+rhsUnknown[i]);
    }
  }

  assert( is_virtual_rule || !unknown_found );

  // parse the attributes;
  istringstream attrIs(attributeStr);

  while (attrIs >> token) { 				   // for each attribute key/value pair
    SP equalIndex = token.find('=', 0);	   // get the equal sign
    if (equalIndex==string::npos)
      throw Rule::bad_format(str,"expected key=value pair for attribute: "+token);


    string keyStr = token.substr(0, equalIndex);
    string valStr = token.substr(equalIndex+1, token.length());
    bool bracketed = false;
    if (valStr.substr(0,3)=="{{{") {
        bracketed=true;
      valStr.erase(0,3);
      //      cerr << "DBG: VASTR: " << valStr << endl;
      if (valStr.rfind("}}}")!=string::npos && (valStr.rfind("}}}")==(valStr.length()-3))) {
	valStr.erase(valStr.length()-3, valStr.length());
      } else {
          string braceStr;
	while (attrIs >> braceStr) {
	  valStr.append(" ");
	  valStr.append(braceStr);
	  if (braceStr.rfind("}}}")!=string::npos && (braceStr.rfind("}}}")==(braceStr.length()-3))) {
	    valStr.erase(valStr.length()-3, valStr.length());
	    break;
	  }
	}
      }
      //      cerr << "DONE" << endl;
    }
    //    cerr << "DBG: (" << keyStr << ")-(" << valStr << ")" << endl;
    attribute_value av = {valStr,bracketed};
	attributes.insert(stlextp::pair<string, attribute_value >(keyStr, av ));

  } // end for each attribute token

}

// Clears the rule content.
void Rule::clear()
{
    attributes.clear();
    stateLabels.clear();
    rhsStates.clear();				   
    rhsLexicalItems.clear();			   
    rhsConstituents.clear();                        
    if (lhsRoot) {
	delete lhsRoot;
	lhsRoot = NULL;
    }
}

string Rule::getAttributeValue(const string &attr)
{
  attribute_map::iterator find_it = attributes.find(attr);
  if (find_it!=attributes.end()) { 			   // if found
    return (*find_it).second.value;
  }

  cerr << "RULE::WARNING: getAttributeValue() called with unknown attribute " << attr << endl;
  return "";
}

string Rule::getStateLabel(const string &state)
{
  stlext::hash_map<string, string, stringHash>::iterator find_it = stateLabels.find(state);
  if (find_it!=stateLabels.end()) { 			   // if found
    return (*find_it).second;
  }

  cerr << "RULE::WARNING: getStateLabel() called with unknown attribute " << state << endl;
  return "";
}

bool Rule::is_lexical(std::size_t index)
{
  assert( (index >= 0u) && (index < rhsStates.size()) );

  return rhsStates.at(index) == "";
}

string Rule::getComment()
{
  return comment;
}

Rule::~Rule()
{
//    clear();
    delete lhsRoot;
}

Rule::attribute_map *Rule::getAttributes()
{
  return &attributes;
}

//! Print (with original quotation) the original attribute list
std::string Rule::getAttributesText() const
{
    return attributeStr;
}



stlext::hash_map<std::string, std::string, stringHash> *Rule::getStateLabels()
{
  return &stateLabels;
}

const vector<string> *Rule::getRHSStates() const
{
  return &rhsStates;
}

vector<string> *Rule::getRHSLexicalItems()
{
  return &rhsLexicalItems;
}

vector<string>* Rule::getRHSConstituents()
{
  return &rhsConstituents;
}

RuleNode *Rule::getLHSRoot()
{
  return lhsRoot;
}

string Rule::get_label()
{
  assert(!binarized_rule || label != "");

  return label;
}

void Rule::set_label(string new_label)
{
  assert(binarized_rule);

  label = new_label;
}

bool Rule::is_binarized_rule()
{
  return binarized_rule;
}

bool Rule::is_virtual_label()
{
  assert(existsAttribute("virtual_label"));

  return getAttributeValue("virtual_label") == "yes";
}

string Rule::pos_lookup(string state) const
{
  string pos = lhsRoot->pos_lookup(state);

  assert(pos != "");

  return pos;
}

string Rule::toString(bool vl_format)
{
  string str = "";

  if( is_binarized_rule() )
  {
    str = label + virtualDelimiter;
  }

  if( vl_format )
  {
    str += lhsRoot->treeString("_", true);
  }
  else
  {
    str += lhsRoot->treeString();

    str += " ->";
  }

  for( int i = 0; i < (int)rhsStates.size(); i++ )
  {
    if( vl_format )
    {
      str += "_";
    }
    else
    {
      str += " ";
    }

    str += rhsElement(i, vl_format);

    if( vl_format && rhsStates.at(i) != "" )
    {
      // add the pos count for this node
      // lookup the pos count for this node
      int pos_count = lhsRoot->pos_count(rhsStates.at(i));

      if( pos_count > 0 )
      {
				ostringstream os;
				os << pos_count;
				str += os.str();
      }
    }
  }

  return str;
}

//! @param pos_only  if true, then only the PoS of the variable is
//!                  displayed.
//! @param addQuote  If true, the lexical words will be quoted.
//! @param tokens
string
Rule::
rhs_string(bool pos_only, bool addQuote, vector<string>* strvec) const
{
	string str = "";
	string tmpstr;

	for( int i = 0; i < (int)rhsStates.size(); i++ )
	{
		if( i != 0 )
		{
			str += " ";
		}

		tmpstr = rhsElement(i, pos_only, addQuote);
		if(strvec) { strvec->push_back(tmpstr);}
		str += tmpstr;

		if( rhsStates.at(i) != "" )
		{
		    if(pos_only){
		  // add the pos count for this node
		  // lookup the pos count for this node
			int pos_count = lhsRoot->pos_count(rhsStates.at(i));

			if( pos_count > 0 )
			{
				ostringstream os;
				os << pos_count;
				tmpstr = os.str();
				if(strvec) { strvec->push_back(tmpstr);}
				str += tmpstr;
			}
		    }
		}
	}

	return str;
}

string Rule::lhs_yield_string(vector<string>* strvec)
{
    if(!lhsRoot){
	return "";
    } else {
	return lhsRoot->yield(strvec);
    }
}

string Rule::entireString()
{
  string str = toString(false);

  // now add on the attribues
  str += " ###";

  //hash_map<string, string, stringHash> attributes

  for(attribute_map::iterator it = attributes.begin(); it != attributes.end(); it++ )
  {
    string key = (*it).first;
    string value = (*it).second.value;

    // output the key
    str += " " + key + "=";

    // see if the value has a space
    if( (! it->second.bracketed) && (value.find(' ') == string::npos) )
    {
      // we didn't find a space so just output the value
      str += value;
    }
    else
    {
      // we did find a space, so enclose it with brackets
      str += "{{{" + value + "}}}";
    }
  }

  return str;
}

string Rule::rhsElement(int i, bool pos_only, bool addQuote) const
{
  string str;

  assert(i < (int)rhsStates.size());

  if( rhsStates.at(i) != "" )
  {
    if( pos_only )
    {
      str = pos_lookup(rhsStates.at(i));
    }
    else
    {
      str = rhsStates.at(i);
    }
  }
  else if( rhsLexicalItems.at(i) != "" )
  {
      if(addQuote){
	str = "\"" + rhsLexicalItems.at(i) + "\"";
      } else {
	str = rhsLexicalItems.at(i);
      }
  }
  else
  {
    assert(rhsConstituents.at(i) != "");
    str = rhsConstituents.at(i);
  }

  return str;
}


//! input the rule from a stream.
istream& operator>>(istream& in, Rule& r)
{
	string line;
	getline(in, line, '\n');
    try {
	r.create(line);
    } catch (const char* s) {
	throw s;
    }

    return in;
}

//! Return the number of internal nodes (excluding preterminals).
//! \TODO make this method const.
size_t Rule::numInternalNodes()
{
    return lhsRoot->numInternalNodes();
}

size_t Rule::numNodes() 
{ return lhsRoot->numNodes();}

//! Constructs an alignment from this rule. This alignment
//! should be the format required by the rule extractor.
bool isVarString(string s)
{ if(s[0] == 'x') { return true;} else {return false;} }
string
 Rule::
constructAlignmentString()
{
    ostringstream ost;

    vector<string> lhsYield;
    vector<string> rhs;
    vector<string>::const_iterator it;

    lhs_yield_string(&lhsYield);
    rhs_string(false, false, &rhs);

    map<string, size_t>  var2index;
    for(it = lhsYield.begin(); it != lhsYield.end(); ++it){
	if(isVarString(*it)){
	    var2index[*it] = it - lhsYield.begin();
	}
    }
    bool flag=false;
    for(it = rhs.begin(); it != rhs.end(); ++it){
	if(isVarString(*it)){
	    if(flag) {ost<<" ";}
	    ost<<var2index[*it]<<"-"<<it-rhs.begin();
	    flag = true;
	}
    }

    return ost.str();
}

string
 Rule::
parseTreeString(const char* format) {
    if(getLHSRoot()){
	return ((RuleNode*)getLHSRoot())->parseTreeString(format);
    } else {
	return "";
    }
}

//! Marks the head nodes according to the head-marker string.
void Rule::mark_head(const string headmarker) 
{
    std::istringstream ist((char*)headmarker.c_str());
    mark_head(ist, (RuleNode*)getLHSRoot());
}

#if 0
void Rule::mark_head(istream& s, RuleNode* node)
{
    if(node->isLexical()){ node->set_as_head(); return;}
    char c;
    s>> c;
    if(c == 'R' || c == 'H') {
        node->set_as_head();
    }

    std::vector<RuleNode *>* kids = node->getChildren();
    assert(kids);

    // since in the head-marker string, the lexical
    // items are not marked, we thus dont read off
    // '(' for pre-terminals here.
    if(kids->size() > 0 && !node->isPreTerminal()) {
        s >> c;
        assert(c=='(');
    }
    for(size_t i = 0; i < kids->size(); ++i){
        mark_head(s, (*kids)[i]);
    }
    
    if(kids->size() > 0 && !node->isPreTerminal()) {
        s >> c;
        assert(c==')');
    }
}
#endif
void Rule::mark_head(istream& s, RuleNode* node)
{
    if(node->isLexical()){ node->set_as_head(); return;}
    char c;
    s>> c;
    if(c == 'R' || c == 'H') {
        node->set_as_head();
    }

    std::vector<RuleNode *> kids_tmp;
    std::vector<RuleNode *>* kids;
    if(node->isLeaf()){
        kids = &kids_tmp;
    } else {
        kids = node->getChildren();
    }
    assert(kids);

    // since in the head-marker string, the lexical
    // items are not marked, we thus dont read off
    // '(' for pre-terminals here.
    if(kids->size() > 0 && !node->isPreTerminal()) {
        s >> c;
        assert(c=='(');
    }
    bool hasHead = false;
    for(size_t i = 0; i < kids->size(); ++i){
        mark_head(s, (*kids)[i]);
        // error tolerance: if there are more than two heads
        // for the same children.
        if(hasHead && (*kids)[i]->isHeadNode()){
            (*kids)[i]->set_as_non_head();

        }
        if((*kids)[i]->isHeadNode()){
            hasHead  = true;
        }


    }

    // if there is no H under the current node, we set the
    // last child as head.
    if(! hasHead && kids->size() && !node->isPreTerminal()){
        // dont set the head for TOP(x0:X) R(D).
        if(getLHSRoot()->getString(false,  " " , false, false) == "TOP")  {
        } else {
            (*kids)[kids->size() -1]->set_as_head();
        }
    }

    if(kids->size() > 0 && !node->isPreTerminal()) {
        s >> c;
        assert(c==')');
    }
}



int Rule::rhsVarIndex(int i) const
{
    ostringstream ost;
    ost<<"x"<<i;
    const vector<string>* rhs_s = getRHSStates();
    int j = -1;
    for(unsigned k = 0; k < rhs_s->size(); ++k){
        if((*rhs_s)[k] != ""){
            j++;
            if((*rhs_s)[k] == ost.str()){
                return j;
            }
        }
    }
    throw bad_format(rhs_string(),"requested rhs variable does not exist");
    
    // impossible.
    return 100;
}

//! setHeads: mark a node as head if the label of the node is 
//! NT-H. otherwise, set the right-most child as head.
void Rule::setHeads(RuleNode* root)
{
    // rule root is always head.
    if(root == getLHSRoot()){
        root->set_as_head();
    }

    if(root){
        string label = root->getLabel();
        if(label.size() > 2 && label[label.size() - 1] == 'H' && label[label.size()-2] == '-'){
            root->set_as_head();
        }

        RuleNode::children_type chldn_tmp;
        RuleNode::children_type* chldn;
        if(root->isLeaf()){
            chldn = &chldn_tmp;
        } else {
            chldn = root->getChildren(); 			   //!< get a handle on the children
        }
        if(chldn){
            RuleNode::children_type::const_iterator it;
            bool isSet = false;
            for(it = chldn->begin(); it != chldn->end(); ++it){
                setHeads(*it);
                if((*it)->isHeadNode()){
                    isSet = true;
                }
            }

            // if non of the child is head, we set the last one as head.
            if(!isSet && chldn->size()){
                chldn->back()->set_as_head();
            }
        }
    }
}

//! convert the leaf nodes labeled by "-RRB-" or "-LRB-"
//! into real round brackets. 
void Rule::restoreRoundBrackets(RuleNode* root)
{
    if(!root) { return;}

    if(root->isLexical()){
        if(root->getLabel() == "-LRB-") {
            root->setLabel("(");
        }  else if(root->getLabel() == "-RRB-") {
            root->setLabel(")");
        }
    }
    RuleNode::children_type* ch;
    RuleNode::children_type chldn_tmp;
    if(root->isLeaf()){
        ch = &chldn_tmp;
    } else {
        ch = root->getChildren(); 			   //!< get a handle on the children
    }
    if(ch){
        RuleNode::children_type::const_iterator it;
        for(it = ch->begin(); it  != ch->end(); ++it){
            restoreRoundBrackets(*it);
        }
    }
}

//! Outputs the deplm string (must have headinfo before calling
//! this fuction.
void Rule::dumpDeplmString(RuleNode*root, std::ostream& o) 
{
    if(!root) { return ;}

    if(root->isLexical()){
        // we dont print any thing for lexical.
    } else if(root == getLHSRoot()){
        o<<"R";
    }  else {
        if(root->isHeadNode()){
            o<<"H";
        } else {
            o<<"D";
        }
    }
    if(!root->isPreTerminal()){
        RuleNode::children_type::const_iterator it;
        //RuleNode::children_type* ch = root->getChildren();
        RuleNode::children_type* ch;
        RuleNode::children_type chldn_tmp;
        if(root->isLeaf()){
            ch = &chldn_tmp;
        } else {
            ch = root->getChildren(); 			   //!< get a handle on the children
        }
        if(ch){
            if(ch->size()){
                o<<"(";
            }
            for(it = ch->begin(); it != ch->end(); ++it){
                dumpDeplmString(*it, o);
                if(it + 1 != ch->end()) {
                    o<<" ";
                }
            }
            if(ch->size()){
                o<<")";
            }
        }
    }
}


}

#endif 
