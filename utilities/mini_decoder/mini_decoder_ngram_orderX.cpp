# if MINI_NGRAM_ORDER

# include "mini_decoder.hpp"
# include "mini_decoder.ipp"

# include <sbmt/edge/greedy_ngram_info.hpp>

// explicit instantiations ////////////////////////////////////////////////////

MINI_INSTANTIATE_NGRAM_ORDER(MINI_NGRAM_ORDER,LWNgramLM);
MINI_INSTANTIATE_NGRAM_ORDER(MINI_NGRAM_ORDER,dynamic_ngram_lm);

typedef ngram_edge<MINI_NGRAM_ORDER>::type edge_t;
MINI_REVEAL_CHART_INITS(edge_t);
MINI_REVEAL_PARSE_ORDERS(edge_t);

////////////////////////////////////////////////////////////////////////////////

# endif
