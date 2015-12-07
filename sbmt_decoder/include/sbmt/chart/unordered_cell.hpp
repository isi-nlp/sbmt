#ifndef SBMT_CHART_UNORDERED_CELL_HPP
#define SBMT_CHART_UNORDERED_CELL_HPP

#include <sbmt/span.hpp>
#include <sbmt/edge/edge.hpp>
#include <sbmt/range.hpp>
#include <sbmt/search/concrete_edge_factory.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <graehl/shared/podcpy.hpp>

namespace sbmt {

namespace detail {

template <class EdgeT>
class unordered_cell {
public:
    typedef edge_equivalence<EdgeT> edge_equiv_type;
    typedef EdgeT edge_type;
private:
    struct hash_key_by_edge {
        typedef edge_type result_type;
        edge_type const& operator()(edge_equiv_type eq) const
        {
            return eq.representative();
        }
    };
    typedef boost::multi_index_container <
                edge_equiv_type
              , boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<
                        hash_key_by_edge
                    >
                >
            > edge_container_t;
public:
    typedef typename edge_container_t::iterator iterator;    
    typedef itr_pair_range<iterator>            range;

    range         edges() const;
    range items() const 
    { return edges(); }
    
    std::pair<iterator,bool> find(edge_type const& e) const;
    indexed_token root() const;
    score_t       best_score() const;
    span_t        span() const;
    
    void insert_edge(edge_equivalence_pool<EdgeT>& epool, edge_type const& e);
    
    void insert_edge(edge_equiv_type const& eq);
    unordered_cell( edge_equivalence_pool<EdgeT>& epool
                  , edge_type const& e);
    
    unordered_cell(edge_equiv_type const& eq);

    std::size_t size() const 
    { return edge_container.size(); }
    
private:
    struct edge_inserter {
        edge_inserter( edge_type const* e )
        : e(e) {}
        edge_type const*  e;
        void operator()(edge_equiv_type& eq) { eq.insert_edge(*e); }
    };
    
    struct edge_merger {
        edge_merger(edge_equiv_type const* eq )
        : eq(eq) {}
        edge_equiv_type const*  eq;
        void operator()(edge_equiv_type& c) { c.merge(*eq); }
    };
    
    edge_merger merging_edges(edge_equiv_type const& eq)
    { return edge_merger(&eq); }

    edge_inserter
    inserting_edge( edge_type const& e )
    {
        return edge_inserter(&e);
    }
    edge_container_t edge_container;
    score_t          best_scr;
};

} // namespace sbmt::detail

////////////////////////////////////////////////////////////////////////////////
/// 
/// passing this structure into the charts template parameter allows the chart
/// to use unordered_cell as its cell_type.  edges in a cell do not appear in
/// any order, but it uses less memory than ordered_chart_edges_policy
///
////////////////////////////////////////////////////////////////////////////////
struct unordered_chart_edges_policy
{
    enum { ordered = false };
    template<class EdgeT> struct cell_type {
        typedef detail::unordered_cell<EdgeT> type;
    };
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/chart/impl/unordered_cell.ipp>

#endif // SBMT_UNORDERED_CELL_HPP
