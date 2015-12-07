# ifndef  XRS_INFO_DECODER__XRS_DECODER_HPP
# define  XRS_INFO_DECODER__XRS_DECODER_HPP
# include <decoder_filters.hpp>
# include <numproc.hpp>
# include <xrs_decoder_logging.hpp>

# include <iterator>
# include <iostream>
# include <stdexcept>
# include <memory>

# include <boost/program_options.hpp>
# include <boost/scoped_ptr.hpp>
# include <boost/tokenizer.hpp>


struct xrs_decoder_options;

template <class EdgeInterface>
struct xrs_decoder {
    typedef xrs_decoder<EdgeInterface> decoder_;

    explicit xrs_decoder(EdgeInterface const& ei = EdgeInterface())
    : edge_interface(ei)
    , retry_needed(false) {}
    
    EdgeInterface edge_interface;
    bool retry_needed;
    
    typedef typename EdgeInterface::edge_type edge_type;
    typedef typename EdgeInterface::ecs_type ecs_type;
    typedef typename EdgeInterface::gram_type gram_type;
        
    std::auto_ptr<xrs_decoder_options> parse_options(int argc, char** argv);
    
    template <class ChartInit, class ParseOrder>
    void decode_( ChartInit chart_init
                , ParseOrder parse_order
                , xrs_decoder_options& opts
                , sbmt::lattice_tree const& lattice 
                , size_t id );
    
    void lazy_decode( xrs_decoder_options& opts
                    , sbmt::lattice_tree const& lattree 
                    , size_t id );
                     
    void decode_lattice( xrs_decoder_options& opts
                       , gusc::lattice_ast const& lat
                       , size_t id );
    
    int run(int argc, char** argv);
};
# endif // XRS_INFO_DECODER__XRS_DECODER_HPP