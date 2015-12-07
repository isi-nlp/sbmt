#ifndef   SBMT_SEARCH_EDGE_QUEUE_HPP
#define   SBMT_SEARCH_EDGE_QUEUE_HPP

#include <sbmt/hash/priority_table.hpp>
#include <sbmt/search/concrete_edge_factory.hpp>
#include <sbmt/edge/edge.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  stores edge equivalents in a priority queue.  equivalent edges are merged
///  into a single edge_equivalence, and the priority relation is maintained 
///
///  edges are stored from lowest to highest score.  that may seem wierd, but
///  its what allows the queue to always keep track easily of highest and lowest
///  score inside, which is what any good filter/pruner needs to know.
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT>
class edge_queue
{
public:
    typedef EdgeT edge_type;
    typedef edge_equivalence<edge_type> edge_equiv_type;
    
    edge_queue();
    
    /// expensive if the item being erased has the highest score in the queue
    bool erase(edge_type const& eq);


    edge_equiv_type const& top() const;
    
    void pop();
    
    score_t highest_score() const;
    score_t lowest_score() const;
    
    std::size_t size() const;
    bool empty() const;
private:
    typedef priority_table< edge_equiv_type
                          , representative_edge<edge_type>
                          , representative_score<edge_type>
                          , boost::hash<edge_type>
                          , std::equal_to<edge_type>
                          , std::greater<score_t>
                          > priority_table_t;
    priority_table_t ptable;
    score_t          high_score;
public:
    typedef typename priority_table_t::iterator iterator;
    iterator begin();
    iterator end();
    iterator find(edge_type const& e) { return ptable.find(e); }
    

    iterator insert( edge_equivalence_pool<EdgeT>& epool, edge_type const& e);
             
    iterator insert(edge_equiv_type const& eq);
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/search/impl/edge_queue.ipp>

#endif // SBMT_SEARCH_EDGE_QUEUE_HPP
