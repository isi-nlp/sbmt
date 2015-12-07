#ifndef   SBMT_EDGE_CHART_HPP
#define   SBMT_EDGE_CHART_HPP

//FIXME: span_max_rt (target_span) should be passed to chart on creation, not divined from maximum over things entered into it

#include <sbmt/span.hpp>
#include <sbmt/chart/unordered_cell.hpp>
#include <sbmt/chart/unordered_span.hpp>
#include <sbmt/search/concrete_edge_factory.hpp>
#include <sbmt/edge/edge.hpp>
#include <sbmt/hash/hash_map.hpp>
#include <stdexcept>

namespace sbmt {

struct empty_chart : std::runtime_error {
    empty_chart() : std::runtime_error("didn't build any toplevel items") {}
};

////////////////////////////////////////////////////////////////////////////////
///
///  basic chart.  Allows user to decide if edges should be sorted by score in
///  an cell, and whether cells should be sorted by best score of an edge within
///  an cell.
///
///  furthermore, by creating new classes with interfaces similar to those in
///  [un]ordered_(cells|spans).hpp, you should be able to customise basic_chart
///  in other ways.
///
////////////////////////////////////////////////////////////////////////////////
template < class EdgeT
         , class IP = unordered_chart_edges_policy
         , class SP = unordered_chart_cells_policy
         >
class basic_chart
{
public:
    typedef typename IP::template cell_type<EdgeT>::type 
            cell_type;
    typedef EdgeT edge_type;
    typedef edge_equivalence<EdgeT> edge_equiv_type;
    typedef basic_chart<EdgeT,IP,SP> chart_type;
private:
    typedef typename SP::template span_type<cell_type>::type
            span_type;
    typedef stlext::hash_map<span_t,span_type,boost::hash<span_t> > 
            span_container_t;
public:
    typedef span_type& span_storage_reference;
    typedef cell_type& cell_storage_reference;
    typedef typename cell_type::iterator edge_iterator;
    typedef typename cell_type::range    edge_range;
    typedef typename span_type::iterator cell_iterator;
    typedef typename span_type::range    cell_range;
    enum { edges_ordered = IP::ordered, cells_ordered = SP::ordered };
    
    basic_chart(std::size_t max_span_length);
    
    span_t target_span() const 
    {
        return span_t(0,max_span_rt);
    }
   
    std::pair<edge_iterator,bool> find(edge_type const& e) const;
    
    edge_range edges(indexed_token t, span_t s) const;

    template <class GramT>
    edge_equiv_type top_equiv(GramT const& g) const 
    {
        edge_range er=edges(g.dict().toplevel_tag(),target_span());
        if (er.begin() == er.end())
            throw empty_chart();
        edge_equiv_type best=*er.begin();
        //FIXME: is this really necessary any more?  
        //I don't think so. should only exist one top equiv.  also, kbest, dfs_forest, etc. rely on traversing recursively starting w/ top_equiv.  assert only one equiv?
        edge_iterator eitr = er.begin();
        ++eitr;
        if (eitr!=er.end())
            for(; eitr != er.end(); ++eitr)
                if (best.representative().score() < eitr->representative().score())
                    best = *eitr;
        return best;
    }
    
    span_storage_reference span_storage(span_t);
    
    void clear(span_t);
    void clear() { reset(max_span_rt); }
    std::pair<cell_iterator,bool> cell(span_t, indexed_token t);
    
    cell_range cells(span_t) const;

    void insert_edge( edge_equivalence_pool<edge_type>& epool
                    , edge_type const& e );
                    
    void insert_edge(edge_equiv_type const& eq);
                    
    void       reset(std::size_t max_span_length);

    struct span_contents
    {
        std::size_t cells;
        std::size_t items;
        std::size_t edges;
        void reset() 
        {
            cells=items=edges=0;
        }
        
        span_contents(chart_type const& c) 
        {
            reset();
            c.visit_items(*this);
        }

        span_contents(chart_type const& c,span_t const& span) 
        {
            reset();
            c.visit_items(*this,span);
        }
        
