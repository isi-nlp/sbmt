// ;; -*- mode: C++; fill-column: 80; comment-column: 59; -*-
// ;;

#include <algorithm>
#include <iostream>
#include <assert.h>
#include <RuleReader/RuleNode.h>
#include <sstream>

using namespace std;

template<class T>
class deletePointer
{
 public:
  inline void operator()(const T &p) const{
    delete p;
  }
};

ns_RuleReader::RuleNode::RuleNode(string _str, RuleNode* _parent, RULENODE_TYPE t) :
  rule_tag(""),
  new_rhs_state(""),
  type(t),
  parent(_parent),
  str(_str),
  rhsIndex(UNDEFINED_RHSINDEX),
  state(""),
  pos(""),
  in_rule(false),
  is_head(false)
{
}

ns_RuleReader::RuleNode::RuleNode(int _rhsIndex, string _str, RuleNode* _parent) :
  rule_tag(""),
  new_rhs_state(""),
  type(RULENODE_POINTER),
  parent(_parent),
  str(_str),
  rhsIndex(_rhsIndex),
  state(""),
  pos(""),
  in_rule(false),
  is_head(false)
{
  // split up the string str into the state part and the pos part
  int found = -1;

  for( int i = 0; i < (int)str.length() && found == -1; i++ )
  {
    if( str.at(i) == ':' )
    {
      found = i;
    }
  }

  assert(found != -1);

  state = str.substr(0, found);
  pos = str.substr(found+1);
}

ns_RuleReader::RuleNode::~RuleNode()
{
  //  for_each(children.begin(), children.end(), deletePointer<RuleNode *>());
  for (vector<RuleNode *>::iterator it=children.begin();it!=children.end();it++) {
    delete (*it);
  }
}

bool ns_RuleReader::RuleNode::isLeaf()
{
  assert((children.empty() && type==RULENODE_POINTER) ||
	 (children.empty() && type==RULENODE_LEXICAL) ||
	 (!children.empty() && type==RULENODE_NONTERMINAL));
  return children.empty();
}

bool ns_RuleReader::RuleNode::isNonTerminal()
{
  return type==RULENODE_NONTERMINAL;
}

//! preterminal.
bool ns_RuleReader::RuleNode::isPreTerminal()
{
    if(isNonTerminal() && children.size() == 1 && children[0]->isLexical()){
	return true;
    } else {
	return false;
    }
}

bool ns_RuleReader::RuleNode::isLexical()
{
  return type==RULENODE_LEXICAL;
}

bool ns_RuleReader::RuleNode::is_lexical_subtree()
{
  bool returnMe = !isPointer();

  if( returnMe && !isLeaf())
  {
    for( int i = 0; i < (int)children.size(); i++ )
    {
      returnMe = returnMe && (children.at(i))->is_lexical_subtree();
    }
  }

  return returnMe;
}

bool ns_RuleReader::RuleNode::isPointer()
{
  return type==RULENODE_POINTER;
}

string
ns_RuleReader::RuleNode::
getString(bool entireLexicalSubtree, bool addQuote)
{
  assert(type==RULENODE_LEXICAL || type==RULENODE_NONTERMINAL || type==RULENODE_POINTER);

  return getString(entireLexicalSubtree, " ", false, addQuote);
}

string
ns_RuleReader::RuleNode::
getString(bool entireLexicalSubtree,
	 string delimiter, bool only_pos, bool addQuote)
{
  assert(type==RULENODE_LEXICAL || type==RULENODE_NONTERMINAL || type==RULENODE_POINTER);

  string returnMe = str;

  if( type==RULENODE_POINTER && only_pos )
  {
    returnMe = pos;
  }

  if( type==RULENODE_LEXICAL )
  {
    if (entireLexicalSubtree) {
	if(addQuote){ returnMe = "\"" + returnMe + "\"";}
    }
  }

  if( !isLeaf() &&
      is_lexical_subtree() &&
      entireLexicalSubtree )
  {
    returnMe += "(";

    for( int i = 0; i < (int)children.size(); i++ )
    {
      if( i == 0 )
      {
	returnMe += (children.at(i))->getString(entireLexicalSubtree, delimiter, only_pos);
      }
      else
      {
	returnMe += delimiter + (children.at(i))->getString(entireLexicalSubtree, delimiter, only_pos);
      }
    }

    returnMe += ")";
  }

  return returnMe;
}

