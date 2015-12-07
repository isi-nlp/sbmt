#ifndef SBMT_UTILITIES__OUTPUT_ARGS
# define SBMT_UTILITIES__OUTPUT_ARGS
# include <graehl/shared/program_options.hpp>
# include <graehl/shared/fileargs.hpp>
# include <graehl/shared/percent.hpp>
# include <graehl/shared/string_to.hpp>
# include <graehl/shared/string_match.hpp>

#include <sbmt/forest/kbest.hpp>
#include <sbmt/forest/global_prune.hpp>
#include <sbmt/forest/count_trees.hpp>
#include <sbmt/forest/used_rules.hpp>
#include <sbmt/forest/outside_score.hpp>
#include <sbmt/forest/print_forest.hpp>
#include <sbmt/forest/forest_em_output.hpp>
# include <sbmt/forest/derivation.hpp>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <sbmt/chart/chart.hpp>

# include <sbmt/testing.hpp>
# include <sbmt/multipass/kbest_output_compare_cells.hpp>
# include <sbmt/multipass/cell_heuristic.hpp>
# include <string>

# include <boost/foreach.hpp>

# include <sbmt/forest/derivation_tree.hpp>
# include <sbmt/forest/intersect_tree_forest.hpp>

# include "logging.hpp"
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(oa_log,"output-args",app_domain);

namespace sbmt {

struct set_log_out {
    void operator()(std::string const& filename)
    {
        io::logfile_registry& reg = io::logfile_registry::instance();
        reg.set_logfile(root_domain,filename);
        if (filename == "-0") reg.loglevel(root_domain,io::lvl_none);
    }
};

struct output_args {

    unsigned nbests;
    unsigned per_estring_nbests;
    graehl::ostream_arg output,
                        nbest_output,
                        used_syntax_rules_file,
                        outside_score_file,
                        print_forest_file,
                        print_forest_em_file,
                        print_brf_forest_file,
                        status_file;
    std::string log_filename;
    std::string find_derivation_tree;
    score_t global_beam;
    size_t max_xrs_equivalents;
    print_forest_options print_forest_opt;
    derivation_output_options deriv_opt ;
    bool also_inside,also_total;
    graehl::size_t_bytes reserve_nbest;
    xrs_forest_andnode_type forest_and_type;
    xrs_forest_ornode_type forest_or_type;
    void set_defaults()
    {
        forest_and_type=xrs_forest_andnode_type::id;
        forest_or_type=xrs_forest_ornode_type::traditional;
        global_beam=1e-100;
        max_xrs_equivalents=0;
        also_inside=false;
        also_total=true;
        per_estring_nbests=0;
        deriv_opt.set_default();
        deriv_opt.info_wt=1; // redundant but just want to be sure (for multilm)
        nbests = 99 ;
        output=graehl::stdout_arg() ;
        status_file=graehl::stderr_arg() ;
        reserve_nbest=200*1024*1024;
    }

    unsigned revise_max_equivalents(unsigned max_equivalents)
    {
        if (max_equivalents > nbests && !print_forest_file && find_derivation_tree.empty()) {
            SBMT_WARNING_STREAM(oa_log,"--max-equivalents was set higher than --nbests "
                                        "(meaningless for nbest output)!");
//            max_equivalents=nbests > 0 ? nbests:1;
        }
        return max_equivalents;
    }

    output_args()
    {
        set_defaults() ;
    }

    double &lm_weight()
    {
        return deriv_opt.info_wt;
    }

    std::ostream& status() { return *status_file; }

    std::ostream &out()
    { return *output; }

  std::ostream &nbest_out() // retarded way to accomplish weird default semantics: nbest output goes to -o unless you specify otherwise
    {
      return nbest_output ? *nbest_output : *output;
    }

  // s was already printed on nbest_out()
  void out_line(std::string const& s) {
    if (nbest_output && nbest_output!=output)
      out()<<s<<std::endl;
  }


    void reset() //FIXME: what is this for?  fairly certain it originally reset more.
    {
        nbest_output.set_none() ;
    }

    void prep_stream(std::ostream &o) const
    {
        sbmt::logmath::set_neglog10(o,deriv_opt.scores_neglog10);
    }

    void validate_output(graehl::ostream_arg const& o,const char *name)
    {
        //throw_unless_valid(o,name);
        prep_stream(o.stream());
    }


    void validate_optional_output(graehl::ostream_arg const& o,const char *name)
    {
        if (o)
            validate_output(o,name);
    }

