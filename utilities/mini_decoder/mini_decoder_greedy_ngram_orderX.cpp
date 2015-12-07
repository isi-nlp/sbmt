# if MINI_NGRAM_ORDER

# include "mini_decoder.hpp"
# include "mini_decoder.ipp"

# include <sbmt/edge/greedy_ngram_info.hpp>

// explicit instantiations ////////////////////////////////////////////////////

MINI_INSTANTIATE_GREEDY_NGRAM_ORDER(MINI_NGRAM_ORDER,LWNgramLM);
MINI_INSTANTIATE_GREEDY_NGRAM_ORDER(MINI_NGRAM_ORDER,dynamic_ngram_lm);

typedef sbmt::edge< edge_info< sbmt::greedy_ngram_info<MINI_NGRAM_ORDER> > > 
        greedy_edge_t;
MINI_REVEAL_CHART_INITS(greedy_edge_t);
MINI_REVEAL_PARSE_ORDERS(greedy_edge_t);

////////////////////////////////////////////////////////////////////////////////

# endif
