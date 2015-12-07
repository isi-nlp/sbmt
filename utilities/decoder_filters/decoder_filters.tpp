#ifndef   UTILITIES__DECODER_FILTERS__DECODER_FILTERS_TPP
#define   UTILITIES__DECODER_FILTERS__DECODER_FILTERS_TPP

#include <sbmt/span.hpp>
#include <sbmt/search/span_filter_interface.hpp>
#include <sbmt/search/edge_filter.hpp>
#include <boost/function.hpp>
#include <graehl/shared/optional_pair.hpp>
#include <graehl/shared/time_series.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <boost/logic/tribool.hpp>

////////////////////////////////////////////////////////////////////////////////
///
/// this is poor mans version of how we really want to specify these 
/// filters.
/// what we really need is a registry that can read portions of a config
/// file to build these up incrementally.
/// similarly for deciding parse-order, multi-or-single threaded parsing,
/// how-to-swap-in-out-grammars, etc.
///
/// that's why this is the "mini_decoder"
///
////////////////////////////////////////////////////////////////////////////////

class filter_args {
public:
    bool          use_cube_heap;
    bool          use_cube_heap_hist_fuzz;
    bool          use_cube_heap_poplimit;
    
    bool          separate_tag_virt;
    
    std::size_t   limit_syntax_length;
    std::string   limit_syntax_glue_label;
    std::size_t   limit_split_diff;
    bool          quasi_bin;
    unsigned      max_equivalents;
    enum beam_type { FULL=0
                   , TAG
                   , VIRT
                   , UNRY
                   , N_BEAM_TYPES
                   };
    enum beam_scope { SPAN=0, CELL, N_BEAM_SCOPES };
    
    double cube_heap_poplimit_multiplier[N_BEAM_TYPES];
    double cube_heap_softlimit_multiplier[N_BEAM_TYPES];
    double cube_heap_fuzz_exp[N_BEAM_TYPES];
    graehl::optional_pair<sbmt::score_t> cube_heap_hist_fuzz[N_BEAM_TYPES];
    graehl::optional_pair<sbmt::score_t> beam[N_BEAM_TYPES][N_BEAM_SCOPES];
    graehl::optional_pair<sbmt::hist_t> hist[N_BEAM_TYPES][N_BEAM_SCOPES];

    bool cell_prune[N_BEAM_TYPES];

    bool specify_unary_pruning;

    static void require_valid_beam_type(beam_type i);
    
    static void require_valid_beam_scope(beam_scope i);

    BOOST_STATIC_CONSTANT(bool,FUZZ=true);
    
    sbmt::beam_retry hist_fuzz_f(beam_type i, beam_scope j, bool fuzz=!FUZZ) const;
    
    sbmt::beam_retry beam_f(beam_type i, beam_scope j, bool fuzz=!FUZZ) const;
    
    sbmt::hist_retry hist_f(beam_type i, beam_scope j) const;
    
    sbmt::hist_retry poplimit_f(beam_type i, beam_scope j) const;

	sbmt::hist_retry softlimit_f(beam_type i, beam_scope j) const;
    
    void set_defaults();
    void set_force_decode_defaults();
    
    double final_hist_fraction;
    double final_beam_exp_fraction;
    
    // 1: jumps almost instantly to minimum.  
    // 0: regular exponential decay.  
    // more negative -> more lienar
    double retry_curvature; 
    unsigned max_retries;

public:
    filter_args()
    { set_defaults(); }
    
    ////////////////////////////////////////////////////////////////////////////
    
    template <class ET, class Hist, class Beam>
    sbmt::edge_filter< ET
                     , typename sbmt::select_pred_type< 
                                    typename Hist::type::pred_type
                                  , typename Beam::type::pred_type
                                >::type
                     >
    create_span_edge_filter( beam_type i=FULL) const;
    
    template <class ET>
    sbmt::edge_filter<ET>
    create_unary_edge_filter(beam_type i=FULL) const;
    
    template <class ET>
    sbmt::edge_filter<ET>
    create_fuzzy_unary_edge_filter(beam_type i=FULL) const;
    
    template <class ET, class Hist, class Beam>
    sbmt::edge_filter< ET
                     , typename sbmt::select_pred_type< 
                                     typename Hist::type::pred_type
                                   , typename Beam::type::pred_type
                                >::type
                     >
    create_cell_edge_filter( beam_type i=FULL) const;
    
    template <class ET>
    sbmt::edge_filter<ET,boost::logic::tribool>
        create_fuzzy_edge_filter( beam_type i=FULL) const;
    
    template <class ET>
    sbmt::edge_filter<ET>
        create_edge_filter( beam_type i=FULL) const;
    
    template <class ET, class GT, class CT> 
    boost::shared_ptr<sbmt::span_filter_factory<ET,GT,CT> >
        create_single_filter( sbmt::span_t total_span
                            , sbmt::concrete_edge_factory<ET,GT>& ef
                            , GT& gram
                            , beam_type i) const;
    
    template <class ET, class GT, class CT> 
    boost::shared_ptr<sbmt::span_filter_factory<ET,GT,CT> >
        create_sep_filter( sbmt::span_t total_span
                         , sbmt::concrete_edge_factory<ET,GT>& ef
                         , GT& gram ) const;
    
    template <class CT, class ET, class GT> 
    boost::shared_ptr<sbmt::span_filter_factory<ET,GT,CT> >
        create_filter( sbmt::span_t total_span
                     , sbmt::concrete_edge_factory<ET,GT>& ef
                     , GT& gram ) const;
    
    std::size_t maximum_retries() const { return max_retries; }
    
    ////////////////////////////////////////////////////////////////////////////
    
    boost::shared_ptr<sbmt::cky_generator> create_cky_generator() const;
    
    graehl::printable_options_description<std::ostream> options();
};

////////////////////////////////////////////////////////////////////////////////

#endif // UTILITIES__DECODER_FILTERS__DECODER_FILTERS_TPP
