#include <iostream>
#include <fstream>
#include "Rule.h"
#include "RuleNode.h"
#include "../../Binarizer/HashHelper.h"
#include <vector>

using namespace std;

typedef hash_map<const std::string, vector<string>*, hash_str> hash_str_vstr;

vector<int>* get_original_counts(char* original_file)
{
  ifstream in(original_file);

  if( !in )
  {
    cerr << "Couldn't open original file: " << original_file << endl;
  }

  vector<int>* returnMe = new vector<int>;

  int line_number = 0;
  string line;

  while (getline(in, line))
  {
    line_number++;
    ns_RuleReader::Rule *myRule;

    if( line.find("$$$", 0) == 0 )
    {
      // ignore this line
      continue;
    }
    
    try {
      myRule = new ns_RuleReader::Rule(line); // this constructor throws a string exception
    } catch (string s) { 
      cerr << "Caught exception: " << s << endl;
      continue;
    }
    
    if( line_number >= (int)returnMe->size() )
    {
      returnMe->resize(line_number+1, 0);
    }

    returnMe->at(line_number) = (int)atof((myRule->getAttributeValue("count")).c_str());
    delete myRule;
  }

  return returnMe;
}

int main(int argc, char *argv[])
{
  //ifstream from("../test/example.rules");
  if( argc != 3 )
  {
    cerr << "stats <binary-rule-file> <original-rule-file>\n";
    return 1;
  }

  ifstream from(argv[1]);  

  if (!from) {
    cerr << "Couldn't open " << argv[1] << endl;
    exit(-1);
  }

  // read in the original rule counts
  vector<int>* original_counts = get_original_counts(argv[2]);

  int num_rules = 0;
  int num_rules_count = 0;
  int non_binary_rules = 0;
  int all_lexical_non_binary = 0;
  int all_lexical_non_binary_count = 0;
  int lm_true = 0;
  int lm_true_count = 0;
  int simple_lm_count = 0;
  int sblm_true = 0;
  int sblm_true_count = 0;
  int simple_sblm_count = 0;
  int original_rule_count = 0;
  int multi_variable_count = 0;

  hash_str_int labels;
  int total_virtual_rules = 0;

  hash_str_vstr original_rules;

  int non_virtual_count = 0;
  int non_virtual_count_count = 0;

  int lm_true_sharing = 0;
  int sblm_true_sharing = 0;

  int rule_id = 0;

  string line;

  while (getline(from, line)) { // for each line in example rules file
    ns_RuleReader::Rule *myRule;

    if( line.find("$$$", 0) == 0 )
    {
      // ignore this line
      continue;
    }
    else
    {
      try {
	//cout << line << endl;
	myRule = new ns_RuleReader::Rule(line); // this constructor throws a string exception
      } catch (string s) { 
	cerr << "Caught exception: " << s << endl;
	continue;
      }
    }

    int original_rule_line_number = (int)atof((myRule->getAttributeValue("rule_file_line_number")).c_str());
    int original_count = original_counts->at(original_rule_line_number);

    if( myRule->getAttributeValue("lm") == "yes" )
    {
      lm_true++;
      lm_true_count += original_count;

      if( !myRule->is_virtual_label() )
      {
	simple_lm_count += original_count;
      }
    }

    if( myRule->getAttributeValue("sblm") == "yes" )
    {
      sblm_true++;
      sblm_true_count += original_count;

      if( !myRule->is_virtual_label() )
      {
	simple_sblm_count += original_count;
      }
    }

    if( (myRule->getLHSRoot())->getString() == myRule->get_label() )
    {
      original_rule_count++;

      if( myRule->getAttributeValue("sblm") == "no" )
      {
	cout << "bad sblm, label: " << myRule->get_label() << endl;
      }
    }

    if( !myRule->existsAttribute("rule_file_line_number") )
    {
      cout << "No rule_file_line_number: " << line << endl;
    }

    string sline_number = myRule->getAttributeValue("rule_file_line_number");
    int line_number = (int)atof(sline_number.c_str());
    
    if( line_number != rule_id )
    {
      if( line_number == rule_id+1 )
      {
	rule_id++;
      }
      else
      {
	rule_id = line_number;
	cout << "bad line jump: " << line_number << endl;
      }
    }

    /*vector<string>* temp = myRule->getRHSLexicalItems();

    if( temp->size() > 2 )
    {
      non_binary_rules++;

      bool all_lexical = true;

      for( int i = 0; i < (int)temp->size() && all_lexical; i++ )
      {
	if( temp->at(i) == "" )
	{
	  all_lexical = false;
	}
      }

      if( all_lexical )
      {
	all_lexical_non_binary++;
	all_lexical_non_binary_count += temp->size()-1;
      }
    }

    temp = myRule->getRHSStates();

    if( temp->size() > 2 )
    {
      int var_count = 0;

      for( int i = 0; i < (int)temp->size(); i++ )
      {
	if( temp->at(i) != "" )
	{
	  var_count++;
	}
      }

      if( var_count > 2 )
      {
	multi_variable_count++;
      }
    }*/

    if( myRule->is_binarized_rule() )
    {
      if( myRule->is_virtual_label() )
      {
	// count how many duplicate rules there are
	string label = myRule->get_label();
	labels[label] = labels[label] + 1;

	if( labels[label] == 1 )
	{
	  // this is the first time we've seen this rule
	  // so get the lm and sblm counts
	  if( myRule->getAttributeValue("lm") == "yes" )
	  {
	    lm_true_sharing++;
	  }

	  if( myRule->getAttributeValue("sblm") == "yes" )
	  {
	    sblm_true_sharing++;
	  }
	}

	// keep track of the original rules that generated these
	if( original_rules.count(label) == 0 )
	{
	  original_rules[label] = new vector<string>;
	}



	(original_rules[label])->push_back(myRule->getAttributeValue("rule_file_line_number"));
      }
      else
      {
	non_virtual_count++;
	non_virtual_count_count += original_count;

	// this is a normal rule so we need to take the label and attach
	// the rhs elements
	/*label = myRule->get_label();

	vector<string>* rhs_states = myRule->getRHSStates();
	vector<string>* rhs_lexical = myRule->getRHSLexicalItems();
	vector<string>* rhs_constituents = myRule->getRHSConstituents();

	assert((int)rhs_states->size() > 0 &&
	       (int)rhs_states->size() <= 2 );


	for( int i = 0; i < (int)rhs_states->size(); i++ )
	{
	  if( rhs_states->at(i) != "" )
	  {
	    label += "_" + myRule->pos_lookup(rhs_states->at(i));
	  }
	  else if( rhs_lexical->at(i) != "" )
	  {
	    label += "_\"" + rhs_lexical->at(i) + "\"";
	  }
	  else
	  {
	    assert(rhs_constituents->at(i) != "");
	    
	    label += "_" + rhs_constituents->at(i);
	  }
	  }*/
      }

      total_virtual_rules++;
    }

    num_rules++;
    num_rules_count += original_count;

    delete myRule;
  }

  int unique_duplicated = 0;

  // figure out how many unique, duplicated rules there are
  for( hash_str_int::iterator it = labels.begin(); it != labels.end(); it++ )
  {
    if( (*it).second > 1 )
    {
      unique_duplicated++;
    }
  }

  cout << "Total rules: " << num_rules << endl;
  cout << "Total rules count: " << num_rules_count << endl;
  cout << "lm rules: " << lm_true << endl;
  cout << "lm rules count: " << lm_true_count << endl;
  cout << "simple binarize lm rules count: " << simple_lm_count << endl;
  cout << "sblm rules: " << sblm_true << endl;
  cout << "sblm rules count: " << sblm_true_count << endl;
  cout << "simple binarize lm rules count: " << simple_sblm_count << endl;
  cout << "> 2 rhs rules: " << non_binary_rules << endl;
  cout << "> 2 all lexical rhs rules" << all_lexical_non_binary << endl;
  cout << "non-lexical binary rules from > 2 rules: " << non_binary_rules - all_lexical_non_binary_count << endl;
  cout << "> 2 variables on rhs: " << multi_variable_count << endl;
  cout << "original rule count: " << original_rule_count << endl;
  cout << "Unique rules: " << labels.size() + non_virtual_count << endl;
  cout << "lm shared rules: " << lm_true_sharing + non_virtual_count << endl;
  cout << "sblm shared rules: " << sblm_true_sharing + non_virtual_count << endl;
  cout << "Unique duplicated rules: " << unique_duplicated << endl;
  cout << "Total virtual rules: " << total_virtual_rules << endl;

  // print out all of the counts
  for( hash_str_int::iterator it = labels.begin(); it != labels.end(); it++ )
  {
    // print out the label and the count
    cout << (*it).first << " " << (*it).second;

    // print out the rule file line number ocurrences
    cout << " {";

    vector<string>* temp = original_rules[(*it).first];

    for( int i = 0; i < (int)temp->size(); i++ )
    {
      if( i != 0 )
      {
	cout << " ";
      }

      cout << temp->at(i);
    }

    cout << "}" << endl;
  }
}
