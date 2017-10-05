// Author: Bryant Huang

#include "Transformer.h"

string blankLabel = "-BLANK-";

Transformer::Transformer(string configFilename, bool showRules)
{
  // load configuration file
  cerr << "Loading configuration file: ";
  loadConfig(configFilename);
  cerr << "done" << endl;

  string line;

  // import binary rules
  if (rules2Filename.length() > 0)
  {
    cerr << "Loading binary rules: ";
    if (baseDir != "") rules2Filename = baseDir + "/" + rules2Filename;
    ifstream rules2File(rules2Filename.c_str());
    if (!rules2File) printError("binary rules file `" + rules2Filename + "' could not be found");
    
    ruleCount = 0;
    while (getline(rules2File, line))
    {
      if (line[0] != '%')
      {
	loadRule(line, 2);
	if (ruleCount % 10000 == 0) cerr << '.';    // progress meter
      }
    }
    cerr << ' ' << ruleCount << " binary rules loaded" << endl;
    rules2File.close();

    useRules2 = true;  // enable binary rules
  }

  // import 3+-ary rules
  if (rules3plusFilename.length() > 0)
  {
    cerr << "Loading 3+-ary rules: ";
    if (baseDir != "") rules3plusFilename = baseDir + "/" + rules3plusFilename;
    ifstream rules3plusFile(rules3plusFilename.c_str());
    if (!rules3plusFile) printError("3+-ary rules file `" + rules3plusFilename + "' could not be found");

    ruleCount = 0;
    while (getline(rules3plusFile, line))
    {
      if (line[0] != '%')
      {
	loadRule(line, 3);
	if (ruleCount % 10000 == 0) cerr << '.';    // progress meter
      }
    }
    cerr << ' ' << ruleCount << " 3+-ary rules loaded" << endl;
    rules3plusFile.close();

    useRules3plus = true;  // enable 3+-ary rules
  }
 
  if (showRules)
  {
    cout << "[BINARY RULES]" << endl;
    printRules(2);
    cout << endl;

    cout << "[3+-ARY RULES]" << endl;
    printRules(3);
    cout << endl;
  }
}

Transformer::~Transformer() {}

void Transformer::loadConfig(string configFilename)
{
     m_regexRestr.push_back(new RegexRestructurer("((\\sCD){2,})", "CD"));
     m_regexRestr.push_back(new RegexRestructurer("((\\sNNP){2,})", "NNP"));

}

void Transformer::loadConfigProperty(string property, string value)
{
  if (property == "baseDir")
  {
    baseDir = value;
    if (baseDir[baseDir.length() - 1] == '/')  // remove final '/'
    baseDir = baseDir.substr(0, baseDir.length() - 1);
  }
  else if (property == "rules-3+")
  {
    rules3plusFilename = value;
  }
  else if (property == "rules-2")
  {
    rules2Filename = value;
  }
  else
  {
    printWarning("property `" + property + "' in configuration file not recognized");
  }
}

void Transformer::loadRule(const string raw, int mode)
{
  stringstream ss(raw);
  vector<string> tokensLHS, tokensRHS;
  string buffer;
  bool isLHS = true;
  while (ss >> buffer)
  {
    if (buffer == "->")		// assuming wellformedness (only one "->" divider in rule)
    {
      isLHS = false;
    }
    else
    {
      if (isLHS)
      {
	tokensLHS.push_back(buffer);
      }
      else
      {
	tokensRHS.push_back(buffer);
      }
    }
  }

  Tree trLHS = getTTETree(tokensLHS);
  Tree trRHS = getTTETree(tokensRHS);

  string id = getID(trLHS, trLHS.begin());

  if (mode == 3)
  {
    rules3plus[id].push_back(Rule(trLHS, trRHS));
  }
  else if (mode == 2)
  {
    rules2[id].push_back(Rule(trLHS, trRHS));
  }
  else
  {
    printWarning("illegal rule mode specified");
  }
  ruleCount++;
}

bool Transformer::checkMatch(Tree& trInput, Tree::pre_order_iterator itInput, Tree& trRule, Tree::pre_order_iterator itRule)
{
  if (itInput->label != itRule->label)
  {
    return false;    // doesn't match if labels differ
  }
  else if (itInput.number_of_children() != itRule.number_of_children())
  {
    if (itRule.number_of_children() == 0)
    {
      return true;   // doesn't match if number of children differ
    }
    return false;
  }

  Tree::sibling_iterator siInput = trInput.child(itInput, 0);
  Tree::sibling_iterator siRule = trRule.child(itRule, 0);
  for (; siInput != siInput.end(); siInput++, siRule++)   // consider each child
  {
    if (siInput->label != siRule->label) return false;
    if (siRule->isState()) siRule->state_link = siInput->tree_index;   // connect state links
    if (!checkMatch(trInput, siInput, trRule, siRule)) return false;
  }
  return true;
}

