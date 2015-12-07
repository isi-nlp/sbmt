# include <composite_edge_interface.hpp>
# include <xrs_decoder_methods/lazy_decode.ipp>
template void
xrs_decoder<composite_edge_interface>::lazy_decode( xrs_decoder_options& opts
                                                  , sbmt::lattice_tree const& lattree 
                                                  , size_t id );
