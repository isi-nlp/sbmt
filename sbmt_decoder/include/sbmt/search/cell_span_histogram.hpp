#if 0
#ifndef   SBMT_SEARCH_CELL_SPAN_HISTOGRAM_HPP
#define   SBMT_SEARCH_CELL_SPAN_HISTOGRAM_HPP

#include <vector>
#include <sbmt/hash/oa_hashtable.hpp>
#include <sbmt/search/span_filter_interface.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>

#include <sbmt/hash/hash_map.hpp>

namespace sbmt
{

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
class cell_span_histogram 
: public span_filter_interface<ET,GT,CT>
{
public:
    typedef span_filter_interface<ET,GT,CT> base_t;
    
    virtual void apply_rule( typename base_t::rule_type const& r
                           , span_t const& first_constituent );
                            
    virtual void finalize();
    virtual bool is_finalized() const;
    
	virtual typename base_t::edge_equiv_type const& top() const {
		return current->second.back();
	}
    virtual bool empty() const;
    virtual void pop();
    
    cell_span_histogram( boost::shared_ptr<base_t> filt
                       , std::size_t span_max
                       , std::size_t cell_max
                       , span_t const& target_span
                       , GT& gram
                       , concrete_edge_factory<ET,GT>& ecs
                       , CT& chart )
    : base_t(target_span,gram,ecs,chart)
    , cell_max(cell_max)
    , span_max(span_max)
    , filt(filt)
    {}
    
    virtual ~cell_span_histogram() {}

private:
    std::size_t cell_max;
    std::size_t span_max;
    
    typedef stlext::hash_map< indexed_token
                               , std::vector<typename base_t::edge_equiv_type>
                               , boost::hash<indexed_token> > cell_table_t;
            
    cell_table_t                    cells;
    typename cell_table_t::iterator current;
    typename cell_table_t::iterator end;
    boost::shared_ptr<base_t>       filt;
    
    bool insert(typename base_t::edge_equiv_type const& eq);
}; 

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void cell_span_histogram<ET,GT,CT>::apply_rule(
                                        typename base_t::rule_type const& r
                                      , span_t const& first_constituent )
{
    filt->apply_rule(r,first_constituent);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void cell_span_histogram<ET,GT,CT>::finalize()
{
    typedef typename base_t::edge_equiv_type edge_equiv_type;
    
    typedef std::vector<typename base_t::edge_equiv_type> edge_vec_t;
    edge_vec_t edge_vec;
    filt->finalize();
    while (!filt->empty()) {
        edge_vec.push_back(filt->top());
        filt->pop();
    }
    std::sort(edge_vec.begin(), edge_vec.end(), greater_edge_score<ET>());
    for (unsigned x = 1; x < edge_vec.size(); ++x)
        assert( edge_vec[x-1].representative().score()
                >= edge_vec[x].representative().score() );
    std::size_t span_count = 0;
    std::size_t vec_count  = 0;
    score_t curr_score;
    while (vec_count != edge_vec.size() and span_count <= span_max) {
        if (insert(edge_vec[vec_count])) ++span_count;
        curr_score = edge_vec[vec_count].representative().score();
        ++vec_count;
    }
    while (vec_count != edge_vec.size() and 
           edge_vec[vec_count].representative().score() == curr_score) {
        /// we allow ties
        insert(edge_vec[vec_count]);
        ++vec_count;
    }
    
    current = cells.begin();
    end     = cells.end();
    while (current != end and current->second.empty()) ++current;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool cell_span_histogram<ET,GT,CT>::insert(
                                    typename base_t::edge_equiv_type const& eq
                                    )
{
    using namespace std;
    
    typedef vector<typename base_t::edge_equiv_type> edge_vec_t;
    
    typename cell_table_t::iterator pos = cells.find(eq.representative().root());
    
    if (pos == cells.end()) {
        pos = cells.insert(make_pair(eq.representative().root(),
                                     edge_vec_t())
                          ).first;
    }
    
    if (pos->second.size() < cell_max) {
        const_cast<edge_vec_t&>(pos->second).push_back(eq);
        return true;
    } 
    else if ( pos->second.back().representative().score() 
              == eq.representative().score()) {
        /// we allow ties, but it doesnt contribute to the span count.
        const_cast<edge_vec_t&>(pos->second).push_back(eq);
        return false;
    }
    return false;
    
}

template <class ET, class GT, class CT>
bool cell_span_histogram<ET,GT,CT>::is_finalized() const
{ return filt->is_finalized(); }

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool cell_span_histogram<ET,GT,CT>::empty() const
{
    return current == end;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void cell_span_histogram<ET,GT,CT>::pop()
{
    typedef std::vector<typename base_t::edge_equiv_type> edge_vec_t;
    
    const_cast<edge_vec_t&>(current->second).pop_back();
    while ((current != end) and current->second.empty()) {
        ++current;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class cell_span_histogram_factory
: public span_filter_factory<EdgeT, GramT, ChartT>
{
public:
    typedef span_filter_factory<EdgeT, GramT, ChartT> base_t;
    
    cell_span_histogram_factory( boost::shared_ptr<base_t> span_filt_f
                               , std::size_t span_max
                               , std::size_t cell_max
                               , span_t const& total_span )
    : base_t(total_span)
    , span_max(span_max)
    , cell_max(cell_max)
    , span_filt_f(span_filt_f) {}
    
    virtual typename base_t::result_type 
    create( span_t const& target
          , GramT& gram
          , concrete_edge_factory<EdgeT,GramT>& ecs
          , ChartT& chart )
    {
        typedef span_filter_interface<EdgeT,GramT,ChartT> span_filter_t;
        boost::shared_ptr<span_filter_t> 
            p(span_filt_f->create(target,gram,ecs,chart));
            
        return new cell_span_histogram<EdgeT,GramT,ChartT>( p
                                                          , span_max
                                                          , cell_max
                                                          , target
                                                          , gram
                                                          , ecs
                                                          , chart );
    }
    
    virtual void print_settings(std::ostream &o) const 
    {
        o << "cell_span_histogram_factory{ ";
        span_filt_f->print_settings(o);
        o << " }";
    }

    //TODO: retry
    virtual bool adjust_for_retry(unsigned i) 
    { return span_filt_f->adjust_for_retry(i); }

    
    virtual ~cell_span_histogram_factory() {}
private:
    std::size_t span_max;
    std::size_t cell_max;
    boost::shared_ptr<base_t> span_filt_f;
};

}

#endif // SBMT_SEARCH_CELL_SPAN_HISTOGRAM_HPP
#endif