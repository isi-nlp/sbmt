#define GRAEHL__SINGLE_MAIN
#include <sbmt/hash/oa_hashtable.hpp>
#include <graehl/shared/program_options.hpp>
#include <graehl/shared/command_line.hpp>
#include <sbmt/grammar/grammar.hpp>
//#include <sbmt/grammar/unique_lmstring_counts.hpp>
#include "grammar_args.hpp"
#include <fstream>
#include <iterator>

unsigned const GRAMMAR_VIEW_VERSION=3;


const char *usage_str="give a sbmt_decoder brf file or archive, and get back something like the original xrs rules, with a totalscore attribute added based on weights you provide";


int main(int argc, char** argv)
{
    using namespace std;
    using namespace sbmt;
    using namespace sbmt::logmath;
    using namespace boost::program_options;
    using namespace graehl;
    typedef options_description OD;
    istream_arg id_file("-");
    ostream_arg output_file("-"),
                all_output_file("-0"),
        binarized_output_file("-0"),
        native_file("-0"),
        unique_lmstring_file("-0");
    bool help;
    OD all(general_options_desc());
    grammar_args gramargs;
    bool print_score=false;
    bool quiet=false;
    bool scores_neglog10=false;
    gramargs.keep_texts=true;


    all.add_options()
        ("help,h", bool_switch(&help), "show usage/documentation")
        ("binarized-output",
         defaulted_value(&binarized_output_file),
         "print the binarized rules that cyk works with"
        )
        ("all-output,a", defaulted_value(&all_output_file), "print ALL the rules to this file")
        ("native-vocab-output,v", defaulted_value(&native_file), "print every native word from rules to this file (one per line)")
        ("id-input,i", defaulted_value(&id_file), "input xrs rule ids (space separated).  a 'w' id shows the feature weights.  -0 for none.")
        ("output,o", defaulted_value(&output_file), "output xrs rules corresponding to --id-file, one per line")
//      ("brf-output",defaulted_value(&brf_output), "output all binarized rules corresponding to --id-file")
        //FIXME: bad idea probably; couldn't reuse brf output anyway because syntax rules must occur before their binarized rules.  also, difficult to match weird brf text format exactly.  however, sub-archive and sub-grammar make sense
        ("print-sbtm-score,s", bool_switch(&print_score), "print (given weights) combined sbmt-score for each rule")
        ("quiet,q", bool_switch(&quiet), "don't prompt for id on STDERR")
        ("cost-neglog10,l",bool_switch(&scores_neglog10), "print neglog10 of score (cost>=0 with greater -> worse)")
//        ("unique-lmstring-output",defaulted_value(&unique_lmstring_file), "print summary of unique lmstrings per binary rhs")
        ;

    all.add(gramargs.options());

    try {
        variables_map vm;
        store(parse_command_line(argc,argv,all),vm);
        notify(vm);

        if (help) {
            std::cout << "\n" << argv[0] << ' ' << "version " << GRAMMAR_VIEW_VERSION << "\n\n"
                      << usage_str << "\n"
                      << all << "\n";
            return -1;
        }
    } catch (std::exception &e) {
        cerr << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
        throw;
    }
    gramargs.prepare(false);

    grammar_in_mem &g=gramargs.get_grammar();

    if (all_output_file) {
        ostream &out=all_output_file.stream();
        set_neglog10(out,scores_neglog10);
        BOOST_FOREACH(grammar_in_mem::rule_type r, g.all_rules())
        {
            if (g.is_complete_rule(r)) {
                out << print(g.get_syntax(r),g.dict());
		std::set<feature_id_type> seen;
		seen.insert(g.feature_names().get_index("id"));
                BOOST_FOREACH(score_byid_type sc, g.rule_scores(r)) {
                    out << ' ' << g.feature_names().get_token(sc.first) << '=' << sc.second;
		    seen.insert(sc.first);
                }
                BOOST_FOREACH(text_byid_type tc, g.rule_text(r)) {
		    if (seen.find(tc.first) == seen.end()) {
		        out << ' ' << g.feature_names().get_token(tc.first) << "={{{" << tc.second << "}}}";
		    }
		}
                if (print_score) {
                    out << " sbtm-score=" << g.rule_score(r);
                    out << " sbmt-heuristic=" << g.rule_score_estimate(r);
                }
                out << '\n';
            }
        }
        out << std::flush;
    }

    if (native_file) {
        ostream_iterator<string> oi(native_file.stream(),"\n");
        g.output_native_vocab(oi);
    }

    if (binarized_output_file) {
        ostream& out = binarized_output_file.stream();
        grammar_in_mem::rule_range rr = g.all_rules();
        grammar_in_mem::rule_iterator ritr = rr.begin(),
                                      rend = rr.end();
        for (; ritr != rend; ++ritr) {
            out << print(*ritr,g);
            if (print_score) {
                out << " sbtm-score=" << g.rule_score(*ritr);
                out << " sbmt-heuristic=" << g.rule_score_estimate(*ritr);
            }
            out << std::endl;
        }
    }
    if (id_file && output_file && !bool(all_output_file)) {
      istream &in=id_file.stream();
      ostream &out=output_file.stream();
      set_neglog10(out,scores_neglog10);
      ostream &log=cerr;
      syntax_id_type id;
      char c;

      oa_hash_map<syntax_id_type,grammar_in_mem::rule_type> syn2bin;
      BOOST_FOREACH(grammar_in_mem::rule_type r, g.all_rules()) {
        if (g.is_complete_rule(r)) {
          syn2bin.insert(make_pair(g.get_syntax(r).id(),r));
        }
      }

      for (;;) {
        if (!quiet)
          log << "syntax-id or exit: ";
        if (in >> c) {
          if (c=='w')
            out << print(g.get_weights(),g.feature_names()) << std::endl;
          else
            in.unget();
        }
        if (in >> id) {
          if (!quiet)
            log << id << endl;
          try {
            out << print(g.get_syntax(id),g.dict());
            std::set<feature_id_type> seen;
            seen.insert(g.feature_names().get_index("id"));
            BOOST_FOREACH(score_byid_type sc, g.rule_scores(syn2bin[id])) {
              out << ' ' << g.feature_names().get_token(sc.first) << '=' << sc.second;
              seen.insert(sc.first);
            }
		    BOOST_FOREACH(text_byid_type tc, g.rule_text(syn2bin[id])) {
              if (seen.find(tc.first) == seen.end()) {
                out << ' ' << g.feature_names().get_token(tc.first) << "={{{" << tc.second << "}}}";
              }
		    }
            if (print_score) {
              out << " sbtm-score=" << g.rule_score(syn2bin[id]);
              out << " sbtm-heuristic=" << g.rule_score_estimate(syn2bin[id]);
            }
          } catch (std::exception const& f) {
            log << "ERROR on id="<<id<<": "<<f.what() << endl;
          }
          out << endl;
        } else
          break;
      }
    }

//    if (unique_lmstring_file) {
//        ostream &out=unique_lmstring_file.stream();
//        unique_lmstring_counts c;
//        c.print_binary_rules_summary(out,g);
//    }

    return 0;
}

