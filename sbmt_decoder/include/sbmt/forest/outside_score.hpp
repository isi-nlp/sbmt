#ifndef SBMT_FOREST__OUTSIDE_SCORE_HPP
#define SBMT_FOREST__OUTSIDE_SCORE_HPP

#include <sbmt/forest/dfs_forest.hpp>
#include <sbmt/chart/chart.hpp>
#include <graehl/shared/maybe_update_bound.hpp>
//#include <graehl/shared/accumulate.hpp>
#include <sbmt/forest/derivation.hpp>
#include <sbmt/token/indexed_token.hpp>

/// we intend to use the edge "total inside+h score" field to store the true total inside+outside score.  so the state from traversal doesn't need to last.  score=0 for unconnected things.

namespace sbmt {

/// create a new outside_score object for each chart/forest you want to compute outsides for.  even though the results are stored in the edge_equivalence::representative() edges themselves, the object caches whether the computation has already been done (to save from repeated work e.g. on print outside table, print forest, then prune).  to be clear: only one call to compute per outside_score object has any effect.
struct outside_score 
{

    ///FIXME: choosing a different accum for outside won't change the inside (parsing) accum from max.  so leave it at default for now
//    template <class Accum=graehl::accumulate_max>
    struct clear_score //: private Accum
    {
//        clear_score(Accum const&accum=Accum()) : Accum(accum) {}
//        clear_score(clear_score<Accum> const& o) : Accum(o) {}
        
        template <class equiv>
        void visit(equiv const& eq) 
        {
            score(eq).set(as_zero()); // we want to be able to use efficient (by ref) max increasing, so just store outside, not total as in //            set_heuristic_score(eq,as_zero());
        }
    };

/// dfs_forest_vistior_base::sort=true so cycles come up later (hopefully,
    /// nonnegative cycles)
    template <class Edge>
    struct outside_score_visitor : public dfs_forest_visitor_base_ptr_local_state<outside_score_visitor<Edge> ,dummy_result,edge_equivalence_impl<Edge> *,true> {
        // visitor part:
        typedef Edge edge_type;
        typedef typename edge_type::edge_equiv_type edge_equiv_type;
        typedef typename edge_equiv_type::impl_type edge_equiv_impl;
        typedef edge_equiv_impl *edge_equiv_ptr;
//        typedef edge_equiv_ptr temp_type;
//        outside_score_visitor() { }
     private:
        struct outside_edge
        {
            outside_edge(score_t score, edge_equiv_ptr parent, edge_equiv_ptr child)
                : score(score),parent(parent),child(child) {}
            score_t score;            
            edge_equiv_ptr parent,child;
        };
        typedef std::vector<outside_edge> plan_type;
        plan_type plan;
        edge_equiv_ptr plan_goal() const 
        {
            return plan.back().parent;
        }
        
//        outside_edge cur;

        
        void add_to_plan(score_t edge_inside,edge_equiv_type const& eq,edge_equiv_ptr parent) 
        {
            plan.push_back(outside_edge(
                               edge_inside/inside_score(eq)
                               , parent
                               , eq.get_raw()
                               ));
        }        
        
        
     public:
        bool plan_goal_is(edge_equiv_type const& top) 
        {
            return !plan.empty() && top.get_raw()==plan_goal();
        }
        
        void start_equiv(edge_equiv_type const&eq,dummy_result &result,edge_equiv_ptr &parent) {
            clear_score().visit(eq);
            parent=eq.get_raw();
        }
        
        void visit_edge_ptr(Edge const&e,dummy_result &result,dummy_result *c1,dummy_result *c2,edge_equiv_ptr parent)
        {
            if (c1) {
                score_t s=e.inside_score();
                assert(e.has_first_child());
                add_to_plan(s,e.first_children(),parent);
                if (c2) {
                    assert(e.has_second_child());
                    add_to_plan(s,e.second_children(),parent);
                }             
            }
        }
        

        // outside computation part:
//        template <class Accum=graehl::accumulate_max>
        void compute_outside_scores() //(Accum const& accum) 
        {
            score(plan_goal()).set(as_one());
//            graehl::accumulate_max accum;
            // plan was in reverse order (dfs finishing time = reverse topo sort)
            for (typename plan_type::const_reverse_iterator i=plan.rbegin(),e=plan.rend();
                 i!=e;++i)
//accum
                graehl::maybe_increase_max(score(i->child),score(i->parent)*i->score);
        }
    };

    /// not only computes outside scores for everything connected to top, but sets e.score()==0 for everything not connected
    template <class Chart,class Gram>
    void compute_chart(Chart const& chart,Gram const& gram) const 
    {
        if (computed)
            return;
        chart.visit_items(clear_score()); // so you can iterate over chart later and see what's not connected to top.  but also so accumulation proceeds correctly (could do 2 pass dfs instead)
        compute_equiv(chart.top_equiv(gram));
    }       


    //FIXME: integrate global beam to skip edges that aren't productive?
    template <class Equiv>
    //,class Accum=graehl::accumulate_max>
    void compute_equiv(Equiv const& top) //,Accum const& accum=Accum()) 
    {
        if (computed)
            return;
        typedef typename Equiv::edge_type ET;
        visited_forest<outside_score_visitor<ET> ,ET> topo;
        topo(top);
        assert(topo.plan_goal_is(top));
        topo.compute_outside_scores();
        computed=true;
    }
    
    template <class Grammar, class EdgeFactory>
    struct print_outside_scores_visitor
        : public dfs_forest_visitor_base<dummy_result,dummy_temp,true> {
        std::ostream &o;
        Grammar const& gram;
        EdgeFactory const& ef;
        bool also_inside,also_total;
        print_outside_scores_visitor(std::ostream &o,Grammar const &gram, EdgeFactory const& ef,bool also_inside,bool also_total)
            : o(o),gram(gram),ef(ef),also_inside(also_inside),also_total(also_total) {}
        template <class Equiv>
        void start_equiv(Equiv const&profile,dummy_result result,dummy_temp t) {
            typename Equiv::edge_type const& e=profile.representative();
            o << ef.hash_string(gram,e);
            o << " outside=" << e.score();
            if (also_inside)
                o << " inside=" << e.inside_score();
            if (also_total)
                o << " total=" << e.score() * e.inside_score();
            o << "\n";
        }
    };

    template <class Chart,class Gram, class EdgeFactory>
    void print_chart(std::ostream &o,Chart const& chart,Gram const& gram,EdgeFactory const& ef,bool also_inside)
    {
        print_equiv(chart.top_equiv(gram),o,gram,ef,also_inside,false);
    }
    
    template <class Equiv, class Grammar, class EdgeFactory>
    void print_equiv(std::ostream &o,Equiv const& top,Grammar const& gram, EdgeFactory const& ef, bool also_inside,bool also_total)
    {
        compute_equiv(top);
        print_outside_scores_visitor<Grammar,EdgeFactory> p(o,gram,ef,also_inside,also_total);
        visited_forest<print_outside_scores_visitor<Grammar,EdgeFactory>,typename Equiv::edge_type> v(p);
        v(top);
    }

    bool computed;
    
    outside_score() : computed(false) {}    
    
};


    

}


#endif
