#include <boost/logic/tribool.hpp>
#include <sbmt/search/logging.hpp>
#ifdef min
#undef min
#endif

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class ET, class BT>
predicate_edge_filter<ET, BT>::predicate_edge_filter(BT const& f)
: filt(f)
, finalized(false) {}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class BT>
typename predicate_edge_filter<ET, BT>::pred_type
predicate_edge_filter<ET, BT>::insert( edge_equivalence_pool<edge_type>& epool
                                     , edge_type const& e )
{
    if (finalized) throw edge_filter_finalized();
    pred_type retval = filt.keep_edge(pqueue, e);
    if (retval) {
        if (e.score() == pqueue.lowest_score()) {
            pqueue.insert(epool, e);
            return retval;
        } else {
            pqueue.insert(epool, e);
        }
    }
    pop_top();
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class BT>
typename predicate_edge_filter<ET, BT>::pred_type
predicate_edge_filter<ET, BT>::insert(edge_equiv_type const& eq)
{
    if (finalized) throw edge_filter_finalized();
    pred_type retval = filt.keep_edge(pqueue, eq.representative());
    if (retval) pqueue.insert(eq);
    pop_top();
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class BT>
void predicate_edge_filter<ET, BT>::pop_top()
{
    transact.clear();
    score_t min(0.0);
    while ((not pqueue.empty()) and filt.pop_top(pqueue)) {
        assert(pqueue.top().representative().score() >= min);
        edge_equiv_type eq = pqueue.top();
        if (transact.empty() or 
            transact.back().representative().score() == eq.representative().score()
           ) transact.push_back(eq);
        else {
            transact.clear();
            transact.push_back(eq);
        }
        min = eq.representative().score();
        pqueue.pop();
    }
    
    if ((not transact.empty()) and 
        (not pqueue.empty()) and
        transact.back().representative().score() == pqueue.top().representative().score()
       ) {
        // weve popped off a portion of items of equal score, but left others on...
        // so lets keep them all
        typename edge_equiv_vec::iterator itr = transact.begin(), 
                                          end = transact.end();
        for (; itr != end; ++itr) pqueue.insert(*itr);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class BT>
typename predicate_edge_filter<ET, BT>::edge_equiv_type const& 
predicate_edge_filter<ET, BT>::top() const
{
    assert(this->is_finalized());    
    return pqueue.top();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class BT>
void predicate_edge_filter<ET, BT>::pop()
{
    assert(this->is_finalized());
    pqueue.pop();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class BT>
void predicate_edge_filter<ET, BT>::finalize()
{
    if (!finalized) {
        finalized = true;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class BT>
bool predicate_edge_filter<ET, BT>::is_finalized() const { return finalized; }

////////////////////////////////////////////////////////////////////////////////

template <class ET, class BT>
bool predicate_edge_filter<ET, BT>::empty() const
{
    return pqueue.empty();
}

template <class ET, class BT>
std::size_t predicate_edge_filter<ET, BT>::size() const
{
    return pqueue.size();
}

////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
typename cell_edge_filter<ET, SP, CP>::pred_type 
cell_edge_filter<ET, SP, CP>::insert( edge_equivalence_pool<edge_type>& epool
                                    , edge_type const& e )
{
    using namespace boost::logic;
    if (finalized) throw_edge_filter_finalized();
    typename cell_map_t::iterator pos = this->get_create(e.root());
    pred_type cell_keep = pos->second.pred.keep_edge(pos->second.queue, e);
    pred_type span_keep; //FIXME: uninit
    if (cell_keep) {
        span_queue.erase(e);
        pos->second.queue.insert(epool,e);
        pop_cell(pos->second,e);
        
        typename edge_queue<edge_type>::iterator pe = 
                                                pos->second.queue.find(e);
        span_keep = span_pred.keep_edge(span_queue, e);
        if (pe != pos->second.queue.end()) {
            
            if (span_keep) {
                span_queue.insert(*pe);
                pop_span();
            }
        } else {
            std::cerr << "ERROR: cell-filter: span-predicate removed an edge "
                      << "immediately after deciding to keep it!!!!!";
        }
    } else {
        span_keep = span_pred.keep_edge(span_queue, e);
    }
    
    SBMT_PEDANTIC_MSG( filter_domain
                     , "cell_edge_filter::insert skeep=%s ckeep=%s"
                     , span_keep % cell_keep );
        
    return span_keep; // or should it be span_keep || cell_keep ?  the fact that
                      // cubes are processed as a whole span, and so is 
                      // exhaustive, suggests it should be span_keep to me.
        
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
typename cell_edge_filter<ET, SP, CP>::pred_type
cell_edge_filter<ET, SP, CP>::insert(edge_equiv_type const& eq)
{
    if (finalized) throw_edge_filter_finalized();
    typename cell_map_t::iterator pos = 
                                  this->get_create(eq.representative().root());
    pred_type cell_keep = pos->second.pred.keep_edge( pos->second.queue
                                                    , eq.representative() );
    pred_type span_keep = false; //FIXME: uninit
    if (cell_keep) {
        span_queue.erase(eq.representative());
        pos->second.queue.insert(eq);
        pop_cell(pos->second,eq.representative());
        
        span_keep = span_pred.keep_edge(span_queue, eq.representative());
        typename edge_queue<edge_type>::iterator pe = 
                            pos->second.queue.find(eq.representative());
        if (pe != pos->second.queue.end()) {
            if (span_keep) {
                span_queue.insert(*pe);
                pop_span();
            }
        } else {
            std::cerr << "ERROR: cell-filter: span-predicate removed an edge "
                      << "immediately after deciding to keep it!!!!!";
        }
    } else {
        span_keep = span_pred.keep_edge(span_queue, eq.representative());
    }
    
    SBMT_PEDANTIC_MSG( filter_domain
                     , "cell_edge_filter::insert skeep=%s ckeep=%s"
                     , span_keep % cell_keep );
        
    return span_keep;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
typename cell_edge_filter<ET, SP, CP>::cell_map_t::iterator
cell_edge_filter<ET, SP, CP>::get_create(indexed_token const& tok)
{
    typename cell_map_t::iterator pos = cell_map.find(tok);
    if (pos == cell_map.end()) {
        pos = insert_into_map(cell_map, tok, queue_and_pred(cell_pred)).first;
    }
    
    return pos;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
void
cell_edge_filter<ET, SP, CP>::pop_cell(queue_and_pred& qp, edge_type const& e)
{
    while (!qp.queue.empty() and qp.pred.pop_top(qp.queue)) {
        //assert(qp.queue.top().representative() != e);
        span_queue.erase(qp.queue.top().representative());
        qp.queue.pop();
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
void
cell_edge_filter<ET, SP, CP>::pop_span()
{
    while (!span_queue.empty() and span_pred.pop_top(span_queue)) {
        typename cell_map_t::iterator pos =
            get_create(span_queue.top().representative().root());
        pos->second.queue.erase(span_queue.top().representative());
        span_queue.pop();
    }   
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
typename cell_edge_filter<ET, SP, CP>::edge_equiv_type const& 
cell_edge_filter<ET, SP, CP>::top() const
{
    assert(this->is_finalized());
    return span_queue.top();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
void cell_edge_filter<ET, SP, CP>::pop()
{
    assert(this->is_finalized());
    span_queue.pop();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
void cell_edge_filter<ET, SP, CP>::finalize()
{
    if (!this->is_finalized()) {
        typename cell_map_t::iterator itr = cell_map.begin(),
                                      end = cell_map.end();
        for (; itr != end; ++itr) {
            typename edge_queue<edge_type>::iterator 
                                        qitr = itr->second.queue.begin(),
                                        qend = itr->second.queue.end();
                                        
            for (;qitr != qend; ++qitr) {
                if (span_queue.find(qitr->representative()) == span_queue.end()
                    and span_pred.keep_edge(span_queue, qitr->representative())) {
                    span_queue.insert(*qitr);
                    while (!span_queue.empty() and 
                           span_pred.pop_top(span_queue)) {
                        span_queue.pop();
                    }
                }
            }
        }   
    }
    finalized = true;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
bool cell_edge_filter<ET, SP, CP>::is_finalized() const 
{ return finalized; }

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
bool cell_edge_filter<ET, SP, CP>::empty() const
{   
    return span_queue.empty();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class SP, class CP>
std::size_t cell_edge_filter<ET, SP, CP>::size() const
{
    return span_queue.size();
}

} //namespace sbmt
