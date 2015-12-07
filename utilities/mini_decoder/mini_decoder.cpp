# include "mini_decoder.hpp"
# include <boost/preprocessor/iteration/local.hpp>
# include <sbmt/grammar/rule_feature_constructors.hpp>
// Do not include mini_decoder.ipp.  use explicit instantiation instead, and
// instantiate the templates you need from mini_decoder.ipp in another
// translation unit -- Michael
//#include "mini_decoder.ipp"

# include <iostream>

char const* ARGV0="mini_decoder";

////////////////////////////////////////////////////////////////////////////////

template <class ChartInit, class ParseOrder>
std::string
mini_decoder::decode(ChartInit cinit, ParseOrder po,std::size_t sentid, span_t tgt,unsigned pass,std::string const& pass_desc)
{
    graehl::stopwatch timer;
    std::string translation;

    edge_stats stats;
    if (na.using_ngram_lm()) {
        if (false) { }
#define NGORDER(n,m) \
            else if (na.ngram_order == n) { \
                typedef typename ngram_edge<n>::type \
                                 edge_t; \
                if(na.using_dependency_lm()){ \
                    if(na.dlm_order == m ) { \
                        typedef typename ngram_dlm_edge<n,m>::type \
                                         edge_t; \
                        if (na.using_dynamic_lm())                                 \
                            decode_ngram_order<n,m,dynamic_ngram_lm>( \
                                                   hide_chart_init<edge_t>(cinit) \
                                                 , hide_parse_order<edge_t>(po) \
                                                 , sentid \
                                                 , tgt \
                                                 , stats \
                                                 , translation \
                                                 , na.dynamic_language_model \
                                                 , pass \
                                                 , na.dependency_language_model);      \
                        else \
                            decode_ngram_order<n,m,LWNgramLM>( \
                                                   hide_chart_init<edge_t>(cinit) \
                                                 , hide_parse_order<edge_t>(po) \
                                                 , sentid \
                                                 , tgt \
                                                 , stats \
                                                 , translation \
                                                   , na.language_model,pass \
                                                 , na.dependency_language_model);      \
                    }  \
                } else {\
                typedef typename ngram_edge<n>::type \
                                 edge_t; \
                if (na.using_dynamic_lm())                                 \
                    decode_ngram_order<n,dynamic_ngram_lm>( \
                                           hide_chart_init<edge_t>(cinit) \
                                         , hide_parse_order<edge_t>(po) \
                                         , sentid \
                                         , tgt \
                                         , stats \
                                         , translation \
                                           , na.dynamic_language_model,pass);      \
                else \
                    decode_ngram_order<n,LWNgramLM>( \
                                           hide_chart_init<edge_t>(cinit) \
                                         , hide_parse_order<edge_t>(po) \
                                         , sentid \
                                         , tgt \
                                         , stats \
                                         , translation \
                                           , na.language_model,pass);      \
                } \
            }

            // whitespace critical to next line
            # define BOOST_PP_LOCAL_LIMITS (MINI_MIN_NGRAM_ORDER, MINI_MAX_NGRAM_ORDER)
            # define BOOST_PP_LOCAL_MACRO(N) NGORDER(N,3)
            # include BOOST_PP_LOCAL_ITERATE()

            //TODO: boost preprocessor loop up to MAX_NGRAM_ORDER ?? (keep in mind LW LM may also need adjusting)
#undef NGORDER
        else {
            throw_if(true,"--ngram-order out of range");
        }
    }  else if (na.using_dependency_lm()){ // TM + DLM (no ngram)
        if(na.dlm_order == 3){
            typedef edge_info_factory<head_history_dlm_info_factory<2,LWNgramLM> > IF;
            typedef typename IF::info_type Info;
            typedef sbmt::edge<Info> edge_t;
            decode_dlm_order<3>( hide_chart_init<edge_t>(cinit)
                            , hide_parse_order<edge_t>(po)
                            , sentid
                            , tgt
                            , stats
                            , translation
                            , pass
                            , na.dependency_language_model );
        } else if(na.dlm_order == 4 ) {
            typedef edge_info_factory<head_history_dlm_info_factory<3,LWNgramLM> > IF;
            typedef typename IF::info_type Info;
            typedef sbmt::edge<Info> edge_t;
            decode_dlm_order<4>( hide_chart_init<edge_t>(cinit)
                            , hide_parse_order<edge_t>(po)
                            , sentid
                            , tgt
                            , stats
                            , translation
                            , pass
                            , na.dependency_language_model );
        }
    } else {
        typedef edge_factory<edge_info_factory<null_info_factory> > null_factory_t;
        typedef edge_info<null_info> Info;

        typedef sbmt::edge<Info> Edge;
        concrete_edge_factory<Edge,grammar_in_mem> ef(boost::in_place<null_factory_t>(weight_tm_heuristic,weight_info_heuristic));
        decode_dispatch( ef
                       , hide_chart_init<Edge>(cinit)
                       , hide_parse_order<Edge>(po)
                       , sentid
                       , tgt
                       , stats
                       , translation
                       , pass );
    }
    SBMT_INFO_MSG(
        decoder_app
      , "sentence #%1%%5% length=%2% time=%3% - %4%.\n"
      , sentid % tgt.right() % timer % stats % pass_desc
    );
    return translation;
}