int ns_RuleReader::RuleNode::getRHSIndex()
{
  assert(type==RULENODE_POINTER);
  return rhsIndex;
}

ns_RuleReader::RuleNode* ns_RuleReader::RuleNode::getParent()
{
  return parent;
}

vector<ns_RuleReader::RuleNode *> *ns_RuleReader::RuleNode::getChildren()
{
  return &children;
}

int ns_RuleReader::RuleNode::get_child_index()
{
  assert(parent != NULL);

  // get the children of my parent
  vector<ns_RuleReader::RuleNode*>* pchildren = parent->getChildren();

  int my_index = -1;

  for( int i = 0; i < (int)pchildren->size() && my_index == -1; i++ )
  {
    // figure out which child this node is
    if( this == pchildren->at(i) )
    {
      my_index = i;
    }
  }

  // we should find this node in the children list
  assert( my_index != -1 );

  return my_index;
}

// NOTE:  Both this operation and the previous one could be sped up using
// hashing, but for now, a linear search should be sufficient
bool ns_RuleReader::RuleNode::is_child(ns_RuleReader::RuleNode* child)
{
  assert(child != NULL);

  bool found_child = false;

  for( int i = 0; i < (int)children.size() && !found_child; i++ )
  {
    if( child == children.at(i) )
    {
      found_child = true;
    }
  }

  return found_child;
}

bool ns_RuleReader::RuleNode::is_in_rule()
{
  return in_rule;
}

void ns_RuleReader::RuleNode::set_in_rule(bool val)
{
  in_rule = val;
}

string ns_RuleReader::RuleNode::get_state()
{
  assert(isPointer());

  return state;
}

string ns_RuleReader::RuleNode::get_pos()
{
  assert(isPointer());

  return pos;
}

string ns_RuleReader::RuleNode::get_lex_string()
{
  assert( is_lexical_subtree() );

  string str = "";

  if( isLeaf() )
  {
    str += getString(true);
  }
  else
  {
    for( int i = 0; i < (int)children.size(); i++ )
    {
      if( i == 0 )
      {
	str += (children.at(i))->get_lex_string();
      }
      else
      {
	str += " " + (children.at(i))->get_lex_string();
      }
    }
  }

  return str;
}

vector<string>* ns_RuleReader::RuleNode::get_lex_vector()
{
  assert(is_lexical_subtree());

  vector<string>* returnMe = NULL;

  if( isLeaf() )
  {
    returnMe = new vector<string>;

    returnMe->push_back(getString(true));
  }
  else
  {
    vector<string>* temp;

    for( int i = 0; i < (int)children.size(); i++ )
    {
      temp = (children.at(i))->get_lex_vector();

      if( i == 0 )
      {
	returnMe = temp;
      }
      else
      {
	// copy the values to returnMe
	for( int j = 0; j < (int)temp->size(); j++ )
	{
	  returnMe->push_back(temp->at(j));
	}

	delete temp;
      }
    }
  }

  return returnMe;
}

string ns_RuleReader::RuleNode::treeString(string delimiter)
{
  return treeString(delimiter, false);
}

string ns_RuleReader::RuleNode::treeString(string delimiter, bool only_pos)
{
  string str = "";

  if( isLeaf() )
  {
    str = getString(true, delimiter, only_pos);
  }
  else
  {
    str = getString(false, delimiter, only_pos) + "(";

    for( int i = 0; i < (int)children.size(); i++ )
    {
      if( i == 0 )
      {
	str += (children.at(i))->treeString(delimiter, only_pos);
      }
      else
      {
	str += delimiter + (children.at(i))->treeString(delimiter, only_pos);
      }
    }

    str += ")";
  }

  return str;
}

/*
 * Convert the tree into the a certain parse format.
 * This method essentially did the same as the treeString(...)
 * method except for the formatting.
 *
 * The following is an example fo the Radu parse:
 * (S~1~1 0.0 ('' ") (S-C x0) (, ,) ('' ') ('' ')
 *	 (NP-C~1~1 0.0 (NPB~1~1 0.0 (PRP he))) (VP~1~1 0.0 (VBD said)) (. x1))
 */
