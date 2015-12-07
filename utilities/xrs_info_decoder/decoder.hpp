# include <decoder_filters.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/edge/edge_equivalence.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <sbmt/edge/composite_info.hpp>
# include <string>
# include <iostream>
# include <sbmt/search/parse_robust.hpp>
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/chart/chart.hpp>
# include <sbmt/chart/ordered_cell.hpp>
# include <sbmt/chart/ordered_span.hpp>
# include <xrs_decoder_logging.hpp>

template <class Edge>
struct decode_traits {
    typedef Edge edge_type;
    typedef sbmt::grammar_in_mem gram_type;
    typedef sbmt::concrete_edge_factory<edge_type,gram_type> ecs_type;
    typedef sbmt::edge_equivalence_pool<edge_type> epool_type;
    typedef sbmt::edge_equivalence<edge_type> edge_equiv_type;
    typedef sbmt::basic_chart<
              edge_type
            , sbmt::ordered_chart_edges_policy > chart_type;
    typedef sbmt::filter_bank<edge_type,gram_type,chart_type> filter_bank_type;
    typedef boost::function<void( chart_type&
                                , ecs_type&
                                , epool_type& )> chart_init_f;
    typedef boost::function<void( filter_bank_type&
                                , sbmt::span_t
                                , sbmt::cky_generator const& )> parse_order_f;
};

////////////////////////////////////////////////////////////////////////////////

// note: returns false if complete failure or retry-then-success
template <class Edge>
bool decode( typename decode_traits<Edge>::chart_init_f cinit
           , typename decode_traits<Edge>::parse_order_f po
           , size_t sentid
           , sbmt::lattice_tree const& lattice
           , xrs_decoder_options& opts
           , sbmt::concrete_edge_factory<Edge,typename decode_traits<Edge>::gram_type> ecs)
{
    using namespace sbmt;
    using namespace sbmt::io;
    using namespace std;
    using namespace boost;

    typedef typename decode_traits<Edge>::edge_type edge_type;
    typedef typename decode_traits<Edge>::edge_equiv_type edge_equiv_type;
    typedef typename decode_traits<Edge>::gram_type gram_type;
    typedef typename decode_traits<Edge>::chart_type chart_type;

    typedef span_filter_factory<edge_type,gram_type,chart_type> spff_type;
    typedef unary_filter_factory<edge_type,gram_type>           uff_type;
    typedef early_exit_from_span_filter_factory< edge_type
                                               , gram_type
                                               , chart_type >   uff_impl_type;

    size_t sz = lattice.root().span().right();
    graehl::stopwatch timer;
    logging_stream& log = registry_log(xrs_info_decoder);

    string onebest;
    typename decode_traits<Edge>::epool_type epool;
	bool retval = true;
    try {
        shared_ptr<cky_generator> cky_gen = opts.create_cky_generator();
        edge_equiv_type::max_edges_per_equivalence(opts.max_equivalents());

        log_time_space_report report(log,lvl_info,"Decoded: ");

        edge_equiv_type top_equiv;

        { // scope to ensure chart disappears before writing nbest -- saving memory
        chart_type chart(0);
        span_t tgt = chart.target_span();

        shared_ptr<spff_type>
            psff=opts.filt_args->template create_filter<chart_type>(tgt,ecs,opts.grammar());
        shared_ptr<uff_type> uff(new uff_impl_type(psff,tgt));

        retval =
        parse_robust( psff
                    , uff
                    , *cky_gen
                    , po
                    , opts.grammar()
                    , ecs
                    , epool
                    , chart
                    , cinit
                    , opts.maximum_retries()
                    , opts.reserve_nbest()
                    );

        SBMT_PEDANTIC_STREAM_TO(
            log
          , print(chart,opts.grammar().dict())
        );
        //DONE PARSING - now nbest/output:

        log << info_msg << "Number of equivalences in memory: "
            << edge_equiv_type::equivalence_count() << endmsg;

        top_equiv = chart.top_equiv(opts.grammar());
        } // chart out of scope
        output_results( opts
                      , top_equiv
                      , opts.grammar()
                      , ecs
                      , sentid
                      );

    } catch (std::bad_alloc const& e) {
        dummy_output_results(opts,sentid, opts.grammar(), ecs, e.what());
    } catch (sbmt::empty_chart const& e) {
        dummy_output_results(opts,sentid, opts.grammar(), ecs, e.what());
    }

    SBMT_TERSE_MSG_TO(
        log
      , "sentence #%1% length=%2% time=%3% - %4%.\n"
      , sentid % sz % timer % ecs.stats()
    );
    opts.grammar().erase_terminal_rules();
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

