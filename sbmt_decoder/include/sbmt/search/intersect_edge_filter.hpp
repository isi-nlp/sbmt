#ifndef SBMT_SEARCH_INTERSECT_PREDICATE_EDGE_FILTER_HPP
#define SBMT_SEARCH_INTERSECT_PREDICATE_EDGE_FILTER_HPP

#include <sbmt/search/edge_filter_predicate.hpp>
#include <sbmt/search/filter_predicates.hpp>


namespace sbmt {

template <class Predicate,class EdgeFilter>
class intersect_predicate_edge_filter
{
public:
    typedef typename EdgeFilter::edge_type edge_type;
    typedef typename EdgeFilter::pred_type p_pred_type;
    typedef typename Predicate::pred_type f_pred_type;
    
    typedef typename select_pred_type<p_pred_type,f_pred_type>::type pred_type;
    
    typedef edge_equivalence<edge_type>   edge_equiv_type;
    
    typedef EdgeFilter filter_type;
    typedef Predicate predicate_type;
    
    intersect_predicate_edge_filter(Predicate const& p,EdgeFilter const& f)
        : pred(p),filter(f)
    {}
    bool adjust_for_retry(unsigned i) 
    {
        return any_change(filter.adjust_for_retry(i),pred.adjust_for_retry(i));
    }

    void print(std::ostream &o) const 
    {
        o << "intersect_predicate_edge_filter{predicate=";
        pred.print(o);
        o << "; filter=";
        filter.print(o);
        o << "}";
    }

    pred_type insert(edge_equivalence_pool<edge_type>& ep, edge_type const& e) 
    {
        pred_type r=pred.keep_edge(filter.get_queue(),e);
        if (!r) return r;
        return filter.insert(ep,e) && r;
    }

    pred_type insert(edge_equiv_type const& eq) 
    {
        pred_type r=pred.keep_edge(filter.get_queue(),eq.representative());
        if (!r) return r;
        return filter.insert(eq) && r;
    }

    void pop() 
    {
        filter.pop();
    }

    edge_equiv_type const& top() const
    {
        return filter.top();
    }

    void finalize() 
    {
        return filter.finalize();
    }

    bool is_finalized() const 
    {
        return filter.is_finalized();
    }
    
    bool empty() const
    {
        return filter.empty();
    }
    
    std::size_t size() const 
    {
        return filter.size();
    }
    
    edge_queue<edge_type> & get_queue() 
    {
        return filter.get_queue();
    }    
    
 private:
    Predicate pred;
    filter_type filter;
};

template <class P,class EF>
intersect_predicate_edge_filter<P,EF>
intersect_edge_filter(P const& p,EF const& ef) 
{
    return intersect_predicate_edge_filter<P,EF>(p,ef);
}

}
             

#endif