    void validate()
    {
        validate_output(output,"output");
        validate_output(status_file,"status");

        validate_optional_output(nbest_output,"nbest-output");
        validate_optional_output(used_syntax_rules_file,"used-syntax-rules");
        validate_optional_output(outside_score_file,"outside-score-file");
        validate_optional_output(print_forest_file,"print-forest-file");
        validate_optional_output(print_forest_em_file,"print-forest-em-file");
        validate_optional_output(print_brf_forest_file,"print-brf-forest-file");
    }

    template <class ET, class GT>
    std::string
    dummy_output_results( std::size_t sentid
                        , GT& gram
                        , concrete_edge_factory<ET,GT>& efact
                        , std::string const& msg
                        , unsigned pass=0
                        )
    {
        using namespace std;
        using namespace graehl;
        validate();

        derivation_interpreter<ET,GT> interp(gram,efact,deriv_opt);

        io::logging_stream& log = io::registry_log(oa_log);
        SBMT_ERROR_MSG( oa_log
                      , "Failed parse of sent=%1%: %2%\n"
                        "Dummy best and nbest list for failed parse:"
                      , sentid % msg );

        count_trees::print_dummy(log << io::info_msg,"");

        if (outside_score_file)
            outside_score_file.stream() << endl << "TOP[0,0] outside=nan"
                                        << endl << endl;

        if (print_forest_file)
            print_forest_file.stream() << endl << "()" << endl;

        if (print_forest_em_file) {
            print_forest_em_file.stream() << "(0<noparse:1> \"NOPARSE\" )" << endl;
        }
        
        if (print_brf_forest_file) {
            print_brf_forest_file.stream() << "()" << endl;
        }

        string fakebest=interp.nbest(sentid,pass);
        fakebest+=" fail-msg={{{ ";
        fakebest+=msg;
        fakebest+=" }}}";
        nbest_out() << fakebest << endl;
        out_line(fakebest);
        return fakebest;
    }

    void log_equiv_change(char const* event,std::size_t before,std::size_t after)
    {
        io::logging_stream& log = io::registry_log(oa_log);
        log << io::info_msg << "After " << event<<",  #items="<< after<<
            "; before="<<before<<"; after/before="<<graehl::percent<4>(after,before)<< io::endmsg;
    }

    //NOTE: chart is cleared.
    template <class ET, class GT>
    std::string output_results( edge_equivalence<ET> const& top_equiv
                              , GT& gram
                                , concrete_edge_factory<ET,GT>& efact
                                , unsigned sentid
                                , unsigned pass=0
                                , cell_heuristic const* pcells=0
                                , extra_english_output_t *eo=0
        )
    {
        outside_score always_recompute_outside;
        return output_results(top_equiv,gram,efact,sentid,always_recompute_outside,pass,pcells,eo);
    }

