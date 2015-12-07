/*!
 * \file TreeNode.cpp
 * \author Bryant Huang
 * \date 6/24/04
 */
// $Id: TreeNode.cpp,v 1.2 2005/10/10 17:37:47 wwang Exp $

#include "treelib/TreeNode.h"

#include <boost/regex.hpp>

using namespace std;
//using namespace pcrepp;



treelib::TreeNode::TreeNode(string s, int f)
{
  format = f;

  // set defaults
  label = "";
  headword = "";
  headPOS = "";
  labelID = -1;
  headwordID = -1;
  headPOSID = -1;
  headPosition = 0;
  numChildren = 0;
  insideProb = 0;
  outsideProb = 0;
  hasVerb = false;
  isPreterminal = false;
  isTerminal = false;
  isVirtual = false;

  // parse out metadata
  string raduPattern = "^\\(([^~]+)~([0-9]+)~([0-9]+)$";   // check for (TOP~1~1 pattern
  string collinsPattern = "^\\(([^~]+)~([^~]+)~([0-9]+)~([0-9]+)$"; // Collins (S~denies~1~1 pattern

  //Pcre radu_re(raduPattern);
  boost::regex radu_re(raduPattern);
  boost::smatch rrm;
  if (boost::regex_search(s,rrm,radu_re))   // matches Radu style, separate out metadata
  {
      label = rrm.str(1); //radu_re[0];
      numChildren = atoi(rrm.str(2).c_str()); //atoi(radu_re[1].c_str());
      headPosition = atoi(rrm.str(3).c_str()); //atoi(radu_re[2].c_str());
  }
  else                    // not Radu style, try to match Collins
  {
    //Pcre collins_re(collinsPattern);
    boost::regex collins_re(collinsPattern);
    boost::smatch crm;
    if (boost::regex_search(s,crm,collins_re))   // matches Collins style
    {
      //label = collins_re[0];
      label = crm.str(1);
      //headword = collins_re[1];
      headword = crm.str(2);
      //numChildren = atoi(collins_re[2].c_str());
      numChildren = atoi(crm.str(3).c_str());
      //headPosition = atoi(collins_re[3].c_str());
      headPosition = atoi(crm.str(4).c_str());
    }
    else                  // matches neither
    {
      if (s[0] == '(' && s != "(")   // ignore special case of (-LRB- ()
      {

	label = s.substr(1);
	isPreterminal = true;
      }
      else
      {
        label = s;
        isTerminal = true;
      }
    }
  }
}

treelib::TreeNode::TreeNode(string s, bool isterminal, int f)
{
  format = f;

  // set defaults
  label = "";
  headword = "";
  headPOS = "";
  labelID = -1;
  headwordID = -1;
  headPOSID = -1;
  headPosition = 0;
  numChildren = 0;
  insideProb = 0;
  outsideProb = 0;
  hasVerb = false;
  isPreterminal = false;
  isTerminal = false;
  isVirtual = false;

  // parse out metadata
  string raduPattern = "^\\(([^~]+)~([0-9]+)~([0-9]+)$";   // check for (TOP~1~1 pattern
  string collinsPattern = "^\\(([^~]+)~([^~]+)~([0-9]+)~([0-9]+)$"; // Collins (S~denies~1~1 pattern

  //Pcre radu_re(raduPattern);
  boost::regex radu_re(raduPattern);
  boost::smatch rrm;
  if (boost::regex_search(s,rrm,radu_re))   // matches Radu style, separate out metadata
  {
      label = rrm.str(1); //radu_re[0];
      numChildren = atoi(rrm.str(2).c_str()); //atoi(radu_re[1].c_str());
      headPosition = atoi(rrm.str(3).c_str()); //atoi(radu_re[2].c_str());
  }
  else                    // not Radu style, try to match Collins
  {
    //Pcre collins_re(collinsPattern);
    boost::regex collins_re(collinsPattern);
    boost::smatch crm;
    if (boost::regex_search(s,crm,collins_re))   // matches Collins style
    {
      //label = collins_re[0];
      label = crm.str(1);
      //headword = collins_re[1];
      headword = crm.str(2);
      //numChildren = atoi(collins_re[2].c_str());
      numChildren = atoi(crm.str(3).c_str());
      //headPosition = atoi(collins_re[3].c_str());
      headPosition = atoi(crm.str(4).c_str());
    }
    else                  // matches neither
    {
      if (s[0] == '(' && s != "(" && !isterminal)   // ignore special case of (-LRB- ()
      {
	label = s.substr(1);
	isPreterminal = true;
      }
      else
      {
        label = s;
        isTerminal = true;
      }
    }
  }
}
ostream& treelib::TreeNode::printRadu(ostream& os) const
{
  os << label;
  os.setf(ios::fixed);
  os << setprecision(5);
//   if (insideProb != 0)
  if (!isVirtual && !isPreterminal && !isTerminal)
  {
    os << '~' << numChildren << '~' << headPosition;
    os << ' ' << insideProb;
//     if (headword != "" && headPOS != "") os << "~[" << headword << '~' << headPOS << ']';
//     if (hasVerb) os << "~#";
  }
//   if (!isVirtual && !isPreterminal && !isTerminal) os << ' ' << insideProb;
//   if (isVirtual) os << "[V]";
//   if (isPreterminal) os << "[P]";
//   if (isTerminal) os << "[T]";
  return os;
}

ostream& treelib::TreeNode::printCollins(ostream& os) const
{
  os << label;
  if (headword != "")
  {
    os << '~' << headword << '~' << numChildren << '~' << headPosition;
  }
  return os;
}

ostream& treelib::TreeNode::toStream(ostream& os) const
{
  switch (format)
  {
  case COLLINS:
    printCollins(os);
    break;
  default:
    printRadu(os);
  }
  return os;
}

ostream& treelib::operator<<(ostream& os, const treelib::TreeNode &tn)
{
  return tn.toStream(os);
}

