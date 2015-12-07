# if (MINI_NGRAM_ORDER)
# if (MINI_DLM_ORDER)
# include "mini_decoder.hpp"
# include "mini_decoder.ipp"

// explicit instantiations ////////////////////////////////////////////////////

MINI_INSTANTIATE_NGRAM_DLM_ORDER( MINI_NGRAM_ORDER
                                , MINI_DLM_ORDER
                                , LWNgramLM );
MINI_INSTANTIATE_NGRAM_DLM_ORDER( MINI_NGRAM_ORDER
                                , MINI_DLM_ORDER
                                , dynamic_ngram_lm );

typedef ngram_dlm_edge<MINI_NGRAM_ORDER,MINI_DLM_ORDER>::type edge_t;
MINI_REVEAL_CHART_INITS(edge_t);
MINI_REVEAL_PARSE_ORDERS(edge_t);

////////////////////////////////////////////////////////////////////////////////
# endif
# endif
