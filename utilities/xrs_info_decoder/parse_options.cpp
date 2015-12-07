# include <composite_edge_interface.hpp>
# include <xrs_decoder_methods/parse_options.ipp>

//template class xrs_decoder<composite_edge_interface>;
//template class xrs_decoder<composite_edge_interface>::options;

template
std::auto_ptr<xrs_decoder_options> 
xrs_decoder<composite_edge_interface>::parse_options(int argc, char** argv);
