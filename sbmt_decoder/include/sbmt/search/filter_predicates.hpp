#ifndef   SBMT_SEARCH_FILTER_PREDICATES_HPP
#define   SBMT_SEARCH_FILTER_PREDICATES_HPP

#include <sbmt/search/edge_filter.hpp>
#include <sbmt/search/retry_series.hpp>
#include <sbmt/search/logging.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/logic/tribool_io.hpp>
#include <boost/compressed_pair.hpp>
#include <iosfwd>

namespace sbmt {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(filters,search);

////////////////////////////////////////////////////////////////////////////////
///
/// uses a threshold to determine if the edge should be pruned.  if an inserted
/// edge is threshold times smaller than the best score seen so far, then it is
/// not stored in the filter.  if a new best scoring edge comes along, then 
/// edges that no longer meet the threshold with the new best score are tossed.
///
////////////////////////////////////////////////////////////////////////////////

class ratio_predicate
{
public:
    typedef bool  pred_type;
    
    ratio_predicate(score_t threshold);
    ratio_predicate(beam_retry const& retry_threshold);

    void print(std::ostream& o) const;
    
    bool adjust_for_retry(unsigned i);

    template <class E> 
    pred_type keep_edge(edge_queue<E> const& pqueue, E const& e) const;
    
    template <class E>
    bool pop_top(edge_queue<E> const& pqueue) const;

private:
    beam_t threshold;
    beam_t fuzz;
    beam_retry retry_threshold;
};

class fuzzy_ratio_predicate
{
public:
    typedef boost::logic::tribool pred_type;
    
    fuzzy_ratio_predicate( score_t threshold, score_t fuzz );
    fuzzy_ratio_predicate( beam_retry const& retry_threshold
                         , beam_retry const& retry_fuzz );

    void print(std::ostream& o) const;
    
    bool adjust_for_retry(unsigned i);

    template <class E> 
    pred_type keep_edge(edge_queue<E> const& pqueue, E const& e) const;
    
    template <class E>
    bool pop_top(edge_queue<E> const& pqueue) const;

private:
    beam_t threshold;
    beam_t fuzz;
    beam_retry retry_threshold;
    beam_retry retry_fuzz;
};

////////////////////////////////////////////////////////////////////////////////

class pass_thru_predicate
{
public:
    typedef bool      pred_type;

    void print(std::ostream& o) const;
    
    bool adjust_for_retry(unsigned i);
    
    template <class ET>
    bool keep_edge(edge_queue<ET> const&, ET const&) const { return true; }
    
    template <class ET>
    bool pop_top(edge_queue<ET> const&) const { return false; }
};

////////////////////////////////////////////////////////////////////////////////

class histogram_predicate
{
public:
    typedef bool      pred_type;
    
    histogram_predicate(std::size_t top_n);
    
    histogram_predicate(hist_retry const& retry_top_n);

    void print(std::ostream& o) const;
    
    bool adjust_for_retry(unsigned i);
    
    template <class E>
    pred_type keep_edge(edge_queue<E> const& pqueue, E const& e) const;
    
    template <class E>
    bool pop_top(edge_queue<E> const& pqueue) const;

private:
    hist_t top_n;
    hist_retry retry_top_n;
};

class fuzzy_histogram_predicate
{
public:
    typedef boost::logic::tribool pred_type;
    
    fuzzy_histogram_predicate(std::size_t top_n, score_t fuzz);
    
    fuzzy_histogram_predicate( hist_retry const& retry_top_n
                             , beam_retry const& retry_fuzz );

    void print(std::ostream& o) const;
    
    bool adjust_for_retry(unsigned i);
    
    template <class E>
    pred_type keep_edge(edge_queue<E> const& pqueue, E const& e) const;
    
    template <class E>
    bool pop_top(edge_queue<E> const& pqueue) const;

private:
    hist_t top_n;
    beam_t fuzz;
    hist_retry retry_top_n;
    beam_retry retry_fuzz;
};

class poplimit_histogram_predicate
{
public:
    typedef boost::logic::tribool pred_type;
    
    poplimit_histogram_predicate( std::size_t top_n
                                , std::size_t poplimit
                                , std::size_t softlimit );
    
    poplimit_histogram_predicate( hist_retry const& retry_top_n
                                , hist_retry const& retry_poplimit
                                , hist_retry const& retry_softlimit );
        
    void print(std::ostream& o) const;
    
    bool adjust_for_retry(unsigned i);
    
    template <class E>
    pred_type keep_edge(edge_queue<E> const& pqueue, E const& e) const;
    
