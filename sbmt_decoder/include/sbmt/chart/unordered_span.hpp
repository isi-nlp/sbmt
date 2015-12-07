#ifndef SBMT_CHART_UNORDERED_SPAN_HPP
#define SBMT_CHART_UNORDERED_SPAN_HPP

#include <sbmt/span.hpp>
#include <sbmt/edge/edge.hpp>
#include <sbmt/range.hpp>
#include <sbmt/search/concrete_edge_factory.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/shared_ptr.hpp>


namespace sbmt {

namespace detail {

template <class CellT>
class unordered_span {
public:
    typedef typename CellT::edge_type       edge_type;
    typedef typename CellT::edge_equiv_type edge_equiv_type;
    typedef typename CellT::range           edge_range;
    typedef typename CellT::iterator        edge_iterator;
    typedef CellT                           cell_type;
private:
    typedef boost::shared_ptr<cell_type> cell_type_ptr;
    struct hash_key_by_root {
        typedef indexed_token result_type;
        indexed_token operator()(cell_type_ptr const& eq) const
        {
            return eq->root();
        }
    };
    
    typedef boost::multi_index_container <
                cell_type_ptr
              , boost::multi_index::indexed_by<
                  boost::multi_index::hashed_unique<
                        hash_key_by_root
                    >
                >
            > cell_container_t;
public:
    typedef boost::indirect_iterator<typename cell_container_t::iterator>
            iterator; 
            
    typedef itr_pair_range<iterator> range;
    
    range         cells() const;
    std::pair<iterator,bool> cell(indexed_token t) const;
    edge_range    edges(indexed_token t) const;
    std::pair<edge_iterator,bool> find(edge_type const& e) const;
    //span_t        span() const { return spn; }
    
     void 
    insert_edge( edge_equivalence_pool<edge_type>& epool
               , edge_type const& e);
    void insert_edge(edge_equiv_type const& eq);
    
    /// we do not want to allow empty cells:  that would force checking
    /// ...six months later: what the hell am i talking about?
    unordered_span( edge_equivalence_pool<edge_type>& epool
                  , edge_type const& e);
    
    unordered_span(edge_equiv_type const& eq);
    
    unordered_span(){ }
    
    void clear() { cell_container.clear(); }
private:

    struct edge_inserter {
        edge_inserter( edge_equivalence_pool<edge_type>* epool
                     , edge_type const* e )
        : epool(epool), e(e) {}
        edge_equivalence_pool<edge_type>* epool;
        edge_type const*  e;
        void operator()(cell_type_ptr& eq) { eq->insert_edge(*epool,*e); }
    };
    
    struct edge_merger {
        edge_merger(edge_equiv_type const* eq )
        : eq(eq) {}
        edge_equiv_type const*  eq;
        void operator()(cell_type_ptr& c) { c->insert_edge(*eq); }
    };
    
    edge_merger merging_edges(edge_equiv_type const& eq)
    { return edge_merger(&eq); }


    edge_inserter
    inserting_edge( edge_equivalence_pool<edge_type>& epool
                  , edge_type const& e )
    {
        return edge_inserter(&epool,&e);
    }
    
    cell_container_t cell_container;
};

} // namespaace sbmt::detail

struct unordered_chart_cells_policy
{
    enum { ordered = false };
    template<class CellT> struct span_type {
        typedef detail::unordered_span<CellT> type;
    };
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/chart/impl/unordered_span.ipp>

#endif // SBMT_UNORDERED_SPAN_HPP
 
