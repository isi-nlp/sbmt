#define GRAEHL__SINGLE_MAIN
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/stopwatch.hpp>
#include <graehl/shared/time_space_report.hpp>
#include <graehl/shared/program_options.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/string_to.hpp>
#include <RuleReader/Rule.h>
#include <RuleReader/RuleNode.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <sstream>
#include <sbmt/grammar/syntax_rule.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <sbmt/grammar/lm_string.hpp>
#include <sbmt/dependency_lm/StupidDLMScorer.hpp>
#include <sbmt/dependency_lm/StupidDLMScorer3g.hpp>
#include <sbmt/ngram/LWNgramLM.hpp>
#include <sbmt/dependency_lm/DLM.hpp>
//#include "extra_rules.hpp"

char const *usage_str="decompose dlm string into model events."
    ;

using namespace std;
using namespace boost;
using namespace sbmt;
using namespace ns_RuleReader;

string decomp_type = "bilexical";
int dlm_order = 3;

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
  using namespace boost::program_options;
  using namespace graehl;
  istream_arg input_file("-");
  ostream_arg output_file("-");
  typedef boost::program_options::options_description OD;
  bool help;
  OD all(general_options_desc());
  ostream &log=cerr;
  all.add_options()
      ( "help,h"
      , bool_switch(&help)
      , "show usage/documentation"
      )
      ( "input,i"
      , defaulted_value(&input_file)
      , "input xrs rules"
      )
      ( "output,o"
      , defaulted_value(&output_file)
      , "output xrs rules"
      )
      ( "type,t"
      , defaulted_value(&decomp_type)
      , "what type of decomposition: bilexical|trigram"
      )
      ( "dlm-order,n"
      , defaulted_value(&dlm_order)
      , "the order of dlm (only for oridinary dlm)"
      )
      ;
  try {
      boost::program_options::variables_map vm;
      store(parse_command_line(argc,argv,all),vm);
      notify(vm);

      if (vm.count("help")) {
          cerr << all << endl;
          return 1;
      }
      
  } catch (std::exception &e) {
      log << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
      throw;
  }
    regex match_comment("\\$\\$\\$(.*)$");
    regex match_blankline("^(\\s)+$");
    string line;
    string rule_str;
    istream &input=input_file.stream();
    ostream &output=output_file.stream();

    in_memory_dictionary dict;

    while(input){
            getline(input, rule_str);

        
        rule_str = regex_replace(rule_str,match_comment,"");
        rule_str = regex_replace(rule_str,match_blankline,"");
        if (rule_str == "") continue;
        
        StupidDLMScorer<LWNgramLM>* dlmscorer = NULL;

        try {
            Rule r(rule_str);


            if(decomp_type == "ordinary-ngram"){
                if(r.existsAttribute("headmarker"))  { 
                    r.mark_head(r.getAttributeValue("headmarker"));
                } else {
                    cerr<<"No head in rule!\n";
                    exit(1);
                }
                ostringstream ost;
                OrdinaryNgramDLM ordDLM;
                ordDLM.setOrder(dlm_order);
                ordDLM.decompose(r, ost);
                (*r.getAttributes())["dep_lm_string"].value = ost.str();
                (*r.getAttributes())["dep_lm_string"].bracketed = true;
                output<< rule_str <<" dep_lm_string={{{ "<<ost.str()<<" }}}"<<" dep_lm=yes"<<endl;
            } else if(decomp_type == "trigram"){
                if(r.existsAttribute("headmarker"))  { 
                    r.mark_head(r.getAttributeValue("headmarker"));
                } else {
                    cerr<<"No head in rule!\n";
                    exit(1);
                }
                ostringstream ost;
                MultiDLM mdlm;
                mdlm.setOrder(dlm_order);
                mdlm.decompose(r, ost);
                (*r.getAttributes())["dep_lm_string"].value = ost.str();
                (*r.getAttributes())["dep_lm_string"].value = true;
                output << rule_str <<" dep_lm_string={{{ "<<ost.str()<<" }}}"<<" dep_lm=yes"<<endl;
            } else {{{

            string lbl = r.getLHSRoot()->getString(false,false);
           
            

            if(! r.existsAttribute("dep_lm_string"))  {
                output << r.entireString()<<endl;
                continue;
            }

            MultiDLM dlm;
            if(decomp_type == "bilexical"){
                dlmscorer = new StupidDLMScorer<LWNgramLM>(dlm);
            } else if(decomp_type == "trigram"){
                dlmscorer = new StupidDLMScorer3g<LWNgramLM>(dlm);
            }
            else if(decomp_type == "ordinary-ngram"){

            } else {
                std::cerr<<"Wrong decomposition type!\n";
                exit(1);
            }

            indexed_lm_string lmstr(r.getAttributeValue("dep_lm_string"), dict);

            indexed_lm_string::const_iterator it;
            deque<string> lmstr1;
            for(it = lmstr.begin(); it != lmstr.end(); ++it){
                if(it->is_token()){
                    if(it->get_token().type() == native_token){
                        lmstr1.push_back(string("\"") + dict.label(it->get_token()) + string("\""));
                    } else {
                        //<H> or </H>..
                        lmstr1.push_back( dict.label(it->get_token()) );
                    }
                } else {
                    ostringstream ost;
                    ost<<it->get_index();
                    lmstr1.push_back(ost.str());
                }
            }

            if(lbl == "TOP"){
                lmstr1.push_front("</H>");
                lmstr1.push_front("\"<top>\"");
                lmstr1.push_front("<H>");
            }

            //cout<<"RULE: "<<rule_str<<endl;
            //cout.flush();
            vector<string> dlmEvents = dlmscorer->getDLMEventsForTree(lmstr1);
            ostringstream ost;
            for(size_t i = 0; i < dlmEvents.size(); ++i){
                ost << dlmEvents[i]<<" ";
            }

            vector<string> unattached = dlmscorer->getUnscoredWords();
            ost<<"<H> ";
            for(size_t i = 0; i < unattached.size(); ++i){
                ost << unattached[i]<<" ";
            }
            ost<<"</H>";

            assert(unattached.size() && unattached[0].length());
            // is head var or token?
            bool isHeadVar = false;
            if(unattached[0][0] != '\"' || 
                    unattached[0][unattached[0].length() - 1] != '\"'){
                isHeadVar = true;
            }

            vector<string> bws = dlmscorer->getBoundaryWords();

            if(bws[0] != "") {
                ost<<" <LB> "<<bws[0]<<" </LB> ";
            } else {
                if(isHeadVar){  ost<<" <LB> "<<unattached[0]<<" </LB> ";}
                else { ost<<" <LB> "<<""<<" </LB> ";}
            }
            if(bws[1] != "") {
                ost<<"<RB> "<<bws[1]<<" </RB> ";
            } else {
                if(isHeadVar){ ost<<"<RB> "<<unattached[0]<<" </RB> ";}
                else { ost<<"<RB> "<<""<<" </RB> ";}
            }

            (*r.getAttributes())["dep_lm_string"].value = ost.str();
            (*r.getAttributes())["dep_lm_string"].bracketed = true;

            
            size_t posit = rule_str.find(string("dep_lm_string"));
            if(posit != string::npos){
                rule_str.replace(posit,  string("dep_lm_string").length(), "old_dep_lm_string");
            }
            output << rule_str <<" dep_lm_string={{{ "<<ost.str()<<" }}}"<<endl;
            //cout.flush();
           delete dlmscorer;
           dlmscorer = NULL;

            }}}
        
        } catch (std::exception const& e) {
            cerr << "exception trying to instantiate rule: " 
                 << rule_str << endl;
            cerr << "message: " << e.what() << endl;
            cerr << "this rule will be filtered out." << endl;
            if(dlmscorer) delete dlmscorer;
        }

    }

    return 0;
}

