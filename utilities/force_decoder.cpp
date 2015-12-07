// todo:
#if !defined (MINI_MAX_NGRAM_ORDER)
# define MINI_MAX_NGRAM_ORDER 9999
#endif

# include <boost/bind.hpp>
# include <boost/shared_ptr.hpp>

# include "grammar_args.hpp"
# include "output_args.hpp"
# include "decode_sequence_reader.hpp"
# include "numproc.hpp"
# include "ngram_args.hpp"
# include <decoder_filters.hpp>
# include <sbmt/search/filter_bank.hpp>
# include <sbmt/search/force_sentence_filter.hpp>
# include <sbmt/search/max_equivs_exceeded.hpp>
# include <sbmt/search/span_filter.hpp>
# include <sbmt/search/parse_order.hpp>
# include <sbmt/search/parse_robust.hpp>
# include <sbmt/search/threaded_parse_cky.hpp>
# include <sbmt/edge/sentence_info.hpp>
# include <sbmt/edge/null_info.hpp>
# include <sbmt/chart/force_sentence_cell.hpp>
# include <sbmt/grammar/tree_tags.hpp>

# include <string>
# include <limits>

namespace sbmt {

class force_decoder
{
public:
    force_decoder();
    void run(int argv, char const** argv);

private:
    void load_grammar_cb(std::string const&, std::string const&, archive_type);
    void force_decode_cb(std::string const&, std::string const&, std::size_t);

    ngram_args na;
    grammar_args ga;
    output_args oa;
    filter_args fa;
    decode_sequence_reader reader;
    graehl::istream_arg instruction_file;
    bool multi_thread;
    bool force_etree;
    bool force_treebank_etree;
    std::size_t max_equivs; // for aborting search before OOM
    std::size_t max_edges;
    std::size_t num_threads;
    std::size_t max_equivalents; // for nbest output

    void parse_args(int argv, char const** argv);
};

template <class ET, class GT>
basic_chart<ET,force_sentence_chart_edges_policy>
fill_chart_from_sentence( indexed_sentence s, concrete_edge_factory<ET,GT>& ef )
{
    edge_equivalence_pool<ET> epool;
    basic_chart<ET,force_sentence_chart_edges_policy> chart(s.size());
    indexed_sentence::iterator i = s.begin(), e = s.end();
    span_t total_span(0,s.size());
    shift_generator sg(total_span,1);
    shift_generator::iterator si = sg.begin();
    for (;i != e; ++i, ++si) {
        chart.insert_edge(epool, ef.create_edge(*i,*si));
    }

    return chart;
}

template <class Sentence, class GT>
class force_chart_from_sentence
{
    Sentence const&s;
    GT & gram;
 public:
    unsigned max_span_rt() const
    {
        return s.length();
    }