Tree Transformer::getTree(const string raw)
{
  Tree tr;
  if (raw == "0" || raw == "") return tr;  // if Radu parser does not parse, it outputs 0

  stringstream ss(raw);
  vector<string> tokens;
  int lrbCount = 0;  // ( count
  int rrbCount = 0;  // ) count
  string buffer;
  while (ss >> buffer)
  {
    if (buffer[0] == '(' && buffer != "()") lrbCount++;   // ignore special case of (-LRB- ()
    if (buffer == ")" || buffer[buffer.length() - 1] == ')') rrbCount++;
    tokens.push_back(buffer);
  }
  if (lrbCount != rrbCount)
  {
    printWarning("incomplete tree");
    return tr;
  }
  
  return getTree(tokens);
}

Tree Transformer::getTree(const vector<string> tokens)
{
  Tree tr;

  // create tree representation
  int tree_index = -1;
  string token;
  Tree::pre_order_iterator it = tr.begin();

  string strippedToken;
  string prevSeen = "";      // previously seen token

  // in Radu's format, there are 4 token types to handle:
  //   1. (TOP~1~1
  //   2. )
  //   3. risk)
  //   4. -115.21142
  for (unsigned int i = 0; i < tokens.size(); i++)
  {
    token = tokens[i];

    if (token[0] == '(' && token != "()")        // type 1: first char is open paren; ignore special case of (-LRB- ()
    {
      if (prevSeen != "") return tr;
      prevSeen = token;
    }
    else if (token == ")")                       // type 2: close paren
    {
      if (prevSeen != "") return tr;
      it = tr.parent(it);                           // set it to its parent

      prevSeen = "";
    }
    else if (token[token.length() - 1] == ')')   // type 3: last char is close paren
    {
      if (prevSeen == "") return tr;
      prevSeen = prevSeen.substr(1);
      it = tr.append_child(it, TreeNode(prevSeen, tree_index--));        // append previously seen as parent

      strippedToken = token.substr(0, token.length() - 1);
      tr.append_child(it, TreeNode(strippedToken, tree_index--));        // append child but don't move it down
      it = tr.parent(it);                           // set it to its parent

      prevSeen = "";
    }
    else
    {
      if (prevSeen != "")                        // type 4: number
      {
	prevSeen = prevSeen.substr(1);
	prevSeen = prevSeen.substr(0, prevSeen.find("~"));
	if (tr.empty())
	{
	  it = tr.set_head(TreeNode(prevSeen, tree_index--));         // set head if tree is empty
	}
	else
	{
	  it = tr.append_child(it, TreeNode(prevSeen, tree_index--)); // append child and move it down
	}

	prevSeen = "";
      }
      else
      {
	printWarning("unexpected token `" + token + "' encountered");
	return tr;
      }
    }
  }

  return tr;
}

Tree Transformer::getTTETree(const vector<string> tokens)
{
  Tree tr;
  unsigned int i = 0;
  int tree_index = -1;
  Tree::pre_order_iterator it = tr.begin();
  string strippedToken = tokens[i].substr(1);
  it = tr.insert(it, TreeNode(strippedToken, tree_index--));   // TODO: check if tokens index 0 exists
  i++;
  bool justCreated = false;	// flag to signal blank labels

  string token;
  string label;
  int sibling_index;
  unsigned int div_index;
  for (; i < tokens.size(); i++)
  {
    token = tokens[i];
    if (token[0] == '(')
    {
      strippedToken = token.substr(1);
      it = tr.append_child(it, TreeNode(strippedToken, tree_index--));   // append child and move it down
      justCreated = true;
    }
    else if (token == ")")
    {
      if (justCreated)
      {
	tr.append_child(it, TreeNode(blankLabel, tree_index--));		// insert blank label
	justCreated = false;
      }
      else
      {
	it = tr.parent(it);    // set it to its parent
      }
    }
    else
    {
      // append child but don't move it down
      div_index = token.find(":");
      if (div_index != string::npos)    // has label (e.g., "x0:NN")
      {
	if (token[0] == 'x')
	{
	  sibling_index = atoi(token.substr(1, div_index - 1).c_str());
	  label = token.substr(div_index + 1);
	  tr.append_child(it, TreeNode(label, tree_index--, sibling_index));
	}
	else
	{
	  printWarning("token found with label but with no state");
	  continue;
	}
      }
      else                              // does not have label (e.g., "x0")
      {
	if (token[0] == 'x')            // has state (e.g., "x0")
	{
	  sibling_index = atoi(token.substr(1, div_index).c_str());
	  tr.append_child(it, TreeNode("", tree_index--, sibling_index));
	}
	else                            // does not have state (e.g., "John")
	{
	  tr.append_child(it, TreeNode(token, tree_index--));
	}
      }
      justCreated = false;
    }
  }
	
  return tr;
}

