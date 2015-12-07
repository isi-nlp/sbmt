#ifndef   SBMT_SEARCH_CUBE_HEAP_SPAN_FILTER_HPP
#define   SBMT_SEARCH_CUBE_HEAP_SPAN_FILTER_HPP

#include <sbmt/hash/hash_map.hpp>
#include <sbmt/edge/edge.hpp> 
#include <sbmt/chart/chart.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/search/edge_filter.hpp>
#include <sbmt/search/cell_filter.hpp>
#include <sbmt/search/span_filter_interface.hpp>
#include <sbmt/search/retry_series.hpp>
#include <sbmt/search/logging.hpp>
#include <sbmt/io/log_auto_report.hpp>
#include <sbmt/search/sorted_cube.hpp>

#include <queue>
#include <list>

#include <boost/logic/tribool.hpp>
#include <boost/logic/tribool_io.hpp>

namespace sbmt {
    
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(cube_heap_domain, "cube_heap", search);

namespace detail {



////////////////////////////////////////////////////////////////////////////////
///
/// a key type for keeping track of the different cubes that feed into a cell...
/// a rules foreign side + the span of the first rhs nonterminal that the rule
/// will be applied to completely determine how a cube can be applied
///
////////////////////////////////////////////////////////////////////////////////
template <class GramT>
struct rule_constituent_pair {
    typedef typename GramT::rule_type rule_type;
    rule_type r;
    span_t    first_constituent;
    rule_constituent_pair(rule_type const& r, span_t const& fc)
    : r(r)
    , first_constituent(fc) {}
};

////////////////////////////////////////////////////////////////////////////////

template <class GramT>
struct equal_foreign_sides {
    bool operator()( rule_constituent_pair<GramT> const& r1
                   , rule_constituent_pair<GramT> const& r2 ) const
    {
        return r1.first_constituent     == r2.first_constituent and
               gram.rule_lhs(r1.r)      == gram.rule_lhs(r2.r) and
               gram.rule_rhs_size(r1.r) == gram.rule_rhs_size(r2.r) and 
               gram.rule_rhs(r1.r,0)    == gram.rule_rhs(r2.r,0) and 
               (gram.rule_rhs_size(r1.r) == 2 ? 
                   gram.rule_rhs(r1.r,1) == gram.rule_rhs(r2.r,1) : true);
    }

    equal_foreign_sides(GramT const& gram) : gram(gram) {}
private:
    GramT const& gram;
};

////////////////////////////////////////////////////////////////////////////////

template <class GramT>
struct foreign_side_hash {
    std::size_t operator()(rule_constituent_pair<GramT> const& r) const
    {
        std::size_t retval(0);
        boost::hash_combine(retval,gram.rule_lhs(r.r));
        boost::hash_combine(retval,gram.rule_rhs(r.r,0));
        if (gram.rule_rhs_size(r.r) == 2) 
            boost::hash_combine(retval,gram.rule_rhs(r.r,1));
        boost::hash_combine(retval,r.first_constituent);
        return retval;
    }

    foreign_side_hash(GramT const& gram) : gram(gram) {}
private:
    GramT const& gram;
};

////////////////////////////////////////////////////////////////////////////////
    
template <class ET,class GT>
struct rule_edge_triple
{
    typedef typename GT::rule_type rule_type;
    typedef edge_equivalence<ET> edge_equiv_type;
    
    rule_type       rule;
    edge_equiv_type edge1;
    edge_equiv_type edge2;
    score_t         scr;
    
    score_t score() const { return scr; }
    
    bool operator < (rule_edge_triple const& other) const
    { return scr < other.scr; }
    
    rule_edge_triple( rule_type const& r
                    , score_t r_score
                    , edge_equiv_type const& e1
                    , edge_equiv_type const& e2 )
    : rule(r)
    , edge1(e1)
    , edge2(e2)
    , scr(r_score * 
          e1.representative().score() *
          e2.representative().score() ){}
};

////////////////////////////////////////////////////////////////////////////////

template <class GramT>
struct rule_applications {
    typedef typename GramT::rule_type rule_type;
    typedef concrete_forward_iterator<rule_type const> iterator;
    typedef boost::tuple<iterator,iterator> range;
    