string
ns_RuleReader:: RuleNode::
parseTreeString(const char* format)
{
  string str = "";
  string raduExtra = "~1~1 0.0";

  if( isLeaf())
  {
      if(isPointer()){
	  // PoS of variable is associated with the variable itself
	  // and is a part of this leaf. So this leaf doesnt have
	  // the parent node for PoS. Therefore we need to add the
	  // brackets here.  I.e., the S-C x0 .
	  str = str + "(" + get_pos() + " " + get_state() + ") " ;
      } else {
	    str += getString(true,      // entire lexical subtree.
			    " ",       // delim
			    false,     // only pos
			    false);    // no quote around word.
      }
  }
  else
  {
    str = "(";

    // Node label.
    str += getString(false,    // only the node label.
	            " " , false, false) ;
    if(children.size()== 1 && children[0]->isLeaf()){ str = str + " "; }
    else {                // i.e., NP-C~1~1 0.0
	ostringstream ost;
	ost << children.size();
	str += "~" + ost.str() + "~1 0.0 ";
    }

    // children.
    for( int i = 0; i < (int)children.size(); i++ ) {
      if( i == 0 ) { str += (children.at(i))->parseTreeString(format); }
      else { str += " " + (children.at(i))->parseTreeString(format); }
    }

    // (A (A a) ) --> the space after (A a).
    //if(children() &&
//	(!children().back()->leaf() && children()().back()->isPointer())) {
//	str = str + " ";
 //   }
    str = str + ") ";
  }

  return str;

}

string ns_RuleReader::RuleNode::pos_lookup(string search_state)
{
  string str = "";

  if( isPointer() &&
      state == search_state )
  {
    str = pos;
  }
  else if( !isLeaf() )
  {
    for( int i = 0; i < (int)children.size() && str == ""; i++ )
    {
      str = (children.at(i))->pos_lookup(search_state);
    }
  }

  return str;
}

int ns_RuleReader::RuleNode::pos_count(string search_state)
{
  int count = -1;
  bool found = pos_count(search_state, pos_lookup(search_state), &count);

  assert(found);
  (void)found; // hey - not unused :)
  
  return count;
}

// count the number of times a PoS appears in the subtree rooted as
// this node. 
bool ns_RuleReader::RuleNode::pos_count(string search_state, string search_pos, int* count)
{
  bool found = false;

  if( isPointer() )
  {
    if( search_pos == pos )
    {
      (*count)++;
    }

    if( state == search_state )
    {
      found = true;
    }
  }
  else if( !isLeaf() )
  {
    for( int i = 0; i < (int)children.size() && !found; i++ )
    {
      found = (children.at(i))->pos_count(search_state, search_pos, count);
    }
  }

  return found;
}


//! How many nodes (terminal + nonterminal) 
size_t ns_RuleReader::RuleNode::numNodes()
{
    size_t num = 0;

    num++;

    if(children.size()){
	for (vector<ns_RuleReader::RuleNode *>::iterator it=
		getChildren()->begin();it!=getChildren()->end();it++) { 
	    num+= (*it)->numNodes();
	}
    }

    return num;

}

//! How many nonterminals (excluding preterminals) are in
//! the subtree rooted at this node.
size_t ns_RuleReader::RuleNode::numInternalNodes()
{
    size_t num = 0;

    if(isNonTerminal() && !isPreTerminal()){
	num++;
    }

    if(children.size()){
	for (vector<ns_RuleReader::RuleNode *>::iterator it=
		getChildren()->begin();it!=getChildren()->end();it++) {
	    num+= (*it)->numInternalNodes();
	}
    }

    return num;

}

//! Returns the yeild of this tree. For example,
//!  for LHS NP-C(NPB(NN("zhang"))), the yield will be zhang.
//! If there is variable, the yield will look like : zhang x1 .
//! No quote for lexical items.
string ns_RuleReader::RuleNode::yield( vector<string>* strvec	)
{
    string str="";
    string sep=" ";				// seperator
    string tmpstr;

    if(isLeaf() && is_lexical_subtree()) {
	tmpstr = getString(true, false);
	if(strvec){ strvec->push_back(tmpstr);}
	str += tmpstr;
    } else if (isPointer() ) {			// variable
	tmpstr = get_state();
	if(strvec){ strvec->push_back(tmpstr);}
	str += tmpstr;
    } else {
	 for( int i = 0; i < (int)children.size(); i++ ) {
	     if(i){ str +=  sep; }
	     str += children[i]->yield(strvec);
	 }
    }

    return str;
}

void ns_RuleReader::RuleNode::set_as_head()
{
        is_head = true;
}

void ns_RuleReader::RuleNode::set_as_non_head()
{
        is_head = false;
}

