# if ! defined(UTILITIES__MINI_DECODER_HPP)
# define       UTILITIES__MINI_DECODER_HPP

//#define DEBUG_TIME_SERIES
//#define DEBUG_ALIGNMENT
#define WEIGHT_LM_NGRAM "weight-lm-ngram"
#define LM_COST "lm-cost"

# if !defined (MINI_MAX_NGRAM_ORDER)
# define MINI_MAX_NGRAM_ORDER 7
# endif

# if !defined (MINI_MIN_NGRAM_ORDER)
# define MINI_MIN_NGRAM_ORDER 1
# endif

# if !defined (MINI_ALL_NGRAMS)
# define MINI_ALL_NGRAMS 1
# endif


#include <decoder_filters/decoder_filters.hpp>
#include "decode_sequence_reader.hpp"
#include "grammar_args.hpp"
#include "output_args.hpp"
#include "ngram_args.hpp"

#include "numproc.hpp"


#include <graehl/shared/program_options.hpp>
#include <graehl/shared/stopwatch.hpp>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/os.hpp>
#include <graehl/shared/makestr.hpp>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/size_mega.hpp>
#include <graehl/shared/is_null.hpp>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/string_match.hpp>
#include <graehl/shared/changelog.hpp>
#ifdef USE_BACKTRACE
#define HAVE_LINUX_BACKTRACE
#include <graehl/shared/backtrace.hpp>
#endif

#include <sbmt/logging.hpp>
#include <sbmt/io/log_auto_report.hpp>
#include <sbmt/edge/ngram_info.hpp>
#include <sbmt/edge/null_info.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <sbmt/sentence.hpp>
#include <sbmt/grammar/brf_archive_io.hpp>
#include <sbmt/grammar/brf_file_reader.hpp>
#include <sbmt/forest/kbest.hpp>
#include <sbmt/forest/derivation.hpp>
#include <sbmt/search/parse_robust.hpp>
#include <sbmt/search/threaded_parse_cky.hpp>
#include <sbmt/io/logfile_registry.hpp>
#include <sbmt/chart/ordered_cell.hpp>
#include <sbmt/edge/head_history_dlm_info.hpp>
#include <sbmt/edge/joined_info.hpp>
#include <sbmt/dependency_lm/DLM.hpp>

#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <fstream>
#include <vector>


/// Do not modify:  these are modified automatically by subversion
#define MINI_DECODER_HEAD_URL "$HeadURL: https://nlg0.isi.edu/svn/sbmt/branches/pust/non-cky/utilities/mini_decoder.cpp $"
#define MINI_DECODER_REVISION_ID "$Id: mini_decoder.cpp 1516 2007-03-07 04:06:29Z pust $"


using namespace sbmt;
using namespace sbmt::logmath;
using namespace std;

//using namespace boost::program_options;
//using namespace boost::filesystem;

typedef grammar_in_mem                              gram_type;

////////////////////////////////////////////////////////////////////////////////

template<class ET>
struct parse_traits {
    typedef ET edge_type;
    typedef basic_chart<edge_type,ordered_chart_edges_policy> chart_type;
    typedef filter_bank<edge_type,gram_type,chart_type>
            filter_bank_type;

    typedef boost::function<void ( chart_type&
                                 , concrete_edge_factory<edge_type,gram_type>&
                                 , edge_equivalence_pool<edge_type>& )>
            chart_init_f;
    typedef boost::function<void ( filter_bank_type& fb
                                 , span_t
                                 , cky_generator const& )>
            parse_order_f;
};

template <class ET, class ParseOrder>
typename parse_traits<ET>::parse_order_f
hide_parse_order(ParseOrder const& po);

template <class ET, class ChartInit>
typename parse_traits<ET>::chart_init_f
hide_chart_init(ChartInit const& ci);

template <unsigned int N>
struct ngram_edge {
    typedef sbmt::edge< edge_info< ngram_info<N> > > type;
};
template <unsigned int N, unsigned int M>
struct ngram_dlm_edge {
    typedef sbmt::edge< joined_info< edge_info< ngram_info<N> >
                                   , edge_info< head_history_dlm_info<M-1> > > >
            type;
};