void Transformer::process(string inputFilename)
{
  // import sentences to be translated
  ifstream inputFile(inputFilename.c_str());
  if (!inputFile) printError("input file `" + inputFilename + "' could not be found");

  string line;
  int lineNo = 1;
  while (getline(inputFile, line))   // TODO: read in all text into a vector first
  {
    //if (showLineNos) cout << '[' << lineNo << "] ";
    if (line == "")
    {
      cout << endl;
    }
    else
    {
      Tree tr = getTree(line);
      if (tr.empty())
      {
	cout << line << endl;
      }
      else
      {
	transduceTree(tr);
	printPennTree(tr, tr.begin());
	cout << endl;
      }
    }
    lineNo++;
  }

  inputFile.close();
}

void Transformer::transduceTree(Tree& tr)
{
    for(size_t i = 0; i < m_regexRestr.size(); ++i){
	m_regexRestr[i]->restructure(tr.begin(), tr);
    }

  //if (showRules) cerr << rules_applied << " rules applied" << endl;
}

string Transformer::getID(const Tree& tr, Tree::pre_order_iterator it) const
{
  stringstream ss;
  ss << it->label << " ";
  Tree::sibling_iterator si = tr.child(it, 0);
  for (; si != si.end(); si++)
  {
    ss << si->label << " ";
  }
  return ss.str();
}

void Transformer::applyRule(Tree& tr, Tree::pre_order_iterator it, Rule r)
{
  Tree::pre_order_iterator old_it = it;   // save original it

  // retrieve state subtrees (assumes they are in consecutive ascending order)
  //hash_map<int, Tree> links;
  vector<Tree> links;
  for (Tree::pre_order_iterator rit = r.lhs.begin(); rit != r.lhs.end(); rit++)
  {
    if (rit->isState())
    {
      // link new subtree to rule LHS
      //links[rit->sibling_index] = tr.subtree(it, it.end());
      links.push_back(tr.subtree(it, it.end()));
      // skip over children to match rule LHS
      it.skip_children();
    }
    it++;
  }

  // build RHS
  Tree new_tree = r.rhs;
  Tree link;
  if (showTransformed) new_tree.begin()->label += "#";
  for (Tree::pre_order_iterator nit = new_tree.begin(); nit != new_tree.end(); nit++)
  {
    if (nit->isState())
    {
      link = links[nit->sibling_index];
      link.begin()->label = nit->label;   // use rule labels
      if (showTransformed) link.begin()->label += "*";
      new_tree.replace(nit, link.begin());
    }
  }

  // substitute RHS into original tree
  tr.replace(old_it, new_tree.begin());
}

bool Transformer::printTree(const Tree& tr, Tree::pre_order_iterator it, Tree::pre_order_iterator end)
{ 
  if (!tr.is_valid(it)) return false;
  int rootDepth = tr.depth(it);
  while (it != end)
  { 
    for (int i = 0; i < tr.depth(it) - rootDepth; ++i) cout << "  ";
    cout << it->label;
    cout << endl << flush;
    ++it;
  }
  return true;
} 

bool Transformer::printPennTree(const Tree& tr, Tree::pre_order_iterator it)
{
  if (!tr.is_valid(it)) return false;
	
  int numChildren = it.number_of_children();	
  if (numChildren == 0)
  {
    if (it->isState())
    {
      cout << 'x' << it->sibling_index;
      if (it->label != "")
      {
	cout << ':' << it->label;
      }
    }
    else
    {
      cout << it->label;
    }
    return true;
  }

  cout << "(" << it->label;
  Tree::sibling_iterator si = tr.child(it, 0);
  if (it.number_of_children() > 1 || (it.number_of_children() > 0 && si.number_of_children() > 0))
  {
    cout << "~" << it.number_of_children() << "~0 -0";
  }
  cout << " ";
  int i = 0;
  bool isLast = false;
  while (si != si.end())
  {
    isLast = printPennTree(tr, si);
    if (i < numChildren - 1) cout << " ";	// only print middle spaces, not final
    si++;
    i++;
  }
  if (!isLast) cout << " ";
  cout << ")";

  return false;
}

void Transformer::printRule(Rule& r)
{
  printPennTree(r.lhs, r.lhs.begin());
  cout << " -> ";
  printPennTree(r.rhs, r.rhs.begin());
  cout << endl;
}

void Transformer::printRules(int mode)
{
  hash_map<string, vector<Rule> >* rules;
  if (mode == 2)
  {
    rules = &rules2;
  }
  else if (mode == 3)
  {
    rules = &rules3plus;
  }
  else
  {
    printWarning("illegal rule mode specified");
  }

  for (hash_map<string, vector<Rule> >::iterator it = rules->begin(); it != rules->end(); it++)
  {
    cout << it->first << ": " << it->second.size() << endl;
  }
//   for (hash_map<string, vector<Rule> >::iterator it = rules->begin(); it != rules->end(); it++)
//   {
//     cout << "-- " << it->first << " --" << endl;
//     for (vector<Rule>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
//     {
//       printRule(*it2);
//     }
//   }
}

void Transformer::printWarning(const string message)
{
  if (message != "") cerr << "transducer: " << message << endl;
}

void Transformer::printError(const string message)
{
  printWarning(message);
  exit(1);
}


