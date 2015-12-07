# include <xrs_decoder.hpp>
# include <xrs_decoder_options.hpp>
# include <decoder.hpp>
# include <sbmt/search/threaded_parse_cky.hpp>
# include <sbmt/search/block_parser.hpp>
# include <sbmt/search/impl/block_lattice_tree.tpp>

template <class EdgeInterface>
template <class ChartInit, class ParseOrder>
void xrs_decoder<EdgeInterface>::decode_( ChartInit chart_init
                                        , ParseOrder parse_order
                                        , xrs_decoder_options& opts
                                        , sbmt::lattice_tree const& lattice 
                                        , size_t id )
{
    ecs_type efactory = edge_interface.create_factory(opts.grammar(),lattice);

    retry_needed = ! decode<edge_type>
          ( chart_init
          , parse_order
          , id
          , lattice
          , opts
          , efactory );
}

template <class EdgeInterface>
void xrs_decoder<EdgeInterface>::decode_lattice( xrs_decoder_options& opts
                                               , gusc::lattice_ast const& lat
                                               , size_t id )
{
    using namespace sbmt;
    if (retry_needed and opts.exit_on_retry) {
        throw std::runtime_error("exit-on-retry");
    }
    lattice_tree lattree = convert(opts.grammar(),lat);
    if (opts.merge_heap_decode) {
        lazy_decode(opts,lattree,id);
    } else {
        block_lattice_chart<gram_type> 
            chart_init(lattree, opts.grammar());
        
        if (opts.multi_thread) {
            block_parse_cky<threaded_parse_cky> 
                parse_order(lattree, threaded_parse_cky(opts.num_threads));

            decode_(chart_init,parse_order,opts,lattree,id);
        } else {
            block_parse_cky<parse_cky>
                parse_order(lattree,parse_cky());
            decode_(chart_init,parse_order,opts,lattree,id);
        }
    }
}
