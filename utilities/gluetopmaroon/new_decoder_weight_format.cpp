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
#include "extra_rules.hpp"
#include <xrsparse/xrs.hpp>

char const *usage_str="converts xrs rules from old decoder to new decoder foramt:\n"
    "converts negative log10 prob costs like 5 to 10^-5\n"
    "adds TOP rule if needed\n"
    "adds derivation-size and text-length features\n"
    ;

using namespace std;
using namespace boost;
using namespace ns_RuleReader;

namespace std {
ostream& operator<<(ostream& out, set<string> const& v)
{
    set<string>::const_iterator itr = v.begin(), end = v.end();
    if (itr != end) {
        out << *itr;
        ++itr;
    }
    for (; itr != end; ++itr) out << " , " << *itr;
    return out;
}
} // namespace std

unsigned int count_lexicals(rule_data const& r)
{
    unsigned int ret = 0;
    BOOST_FOREACH(lhs_node const& lhs, r.lhs) {
        if (not lhs.indexed and not lhs.children) ++ret;
    }
    return ret;
}

unsigned int count_productions(sbmt::fat_syntax_rule::tree_node const& n)
{
    unsigned int x = 0;
    if (n.indexed() or n.children_begin()->lexical()) return x;
    x += 1;
    BOOST_FOREACH(sbmt::fat_syntax_rule::tree_node const& cnd,n.children()) {
        x += count_productions(cnd);
    }
    return x;
}

unsigned int count_productions(rule_data const& r)
{
    sbmt::fat_token_factory tf;
    sbmt::fat_syntax_rule sr(r,tf);
    return count_productions(*sr.lhs_root());
}

unsigned count_foreign_lexicals(rule_data const& r)
{
    unsigned int ret = 0;
    BOOST_FOREACH(rhs_node const& rhs, r.rhs) {
        if (not rhs.indexed) ++ret;
    }
    return ret;
}

template <class V>
void set_feature(rule_data& r, string const& name, V const& value, bool overwrite = true)
{
    BOOST_FOREACH(feature& f, r.features) {
        if (f.key == name) {
            if (overwrite) {
                f.str_value = lexical_cast<string>(value);
            }
            return;
        }
    }
    feature f(std::make_pair(name,feature_value<string>(lexical_cast<string>(value))));
    f.compound=false;
    r.features.push_back(f);
}

void gen_corpus_lines( std::istream& corpus_file
                     , std::set<std::string>& corpus_lines )
{
  using namespace std;
  string line;
  while(corpus_file) {
    getline(corpus_file, line);
    corpus_lines.insert(line);
  }
}

namespace {
  template <class I,class To>
  bool try_stream_into(I & i,To &to,bool complete=true)
  {
    i >> to;
    if (i.fail()) return false;
    if (complete) {
      char c;
      return !(i >> c);
    }
    return true;
  }

  template <class To>
  bool try_string_into(std::string const& str,To &to,bool complete=true)
  {
    std::istringstream i(str);
    return try_stream_into(i,to,complete);
  }
}

