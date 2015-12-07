#ifndef SBMT_FOREST__GLOBAL_PRUNE_HPP
#define SBMT_FOREST__GLOBAL_PRUNE_HPP

#include <sbmt/forest/outside_score.hpp>
#include <sbmt/forest/derivation.hpp>

namespace sbmt {

struct global_prune
{
 private:
    struct stats
    {
        std::size_t total;   /// not really a total; since we don't even recurse to pruned items.  just measures fringe alts removed
        std::size_t removed;
    };
    struct prune_visitor : public dfs_forest_visitor_base_ptr<prune_visitor,dummy_result>
    {
        score_t total_worse_than;
        
        prune_visitor(score_t total_worse_than) : total_worse_than(total_worse_than) {}
        
        template <class Edge>
        void visit_edge_ptr(Edge &edge,dummy_result &r,dummy_result *c1,dummy_result *c2)  {
        }

        /// FIXME: make sure that holding non-smart ptrs in dfs_forest doesn't result in early freeing (should be revisited if cycles are ever allowed)
        template <class Equiv>
        void start_equiv(Equiv const&equiv,dummy_result &result,dummy_temp const& temp) 
        {
            score_t outside=score(equiv); // NOTE: requires outside_score computed before
            equiv.prune_inside(total_worse_than/outside);
        }
        
    };
 public:
    template <class Edge>
    static void prune(edge_equivalence<Edge> const & top,score_t global_beam,outside_score &outside)
    {
        outside.compute_equiv(top);
        visited_forest<prune_visitor,Edge> v(inside_score(top)*global_beam);
        v.compute(top);
    }
};

    
}

#endif
