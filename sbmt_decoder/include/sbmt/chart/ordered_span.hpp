#ifndef SBMT_CHART_ORDERED_SPAN_HPP
#define SBMT_CHART_ORDERED_SPAN_HPP

#include <sbmt/span.hpp>
#include <sbmt/edge/edge.hpp>
#include <sbmt/range.hpp>
#include <sbmt/search/concrete_edge_factory.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>


namespace sbmt {

namespace detail {

/// ordered spans return their cells from highest cell.best_score to
/// lowest cell.best_score
template <class CellT>
class ordered_span {
public:
    typedef typename CellT::edge_type edge_type;
    typedef typename CellT::iterator  edge_iterator;
    typedef typename CellT::range     edge_range;
    typedef CellT cell_type;
private:
    struct order_by_score {
        bool operator() (cell_type const& eq1, cell_type const& eq2) const
        {
            return eq1.best_score() > eq2.best_score();
        }
    };
    struct hash_key_by_root {
        typedef indexed_token result_type;
        indexed_token operator()(cell_type const& eq) const
        {
            return eq.root();
        }
    };
    
    typedef boost::multi_index_container <
                cell_type
              , boost::multi_index::indexed_by<
                    boost::multi_index::ordered_non_unique<
                        boost::multi_index::identity<cell_type>
                      , order_by_score
                    >
                  , boost::multi_index::hashed_unique<
                        hash_key_by_root
                    >
                >
            > cell_container_t;
public:
    typedef typename cell_container_t::iterator iterator;    
    typedef itr_pair_range<iterator> range;
    
    range         cells() const;
    edge_range    edges(indexed_token t) const;
    std::pair<edge_iterator,bool> find(edge_type const& e) const;

    //span_t        span() const;
    
    /// \note: any edge inserted must have the same span as every other
    /// edge in this container
    void insert_edge( edge_equivalence_pool<edge_type>& epool
                    , edge_type const& e );
    
    /// we do not want to allow empty spans:  that would force checking 
    /// for emptiness in chart algorithms.
    /// ...six months later: what the hell am i talking about?!
    ordered_span( edge_equivalence_pool<edge_type>& epool
                , edge_type const& e );
                
    ordered_span(){}
    
    void clear() { cell_container.clear(); }
private:

    /// edge_inserter<> and the associated inserting_edge() is just a detail 
    /// related to the proper proceedure for modifying
    /// elements in a multi_index_container.
    /// its use can be found in the method insert_edge()
    struct edge_inserter {
        edge_inserter( edge_equivalence_pool<edge_type>* epool
                     , edge_type const* e )
        : epool(epool), e(e) {}
        edge_equivalence_pool<edge_type>* epool;
        edge_type const*  e;
        void operator()(cell_type& eq) { eq.insert_edge(*epool,*e); }
    };
    
    /// see edge_inserter<>
    edge_inserter
    inserting_edge( edge_equivalence_pool<edge_type>& epool
                  , edge_type const& e )
    {
        return edge_inserter(&epool,&e);
    }
    
    cell_container_t cell_container;
};

} // namespaace sbmt::detail

////////////////////////////////////////////////////////////////////////////////
/// 
/// passing this structure into the chart template parameter allows the chart
/// to use ordered_span as its span_type, which allows cells to be sorted by
/// the best_score() for an cell
///
////////////////////////////////////////////////////////////////////////////////
struct ordered_chart_cells_policy
{
    enum { ordered = true };
    template<class ItemT> struct span_type {
        typedef detail::ordered_span<ItemT> type;
    };
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/chart/impl/ordered_span.ipp>

#endif // SBMT_ORDERED_SPAN_HPP
