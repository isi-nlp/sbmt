#ifndef SBMT_FOREST__COUNT_TREES_HPP
#define SBMT_FOREST__COUNT_TREES_HPP

#include <sbmt/forest/dfs_forest.hpp>

namespace sbmt {

struct forest_counts 
{
    typedef double tree_count_type;
    tree_count_type trees;
    std::size_t items,edges;
    void reset() 
    {
        trees=0;
        items=0;
        edges=0;
    }
    forest_counts() { reset(); }
    template <class O>
    void print(O &o) const 
    {
        o << trees << " trees in " << items << " items connected by " << edges << " edges";
    }
};

    

/// Ignoring cycles, how many derivations are in the forest?
/// dfs_forest_vistior_base::sort=true so cycles come up later (hopefully,
/// nonnegative cycles)
struct count_trees : public dfs_forest_visitor_base<forest_counts::tree_count_type,dummy_temp> {
    forest_counts n;
    
    template <class Equiv>
    void start_equiv(Equiv const&profile,result_type &result,dummy_temp const& d) {
        ++n.items;
        result=0;
        SBMT_PEDANTIC_STREAM(forest_domain, "count_trees: cleared count for item "<<profile<<".  # trees: "<<result);
    }
    
    template <class Edge>
    void visit_edge(Edge const&edge,result_type &result,dummy_temp const& d) // initial
    {
        result+=1;++n.edges;
        SBMT_PEDANTIC_STREAM(forest_domain, "count_trees: 0-child edge, 1 tree.  # trees: "<<result);
    } 
    
    template <class Edge>
    void visit_edge(Edge const&edge,result_type &result,result_type &child,dummy_temp const& d)
    {
        result+=child;++n.edges;
        SBMT_PEDANTIC_STREAM(forest_domain, "count_trees: 1-child edge, "<<child<<" trees.  # trees: "<<result);
    } // unary
    
    template <class Edge>
    void visit_edge(Edge const&edge,result_type &result,result_type &left,result_type &right,dummy_temp const& d)
    {
        result+=left*right;++n.edges;
        SBMT_PEDANTIC_STREAM(forest_domain, "count_trees: 2-child edge, "<<(left*right)<<" trees (left="<<left<<" right="<<right<<").  # trees: "<<result);
    } // binary

    template <class Equiv>
    void finish_equiv(Equiv const&profile,result_type &result,dummy_temp const& d) {
//        INF9("count_trees","Profile #" << n_items << " at " << &profile << profile.front()->getRootSpan()  << "(size=" << profile.size() << ") has " << result << " trees.");
        SBMT_PEDANTIC_STREAM(forest_domain, "count_trees: final count for item "<<profile<<".  # trees: "<<result);
    }
    static char const* prelude() 
    {
        return "parse forest has (loops excluded) ";
    }
    
    template <class Equiv,class O>
    static void print_equiv(Equiv const& top_equiv,O &o,const char *pre="") 
    {
        visited_forest<count_trees,typename Equiv::edge_type> v;
        v.n.trees=v.compute(top_equiv);
        o << pre << prelude();
        v.n.print(o);
        o << '.' << std::endl;
    }
    template <class O>
    static void print_dummy(O &o,const char* pre="") 
    {
        o << pre << prelude();
        forest_counts().print(o);
        o << '.' << std::endl;
    }
    
};

}


#endif