    rule_applications(range const& rr, span_t const& s)
    : left(s)
    , begin_(rr.template get<0>())
    , end_(rr.template get<1>()) {}
    
    rule_type const& rule() const { return *(begin_); }
    span_t const& left_constituent() const { return left; }
    
    iterator begin() const { return begin_; }
    iterator end() const { return end_; }
private:
    span_t left;
    iterator  begin_;
    iterator  end_;
};

template <class GramT>
struct equal_applications {
    bool operator()( rule_constituent_pair<GramT> const& r1
                   , rule_constituent_pair<GramT> const& r2 ) const
    {
        return r1.first_constituent == r2.first_constituent and
               gram.rule_rhs_size(r1.r) == gram.rule_rhs_size(r2.r) and 
               gram.rule_rhs(r1.r,0) == gram.rule_rhs(r2.r,0) and 
               (gram.rule_rhs_size(r1.r) == 2 ? 
                   gram.rule_rhs(r1.r,1) == gram.rule_rhs(r2.r,1) : true);
    }
    
    equal_applications(GramT const& gram) : gram(gram) {}
private:
    GramT const& gram;
};

////////////////////////////////////////////////////////////////////////////////

template <class GramT>
struct application_hash {
    std::size_t operator()(rule_constituent_pair<GramT> const& r) const
    {
        std::size_t retval(0);
        boost::hash_combine(retval,gram.rule_rhs(r.r,0));
        if (gram.rule_rhs_size(r.r) == 2) 
            boost::hash_combine(retval,gram.rule_rhs(r.r,1));
        boost::hash_combine(retval,r.first_constituent);
        return retval;
    }
    
    application_hash(GramT const& gram) : gram(gram) {}
private:
    GramT const& gram;
};

} // namespace detail

////////////////////////////////////////////////////////////////////////////////

template <class FilterF>
class fuzzless_edge_filter
{
public:
    typedef bool                              pred_type;
    typedef typename FilterF::edge_type       edge_type;
    typedef typename FilterF::edge_equiv_type edge_equiv_type;
    
    fuzzless_edge_filter(FilterF f) : filt(f) {}
        
    bool adjust_for_retry(unsigned int i) 
    { 
        assert(!is_finalized());
        assert(filt.empty());
        return filt.adjust_for_retry(i); 
    }

    void print(std::ostream& o) const 
    { o << "fuzzless_edge_filter:"; filt.print(o); }

    pred_type insert(edge_equivalence_pool<edge_type>& ep, edge_type const& e)
    { return filt.insert(ep,e) == true; }

    pred_type insert(edge_equiv_type const& eq)
    { return filt.insert(eq) == true; }
    
    edge_equiv_type const& top() const { return filt.top(); }
    
    void pop() { filt.pop(); }
    
    void finalize() { filt.finalize(); }
    bool is_finalized() const { return filt.is_finalized(); }
    
