#ifndef   SBMT_SEARCH_EDGE_FILTER_PREDICATE_HPP
#define   SBMT_SEARCH_EDGE_FILTER_PREDICATE_HPP

#include <sbmt/edge/edge.hpp>
#include <sbmt/search/edge_queue.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
/// boiler-plate type-erasure code to contain filter predicates.  ie pruners.
/// a filter predicate decides whether or not to let an edge get stored in a
/// priority queue.
///
/// used by edge_filter
///
/// using a different BoolT like boost::tribool allows the implementation of
/// "fuzzy filters" suitable for use in the cube-heap span filter.  in this case
///  - true = edge is saved.
///  - false = edge is outside of the fuzz range.
///  - maybe = edges is not saved, but within fuzz range
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT, class BoolT = bool>
class edge_filter_predicate
{
public:
    typedef BoolT                       pred_type;
    typedef EdgeT                       edge_type;
    typedef edge_equivalence<edge_type> edge_equiv_type;
    
    template <class PredF>
    edge_filter_predicate(PredF const& f)
    : pimpl(new impl_derived<PredF>(f)) 
    {
        BOOST_STATIC_ASSERT((
	     boost::is_same< pred_type
	                   , typename PredF::pred_type >::value
	));
    }
    
    edge_filter_predicate(edge_filter_predicate const& other)
    : pimpl(other.pimpl->clone()) {}
    
    edge_filter_predicate& operator=(edge_filter_predicate const& other)
    {
        if (this != &other) {
            pimpl.reset(other.pimpl->clone());
        }
        return *this;
    }

    void print(std::ostream &o) const
    {
        pimpl->print(o);
    }
    
    bool adjust_for_retry(unsigned i) 
    { return pimpl->adjust_for_retry(i); }
    
    pred_type keep_edge(edge_queue<edge_type>& pqueue, edge_type const& e)
    { return pimpl->keep_edge(pqueue, e); }
    
    bool pop_top(edge_queue<edge_type> const& pqueue)
    { return pimpl->pop_top(pqueue); }

private:
    class impl
    {
    public:
        virtual void print(std::ostream &o) const = 0;

        virtual bool adjust_for_retry(unsigned i) = 0;
        
        virtual pred_type keep_edge( edge_queue<edge_type> const& pqueue 
                                   , edge_type const& e ) = 0;

        virtual bool pop_top(edge_queue<edge_type> const& pqueue) = 0;
        
        virtual impl* clone() const = 0;
        
        virtual ~impl(){}
    };
    
    template <class ImplT>
    class impl_derived : public impl
    {
    public:
        impl_derived(ImplT const& m_impl)
        : m_impl(m_impl) {}

        virtual void print(std::ostream &o) const 
        { return m_impl.print(o); }
        virtual bool adjust_for_retry(unsigned i)
        { return m_impl.adjust_for_retry(i); }
        
        virtual pred_type keep_edge( edge_queue<edge_type> const& pqueue 
                                   , edge_type const& e )
        { return m_impl.keep_edge(pqueue,e); }

        virtual bool pop_top(edge_queue<edge_type> const& pqueue)
        { return m_impl.pop_top(pqueue); }
        
        virtual impl* clone() const { return new impl_derived(*this); }

        virtual ~impl_derived(){}
    private:
        ImplT m_impl;
    };
    
    boost::shared_ptr<impl> pimpl;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT_SEARCH_EDGE_FILTER_PREDICATE_HPP
