#define GRAEHL__SINGLE_MAIN
#include <RuleReader/Rule.h>
#include <RuleReader/RuleNode.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <sstream>

# include <graehl/shared/fileargs.hpp>

char const *usage_str="Add headmarker field to rules. n"
    "dont care if there has already headmarker in rules.\n"
    "Rule LHS must have head marked. If no, then assume the right-most child will be the head.\n"
    ;

using namespace std;
//using namespace boost;
using namespace ns_RuleReader;


int main(int argc, char** argv)
{
  //using namespace boost::program_options;
  using namespace graehl;
  //typedef printable_options_description<std::ostream> OD;
  istream_arg input_file("-");
  istream_arg corpus_file;
  ostream_arg output_file("-");
  std::string foreign_start_token="<foreign-sentence>";
#if 0
//  bool use_epsilon_native;
  //OD all(general_options_desc());
  ostream &log=cerr;
  //all.add_options()
   //   ( "help,h"
    //  , bool_switch(&help)
     // , "show usage/documentation"
      //)
      //;
//  positional_options_description po;
//  po.add("input",1);
//  po.add("output",1);
  try {
      variables_map vm;
      all.parse_options(argc,argv,vm);
      log << "### COMMAND LINE:\n" << graehl::get_command_line(argc,argv,NULL) << "\n\n";
      log << "### CHOSEN OPTIONS:\n";
      all.print(log,vm,OD::SHOW_DEFAULTED);
            
      if (help) {
          log << "\n" << argv[0]<<"\n\n"
                    << usage_str << "\n"
                    << all << "\n";
          return false;
      }
  } catch (std::exception &e) {
      log << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
      throw;
  }
#endif

    string rule_str;
    
    while(getline(cin, rule_str)) {
        //std::getline(in,rule_str);
        if (rule_str == "") continue;
        
        try {
            Rule r(rule_str);
            r.setHeads(r.getLHSRoot());
            r.restoreRoundBrackets(r.getLHSRoot());
            ostringstream ost;
            r.dumpDeplmString(r.getLHSRoot(), ost);
            if (r.getAttributes()->find("headmarker") == r.getAttributes()->end()) {
                Rule::attribute_value av;
                av.value = ost.str();
                av.bracketed = true;
                r.getAttributes()->insert(make_pair("headmarker",av));
            } else {
                r.getAttributes()->erase("headmarker");
            }
            cout << r.entireString() << std::endl;
        } catch (std::exception const& e) {
            cerr << "exception trying to instantiate rule: " 
                 << rule_str << endl;
            cerr << "message: " << e.what() << endl;
            cerr << "this rule will be filtered out." << std::endl;
        }
    }

    return 0;
}