    template <class E>
    bool pop_top(edge_queue<E> const& pqueue) const;
private:
    hist_t top_n;
    hist_t poplimit;
    hist_t softlimit;
    mutable hist_t num_keep_queries;
    hist_retry retry_top_n;
    hist_retry retry_poplimit;
    hist_retry retry_softlimit;
};

////////////////////////////////////////////////////////////////////////////////
/// 
///  used for composite predicates.
///  basically, composing a fuzzy filter with any other kind of filter gives you
///  a fuzzy filter
///
////////////////////////////////////////////////////////////////////////////////
template <class P1, class P2>
struct select_pred_type
{ };

template <>
struct select_pred_type<boost::logic::tribool, boost::logic::tribool>
{ typedef boost::logic::tribool type; };

template <> 
struct select_pred_type<bool, boost::logic::tribool>
{ typedef boost::logic::tribool type; };

template <>
struct select_pred_type<boost::logic::tribool, bool>
{ typedef boost::logic::tribool type; };

template <>
struct select_pred_type<bool, bool>
{ typedef bool type; };

////////////////////////////////////////////////////////////////////////////////

template <class Filter1T, class Filter2T>
class intersection_predicate
: public boost::compressed_pair<Filter1T, Filter2T>
{
    typedef boost::compressed_pair<Filter1T, Filter2T> base_t;
public:
    typedef Filter1T first_filter_type;
    typedef Filter2T second_filter_type;
    typedef typename select_pred_type<typename Filter1T::pred_type
                                     ,typename Filter2T::pred_type>::type 
            pred_type;
    
    intersection_predicate( first_filter_type first_filter
                          , second_filter_type second_filter )
    : base_t(first_filter,second_filter) {}

    void print(std::ostream& o) const 
    {
        o << "intersection_predicate { 1=";
        base_t::first().print(o);
        o << "; 2=";
        base_t::second().print(o);
        o << " }";
    }
    
    bool adjust_for_retry(unsigned i) 
    {
        return any_change( base_t::first().adjust_for_retry(i)
                         , base_t::second().adjust_for_retry(i)
                         );
    }

    template <class E>
    pred_type keep_edge(edge_queue<E> const& pqueue, E const& e) const
    { 
        pred_type p = base_t::first().keep_edge(pqueue,e) and 
                      base_t::second().keep_edge(pqueue,e);
        SBMT_PEDANTIC_STREAM(
            filters
          , "intersection_predicate::keep_edge(queue, e) : " 
            << std::boolalpha << p 
        );
        return p;
    }

    template <class E>
    bool pop_top(edge_queue<E> const& pqueue) const
    { 
        return base_t::first().pop_top(pqueue) or 
               base_t::second().pop_top(pqueue);
    }
};

template<class F1, class F2>
intersection_predicate<F1, F2> 
intersect_predicates(F1 const& filt1, F2 const& filt2)
{
    return intersection_predicate<F1, F2>(filt1,filt2);
}

////////////////////////////////////////////////////////////////////////////////

template <class Filter1T, class Filter2T>
class union_predicate
: public boost::compressed_pair<Filter1T, Filter2T>
{
    typedef boost::compressed_pair<Filter1T, Filter2T> base_t;
public:
    typedef Filter1T first_filter_type;
    typedef Filter2T second_filter_type;
    typedef typename select_pred_type<typename Filter1T::pred_type
                                     ,typename Filter2T::pred_type>::type 
            pred_type;
    
    union_predicate( first_filter_type first_filter
                   , second_filter_type second_filter )
    : base_t(first_filter, second_filter) {}
                          
    template <class O>
    void print(O &o) const 
    {
        o << "union_predicate { 1=";
        base_t::first().print(o);
        o << "; 2=";
        base_t::second().print(o);
        o << " }";
    }
    
    bool adjust_for_retry(unsigned i) 
    {
        return any_change( base_t::first().adjust_for_retry(i)
                         , base_t::second().adjust_for_retry(i) );
    }

    template <class E>
    pred_type keep_edge(edge_queue<E> const& pqueue, E const& e) const
    { 
        pred_type p = base_t::first().keep_edge(pqueue,e) or 
                      base_t::second().keep_edge(pqueue,e) ;
                      
        SBMT_PEDANTIC_STREAM(
            filters
          , "union_predicate::keep_edge(queue, e) : " 
            << std::boolalpha << p 
        );
        return p;
    }

    template <class E>
    bool pop_top(edge_queue<E> const& pqueue) const
    { 
        return base_t::first().pop_top(pqueue) and 
               base_t::second().pop_top(pqueue) ;
    }
};

template<class F1, class F2>
union_predicate<F1, F2> 
union_predicates(F1 const& filt1, F2 const& filt2)
{
    return union_predicate<F1, F2>(filt1,filt2);
}

////////////////////////////////////////////////////////////////////////////////

template <class RP, class HP>
intersection_predicate<ratio_predicate, histogram_predicate>
intersect_ratio_histogram(RP const& rp, HP const& hp)
{
    return intersect_predicates(ratio_predicate(rp), histogram_predicate(hp));
}

template <class RP, class HP>
union_predicate<ratio_predicate, histogram_predicate>
union_ratio_histogram(RP const& rp, HP const& hp)
{
    return union_predicates(ratio_predicate(rp), histogram_predicate(hp));
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/search/impl/filter_predicates.ipp>

#endif // SBMT_SEARCH_FILTER_PREDICATES_HPP