////////////////////////////////////////////////////////////////////////////////


void mini_decoder::log_multipass_options()
{
    SBMT_INFO_STREAM(decoder_app,"Multipass options changed:");
    for (unsigned i=0;i<multipass_opts.size();++i)
    {
        SBMT_INFO_STREAM(decoder_app,
                         "pass="<<i+1<<" options: "<<boost::trim_copy(multipass_opts[i]));
    }
}


////////////////////////////////////////////////////////////////////////////////

void mini_decoder::pre_decode_hook()
{
    if (num_threads==0)
        multi_thread=false;

    if (!use_kept_cells && !compare_kept_cells)
        cells.reset(0);

    if (recompute_weights)
        reload_weights();
}

void mini_decoder::validate()
{
    using namespace graehl;

    na.validate();

    if(instruction_file.valid())
        throw_if( foreign.valid()
                , "don't specify both --instruction-file and --foreign" );
    else
        throw_unless( foreign.valid()
                    , "specify either --foreign or --instruction-file" );


    fa.max_equivalents=oargs.revise_max_equivalents(fa.max_equivalents);
}

////////////////////////////////////////////////////////////////////////////////

bool mini_decoder::run()
{

    if (!prepared)
        prepare();
    string sent;
    graehl::stopwatch timer;

    using namespace boost;

    decode_sequence_reader reader;
    reader.set_multipass_options_callback(
        bind(&mini_decoder::multipass_options,this,_1)
        );
    reader.set_change_options_callback(
        bind(&mini_decoder::change_options,this,_1)
        );
    reader.set_load_dynamic_ngram_callback(
        bind(&mini_decoder::load_dynamic_lm,this,_1,_2)
        );
    reader.set_decode_callback(
                bind(&mini_decoder::decode_out,this,_1,_2)
           );
    reader.set_load_grammar_callback(
                bind(&mini_decoder::load_grammar,this,_1,_2,_3)
           );
    reader.set_weights_callback(bind(&grammar_args::set_feature_weights,ref(ga),_1));
    reader.set_push_grammar_callback(bind(&mini_decoder::push_grammar,this,_1,_2));
    reader.set_pop_grammar_callback(bind(&mini_decoder::pop_grammar,this));
    reader.set_decode_lattice_callback(
                bind(&mini_decoder::decode_lattice_out,this,_1,_2)
            );
    if (no_instruction_filename()) {
        reader.read(instruction_file.stream());
    } else {
        reader.read(instruction_file.name);
    }

    SBMT_INFO_MSG(
        decoder_app
      , "total time for sentences: %1%\n"
        "%2% failed parses and %3% failed nbests"
      , timer % parses_failed % nbests_failed
    );

    return !(parses_failed || nbests_failed);
}

////////////////////////////////////////////////////////////////////////////////