    //NOTE: chart is cleared.
    template <class ET, class GT>
    std::string output_results( edge_equivalence<ET> const& top_equiv
                              , GT& gram
                              , concrete_edge_factory<ET,GT>& efact
                              , unsigned sentid
                              , outside_score &outside
                              ,  unsigned pass=0
                              , cell_heuristic const* pcells=0
                              , extra_english_output_t *eo=0
                              )
    {
        edge_equivalence_pool<ET> epool; //dummy arg
        validate();

        typedef ET edge_type;
        typedef GT gram_type;
        typedef edge_equivalence<ET> edge_equiv_type ;
        typedef derivation_interpreter<ET,GT> Interp ;

        using namespace std ;
        using namespace graehl ;

        io::logging_stream& log = io::registry_log(oa_log);
        log << io::info_msg << "Succesful parse of sent="<<sentid<<io::endmsg;

        ostream &nbest_out=this->nbest_out(); // if nbests, they'll go here

        unsigned compare_verbose=0;
        bool compare_missing_ends_nbest=false;

        if (pcells)
            pcells->check_target_span(top_equiv.representative().span());

        // comparing cells is done to gather info about the impact of multipass cell restrictions
        kbest_output_compare_cells<edge_type,gram_type>
            kout(nbest_out,gram,efact,epool,deriv_opt,pcells,
                 compare_verbose,compare_missing_ends_nbest);


        # ifndef NDEBUG
        edge_type topedge = top_equiv.representative();
        BOOST_FOREACH(edge_type const& topalt, top_equiv)
        {
            assert(topedge.score() >= topalt.score());
            assert(topedge.inside_score() >= topalt.inside_score());
        }
        # endif

        if (used_syntax_rules_file) {
            print_used_syntax_rules(
                top_equiv,used_syntax_rules_file.stream(),
                gram,"parse-forest-uses",true);
        }

        if (global_beam.is_zero()) {
            count_trees::print_equiv(top_equiv, log << io::info_msg,"");
        } else {
            count_trees::print_equiv(top_equiv, log << io::info_msg<<"pre-pruning (global beam="<<global_beam<<", ");
            global_prune::prune(top_equiv,global_beam,outside);
            count_trees::print_equiv(top_equiv, log << io::info_msg<<"post-pruning (global beam="<<global_beam<<", ");
        }
	/* DOES NOT COMPILE
        if (!find_derivation_tree.empty()) {
	    syntax_derivation_tree dt = boost::lexical_cast<syntax_derivation_tree>(find_derivation_tree);

            log << io::info_msg<<"Looking for derivation tree in forest: "<<dt<<io::endmsg;
//FIXME: build index only once per grammar?
            binary_derivation_tree_p bd=syntax_to_binary_derivation<gram_type>(gram).to_binary_p(dt);
            log << io::info_msg<<"Looking for binarized derivation tree in forest: ";
            print_binary_derivation(continue_log(log),*bd,gram);
            continue_log(log) << io::endmsg;
            //print_tree_in_chart(log << io::info_msg,top_equiv,chart,*bd,sentid);
            continue_log(log) << io::endmsg;
        }
        */
        if (outside_score_file) {
            outside.print_equiv(
                outside_score_file.stream(),
                top_equiv,
                gram,
                efact,
                also_inside,
                also_total);
            outside_score_file.stream() << endl;
        }

        if (print_forest_file) {
            print_forest(
                print_forest_file.stream(),
                top_equiv,
                gram,
                efact,
                print_forest_opt,
                outside
                );
        }
;
        if (print_forest_em_file) {
            print_forest_em( print_forest_em_file.stream()
                           , gram
                           , efact
                           , top_equiv
                           , forest_and_type
                           , forest_or_type
                           , max_xrs_equivalents );
            print_forest_em_file.stream() << endl;
        }
        
        if (print_brf_forest_file) {
            brf_forest_printer<ET,GT>().print_forest(print_brf_forest_file.stream(),top_equiv,gram);
            print_brf_forest_file.stream() << endl;
        }

        nbest_out << flush;

        SBMT_INFO_MSG_TO( log
                        , "Enumerating (up to) %1% best derivations:\n"
                          "best score: %2%"
                        , nbests % top_equiv.representative().inside_score()
                        );
        if (nbests) {
            try {
                io::log_time_space_report report(log, io::lvl_info, "Nbest: ");
                kout.print_kbest(top_equiv,sentid,pass,nbests,per_estring_nbests,eo); // goes to nbest_out()
            } catch (exception& e) {
                //++nbests_failed;
                SBMT_ERROR_MSG_TO( log
                                 , "nbest exception: %1%"
                                 , e.what() );
            }
        }
        SBMT_INFO_MSG_TO(
            log
          , "(After nbest) number of equivalences in memory: %1%"
          , edge_equiv_type::equivalence_count()
        );

        string best_str = kout.interp.nbest(sentid,pass,best_derivation(top_equiv),0,eo);
        out_line(best_str);
        return best_str;
    }
    boost::program_options::options_description
    options()
    {
        using namespace boost::program_options;

        options_description od("output options");
        od.add_options()
            ( "reserve-for-nbest"
            , value(&reserve_nbest)
            , "ensure this much memory is still available for nbest extraction "
              "after decoding completes"
            )
            ( "nbests,n"
            , value(&nbests)
            , "display this many translations starting with the best found, "
              "sorted by decreasing score"
            )
            ( "per-estring-nbests"
              , value(&per_estring_nbests)
              , "skip all but this many derivations with the same estring (0 => unlimited)")
            ( "output,o"
            , value(&output)
            , "print 1-best translations here"
            )
            ( "used-syntax-rules"
            , value(&used_syntax_rules_file)
            , "print syntax rules that are used in any found "
              "complete parse to this file"
            )
            ( "nbest-output"
            , value(&nbest_output)
            , "(if not -0) print nbest lists here "
              "and print just 1best to --output"
            )
            ( "log-file,l"
            , value(&log_filename)->notifier(set_log_out())
            , "Send log messages here"
            )
            ( "status-log"
            , value(&status_file)
            , "send decoder status messages here (start-decode, finish-decode, "
              "grammar-load start, grammar-load finish)"
            )
            ( "outside-score-file"
            , value(&outside_score_file)
            , "print outside score for all useful items (participating in any full parse)"
            )
            ( "also-total-score"
            , value(&also_total)
            , "print total as well as outside for --outside-score-file"
            )
            ( "print-forest-em-file"
            , value(&print_forest_em_file)
            , "print in forest-em format an unweighted forest"
            )
            ( "print-brf-forest-file"
            ,  value(&print_brf_forest_file)
            , "currently only used to determined binarized rule span coverage"
            )
            ( "print-forest-em-and-node-type"
            , value(&forest_and_type)
            , "type of and-node in em-forest. can be id, target_string, target_tree"
            )
            ( "print-forest-em-or-node-type"
            , value(&forest_or_type)
            , "type of or-node in em-forest. can be traditional, mergeable"
            )
            ( "print-forest-file"
            , value(&print_forest_file)
            , "prints the (pruned) parse forest to this file")
            ( "print-forest-outline"
            , value(&print_forest_opt.emacs_outline_mode)
            , "indent forest using emacs outline-mode **** indentation"
            )
            ( "print-forest-outside"
            , value(&print_forest_opt.outside_score)
            , "print outside and global_beam=inside*outside/best_inside in --print-forest"
            )
            ( "print-forest-meaningful-names"
            , value(&print_forest_opt.meaningful_names)
            , "print human-understandable item names (false -> opaque numeric ids)"
            )
            ( "print-forest-whole-syntax-rule"
            , value(&print_forest_opt.whole_syntax_rule)
            , "print the entire syntax rule on every use"
            )
            ( "quote-always"
            , value(&deriv_opt.quote_always)
            , "always quote tags/tokens (default: only as needed)"
            )
            ( "print-lattice-align",
              value(&deriv_opt.print_lattice_align),
              "if rules have 'align' attribute, print derivation source-lattice-align attribute as well as align"
                )
            ( "quote-never"
            , value(&deriv_opt.quote_never)
            , "never quote tags/tokens, even if they contain special chars "
              "- dangerous for some grammars!"
            )
            ( "new-fieldnames"
            , value(&deriv_opt.new_fieldnames)
            , "estring,etree instead of hyp,tree"
            )
            ( "binarized-tree-foreign"
            , value(&deriv_opt.print_foreign_bin_tree)
            , "show complete foreign-side binarized tree"
            )
            ( "edge-information"
            , value(&deriv_opt.print_edge_information)
            , "show information about the edges in the final structure"
            )
            ( "show-spans"
            , value(&deriv_opt.show_spans)
            , "show [fspan][espan] after each etree rule node, "
              "and [fspan] before each ftree node"
            )
            ( "lisp-style-trees"
            , value(&deriv_opt.lisp_style_trees)
            , "lisp-style trees e.g. (0 (1 2) 3).  non-lisp: 0(1(2) 3))"
            )
            ( "cost-neglog10"
            , value(&deriv_opt.scores_neglog10)
            , "print neglog10 of score (cost>=0 with greater -> worse)"
            )
            ( "print-used-rules"
            , value(&deriv_opt.print_used_rules)
            , "print used xrs rules, # occurences, and weighted score"
            )
            ( "print-foreign-binary-tree"
            , value(&deriv_opt.print_foreign_bin_tree)
            , "show (binary) foreign parse tree"
            )
            ( "print-foreign-tree"
            , value(&deriv_opt.print_foreign_tree)
            , "show implied foreign parse tree (english labels over foreign words)")
            ( "print-foreign-string"
            , value(&deriv_opt.print_foreign_string)
            , "show foreign sentence for which the estring is a translation"
            )
            ( "more-info-details"
            , value(&deriv_opt.more_info_details)
            , "for each nbest, print as many info component scores as possible "
              "(e.g. individual ngram prob/backoffs)"
            )
            ( "max-xrs-equivalents"
            , value(&max_xrs_equivalents)
            , "keep only up to this many AND nodes in an OR node.  0 disables"
            )
            ( "prune-worse-by"
            , value(&global_beam)
            , "after parsing, remove any hypotheses that are worse than score*this "
              "(e.g. 10^-100).  0 keeps all, 1 removes all but 1best.  "
              "no impact on --keep-cell-global-beam"
            )
            ( "check-info-score"
            , value(&deriv_opt.check_info_score)
            , "complain when nbest computed info score doesn't match info score from decoding. "
              "disable e.g. for first pass with lower --ngram-order"
            )
            ( "reserve-for-nbest"
            , value(&reserve_nbest)
            , "ensure this much memory is still available for nbest extraction "
              "after decoding completes"
            )
            ("find-derivation-tree"
            , value(&find_derivation_tree)
            , "search for this derivation in the pruned derivations forest"
            )
            ;
        return od;
    }
} ;

} // namespace sbmt
#endif
