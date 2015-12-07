#ifndef   SBMT_SEARCH_FILTER_TESTER_HPP
#define   SBMT_SEARCH_FILTER_TESTER_HPP

#include <ext/hash_set>
#include <vector>
#include <sbmt/search/span_filter_interface.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
/// used as a sanity check on filter interfaces.
/// you provide a test functor that has an additional call named reset()
/// when reset is called, you are given every edge that was considered by the
/// filter being tested.  from there, your functor will be called repeatedly, and
/// you say whether or not the filter should have accepted the edge (true means
/// accept, false means prune)
///
/// note: not for use when running production system.
///
////////////////////////////////////////////////////////////////////////////////
template <class ET, class GT, class CT>
class filter_tester
: public span_filter_interface<ET,GT,CT>
{
public:
    typedef span_filter_interface<ET,GT,CT> base_t;
    
    typedef __gnu_cxx::hash_set< typename base_t::edge_type
                               , boost::hash<typename base_t::edge_type> >
            edge_set_t;
    
    typedef typename edge_set_t::iterator edge_iterator;
    
    class tester
    {
    public:
        virtual void reset(edge_iterator begin, edge_iterator end) = 0;
        virtual bool operator()(typename base_t::edge_type const& e) const = 0;
        virtual ~tester(){}
    };
    
    filter_tester( boost::shared_ptr<base_t> filt
                 , tester& test_func
                 , span_t const& target_span
                 , GT& gram
                 , concrete_edge_factory<ET,GT>& ecs
                 , CT& chart )           
    : base_t(target_span,gram,ecs,chart)
    , filt(filt)
    , test_func(test_func) {}
    
    ////////////////////////////////////////////////////////////////////////////
    
    virtual void apply_rule( typename base_t::rule_type const& r
                           , span_t const& first_constituent )
    {
        assert(base_t::gram.rule_rhs_size(r) == 2);
        typename chart_traits<CT>::edge_range er1 = 
                     base_t::chart.edges( base_t::gram.rule_rhs(r,0)
                                        , first_constituent );
                                        
        span_t second_constituent( first_constituent.right()
                                 , base_t::target_span.right() );    
                                       
        typename chart_traits<CT>::edge_range er2 = 
                     base_t::chart.edges( base_t::gram.rule_rhs(r,1)
                                        , second_constituent );
        
        typename chart_traits<CT>::edge_iterator e1itr = er1.begin();
        typename chart_traits<CT>::edge_iterator e1end = er1.end();
        typename chart_traits<CT>::edge_iterator e2itr = er2.begin();
        typename chart_traits<CT>::edge_iterator e2end = er2.end();
        
        for (; e1itr != e1end; ++e1itr) {
            for (; e2itr != e2end; ++e2itr) {
                typename base_t::edge_type e = base_t::ecs.create_edge(
                                                   base_t::gram
                                                 , r
                                                 , *e1itr
                                                 , *e2itr
                                               );
                insert(all_edges, e);
            }
        }
        
        filt->apply_rule(r,first_constituent);
    }
    
    ////////////////////////////////////////////////////////////////////////////
    
    virtual void finalize() 
    {
        indexed_token_factory &tf = base_t::gram.dict();
        filt->finalize();
        while (not filt->empty()) {
            typename base_t::edge_equiv_type eq = filt->top();
            insert(kept_edges, eq.representative(), true);
            edge_q.push_back(eq);
            filt->pop();
        }
        
        test_func.reset(all_edges.begin(), all_edges.end());
        
        edge_iterator itr = all_edges.begin();
        edge_iterator end = all_edges.end();
        for (; itr != all_edges.end(); ++itr) {
            edge_iterator pos = kept_edges.find(*itr);
            bool should_keep = test_func(*itr);
            bool did_keep = (pos != kept_edges.end());
            if (did_keep) {
                assert(max(pos->score(),itr->score())/min(pos->score(),itr->score())
                       < score_t(1.0 + 1e-5));
            }
            if (should_keep != did_keep) {
                std::cout << "ERROR: filtering mistake: edge: " 
                          << print(*itr,tf) << "expected to be kept? "
                          << (should_keep?"true":"false") << std::endl;
            }
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    
    virtual bool is_finalized() const { return filt->is_finalized(); }
    
    virtual typename base_t::edge_equiv_type const & top() const
    {
        return edge_q.back();
    }
    
    virtual void pop() { edge_q.pop_back(); }
    
    virtual bool empty() const { return edge_q.empty(); }
    
    ////////////////////////////////////////////////////////////////////////////
    
private:    
    typedef std::vector< typename base_t::edge_equiv_type >
            edge_queue_t;
    
    edge_set_t   all_edges;
    edge_set_t   kept_edges;
    edge_queue_t edge_q;
    
    boost::shared_ptr<base_t> filt;
    tester& test_func;
    
    void insert(edge_set_t& eset, typename base_t::edge_type const& e,bool unique=false)
    {
        typename edge_set_t::iterator pos = eset.find(e);
        if (pos != eset.end()) {
            if (unique) 
                std::cout<<"ERROR: filter returned duplicate edge"<<std::endl;
            if (pos->inside_score() < e.inside_score()) {
                eset.erase(pos);
                eset.insert(e);
            }
        }
        else eset.insert(e);
    }
};

template <class ET, class GT, class CT>
class filter_tester_factory
: public span_filter_factory<ET,GT,CT>
{
public:
    typedef span_filter_factory<ET,GT,CT> base_t;
    typedef filter_tester<ET,GT,CT> product_t;
    typedef boost::shared_ptr<span_filter_interface<ET,GT,CT> > product_ptr_t;
    typedef typename product_t::tester tester;
    filter_tester_factory( boost::shared_ptr<base_t> factory
                         , tester& test_func
                         , span_t const& target_span )
    : base_t(target_span)
    , factory(factory)
    , test_func(test_func) {}
    
    virtual typename base_t::result_type 
    create( span_t const& target_span
          , GT& gram
          , concrete_edge_factory<ET,GT>& ecs
          , CT& chart )
    {
        product_ptr_t p(factory->create(target_span,gram,ecs,chart));
        return new product_t(p,test_func,target_span,gram,ecs,chart);
    }
private:
    boost::shared_ptr<base_t> factory;
    tester& test_func;
};
    
} // namespace sbmt

#endif // SBMT_SEARCH_FILTER_TESTER_HPP