    force_chart_from_sentence(Sentence const& s, GT& gram) : s(s), gram(gram) {}
    template <class CT, class ET>
    void operator()( CT &chart
                   , concrete_edge_factory<ET,GT>& ef
                   , edge_equivalence_pool<ET>& epool ) const
    {
        chart=fill_chart_from_sentence(s,ef);
    }
};

template <class Sentence,class GT>
inline force_chart_from_sentence<Sentence,GT>
make_force_chart_from_sentence(Sentence const& s,GT& gram)
{
    return force_chart_from_sentence<Sentence,GT>(s,gram);
}


void force_decoder::run(int argc, char const** argv)
{
    parse_args(argc, argv);
    if (instruction_file.name.size() == 0 or
        instruction_file.name[0] == '-' ) {
        reader.read(instruction_file.stream());
    } else {
        reader.read(instruction_file.name);
    }
}

void force_decoder::parse_args(int argc, char const** argv)
{
    using namespace graehl;
    using namespace std;
    using namespace boost;
    using namespace boost::program_options;
    printable_options_description<ostream>
        options(general_options_desc()) ,
        //gram_options("grammar options") ,
        //output_options("output options") ,
        lm_opts("Language model options");


    //ga.add_options(gram_options);
    //oa.add_options(output_options);
    na.add_options(lm_opts);

    bool help;

    options.add_options()
        ( "help,h"
        , bool_switch(&help)
        , "show usage/documentation"
        )
        ( "force-etree"
        , bool_switch(&force_etree)
        , "forced english tree mode - input is a LISP style tree without "
          "quotes for lexical leaves, e.g. '(S (NP dogs) (VB lie) )'"
          " - space between closeparens"
        )
        ( "force-treebank-etree"
        , bool_switch(&force_treebank_etree)
        , "forced english tree mode - input is a treebank style tree, e.g. "
          "'S(NP(\"dogs\") VB(\"lie\"))'"
        )
        ( "instruction-file"
        , defaulted_value<istream_arg>(&instruction_file)
        )
        ( "multi-thread"
        , bool_switch(&multi_thread)
        , "dispatch cky-span-combining to multiple threads"
        )
        ( "num-threads"
        , defaulted_value(&num_threads)
        , "number of threads to use with multi-threaded cky"
        )
        ( "max-equivalences"
        , defaulted_value(&max_equivs)
        , "force_decoder will abandon any decode that creates "
          "more equivalences than this value"
        )
        ( "max-edges"
        , defaulted_value(&max_edges)
        , "force_decoder will abandon any decode that creates "
          "more edges than this value"
        )
        ( "max-equivalents"
        , defaulted_value(&max_equivalents)
        , "keep only this many equivalent edges; at most you need as many as "
          "you want nbests; usually 4 or so is fine.  1 will result in only a "
          "1best derivation but no other harm (0=unlimited)"
        )
       ;
    options.add(ga.options());
    options.add(oa.options());
    options.add(lm_opts);
    options.add(fa.options());
    options.add(io::logfile_registry::instance().options());

    try {
        variables_map vm;
        options.parse_options(argc,argv,vm);

        if (help) {
            cout << "\n" << options << endl;
            exit(-1);
        }

        io::logging_stream& log = io::registry_log(decoder_app);
        log << io::info_msg << "### CHOSEN OPTIONS:\n";
        options.print( continue_log(log)
                     , vm
                     , printable_options_description<ostream>::SHOW_DEFAULTED );

    } catch (exception &e) {
        cerr << endl;
        cerr << "error parsing options " << e.what() << endl;
        cerr << options << endl;
        exit(-1);
    }

    oa.validate();
    max_equivalents=oa.revise_max_equivalents(max_equivalents);
    na.validate();
    ga.prepare(false);
/* // now defered until parse succeeds
          na.prepare(ga.grammar);
        na.set_weights(ga.combine); // we don't need to set weights to see feature values
*/
}

force_decoder::force_decoder()
: instruction_file("-")
, multi_thread(false)
, max_equivs(std::numeric_limits<std::size_t>::max())
, max_edges(std::numeric_limits<std::size_t>::max())
, num_threads(numproc_online())
, max_equivalents(4)
{
    oa.nbests=1;
    using namespace boost;
    reader.set_load_grammar_callback(bind( &force_decoder::load_grammar_cb
                                         , this
                                         , _1
                                         , _2
                                         , _3 ));
    reader.set_force_decode_callback(bind( &force_decoder::force_decode_cb
                                         , this
                                         , _1
                                         , _2
                                         , _3 ));
    fa.set_force_decode_defaults();
}

void force_decoder::load_grammar_cb( std::string const& filename
                                   , std::string const& weight_str
                                   , archive_type atype )
{
    ga.load_grammar(filename,weight_str,atype);
}

void force_decoder::force_decode_cb( std::string const& source
                                   , std::string const& target
                                   , std::size_t id )
{
    using namespace boost;
    using namespace std;

    typedef edge<edge_info<sentence_info> >                       edge_t;
    typedef edge_equivalence<edge_t>                              edge_equiv_type;

    typedef grammar_in_mem                                        gram_t;
    typedef force_grammar<gram_t>                                 fgram_t;
    typedef basic_chart<edge_t,force_sentence_chart_edges_policy> chart_t;

    edge_equiv_type::max_edges_per_equivalence(max_equivalents);

    grammar_in_mem& gram = ga.get_grammar();

    io::logging_stream& log = io::registry_log(decoder_app);

    typedef filter_bank<edge_t,fgram_t,chart_t>            filter_bank_t;
    typedef force_sentence_factory<edge_t,fgram_t,chart_t> force_filter_factory_t;
    typedef span_filter_factory<edge_t,fgram_t,chart_t>    filter_factory_t;
    typedef exhaustive_unary_from_span_filter_factory<edge_t,fgram_t,chart_t>
            unary_filter_factory_t;
    typedef edge_filter<edge_t> edge_filter_t;

    vector<indexed_token> out;

    if (force_etree) { out = lisp_tree_tags(target,gram.dict()); }
    else if (force_treebank_etree) { out=tree_tags(target,gram.dict()); }
    else {
        indexed_sentence outsent = native_sentence(target,gram.dict());
        out = vector<indexed_token>(outsent.begin(),outsent.end());
    }

    indexed_sentence in = foreign_sentence(source, gram.dict());

    log << io::info_msg << "source: " << print(in,gram.dict()) << io::endmsg;
    log << io::info_msg << "target:";
    for(vector<indexed_token>::iterator i = out.begin(); i != out.end(); ++i) {
        if(i->type() == native_token) continue_log(log) << " \"" << print(*i,gram.dict()) << "\"";
        else continue_log(log) << " (" << print(*i,gram.dict()) << ")";
    }
    continue_log(log) << io::endmsg;

    fgram_t fgram(gram,out);
    typedef  edge_info_factory<sentence_info_factory> ifact_type;
    typedef edge_factory<ifact_type> efact_type;
    concrete_edge_factory<edge_t,fgram_t> efact(boost::in_place<efact_type>(ifact_type()));
    edge_equivalence_pool<edge_t> epool;

    chart_t chart=in.size();
//    chart = fill_chart_from_sentence<edge_t>(in,efact);

    span_t target_span(0,in.size());

    shared_ptr<filter_factory_t>
        ffact(new force_filter_factory_t(out,target_span
                                         ,fa.create_edge_filter<edge_t>()
                                         ,fa.create_unary_edge_filter<edge_t>()
                  ));

    typedef max_edges_exceeded_factory<edge_t,fgram_t,chart_t>
            max_edges_filter_factory_t;
    ffact.reset(new max_edges_filter_factory_t( ffact
                                              , target_span
                                              , max_equivs
                                              , max_edges
                                              )
               );

    shared_ptr<unary_filter_factory_t>
        uffact(new unary_filter_factory_t(ffact,target_span));

    filter_bank_t::default_max_unary_loop_count =
                       std::numeric_limits<unsigned int>::max();
    edge_equivalence<edge_t>::max_edges_per_equivalence(
                                  std::numeric_limits<unsigned int>::max()
                              );

    filter_bank_t fbank(ffact, uffact, fgram, efact, epool, chart, target_span);

    function<void(filter_bank_t&,span_t,cky_generator const&)> parse_fn;
    if (!multi_thread) parse_fn = parse_cky();
    else parse_fn = threaded_parse_cky(num_threads);
    try {
        boost::shared_ptr<cky_generator> cky_gen = fa.create_cky_generator();
        parse_robust(ffact
                     ,uffact
                     ,*cky_gen
                     ,parse_fn
                     ,fgram
                     ,efact
                     ,epool
                     ,chart
                     ,make_force_chart_from_sentence(in,fgram)
                     ,fa.max_retries
                     , oa.reserve_nbest
            );
//        parse_fn(fbank, target_span);

        na.maybe_prepare(ga.grammar);
        na.set_weights(ga.combine); // we don't need to set weights to see feature values

        extra_english_output_t e=na.extra_output(oa.deriv_opt.more_info_details);
        oa.output_results(chart, fgram, efact, epool, id,0,0,na.using_lm() ? &e : 0);
    } catch(std::exception& e) {
        oa.dummy_output_results<edge_t>(id, fgram, efact, e.what());
        log << io::debug_msg << print(chart,gram.dict()) << io::endmsg;
    } catch(...) {
        oa.dummy_output_results<edge_t>(id, fgram, efact, "unknown error");
        log << io::debug_msg << print(chart,gram.dict()) << io::endmsg;
    }
}

} // namespace sbmt

int main(int argc, char const** argv)
{
    sbmt::force_decoder app;
    app.run(argc, argv);
    return 0;
}
