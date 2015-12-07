# include "mini_decoder.hpp"
# include "mini_decoder.ipp"

// explicit instantiations /////////////////////////////////////////////////////

typedef sbmt::edge< edge_info<null_info> > edge_t;

template
void mini_decoder::decode_dispatch< edge_t >
( concrete_edge_factory< edge_t, gram_type > &ef
, parse_traits< edge_t >::chart_init_f ci
, parse_traits< edge_t >::parse_order_f po
, std::size_t sentid
, span_t target_span
, edge_stats &stats
, std::string &translation
, unsigned pass
);

////////////////////////////////////////////////////////////////////////////////

MINI_REVEAL_CHART_INITS(edge_t);
MINI_REVEAL_PARSE_ORDERS(edge_t);

////////////////////////////////////////////////////////////////////////////////