////////////////////////////////////////////////////////////////////////////////

# include "logging.hpp"
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(decoder_app,"mini_decoder",app_domain);

struct mini_decoder
{
    bool keep_cells;
    bool compare_kept_cells;
    bool recompute_weights;

    cell_heuristic cells;
    score_t keep_cells_beam;
    bool use_kept_cells;
    double weight_cell_outside;
    double weight_tm_heuristic;
    double weight_info_heuristic;
    bool allow_new_glue;

    void pre_decode_hook(); // fix whatever settings may change between passes here

    typedef std::vector<std::string> multipass_opts_t;

    multipass_opts_t multipass_opts; // multipass_opts[0] is (unparsed command line options string) for the first pass, and so on ... executed for every decode if nonempty

    void multipass_options(std::vector<std::string> const& o)
    {
        multipass_opts=o;
        log_multipass_options();
    }

    void log_multipass_options();

    graehl::ostream_arg print_cells_file;
    graehl::changelog changes;

    ~mini_decoder();

    mini_decoder();

    graehl::changelog::version_t get_version() const
    { return changes.max_version(); }

    graehl::istream_arg foreign, instruction_file;

    double weight_lm_ngram;

    grammar_args ga;
    output_args oargs;

    std::stringstream instruction_sstream;

    graehl::size_t_metric max_items;

    filter_args fa;

    bool prepared;

    bool multi_thread;
    std::size_t num_threads;

    static
    std::ostream &
    print_pass_id(std::ostream &o,std::size_t id,unsigned pass)
    {
        return o << " id="<<id<<" pass="<<pass;
    }

    template <class IF>
    void log_info_stats(IF & inf,std::size_t sentid,unsigned pass) const
    {
        inf.print_stats(print_pass_id(io::registry_log(decoder_app) << io::info_msg << "Info stats for ",sentid,pass)<<":");
    }



    void set_defaults();

    void reload_weights();

    /// default constructor does this already
    void init();

    unsigned nbests_failed,parses_failed;

    /// can be used on already prepared decoder
    void reset()
    {
        nbests_failed=0;
        parses_failed=0;
        na.reset();
        ga.reset();
        oargs.reset();
        foreign.set_none();
        init();
    }

    double &lm_weight()
    {
        return oargs.lm_weight();
    }

    double  dlm_weight() {
        return get(grammar().get_weights(),grammar().feature_names(),"deplm");
    }

    std::ostream& status() { return oargs.status() ; }

    std::ostream &out()
    { return oargs.out() ; }

    std::ostream &nbest_out()
    {
        return oargs.nbest_out() ;
    }

    std::auto_ptr<graehl::teebuf> teebufptr;
    std::auto_ptr<std::ostream> teestreamptr;

    std::string cmdline_str;

    ngram_args na;

    bool parse_args(int argc, char const **argv,bool quiet=false);

    static inline void throw_if(bool cond,std::string const& reason)
    { if (cond) throw std::runtime_error(reason); }

    static inline void throw_unless(bool cond,std::string const& reason)
    { throw_if(!cond,reason); }

    void validate();

    bool no_instruction_filename() const
    {
        return instruction_file.name.size() == 0 or instruction_file.name[0] == '-';
    }

    void load_dynamic_lm(std::string const& spec,boost::filesystem::path const& relative_base=boost::filesystem::initial_path())
    {
        if (spec == "none")
            na.reset();
        else {
            na.load_dynamic_lm(spec,relative_base);
            na.set_dynamic_grammar(grammar());
        }
    }

    void load_lw_lm(std::string const& spec)
    {
        na.load_lw_lm(spec);
        na.set_lw_grammar(grammar());
    }

    void load_dependency_lm(std::string const& spec)
    {
        na.load_dependency_lm(spec);
        na.set_dependency_lm_grammar(grammar());
    }