void mini_decoder::push_grammar( std::string const& filename, sbmt::archive_type a )
{
    status() << "push-grammar-start path=" << filename << endl;
    try {
        ga.push_grammar(filename,a);
    } catch(std::exception& e) {
        status() << "push-grammar-finish path=" << filename
                 << " status=fail" << endl;
        SBMT_ERROR_MSG(
            decoder_app
          , "failed to push grammar %1%. %2%"
          , filename % e.what()
        );
        throw;
    }
    status() << "push-grammar-finish path=" << filename
             << " status=success" << endl;
}

void mini_decoder::pop_grammar()
{
    status() << "pop-grammar-start " << endl;
    try {
        ga.pop_grammar();
    } catch(std::exception& e) {
        status() << "pop-grammar-finish status=fail" << endl;
        SBMT_ERROR_MSG(
            decoder_app
          , "failed to pop grammar. %2%"
          , e.what()
        );
        throw;
    }
    status() << "pop-grammar-finish status=success" << endl;
}

void mini_decoder::load_grammar( std::string const& filename
                               , std::string const& weight_str
                               , sbmt::archive_type a )
{
    status() << "load-grammar-start"
             << " path=" << filename
             << " weight-string=" << weight_str
             << endl;
    try {
        ga.load_grammar(filename,weight_str,a);
        //        na.set_grammar(grammar()); // if you're paranoid only - otherwise signal should manage it for you
    } catch(std::exception& e) {
        status() << "load-grammar-finish"
                 << " path=" << filename
                 << " weight-string=" << weight_str
                 << " status=fail" << endl;
        SBMT_ERROR_MSG(
            decoder_app
          , "failed to load grammar %1%.  %2%"
          , filename % e.what()
        );
        throw;
    }
    status() << "load-grammar-finish"
                 << " path=" << filename
                 << " weight-string=" << weight_str
                 << " status=success" << endl;
}

////////////////////////////////////////////////////////////////////////////////

