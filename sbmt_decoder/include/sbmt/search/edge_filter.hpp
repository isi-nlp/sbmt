#ifndef   SBMT_SEARCH_EDGE_FILTER_HPP
#define   SBMT_SEARCH_EDGE_FILTER_HPP

#include <stdexcept>

#include <sbmt/edge/edge.hpp>
#include <sbmt/search/edge_filter_predicate.hpp>
#include <sbmt/search/retry_series.hpp>

#include <utility>
#include <boost/scoped_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <sbmt/hash/hash_map.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

class edge_filter_finalized : public std::logic_error
{
public:
    edge_filter_finalized()
    : std::logic_error("attempted to insert into filter after finalized") {}
    virtual ~edge_filter_finalized() throw() {}
};

void throw_edge_filter_finalized();


////////////////////////////////////////////////////////////////////////////////
///
/// note: edge_filter construction is tricky.  if you copy one, you create a new
/// underlying queue (empty) with the same parameters as original.  so there is
/// no factory interface.  pass by value = make a new (empty) one.
///
/// predicate_edge_filter uses an edge_queue<> and a predicate function to
/// prioritize edges that meet the predicate functions tests.  the predicate
/// function has access to the edge queue, making it possible to implement
/// pruning strategies like: "keep the top 10 edges", or "throw out edges below
/// a given threshold"
///
/// using a different BoolT like boost::tribool allows the implementation of
/// "fuzzy filters" suitable for use in the cube-heap span filter.  in this case
///  - true = edge is saved.
///  - false = edge is outside of the fuzz range.
///  - maybe = edges is not saved, but within fuzz range.
///
/// valid PredicateF functions need to have
/// bool  f::keep_edge(edge_queue const& pqueue, edge_type const& candidate)
/// void  f::finalize(edge_queue& pqueue)
///
////////////////////////////////////////////////////////////////////////////////



template <class EdgeT, class Predicate>
class predicate_edge_filter
{
public:
    typedef typename Predicate::pred_type pred_type;
    typedef EdgeT                         edge_type;
    typedef edge_equivalence<edge_type>   edge_equiv_type;
    
    predicate_edge_filter(Predicate const& f);
        
    bool adjust_for_retry(unsigned int i) 
    { 
        assert(!is_finalized());
        assert(pqueue.empty());
        return filt.adjust_for_retry(i); 
    }

    void print(std::ostream& o) const 
    { o << "predicate_edge_filter="; filt.print(o); }

    pred_type insert(edge_equivalence_pool<edge_type>& ep, edge_type const& e);

    pred_type insert(edge_equiv_type const& eq);
    
    edge_equiv_type const& top() const;
    
    /// after pop is called, you may not insert any more edges.
    void pop();
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// sometimes the filter predicate may want to do a final prune.
    /// when finalize is called, that prune is carried out.
    /// if you dont call finalized, it is called on the first call to pop.
    ///
    /// after finalize is called, you may not call insert
    ///
    ////////////////////////////////////////////////////////////////////////////
    void finalize();
    bool is_finalized() const;
    
    bool empty() const;
    std::size_t size() const;

    edge_queue<edge_type>& get_queue() 
    {
        return pqueue;
    }
    
    Predicate& get_filter()
    {
        return filt;
    }
    
private:
    typedef std::vector<edge_equiv_type> edge_equiv_vec;
    // transact is used to ensure we always either pop off all edges of the same
    // score or no edges of the same score, when maintaining the priority queue.
    edge_equiv_vec transact; 
    void pop_top();
    Predicate             filt;
    edge_queue<edge_type> pqueue;
    bool                  finalized;
};

template<class E, class P>
predicate_edge_filter<E,P> make_predicate_edge_filter(P const& p)
{
    return predicate_edge_filter<E,P>(p);
}

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class SpanPredicate, class CellPredicate>
class cell_edge_filter
{
public:
    typedef typename SpanPredicate::pred_type pred_type;
    typedef EdgeT                             edge_type;
    typedef edge_equivalence<edge_type>       edge_equiv_type;
    

    cell_edge_filter( SpanPredicate const& span_predicate
                    , CellPredicate const& cell_predicate )
      : span_pred(span_predicate)
      , cell_pred(cell_predicate)
      , finalized(false) {}
        
    bool adjust_for_retry(unsigned int i)
    {
        assert(!is_finalized());
        assert(span_queue.empty());
        assert(cell_map.empty());
               
        return any_change( span_pred.adjust_for_retry(i)
                         , cell_pred.adjust_for_retry(i) );
    }
    
    void print(std::ostream& o) const
    { 
        o << "cell_edge_filter{cell-filt=";
        cell_pred.print(o);
        o << "; span-filt=";
        span_pred.print(o);
        o << "}";
    }
    

    pred_type insert(edge_equivalence_pool<edge_type>& ep, edge_type const& e);
        
    pred_type insert(edge_equiv_type const& eq);
    
    edge_equiv_type const& top() const;

    void pop();
    
    void finalize();
    bool is_finalized() const;
    
    bool empty() const;
    std::size_t size() const;

    edge_queue<edge_type> &get_queue() 
    {
        return span_queue;
    }
    
private:
    struct queue_and_pred {
        edge_queue<edge_type> queue;
        CellPredicate pred;
        queue_and_pred(CellPredicate const& p) : pred(p) {}
    };

    //FIXME: I believe we rely on node-based linked list buckets (no copy construction after emplaced in map)?  default constructor should copy edge_queue though, so maybe not
    typedef stlext::hash_map< indexed_token
                            , queue_and_pred
                            , boost::hash<indexed_token>
                            > cell_map_t;
                               