    bool empty() const { return filt.empty(); }
    std::size_t size() const { return filt.size(); }
    edge_queue<edge_type> & get_queue() { return filt.get_queue(); }
private:
    FilterF filt;
};

template <class FilterF> 
fuzzless_edge_filter<FilterF> remove_fuzz(FilterF const& f)
{ return fuzzless_edge_filter<FilterF>(f); }

template <class EdgeT, class GramT, class ChartT>
class cube_heap_factory;

template <class EdgeT, class GramT, class ChartT>
class cube_heap_span_filter 
: public span_filter_interface<EdgeT, GramT, ChartT>
{
    typedef span_filter_interface<EdgeT, GramT, ChartT> base_t;
public:
    typedef EdgeT edge_type;
    typedef typename GramT::rule_type rule_type;
    typedef edge_equivalence<edge_type> edge_equiv_type;
    typedef typename base_t::rule_range rule_range;
    typedef typename base_t::rule_iterator rule_iterator;
    typedef typename base_t::edge_range edge_range;

    template <class FuzzyFunc>
    cube_heap_span_filter( FuzzyFunc f
                         , span_t const& target_span
                         , GramT& gram
                         , concrete_edge_factory<EdgeT,GramT>& ecs
                         , ChartT& chart
                         , cube_heap_factory<EdgeT,GramT,ChartT>* parent);
    
    virtual void apply_rules( rule_range const& rr
                            , edge_range const& er1
                            , edge_range const& er2 );

    virtual void finalize();
    virtual bool is_finalized() const;
    
//    score_t rule_score_estimate(typename GramT::rule_type r);
    
    virtual void pop();
    virtual edge_equiv_type const& top() const;
    virtual bool empty() const;
    
    virtual ~cube_heap_span_filter() {}
private:
    typedef sorted_cube< rule_iterator
                       , typename edge_range::iterator
                       , typename edge_range::iterator
                       > sorted_cube_t;

    typedef std::list<sorted_cube_t> sorted_cube_list_t;
    sorted_cube_list_t sorted_cube_list;

    void fill_heap_from_cube(
          std::priority_queue< detail::rule_edge_triple<EdgeT,GramT> >& pq
        , detail::rule_applications<GramT>& apps
        , score_t& current_best 
    );

    edge_filter<EdgeT,boost::logic::tribool>       fuzzy_filter;
    bool                                           finalized;
    cube_heap_factory<EdgeT,GramT,ChartT>*         parent;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class cube_heap_factory
: public span_filter_factory<EdgeT, GramT, ChartT>
{
    typedef span_filter_factory<EdgeT, GramT, ChartT> base_t;
public:
    template <class O>
    void print(O &o) const
    {
        o << "cube_heap_factory{";
        o << "filter=" << fuzzy_filter;
        o << "}";
    }
    virtual void print_settings(std::ostream &o) const 
    {
        print(o);
    }
    
    virtual bool adjust_for_retry(unsigned i) 
    {
        return any_change( fuzzy_filter.adjust_for_retry(i)
                         , u_filter.adjust_for_retry(i) );
    }
    
    virtual edge_filter<EdgeT> unary_filter(span_t const& target_span)
    {
        return u_filter;
    }

    template <class FuzzyFiltF>
    cube_heap_factory( FuzzyFiltF f
                     , concrete_edge_factory<EdgeT,GramT>& ef
                     , GramT& gram
                     , span_t const& total_span )
    : base_t(total_span)
    , fuzzy_filter(f)
    , u_filter(remove_fuzz(f)) {}
    
    template <class FuzzyFiltF, class UnaryFiltF>
    cube_heap_factory( FuzzyFiltF f
                     , UnaryFiltF uf
                     , concrete_edge_factory<EdgeT,GramT>& ef
                     , GramT& gram
                     , span_t const& total_span )
    : base_t(total_span)
    , fuzzy_filter(f)
    , u_filter(uf) {}

    virtual typename base_t::result_type 
    create( span_t const& target
          , GramT& gram
          , concrete_edge_factory<EdgeT,GramT>& ecs
          , ChartT& chart )
    {
        return new cube_heap_span_filter<EdgeT,GramT,ChartT> ( fuzzy_filter
                                                             , target
                                                             , gram
                                                             , ecs
                                                             , chart 
                                                             , this );
    }
    
//    score_t rule_score_estimate(typename GramT::rule_type r)
//    {
//        //return ecs.score_estimate(gram,r);
//        typename rule_score_map_t::iterator pos = rule_score_map.find(r);
//        return pos->second;
//        else {
//            score_t retval = ecs.score_estimate(gram,r);
//            rule_score_map.insert(std::make_pair(r,retval));
//            return retval;
//        }
//    }
    
    virtual ~cube_heap_factory(){}
private:
    edge_filter<EdgeT,boost::logic::tribool> fuzzy_filter;
    edge_filter<EdgeT,bool> u_filter;
    
    //typedef oa_hashtable< std::pair<typename GramT::rule_type, score_t>
    //                    , first<typename GramT::rule_type, score_t> >
    //        rule_score_map_t;
    //rule_score_map_t rule_score_map;
};

////////////////////////////////////////////////////////////////////////////////

}

#include <sbmt/search/impl/cube_heap_span_filter.ipp>

#endif // SBMT_SEARCH_CUBE_HEAP_SPAN_FILTER_HPP

