# include <xrs_decoder.hpp>
# include <xrs_decoder_options.hpp>
# include <sbmt/search/lazy.hpp>

template <class EdgeInterface>
void xrs_decoder<EdgeInterface>::lazy_decode( xrs_decoder_options& opts
                                            , sbmt::lattice_tree const& lattree 
                                            , size_t id )
{
    using namespace sbmt;
    using namespace sbmt::lazy;
    
    span_t total_span = lattree.root().span();
    
    ecs_type efactory = edge_interface.create_factory(opts.grammar(),lattree);
    
    exhaustive_top_addition<edge_type> tops(opts.grammar(), efactory);
    
    first_experiment_cell_proc<edge_type>
        cell_proc( opts.grammar()
                 , efactory
                 , lattree
                 , opts.filt_args->template create_fuzzy_edge_filter<edge_type>()
                 , opts.filt_args->template create_fuzzy_unary_edge_filter<edge_type>()
                 , opts.merge_heap_lookahead );
    
    boost::shared_ptr<cky_generator> 
        ckygen = opts.create_cky_generator();
    
    try {
        edge_equivalence<edge_type>::max_edges_per_equivalence(opts.max_equivalents());
        edge_equivalence<edge_type> top_equiv;
        
        boost::tie(
          top_equiv
        , retry_needed
        ) = block_cky( lattree
                     , *ckygen
                     , cell_proc
                     , tops
                     , efactory
                     , opts.multi_thread ? opts.num_threads : 1 
                     , opts.maximum_retries()
                     , opts.reserve_nbest()
                     );
        
        output_results( opts
                      , top_equiv
                      , opts.grammar()
                      , efactory
                      , id
                      );
    } catch (sbmt::empty_chart const& e) {
        dummy_output_results(opts, id, opts.grammar(), efactory, e.what());
    } catch (std::exception const& e) {
        if (opts.exit_on_retry) throw;
        dummy_output_results( opts
                            , id
                            , opts.grammar()
                            , efactory
                            , e.what()
                            );
    } catch (...) {
        if (opts.exit_on_retry) throw;
        dummy_output_results( opts
                            , id
                            , opts.grammar()
                            , efactory
                            , "unknown error"
                            );
    }
    opts.grammar().erase_terminal_rules();
}