    void pop_span();
    void pop_cell(queue_and_pred& pq, edge_type const& e);
    typename cell_map_t::iterator get_create(indexed_token const& tok);
    
    cell_map_t cell_map;
    edge_queue<edge_type> span_queue;
    SpanPredicate span_pred;
    CellPredicate cell_pred;
    bool finalized;
};

template<class E, class PS, class PC>
cell_edge_filter<E,PS,PC> make_cell_edge_filter(PS const& ps, PC const& pc)
{
    return cell_edge_filter<E,PS,PC>(ps,pc);
}

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class BoolT = bool>
class edge_filter
{
public:
    typedef BoolT                       pred_type;
    typedef EdgeT                       edge_type;
    typedef edge_equivalence<edge_type> edge_equiv_type;
    typedef edge_queue<edge_type> queue_type;
private:
    struct impl {
        virtual bool adjust_for_retry(unsigned int i) = 0;
        virtual void print(std::ostream& o) const = 0;
        virtual pred_type insert( edge_equivalence_pool<edge_type>& epool
                                , edge_type const& e ) = 0;
        virtual pred_type insert(edge_equiv_type const& eq) = 0;
        virtual edge_equiv_type const& top() const = 0;
        virtual void pop() = 0;
        virtual void finalize() = 0;     
        virtual bool is_finalized() const = 0;
        virtual bool empty() const = 0;
        virtual std::size_t size() const = 0;
        virtual impl* clone() const = 0;
        virtual queue_type & get_queue() = 0;
        virtual ~impl(){}
    };
    
    template <class ImplT>
    struct impl_derived : public impl {
        impl_derived(ImplT const& mimpl) : mimpl(mimpl) {}
        
        virtual bool adjust_for_retry(unsigned int i)
        { return mimpl.adjust_for_retry(i); }
        
        virtual void print(std::ostream& o) const
        { mimpl.print(o); }
        
        virtual pred_type insert( edge_equivalence_pool<edge_type>& epool
                                , edge_type const& e )
        { return mimpl.insert(epool,e); }
        
        virtual pred_type insert(edge_equiv_type const& eq)
        { return mimpl.insert(eq); }
        
        virtual edge_equiv_type const& top() const
        { return get_queue().top();
//                mimpl.top();
        }
        
        virtual void pop() 
        { return mimpl.get_queue().pop();
            //mimpl.pop();
        }
        
        virtual void finalize()
        { mimpl.finalize(); }
        
        virtual bool is_finalized() const
        { return mimpl.is_finalized(); }
        
        virtual bool empty() const
        { return get_queue().empty();
            //mimpl.empty();
        }
        
        virtual std::size_t size() const
        { return get_queue().size();
            //mimpl.size();
        }
        
        virtual queue_type & get_queue() 
        {
            return mimpl.get_queue();
        }

        queue_type const& get_queue() const 
        {
            return const_cast<ImplT&>(mimpl).get_queue();
        }
        
        virtual impl* clone() const
        { return new impl_derived<ImplT>(mimpl); }

        virtual ~impl_derived(){}
        
        ImplT mimpl;
    };
    
    boost::scoped_ptr<impl> filt;

public:    
    template <class EdgeFilterImpl>
    edge_filter(EdgeFilterImpl f)
    : filt(new impl_derived<EdgeFilterImpl>(f)) 
    {
        BOOST_STATIC_ASSERT((
	     boost::is_same< typename EdgeFilterImpl::pred_type
	                   , pred_type >::value
        ));  
    }

    template <class EdgeFilterImpl>
    edge_filter(EdgeFilterImpl f,bool dummy)
        : filt(new impl_derived<EdgeFilterImpl>(f))  // to allow wrapping of edge_filter as impl
    {
    }
    
    edge_filter(edge_filter const& other)
    { filt.reset(other.filt->clone()); }
    
    edge_filter& operator=(edge_filter const& other)
    { filt.reset(other.filt->clone()); return *this; }
    
    template <class EdgeFilterImpl>
    edge_filter& operator=(EdgeFilterImpl const& f)
    { filt.reset(new impl_derived<EdgeFilterImpl>(f)); return *this; }
        
    bool adjust_for_retry(unsigned int i) 
    { return filt->adjust_for_retry(i); }
    
    void print(std::ostream& o) const 
    { filt->print(o); }
    
    pred_type insert( edge_equivalence_pool<edge_type>& epool
                    , edge_type const& e )
    { return filt->insert(epool,e); }
        
    pred_type insert(edge_equiv_type const& eq)
    { return filt->insert(eq); }
    
    edge_equiv_type const& top() const
    { return filt->top(); }
    
    void pop()
    { return filt->pop(); }
    
    void finalize()
    { return filt->finalize(); }
    
    bool is_finalized() const
    { return filt->is_finalized(); }
    
    bool empty() const
    { return filt->empty(); }
    
    std::size_t size() const
    { return filt->size(); }

    // FIXME: hey languageweaver guys: edge_filter::get_queue was virtual ... 
    // but shouldn't have been.  if you were inheriting and overriding, 
    // just use the type-erasure interface already provided.
    queue_type & get_queue() 
    {
        return filt->get_queue();
    }
    
    queue_type const& get_queue() const 
    {
        return filt->get_queue();      
//        return const_cast<edge_filter&>(*this).get_queue();
    }
    
};

template <class EdgeT, class BoolT>
std::ostream& operator << ( std::ostream& out
                          , edge_filter<EdgeT,BoolT> const& e)
{
    e.print(out);
    return out;
}

} // namespace sbmt

#include <sbmt/search/impl/edge_filter.ipp>

#endif // SBMT_SEARCH_EDGE_FILTER_HPP