        void visit_cell(cell_type const& cell) 
        {
            cells+=cell.size();
        }

        void visit(edge_equiv_type const &e) 
        {
            ++items;
            edges+=e.size();
        }

        template <class O>
        void print(O &o) const 
        {
            o<<cells<<" cells, "<<items <<" items, "<<edges<<" edges";
        }        
    };

    span_contents contents() const 
    {
        return span_contents(*this);
    }

    span_contents contents(span_t const& span) const 
    {
        return span_contents(*this,span);
    }
    
        
    /// v.visit(edge_equiv_type const& e)
    template <class V>
    void visit_items(V & v) const
    {
        for (typename span_container_t::const_iterator
                 i=span_container.begin(),e=span_container.end();i!=e;++i) {
//            span_type const& span=*i;
            BOOST_FOREACH(cell_type const& cell, i->cells()) {
                v.visit_cell(cell);
                BOOST_FOREACH(edge_equiv_type const& e, cell.items()) {
                    v.visit(e);
                }
            }
        }        
    }

    /// v.visit(edge_equiv_type const& e)
    template <class V>
    void visit_items(V & v,span_t const& span) const
    {
        BOOST_FOREACH(cell_type const& cell, cells(span)) {
            v.visit_cell(cell);
            BOOST_FOREACH(edge_equiv_type const& e, cell.items()){
                v.visit(e);
            }
        }
    }
    
    
    bool empty_span(span_t const& span) const
    {
        cell_range c=cells(span);
        return c.begin()==c.end();
    }
    
    template <class O>
    void print_cells_for_span(O &o,span_t const& span,indexed_token_factory const& tf,bool andcounts=true,char sep=' ',char countsep=':') const  
    {
        graehl::word_spacer sp(sep);
        BOOST_FOREACH(cell_type const& cell, cells(span)) {
            o << sp << tf.label(cell.root());
            if (andcounts && cell.size() > 1)
                o << countsep << cell.size();
        }
    }
    
private:
    span_index_t   max_span_rt;
    span_container_t span_container;
    
    template <class ET, class I, class S> friend 
    void print( std::ostream&
              , basic_chart<ET,I,S> const& 
              , indexed_token_factory const& );
};

////////////////////////////////////////////////////////////////////////////////
///
/// traits have a couple of advantages:
/// - lowers the number of templates involved in writing a generic chart algo:
///   instead of 
///   \code 
///   void template <E,I,S> func(basic_chart<E,I,S>& chart)
///   {
///      basic_chart<E,I,S>::cell_range = chart.cells(span_t(1,2));
///   }
///   void template <C> func(C& chart)
///   {
///      chart_traits<C>::cell_range = chart.cells(span_t(1,2));
///   }
///   \endcode  
/// - allows for an entirely different chart class to be written for 
///   optimization reasons to still work (of course interface changes still 
///   requires new code)
///
/// folks familiar with templates might say first function has advantage over 
/// second because it lowers the number of overloads that exist, but thats a
/// red herring, because there are a number of things one can do to keep the
/// overload set restricted to valid charts (see boost::enable_if)
///
////////////////////////////////////////////////////////////////////////////////
template <class ChartT>
struct chart_traits
{
    typedef typename ChartT::cell_type     cell_type;
    typedef typename ChartT::cell_iterator cell_iterator;
    typedef typename ChartT::cell_range    cell_range;
    typedef typename ChartT::edge_type     edge_type;
    typedef typename ChartT::edge_iterator edge_iterator;
    typedef typename ChartT::edge_range    edge_range;
    enum { cells_ordered = ChartT::cells_ordered
         , edges_ordered = ChartT::edges_ordered };
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class IP, class SP>
void print( std::ostream& out
          , basic_chart<ET,IP,SP> const& chart
            , indexed_token_factory const& tf );

} // namespace sbmt

#include <sbmt/chart/impl/chart.ipp>

#endif // SBMT_EDGE_CHART_HPP
