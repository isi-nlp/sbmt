# if ! defined(UTILITIES__MINI_DECODER_FILTERS_IPP)
# define       UTILITIES__MINI_DECODER_FILTERS_IPP

#include <sbmt/search/cube_heap_span_filter.hpp>
#include <sbmt/search/separate_bins_filter.hpp>
#include <sbmt/search/old_decoder_filter.hpp>
#include <sbmt/search/limit_syntax_length_filter.hpp>
#include <sbmt/search/filter_bank.hpp>
#include <sbmt/search/quasi_bin_filter.hpp>
#include <sbmt/search/special_lhs_factory.hpp>
#include <sbmt/logging.hpp>

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME( fa_domain
                                       , "filter-args"
                                       , sbmt::root_domain );

namespace filters {

////////////////////////////////////////////////////////////////////////////////

template <class ET, class F1, class F2> 
struct cell_tag {
    typedef sbmt::cell_edge_filter<ET, typename F1::type, typename F2::type> 
            type;
};

template <class ET, class F1, class F2> 
cell_tag<ET,F1,F2> cell(F1 const&, F2 const&) { return cell_tag<ET,F1,F2>(); }

////////////////////////////////////////////////////////////////////////////////

template <class ET, class F> 
struct span_tag {
    typedef sbmt::predicate_edge_filter<ET, typename F::type> type;
};

template <class ET, class F> 
span_tag<ET,F> span(F const&) { return span_tag<ET,F>(); }

////////////////////////////////////////////////////////////////////////////////

template <class F1, class F2> struct intersect_tag {
    typedef sbmt::intersection_predicate< typename F1::type
                                        , typename F2::type 
                                        > type;
};

template <class F1, class F2> 
intersect_tag<F1,F2> intersect(F1 const&, F2 const&) 
{ return intersect_tag<F1,F2>(); }

////////////////////////////////////////////////////////////////////////////////

struct poplimit {
    typedef sbmt::poplimit_histogram_predicate type;
};

struct fuzzy_histogram {
    typedef sbmt::fuzzy_histogram_predicate type;
};

struct fuzzy_ratio {
    typedef sbmt::fuzzy_ratio_predicate type;
};

struct histogram {
    typedef sbmt::histogram_predicate type;
};

struct ratio {
    typedef sbmt::ratio_predicate type;
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class F1, class F2>
typename cell_tag<ET,F1,F2>::type
filter( cell_tag<ET,F1,F2> const&
      , filter_args const& params
      , filter_args::beam_type i )
{
    return sbmt::make_cell_edge_filter<ET>( filter(F1(),params,i,filter_args::SPAN)
                                          , filter(F2(),params,i,filter_args::CELL)
                                          );
}

template <class ET, class F>
typename span_tag<ET,F>::type
filter( span_tag<ET,F> const&
      , filter_args const& params
      , filter_args::beam_type i )
{
    return sbmt::make_predicate_edge_filter<ET>(filter(F(),params,i,filter_args::SPAN));
}

template <class F1, class F2>
typename intersect_tag<F1,F2>::type
filter( intersect_tag<F1,F2> const&
      , filter_args const& params
      , filter_args::beam_type i
      , filter_args::beam_scope j )
{
    return intersect_predicates(
                filter(F1(),params,i,j)
              , filter(F2(),params,i,j)
           );
}

inline poplimit::type 
filter( poplimit const& ft
      , filter_args const& params
      , filter_args::beam_type i
      , filter_args::beam_scope j )
{
    return sbmt::poplimit_histogram_predicate( params.hist_f(i,j)
                                             , params.poplimit_f(i,j)
                                             );
}

inline fuzzy_ratio::type
filter( fuzzy_ratio const& 
      , filter_args const& params
      , filter_args::beam_type i
      , filter_args::beam_scope j )
{
    return sbmt::fuzzy_ratio_predicate( params.beam_f(i,j)
                                      , params.beam_f(i,j,filter_args::FUZZ) 
                                      );
}

inline fuzzy_histogram::type
filter( fuzzy_histogram const&
      , filter_args const& params
      , filter_args::beam_type i
      , filter_args::beam_scope j )
{
    return sbmt::fuzzy_histogram_predicate( params.hist_f(i,j)
                                          , params.beam_f(i,j,filter_args::FUZZ) 
                                          );
}

inline ratio::type
filter( ratio const& 
      , filter_args const& params
      , filter_args::beam_type i
      , filter_args::beam_scope j )
{
    return sbmt::ratio_predicate(params.beam_f(i,j));
}

inline histogram::type
filter( histogram const&
      , filter_args const& params
      , filter_args::beam_type i
      , filter_args::beam_scope j )
{
    return sbmt::histogram_predicate(params.hist_f(i,j));
}

} // namespace filter

////////////////////////////////////////////////////////////////////////////////

template <class ET, class Hist, class Beam>
sbmt::edge_filter< ET
                 , typename sbmt::select_pred_type< typename Hist::type::pred_type
                                                  , typename Beam::type::pred_type
                                                  >::type
                 >
filter_args::create_cell_edge_filter( beam_type i) const
{
    using namespace filters;
    SBMT_DEBUG_MSG( fa_domain
                  , "create_cell_edge_filter, beamspan_is_0=%s, beamcell_is_0=%s, beam_type=%s"
                  , (beam_f(i,SPAN)() == 0) % (beam_f(i,CELL)() == 0) % i );
    if (beam_f(i,SPAN)() == 0) {
        if (beam_f(i,CELL)() == 0) {
            return filter(cell<ET>(Hist(),Hist()),*this,i);
        } else {
            return filter(cell<ET>( Hist()
                                  , intersect(Hist(), Beam())
                                  ) , *this, i );
        }
    } else {
        if (beam_f(i,CELL)() == 0) {
            return filter(cell<ET>( intersect(Hist(), Beam())
                                  , Hist()
                                  ) , *this, i );
        } else {
            return filter(cell<ET>( intersect(Hist(), Beam())
                                  , intersect(Hist(), Beam())
                                  ) , *this, i );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class Hist, class Beam>
sbmt::edge_filter< ET
                 , typename sbmt::select_pred_type< typename Hist::type::pred_type
                                                  , typename Beam::type::pred_type
                                                  >::type
                 >
filter_args::create_span_edge_filter( beam_type i) const
{
    using namespace filters;
    
    SBMT_DEBUG_MSG( fa_domain
                  , "create_span_edge_filter, bfspan_is_0=%s beam_type=%s"
                  , (beam_f(i,SPAN)() == 0) % i );
    if (beam_f(i,SPAN)() == 0) {
        return filter(span<ET>(Hist()),*this,i);
    } else {
        return filter(span<ET>(intersect(Hist(),Beam())),*this,i);
    }
}

template <class ET>
sbmt::edge_filter<ET>
filter_args::create_unary_edge_filter( beam_type i) const
{
    SBMT_DEBUG_MSG( fa_domain
                  , "create_unary_edge_filter, sup=%s beam_type=%s"
                  , specify_unary_pruning % i );
    if (specify_unary_pruning) return this->create_edge_filter<ET>(UNRY);
    else return this->create_edge_filter<ET>(i);
}

template <class ET>
sbmt::edge_filter<ET>
filter_args::create_fuzzy_unary_edge_filter( beam_type i) const
{
    SBMT_DEBUG_MSG( fa_domain
                  , "create_fuzzy_unary_edge_filter, sup=%s beam_type%s"
                  , specify_unary_pruning % i );
    if (specify_unary_pruning) 
        return remove_fuzz(this->create_fuzzy_edge_filter<ET>(UNRY));
    else return remove_fuzz(this->create_fuzzy_edge_filter<ET>(i));
}

////////////////////////////////////////////////////////////////////////////////

/// ack! is there perhaps a tag-dispatching way to do this easier?  without resorting
/// to type-erasure?
template <class ET>
sbmt::edge_filter<ET,boost::logic::tribool>
filter_args::create_fuzzy_edge_filter( beam_type i) const
{
    using namespace sbmt;
    using namespace filters;
    bool uchp=use_cube_heap_poplimit && cube_heap_poplimit_multiplier!=0;
    SBMT_DEBUG_MSG( fa_domain
                  , "create_fuzzy_edge_filter, uchp=%s use-cell=%s beam_type=%s"
                  , use_cube_heap_poplimit
                  % (cell_prune[i])
                  % i );
    if (cell_prune[i]) {
        if (uchp) {
            return create_cell_edge_filter<ET,poplimit,fuzzy_ratio>(i);
        } else {
            return create_cell_edge_filter<ET,fuzzy_histogram,fuzzy_ratio>(i);
        }
    } else {
        if (uchp) {
            return create_span_edge_filter<ET,poplimit,fuzzy_ratio>(i);
        } else {
            return create_span_edge_filter<ET,fuzzy_histogram,fuzzy_ratio>(i);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
sbmt::edge_filter<ET>
filter_args::create_edge_filter( beam_type i) const
{
    using namespace sbmt;
    using namespace filters;
    
    SBMT_DEBUG_MSG( fa_domain
                  , "create_edge_filter, use-cell=%s beam_type=%s"
                  , (cell_prune[i])
                  % i );
    if (cell_prune[i]) {
        return create_cell_edge_filter<ET,histogram,ratio>(i);
    } else {
        return create_span_edge_filter<ET,histogram,ratio>(i);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT> 
boost::shared_ptr<sbmt::span_filter_factory<ET,GT,CT> >
filter_args::create_single_filter( sbmt::span_t total_span
                                 , sbmt::concrete_edge_factory<ET,GT>& ef
                                 , GT& gram
                                 , beam_type i ) const
{
    SBMT_DEBUG_MSG( fa_domain
                  , "create_single_filter ch=%s, beam_type=%s"
                  , use_cube_heap % i );
    require_valid_beam_type(i);
    typedef boost::shared_ptr<sbmt::span_filter_factory<ET,GT,CT> > Ret;
    if (use_cube_heap) {
        return Ret(new sbmt::cube_heap_factory<ET,GT,CT>(
                               this->create_fuzzy_edge_filter<ET>(i)
                             , this->create_fuzzy_unary_edge_filter<ET>(i)
                             , ef
                             , gram
                             , total_span 
                       )
                  );
    }
    else
        return Ret(new sbmt::exhaustive_span_factory<ET,GT,CT>( 
                               this->create_edge_filter<ET>(i)
                             , this->create_unary_edge_filter<ET>(i)
                             , total_span
                       )
                  );
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT> 
boost::shared_ptr<sbmt::span_filter_factory<ET,GT,CT> >
filter_args::create_sep_filter( sbmt::span_t total_span
                              , sbmt::concrete_edge_factory<ET,GT>& ef
                              , GT& gram ) const
{
    typedef boost::shared_ptr<sbmt::span_filter_factory<ET,GT,CT> > Ret;
    if (separate_tag_virt)
        return Ret(new sbmt::separate_bins_factory<ET,GT,CT>(
                       create_single_filter<ET,GT,CT>(total_span,ef,gram,TAG),
                       create_single_filter<ET,GT,CT>(total_span,ef,gram,VIRT),
                       total_span));
    else
        return create_single_filter<ET,GT,CT>(total_span,ef,gram,FULL);
}

////////////////////////////////////////////////////////////////////////////////

template <class CT, class ET, class GT> 
boost::shared_ptr<sbmt::span_filter_factory<ET,GT,CT> >
filter_args::create_filter( sbmt::span_t total_span
                          , sbmt::concrete_edge_factory<ET,GT>& ef
                          , GT& gram ) const
{
    sbmt::indexed_token glue_tok = gram.dict().tag("GLUE");
    using namespace sbmt;
    typedef boost::shared_ptr< span_filter_factory<ET,GT,CT> > 
            retval_t;
        retval_t retval = create_sep_filter<ET,GT,CT>(total_span,ef,gram);
//        retval_t glue_filt = create_sep_filter<ET,GT,CT>(total_span,ef,gram);
//        retval = retval_t(new sbmt::special_lhs_factory<ET,GT,CT>( glue_filt
//                                                                 , retval
//                                                                 , glue_tok
//                                                                 , total_span )
//                         );
                         
    if (limit_syntax_length > 0) {
        retval = retval_t(new limit_syntax_length_factory<ET,GT,CT>(
                                  limit_syntax_glue_label
                                , limit_syntax_length
                                , retval
                                , total_span
                              )
                 );
    }
    if (quasi_bin) {
        retval = retval_t(new quasi_bin_factory<ET,GT,CT>(retval,total_span));
    }
    SBMT_DEBUG_MSG(fa_domain,"create_filter: return %s", *retval);
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

# endif //     UTILITIES__MINI_DECODER_FILTERS_IPP