    void change_options(std::string const& cmdline)
    {
        change_options(cmdline,false);
    }

    void change_options(std::string const& cmdline,bool quiet)
    {
        graehl::argc_argv a(cmdline,"change-options");
        bool save_multi_thread=multi_thread; // hack.  FIXME, don't use bool_switch
        parse_args(a.argc(),a.argv(),quiet);
        multi_thread=save_multi_thread;
    }

    void prepare();

    // return true on success, false if any parse or nbest failed
    bool run();

    void push_grammar(std::string const& filename, sbmt::archive_type a);
    void pop_grammar();
    void load_grammar( std::string const& filename
                     , std::string const& weight_str
                     , sbmt::archive_type a );

    // cycle through all the multipass option changes in order to catch errors early, and to ensure repeatability (first-pass settings for first sentence decoded are the same as the Nth
    void preapply_multipass_options();


    // executes several passes if multipass_opts isn't empty.  set lattice=false for cky
    template <class ChartInit,class ParseOrder>
    std::string decode_passes_out( ChartInit cinit
                                 , ParseOrder po
                                 , std::size_t sentid
                                 , span_t target_span );


    template <class ChartInit, class ParseOrder>
    std::string decode( ChartInit cinit
                      , ParseOrder po
                      , std::size_t sentid
                      , span_t target_span
                      , unsigned pass=1
                      , std::string const& pass_desc="");

    template <class ET>
    void decode_dispatch( concrete_edge_factory<ET,gram_type> &ef
                        , typename parse_traits<ET>::chart_init_f ci
                        , typename parse_traits<ET>::parse_order_f po
                        , std::size_t sentid
                        , span_t target_span
                        , edge_stats &stats
                        , std::string &translation
                        , unsigned pass
                          );

    template <unsigned int N,class LM> void
    decode_ngram_order( typename parse_traits<typename ngram_edge<N>::type>::chart_init_f ci
                      , typename parse_traits<typename ngram_edge<N>::type>::parse_order_f po
                      , std::size_t sentid
                      , span_t target_span
                      , edge_stats& stats
                      , std::string& translation
                      , boost::shared_ptr<LM> lm
                      , unsigned pass
        );

    template <unsigned int N, unsigned int M, class LM> void
    decode_ngram_order( typename parse_traits<typename ngram_dlm_edge<N,M>::type>::chart_init_f ci
                      , typename parse_traits<typename ngram_dlm_edge<N,M>::type>::parse_order_f po
                      , std::size_t sentid
                      , span_t target_span
                      , edge_stats& stats
                      , std::string& translation
                      , boost::shared_ptr<LM> lm
                      , unsigned pass
                  //, std::vector<boost::shared_ptr<LWNgramLM> > dlm
                      , boost::shared_ptr<MultiDLM> dlm
        );

    // tm + dlm decoding (no ngram decoding).
    template <unsigned int M>
    void
    decode_dlm_order( typename parse_traits<sbmt::edge< edge_info<head_history_dlm_info<M-1> > > >::chart_init_f ci
                    , typename parse_traits<sbmt::edge< edge_info<head_history_dlm_info<M-1> > > >::parse_order_f po
                    , std::size_t sentid
                    , span_t target_span
                    , edge_stats& stats
                    , std::string& translation
                    , unsigned pass
                    , boost::shared_ptr<MultiDLM> dlm
    );

    grammar_in_mem &grammar()
    {
        return ga.grammar;
    }

 private:

    void decode_out(std::string const& sent, std::size_t sid);

    void decode_lattice_out(lattice_tree const& ltree, std::size_t sid);

    template <class ET, class GT>
    std::string decode( typename parse_traits<ET>::chart_init_f ci
                      , typename parse_traits<ET>::parse_order_f po
                      , concrete_edge_factory<ET,GT>& ef
                      , std::size_t sentid
                      , span_t target_span
                        ,  unsigned pass
                        );
};

# endif //     UTILITIES__MINI_DECODER_HPP