bool mini_decoder::parse_args(int argc, char const **argv,bool quiet)
{
    cmdline_str=graehl::get_command_line(argc,argv,NULL);

    using namespace boost::program_options;
    using namespace graehl;
    typedef printable_options_description<std::ostream> OD;

    double changes_since;
    set_null(changes_since);
    bool help=false;

    OD general_opts(general_options_desc());
    general_opts.add_options()
        ("help,h", bool_switch(&help),
         "show usage/documentation")
        ("changelog", value<double>(&changes_since),
         "show changelog since version (negative = N revisions ago)")
        ("foreign,f", defaulted_value(&foreign),
         "bare, tokenized foreign sentences to translate, one per line")
        ( "instruction-file"
        , defaulted_value(&instruction_file)
        , "file containing a sequence of grammar loads or decodes.  "
          "alternative to ---foreign, --brf-grammar, --grammar-archive"
        )
        ( "multi-thread"
          , bool_switch(&multi_thread) //FIXME: bool_switch is evil, causes problems for multipass and change-options (gets disabled if not reiterated)
        , "dispatch cky-span-combining to multiple threads"
        )
        ( "num-threads"
        , defaulted_value(&num_threads)
        , "number of threads to use with multi-threaded cky. (0 disables multi-thread)"
        )
        ( "max-items"
        , defaulted_value(&max_items)
        , "abort a decoding attempt after this many items are entered into "
          "the chart (0=unlimited)"
        )
        ;
    OD grammar_opts = ga.options();
//    ga.add_options(grammar_opts);

    OD lm_opts("Language model options");
    na.add_options(lm_opts);
    lm_opts.add_options()
        (WEIGHT_LM_NGRAM , defaulted_value(&weight_lm_ngram),
         "weight for language model - overrides weight-lm-ngram in grammar weights file/string");

    OD output_opts = oargs.options();

    //oargs.add_options(output_opts);

    OD multipass_opts("Multipass options");
    multipass_opts.add_options()
        ("keep-cells", defaulted_value(&keep_cells),
         "save the actual outside (heuristic) score for each cell, for use in the next decode (of the same sentence)")
        ("keep-cell-global-beam", defaulted_value(&keep_cells_beam),
         "only keep cells whose best derivation prob is at least keep-cells-global-beam * (global) 1best")
        ("use-kept-cells", defaulted_value(&use_kept_cells),
         "the next decode uses the previously kept cells table (if false, the table is cleared before decode)")
        ("weight-cell-outside", defaulted_value(&weight_cell_outside),
         "if use-kept-cells=1, then use the best outside cost for a cell times this weight, as a heuristic")
        ("weight-tm-heuristic", defaulted_value(&weight_tm_heuristic),
         "at 1, accurately access virtual nonterminals' best real completion cost.  at 0, do not (would favor virtuals without weight-cell-outside=1)")
        ("weight-info-heuristic", defaulted_value(&weight_info_heuristic),
         "at 1, include e.g. lower order ngram estimate.  but if you have --weight-cell-outside=1 and previous pass ngram scores, you might want to disable at 0.  selectively assessing only order higher than N heuristic costs isn't yet an option.")
        ("print-cells-file", defaulted_value(&print_cells_file),
         "print the best-outside cells table here")
        ("recompute-weights", defaulted_value(&recompute_weights),
         "prior to each decode, recompute score/heuristic for rules, applying updates in weight-string and prior-weight (cannot yet change prior bonus count or prior file)")
        ("allow-new-glue", defaulted_value(&allow_new_glue),
         "always allow GLUE cells, even if they were off the keep-cell-global-beam")
        ("compare-kept-cells",defaulted_value(&compare_kept_cells),
         "any cells in the nbests that were *not* --keep-cells from a previous pass are logged")
        ;


    OD all;

    all.add(general_opts);
    all.add(grammar_opts);
    all.add(lm_opts);
    all.add(fa.options());
    all.add(output_opts);
    all.add(multipass_opts);
    all.add(sbmt::io::logfile_registry::instance().options());


    const char *usage_str="";

    try {
        variables_map vm;
        all.parse_options(argc,argv,vm);

        std::ostream &help_out=std::cout; // std::clog
        sbmt::io::logging_stream& log = sbmt::io::registry_log(decoder_app);

        if (!is_null(changes_since)) {
            changes.print(help_out,changes_since);
            return false;
        }

        if (help) {
            help_out << "\n" << argv[0] << ' ' << "version " << get_version() << "\n\n"
                      << usage_str << "\n"
                      << all << "\n"
                     << "\nYou can find (some) real documentation at:"
                "\nhttps://nlg0.isi.edu/projects/sbmt/wiki/DecoderUtilities\n";
            return false;
        }

        if (!quiet) {
            graehl::print_command_header(log << sbmt::io::info_msg,argc,argv);
            log << sbmt::io::info_msg << "### CHOSEN OPTIONS:\n";
            all.print(continue_log(log),vm,OD::SHOW_DEFAULTED | OD::SHOW_HIERARCHY);
        }
    } catch (std::exception &e) {
        std::cerr << "ERROR:"<< e.what() << "\n"
            << "CMDLINE: "<<cmdline_str<<"\n"
                  << "Try '" << ARGV0 << " -h' for help\n\n";
        throw;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void mini_decoder::prepare()
{
    sbmt::io::logging_stream& log = sbmt::io::registry_log(decoder_app);
    log << sbmt::io::info_msg
        << "mini_decoder version "
        << get_version() <<": "
        << MINI_DECODER_HEAD_URL << "  "
        << MINI_DECODER_REVISION_ID << sbmt::io::endmsg;

    assert(!prepared);

    ga.pc.register_constructor("lm_string", sbmt::lm_string_constructor(),"");
    ga.pc.register_constructor("dep_lm_string", sbmt::lm_string_constructor(),"");
    validate();
    oargs.validate();
    ga.prepare(not instruction_file.valid());

    /// turn a foreign file into a trivial instruction file
    if( !instruction_file.valid()) {
        log << sbmt::io::info_msg
            << "Generating implicit instruction-file from foreign text:"
            << sbmt::io::endmsg;

        log << sbmt::io::debug_msg;
        while (*foreign) {
            std::string sent;
            if (!getline(*foreign,sent)) break;
            if (sent.empty()) continue;
            instruction_sstream << "decode "<< sent << "\n";
            continue_log(log) << "decode "<< sent << endl;
        }
        continue_log(log) << sbmt::io::endmsg;
        instruction_file.set(instruction_sstream);
    }

    na.prepare(grammar());

    prepared=true;
}


void mini_decoder::reload_weights()
{
    ga.update_feature_weights();
}

////////////////////////////////////////////////////////////////////////////////

mini_decoder::mini_decoder()
{
    reset();
    // NOTE: fractional versions are OK too :)
    changes("mini_decoder")
        (13.6,"--find-derivation-tree '(124 (123 122))'; unimplemented for force_decoder but works in mini_decoder")
        (13.5,"multi-lm with big-lm handles <unk> properly (was bugged since 13)")
        (13.4,"DLM changes from Wei")
        (13.3,"python changes from David")
        (13.2,"end of ins-file whitespace no longer causes error message+exit code")
        (13.1,"foreign-tree TOP node has proper tree punctuation.  foreign string output defaults off")
        (13,"dependency LM supports : separator as well as the previous ;")
//FIXME: I know some changes happened between here ;)  also, should this be the place to notate changes to other utilities/pipeline?  That seems a little ridiculous, but md version # = overall release # apparently.
        (11.7,"--print-forest-em-file f --print-forest-cons-id 0")
        (11.7,"--print-lattice-align 1 - show alignment of english to foreign lattice")
        (11.6,"--compare-kept-cells 1 --use-kept-cells 0 - show what cells in a parse not using the previous pass cells table, would have been prohibited if --use-kept-cells 1")
        (11.5,"--ngram-cache-size 24000 is default")
        (11.5,"--ngram-cache-size 10000 (for dynamic ngram only).  0 => no cache")
        (11.5,"--weight-tm-heuristic 0 (and maybe --weight-info-heuristic 0), intended for multipass with --weight-cell-outside 1")
        (11.5,"Useful Cell Multipass properly restricts and re-sorts unary rules, not just binary")
        (11.5,"bug fix: may specify @ lm option in parent for more recombination : e.g. multi[@][...]")
        (11.5,"bug fix: cell beams work again")
        (11.5,"correct text-length, lm-cost, and sbtm-cost for e.g. @UNKNOWN@ but empty-lmstring rules")
        (11.5,"verify tm score on binary rules vs syntax rules in nbest output")
        (11.5,"--recompute-weights 1.  prior to each decode, recompute score/heuristic for rules, applying updates in weight-string and prior-weight (cannot yet change prior bonus count or prior file)")
        (11.5,"--weight-tag-prior 1, --tag-prior-bonus 1.  real rules (tag result) now use their prior value (used to be 1.0).  tag-prior-bonus can adjust back toward favoring tags over virtuals as desired")
        (11.5,"Useful Cell Multipass (https://nlg0.isi.edu/projects/sbmt/wiki/CellMultipass): change-options --keep-cells 1 --keep-cell-global-beam 10^-5 --ngram-order 1 --check-info-score 0 --max-equivalents 20, DECODE x, change-options use-kept-cells 1 --weight-cell-outside 1 --ngram-order 5 --max-equivalents 3 --check-info-score 1, DECODE x.")
        (11.5,"instruction file: multipass-options --pass1 cmdline\n--pass2 cmdline\n")
        (11.5,"--print-cells-file file: print per-cell outside")
        (11.5,"--ngram.level debug (and pedantic): additional (coarser granularity) information about ngram_info computation.  p(a,b!c,d) = p(c|a)*p(d|a,b,c)")
        (11.5,"--more-info-details 1:  print individual ngram prob/backoff entries in nbests.  chance to automatically detect ngram miscomputations")
        (11.5,"wrong in-decoding lm scores reported at nbest output (when --ngram-shorten 1, you risk this)")
        (11.5,"instruction file: load-dynamic-ngram <lmspec>.  now frees memory from previous LM before loading (not after).  with  lmspec=\"none\", unloads LM (i.e. TM decode)")
        (11.5,"instruction file: change-options <cmdline>.  note: options involving input files / weights won't take effect")
        (11.3,"default --ngram-shorten 0")
        (11.2,"reduce nbest output by disabling: --print-used-rules, --print-foreign-binary-tree, --print-foreign-tree, --print-foreign-string")
        (11.2,"--ngram-shorten 0 disables left/right context equivalency testing (shortest seen prefix/suffix).  1 enables (default)")
        (11.2,"multi lm members w/o feature weight -> 0 weight instead of 1")
        (11.2,"--dynamic-lm big[c][filename.biglm] - David Chiang noisy LM support")
        (11.2,"--prune-worse-by 10^-5: global post-parse (post-multipass) 'relatively useless' prob. pruning")
        (11.1,std::string("dynamic-lm-ngram: ") + ngram_args::dynamic_lm_ngram_doc())
        (11.1,"logging options.  separate 'ngram' facility for recording actual LM queries")
        (11,"--per-estring-nbests n (remove duplicate translations during nbest)")
        (11,"'<epsilon/>' word now elided from estring")
        (10.99,"text attributes preserved in grammar archives")
        (10.99, "syntax rules with align={{{[#s=2 #t=3 0,1 1,3 2,2]}}} attributes give nbest word alignments (source means foreign/input)")
        (10.99,"--print-forest-file <file> prints the (pruned) parse forest")
        (10.99,"--outside-score-file <file> prints useful items' (best) outside scores")
        (10,"ngram left and right border words are tested for equivalence against the actual ngrams in the lm")
        (9,"unigram (--ngram-order=1) works")
        (9,"correct heuristic for ngram items.  fixed bug when LM order different from --ngram-order")
        (9,"--higher-ngram-order 5.  use 5grams when rule permits, when using a lower --ngram-order for items")
        (8,"improved sbtm-score.pl (validates dotproduct of features,weights), grammar_view")
        (6,"sbtm-score.pl and lw-lm-score.pl compute and verify tm and ngram scores respectively")
        (5, "--open-class-lm arg (=0)           use unigram p(<unk>) in your LM (which  must be trained accordingly).")
        (5,"--unknown-word-prob arg (=10-20)  lm probability assessed for each unknown word (when not using --open-class-lm)")
        (4,"--show-spans 1 prints e/f span alignments following rule-constituents in etree")
        (4,"blank input sentences translate to blank outputs")
        (4,"print foreign-tree")
        (3,"TOP items not subject to pruning")
        (3,"heuristic lm score used for ranking rules in cube pruning")
        (2,"robustness: when OOM, try again (and again) with tighter beams")
        (2,"--max-items N - to trigger retries without waiting for OOM")
        (2,"split-byline.pl --parens for ( <BYLINE> ) handling")
        (2,"glue rules (and their top rules) generated by new_decoder_weight_format")
        (1,"initial release")
        ;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const** argv)
{
    //feenableexcept(FE_INVALID); //used to throw NaN errors.
    graehl::native_filename_check();
    using namespace std;
    mini_decoder m;
    try {
        if (!m.parse_args(argc,argv))
            return 0;
        m.prepare();
        m.status() << "mini-decoder-start" << endl;
        if (m.run()) {
            m.status() << "mini-decoder-finish status=success" << endl;
            return 0;
        } else {
            m.status() << "mini-decoder-finish status=fail" << endl;
            return 2;
        }
    } catch(bad_alloc&) {
        SBMT_ERROR_MSG(decoder_app, "%1%\n", "ran out of memory (or bad alloc)");
        goto fail;
    }
    catch(exception& e) {
        SBMT_ERROR_MSG(decoder_app, "%1%\n", e.what());
        goto fail;
    }
    catch(const char * e) {
        SBMT_ERROR_MSG(decoder_app, "%1%\n", e);
        goto fail;
    }
    catch(...) {
        SBMT_ERROR_MSG(decoder_app, "%1%\n", "exception of unknown type!");
        return 2;
    }
    return 0;
fail:
    m.status() << "mini-decoder-finish status=fail" << endl;
    SBMT_ERROR_MSG(decoder_app, "try '%1% --help' for help", argv[0]);
#ifdef USE_BACKTRACE
    BackTrace::print(sbmt::io::registry_log(decoder_app) << sbmt::io::debug_msg);
#endif
    return 1;
}

////////////////////////////////////////////////////////////////////////////////

void mini_decoder::decode_out(std::string const& sent, std::size_t sid)
{
    fat_sentence fs = foreign_sentence(sent);
    chart_from_sentence<fat_sentence,gram_type> ci(fs, grammar());
    span_t tgt(0,fs.size());

    if (multi_thread) {
        decode_passes_out(ci,threaded_parse_cky(num_threads),sid,tgt);
    } else {
        decode_passes_out(ci,parse_cky(),sid,tgt);
    }
}

////////////////////////////////////////////////////////////////////////////////

void mini_decoder::decode_lattice_out(lattice_tree const& ltree, std::size_t sid)
{
    block_lattice_chart<gram_type> ci(ltree,grammar());
    span_t tgt = ltree.root().span();

    if (multi_thread) {
        decode_passes_out(ci
                            , block_parse_cky<threaded_parse_cky>(
                                ltree, threaded_parse_cky(num_threads)
                                )
                          ,sid,tgt);
    } else {
        decode_passes_out(ci
                          , block_parse_cky<parse_cky>(ltree,parse_cky())
                          ,sid,tgt);

    }
}


////////////////////////////////////////////////////////////////////////////////


void mini_decoder::preapply_multipass_options()
{
    for (multipass_opts_t::const_iterator i=multipass_opts.begin(),e=multipass_opts.end();
         i!=e;++i) {
        SBMT_VERBOSE_STREAM(decoder_app,"checking multipass pass="<<(1+i-multipass_opts.begin())<<" options="<<*i);
        change_options(*i,true);
    }
}

template <class ChartInit,class ParseOrder>
std::string
mini_decoder::decode_passes_out(ChartInit ci, ParseOrder po, std::size_t sid, span_t tgt)
{
    pre_decode_hook();
    preapply_multipass_options();

    std::string best;
    /*
    SBMT_LOG_TIME_SPACE(
        decoder_app
      , info
      , boost::str(
            boost::format("Decoded sentence %1% length=%2%: ")
            % sid % tgt.right()
        )
    );
    */
    ga.grammar_summary();

    bool using_passes=!multipass_opts.empty();
    unsigned n_passes=using_passes ? multipass_opts.size() : 1;

    std::string translation;
    for (unsigned pass=1;pass<=n_passes;++pass) {
        nbest_out() << endl;

        std::string pass_desc="";
        if (using_passes) {
            std::string const &opt=multipass_opts[pass-1];
            pass_desc=boost::str(boost::format(" pass=%1% options=%2%") % pass % boost::trim_copy(opt));
            change_options(opt);
            pre_decode_hook(); // options may have changed
        }

        best=decode(ci,po,sid, tgt,pass,pass_desc);
        if (oargs.nbest_output) //FIXME: detect failed parse and return last non-failed pass?
            out() << best << endl;
    }
    return best;
}

////////////////////////////////////////////////////////////////////////////////

void mini_decoder::set_defaults()
{
    oargs.set_defaults();
    //fa.set_defaults(); // why this is commented out, I don't know.  but default construction of fa has set_defaults() already.  likely for oargs as well

    allow_new_glue=true;
    recompute_weights=false;
    use_kept_cells=false;
    weight_cell_outside=1;
    keep_cells=false;
    compare_kept_cells=false;
    keep_cells_beam=0;
    na.set_defaults();
    set_null(weight_lm_ngram);
    max_items=std::numeric_limits<std::size_t>::max();
    weight_info_heuristic=weight_tm_heuristic=1;
    lm_weight()=1;
    multi_thread=false;
    num_threads=numproc_online();

}

////////////////////////////////////////////////////////////////////////////////

/// default constructor does this already
void mini_decoder::init()
{
    prepared=false;
    set_defaults();
}

mini_decoder::~mini_decoder()
{
}

////////////////////////////////////////////////////////////////////////////////
