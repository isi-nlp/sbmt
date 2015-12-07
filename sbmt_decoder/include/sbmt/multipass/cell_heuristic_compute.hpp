#ifndef SBMT__MULTIPASS_CELL_HEURISTIC_COMPUTE_HPP
#define SBMT__MULTIPASS_CELL_HEURISTIC_COMPUTE_HPP

#include <sbmt/multipass/cell_heuristic.hpp>
#include <sbmt/forest/dfs_forest.hpp>
#include <sbmt/forest/outside_score.hpp>
# include <sbmt/multipass/logging.hpp>
# include <sbmt/io/logging_macros.hpp>

namespace sbmt {

struct cell_heuristic_compute :
    public dfs_forest_visitor_base<dummy_result,dummy_temp>
{
    cell_heuristic &cells;
    score_t reject_worse_than;
    indexed_token_factory *ptf;
    
    cell_heuristic_compute(cell_heuristic &cells) : cells(cells),reject_worse_than(as_zero()) {}
    
    template <class Chart,class Gram>
    void compute(Chart &chart,Gram const& gram,score_t global_beam=1e-20) 
    {
        outside_score outside;
        compute(chart.top_equiv(gram),gram.dict(),chart.target_span().right(),outside,global_beam);
    }

    // if a cell's best derivs' p < global_beam*global_best, omit the cell
    template <class Edge>
    void compute(edge_equivalence<Edge> const & top,indexed_token_factory &tf,unsigned max_span_rt,outside_score &outside, score_t global_beam=1e-20)
    {
        ptf=&tf;
        score_t best=top.representative().inside_score();
        reject_worse_than=global_beam*best;
        SBMT_INFO_STREAM(limit_cells,"global 1best score: "<<best<< " ; keep-cell-global-beam: "<<global_beam<<" ; rejecting cells with no derivations better than " << reject_worse_than);
        
        outside.compute_equiv(top);
        cells.reset(max_span_rt);
        visit_forest(*this,top);
    }

    template <class Equiv>
    void start_equiv(Equiv const& eq,dummy_result &result,dummy_temp &temp) {
        typename Equiv::edge_type const& e=eq.representative();        
        if (e.outside_score()*e.inside_score() < reject_worse_than) {
            SBMT_PEDANTIC_STREAM(limit_cells,print(e.root(),*ptf)<<e.span()<<" worse than keep-global-cell-beam*1best: " << e.outside_score()*e.inside_score() << " < " << reject_worse_than << " (outside="<<e.outside_score()<<")");
        } else {
            cells.maybe_improve_cell(e);
        }
    }
};

    
}


#endif