int main(int argc, char** argv)
{
    bool drop_zeros=true;
    string zero_string="10^0";
    
    set<string> rhs_set;
    string native_string;

    set<string> corpus_lines;

    regex match_cluster_feature_functional_zero(" \\S+\\.[0-9]+=(e\\^-100|10\\^-20)(\\.0*)*");
    regex match_cluster_feature(".*\\.[0-9]+$");
    regex match_functional_zero("^(e\\^-100|10\\^-20)(\\.0*)*$");    
    regex match_e_hat("^e\\^(.*)");
    regex match_10_hat("^10\\^(.*)");
    regex match_comment("\\$\\$\\$(.*)$");
    regex match_blankline("^(\\s)+$");

    graehl::time_space_report report(cerr,"Finished converting xrs rules: ");
    
    /*
    graehl::stopwatch timer;
    graehl::istream_arg input_file_arg(argv[1]);
    graehl::ostream_arg output_file_arg(argv[2]);
    istream &input=input_file_arg.stream();
    ostream &output=output_file_arg.stream();
    */
    
    // copied from binal.cc
  using namespace boost::program_options;
  using namespace graehl;
  typedef options_description OD;
  istream_arg input_file("-");
  istream_arg corpus_file;
  ostream_arg output_file("-");
  bool use_glue = false;
  bool use_maroon = false;
  rule_data::rule_id_type id_origin=0;
  bool intro_f=true;
  bool old_top_glue=false;
  bool align_fully=false,strip_align=false;
  std::string foreign_start_token="<foreign-sentence>";
  bool help;
  bool generate_top=false;
  bool add_headmarker =  false;
//  bool use_epsilon_native;
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
      , "output xrs rules, with TOP rule and text-length, derivation-size"
      )
      ( "use-glue"
      , defaulted_value(&use_glue)
      , "build glue rules"
      )
      ( "use-maroon"
      , defaulted_value(&use_maroon)
      , "build word-skipping (maroon) rules"
      )
      ( "add-headmarker"
      , defaulted_value(&add_headmarker)
      , "add the headmarker to the glue and maroon rules"
      )
      ( "generate-top"
      , defaulted_value(&generate_top)
      , "generate a dummy TOP -> S rule, if no toplevel rule is seen"
      )
      ( "corpus-file"
      , defaulted_value(&corpus_file)
      , "raw input sentence file (required for maroon rules)"
      )
      ( "introduce-foreign-start"
      , defaulted_value(&intro_f)
      , "for the automatically built TOP rule, expect a --foreign-start-token at the start of the foreign"
      )
      ( "foreign-start-token"
        ,  defaulted_value(&foreign_start_token)
        ,  "used for TOP rule if --intro"
      )
      ( "old-top-glue-rule"
      , defaulted_value(&old_top_glue)
      , "preserve backwards compatability with earlier runs by wrongly adding glue-rule=10^-1 derivation-size=10^-1 to the TOP(GLUE) rule (if disabled, add separate top-glue-rule=10^-1 instead)"
      )
      ("id-origin", defaulted_value(&id_origin), "if [id-origin] is greater than max seen ruleid, then start ruleids for top/glue rules here")
      ("align-fully",defaulted_value(&align_fully), "if align= attribute is missing, generate one, fully connected (for lexemes), and normally connected for variables")
      ("strip-align",defaulted_value(&strip_align), "remove any align= attribute on input")
      ;
  positional_options_description po;
  po.add("input",1);
  po.add("output",1);
  try {
      basic_command_line_parser<char> cmd(argc,argv);
      variables_map vm;
      store(cmd.options(all).positional(po).run(),vm);
      notify(vm);
      if (help) {
          log << "\n" << argv[0]<<"\n\n"
                      << usage_str << "\n"
                      << all << "\n";
          return 1;
      }
  } catch (std::exception &e) {
      log << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
      throw;
  }
  istream &input=input_file.stream();
  ostream &output=output_file.stream();

  if (corpus_file) gen_corpus_lines(corpus_file.stream(),corpus_lines);

    string rule_str;
    
    bool top_rule_seen = false;
    rule_data::rule_id_type max_id = 0;
    
    while(input) {
        std::getline(input,rule_str);
        bool have_align=false;
        
        rule_str = regex_replace(rule_str,match_comment,"");
        rule_str = regex_replace(rule_str,match_blankline,"");
        rule_str = regex_replace(rule_str,match_cluster_feature_functional_zero,"");
        if (rule_str == "") continue;
        
        try {
            rule_data r = parse_xrs(rule_str);
            
            max_id = std::max(max_id,r.id);
            
            string lbl = r.lhs[0].label;
            if (lbl != "TOP") rhs_set.insert(lbl);
            else top_rule_seen = true;

            set_feature(r,"text-length",count_lexicals(r));
            set_feature(r,"foreign-length",count_foreign_lexicals(r));
            set_feature(r,"derivation-size",1);
            set_feature(r,"productions",count_productions(r));
            
            BOOST_FOREACH(feature& f, r.features) {
                string& key=f.key;
                string& val=f.str_value;

                double val_num;
                bool is_num = (not f.compound) and try_string_into(val,val_num);
                //if (regex_match(key,match_cluster_feature)) cerr << "CLUSTER FEATURE " << key << "=" << val << "\n";
                //if (regex_match(val,match_functional_zero)) cerr << "FUNCTIONAL ZERO " << key << "=" << val << "\n";
                if (regex_match(key,match_cluster_feature) and regex_match(val,match_functional_zero)) {
		    key="";
		    val="";
                    cerr << "FUNCTIONAL ZERO CLUSTER FEATURE FOUND\n";
		    continue;
                } else if ( key != id_feat
                     and
                     is_num
                     and
                     !regex_match(val,match_e_hat) 
                     and 
                     !regex_match(val,match_10_hat)
                    ) {
                    if (val_num==0.0) {
                        if (drop_zeros) {
                            key="";
                            val="";
                            continue;
                        } else {
                            val=zero_string;
                        }   
                    } else {
                        val="10^-"+val;
                    }
                } else if (key == "align") {
                    if (strip_align) {
                        key="";
                        val="";
                        continue;
                    } else
                        have_align=true;
                }
            }       
            
            output << r;
            
            if (!have_align && align_fully) {
                sbmt::fat_token_factory tf;
                sbmt::fat_syntax_rule sr(r,tf);
                output << " "+align_feat+"={{{";
                output << sr.default_alignment(true);
                output << "}}}";
            }
            
            output << endl;
        
        } catch (std::exception const& e) {
            cerr << "exception trying to instantiate rule: " 
                 << rule_str << endl;
            cerr << "message: " << e.what() << endl;
            cerr << "this rule will be filtered out." << endl;
        }
    }
    std::string fss;
    std::string const * use_top_align=&unary_align, *use_binary_align=&binary_align, *use_unary_align=&unary_align;
    if (intro_f) {
        fss="\""+foreign_start_token+"\" ";
        use_top_align=&top_align_fss;
    }

    std::string noalign;
    if (strip_align && !align_fully)
        use_top_align=use_binary_align=use_unary_align=&noalign;
    
    std::string fss_rhs=" -> "+fss+"x0";
    if (id_origin > max_id)
        max_id=id_origin;
    if ((not top_rule_seen) and generate_top) {
        output << top_string<< "(x0:S)"
               <<fss_rhs<<attr_sep
               <<id_feature<< (++max_id) << *use_top_align
               <<" headmarker={{{ R(D) }}} derivation-size=10^-1"
               << endl;
        cerr << "TOP->S rule id="<<max_id<<endl;
    }
    
    if (not rhs_set.empty() and use_glue) {
      cerr << "building glue rules for " << rhs_set << endl;
      max_id = glue_rules( output
                         , rhs_set
                         , max_id
                         , align_fully
                         , foreign_start_token
                         , add_headmarker
                         , false ); 
    }
    if (use_maroon) 
        max_id = maroon_rules(output, rhs_set, corpus_lines, max_id, add_headmarker);

    cerr << "new xrs rules written to " << output_file << endl;
    //<< " - time elapsed: "<< timer << endl;

    return 0;
}

