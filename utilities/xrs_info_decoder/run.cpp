# include <composite_edge_interface.hpp>
# include <xrs_decoder_methods/run.ipp>
# include <xrs_decoder_options.ipp>

template int xrs_decoder<composite_edge_interface>::run(int argc, char** argv);

typedef composite_edge_interface::edge_type edge_type;
template std::string
output_results<edge_type>
                    ( xrs_decoder_options& opts
                    , sbmt::edge_equivalence<edge_type> const& top_equiv
                    , sbmt::grammar_in_mem& gram
                    , sbmt::concrete_edge_factory<edge_type,sbmt::grammar_in_mem>& efact
                    , unsigned sentid
                    , unsigned pass
                    );
                    
template std::string
dummy_output_results<edge_type>
                    ( xrs_decoder_options& opts
                    , std::size_t sentid
                    , sbmt::grammar_in_mem& gram
                    , sbmt::concrete_edge_factory<edge_type,sbmt::grammar_in_mem>& efact
                    , std::string const& msg
                    , unsigned pass
                    );
