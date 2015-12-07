# if MINI_DLM_ORDER

# include "mini_decoder.hpp"
# include "mini_decoder.ipp"

MINI_INSTANTIATE_DLM_ORDER(MINI_DLM_ORDER);

typedef sbmt::edge< edge_info<sbmt::head_history_dlm_info<MINI_DLM_ORDER-1, unsigned int> > > edge_t;
MINI_REVEAL_CHART_INITS(edge_t);
MINI_REVEAL_PARSE_ORDERS(edge_t);



# endif

////////////////////////////////////////////////////////////////////////////////

