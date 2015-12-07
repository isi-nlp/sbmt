#ifndef SBMT__DFS_FOREST_HPP
#define SBMT__DFS_FOREST_HPP

//#define DFS_FOREST_GET_RAW
// the above was a hack needed at one point due to compiler type confusion (possibly my confusion?)

#include <sbmt/forest/logging.hpp>
#include <sbmt/hash/hash_map.hpp>
#include <boost/functional/hash.hpp>
#include <sbmt/forest/derivation.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/foreach.hpp>

namespace boost {

/* todo make available in older boost (older than 1.49? 
template <class T>
std::size_t hash_value(boost::shared_ptr<T> const& ptr)
{
  return hash_value(ptr.get());
}

template <class T>
std::size_t hash_value(boost::intrusive_ptr<T> const& ptr)
{
  return hash_value(ptr.get());
}
*/
}

namespace sbmt {

/**

  \defgroup Depth Depth-First Forest Traversal

  A parse forest has edge equivs as the OR nodes, each containing a list of AND
  nodes, i.e. edges (nullary,unary, or binary).

   dfs_forest performs a (lazy, memoized) bottom-up depth first
   computation of a per-equivalence result via a Visitor
   (e.g. count_trees_visitor, used_rules_visitor).  Visitor will be called by
   reference, so may have state affected by visits.

   POSSIBLE TODO: pass to visit_edge: parent equiv (or node) and visitor defined
   local state (otherwise, you need your own stack to remember)

   A helper class, visited_forest, may be used:

   \code
            visited_forest<count_trees_visitor,edge_type> counts;
            log_out << "\nParse forest has (loops excluded) " << counts(top_equiv) << " trees in ";
            log_out << counts.n_items << " items connected by " << counts.n_edges << " edges."<<endl;
   \endcode

   Visitor must have the interface (you may optionally inherit from dfs_forest_visitor_base<result_type,temp_type>):

\code
class Visitor {
 private:  // note: private -> not part of required signature
    typedef edge<whatever> edge_type;
    typedef typename edge_type::edge_equiv_type edge_equiv_type;
 public:
    /// do you require eges visited best->worst?  if not, you still get best (representative) first.
    BOOST_STATIC_CONSTANT(bool,sorted_equiv=false);
    typedef int result_type; // values memoized for each equiv
    typedef dummy_temp temp_type; // local context on stack for a sequence of start ... visit ... finish for an item

    /// called when a new equiv is to be visited (after all its
    /// antecedents are visited)
    result initial_result() { return result_type(); }
    BOOST_STATIC_CONSTANT(bool,require_sorted=false);

    void start_equiv(edge_equiv_type equiv,result_type &result,temp_type &t); /// note: you may modify the equivalence (remove alts, etc.) safely

    /// edges within an equiv will be visited in best->worst orde r(by
    /// inside and total cost; hcost must be constant).  result is the
    /// same one as in start_equiv for the equivalence the edges are
    /// visited from.  the child results are already finished (have had
    /// finish_equiv called) and should usually not be modified (but may
    /// be)
    void visit_edge(Edge &edge,result_type &result,temp_type &t); // nullary
    void visit_edge(Edge &edge,result_type &result,result_type &child,temp_type &t); // unary
    void visit_edge(Edge &edge,result_type &result,result_type &first_child,result_type &second_child,temp_type &t); // binary

    /// called after all the edges in an equiv are visited.  at this point
    /// the result can be finalized
    void finish_equiv(edge_equiv_type equiv,result_type &result,temp_type &t);
};
\endcode
\{
**/

struct dummy_result {};
struct dummy_temp {};

template <class Visitor,class Edge>
class dfs_forest {
 public:
    typedef dfs_forest<Visitor,Edge> dfs_forest_type;
    typedef Visitor visitor_type;
    typedef Edge edge_type;

    typedef typename edge_type::edge_equiv_type edge_equiv_type;
    typedef typename edge_equiv_type::impl_type edge_equiv_impl;
#ifdef DFS_FOREST_GET_RAW
# define DFS_FOREST_GET_PTR get_raw
    typedef edge_equiv_impl *edge_equiv_ptr; // get_raw - unsafe if deletion happens during visit?
#else
     typedef typename edge_equiv_type::impl_shared_ptr edge_equiv_ptr; //FIXME: is this necessary?  maybe only if deletion happens during visits.  get_shared
# define DFS_FOREST_GET_PTR get_shared
#endif
    typedef typename visitor_type::result_type result_type;
    typedef typename visitor_type::temp_type temp_type;

    typedef stlext::hash_map<edge_equiv_ptr,result_type,boost::hash<edge_equiv_ptr> > result_map_type; // PER EQUIV  - Result held by value; when dfs_forest is destroyed, so will all the results.

    dfs_forest(visitor_type &vis_) : visitor(vis_) {}
//    dfs_forest(const visitor_type &vis_) : const_visitor(vis_), visitor(const_visitor) {}

    result_type &compute_log_oom(edge_equiv_type const& e)
    {
        graehl::memory_change m;
        try {
            return compute(e);
        } catch (std::bad_alloc const&) {
            SBMT_ERROR_STREAM(forest_domain,"Exhausted memory during forest visit: "<<m);
            throw;
        }
    }

    result_type &compute(edge_equiv_type const &e)
    {
        return compute(e.DFS_FOREST_GET_PTR());
    }

    /// result owned by dfs_forest (same lifetime)
    result_type &compute(edge_equiv_ptr equiv) {
        stlextp::pair<typename result_map_type::iterator,bool> w=
            results.insert(typename result_map_type::value_type(equiv,visitor.initial_result())); //FIXME: maybe init in start_equiv instead (rather than this conditional add)
        result_type &result=w.first->second; // may be modified by visitor
        if (w.second) { // result wasn't already computed
            // visitor hook
            temp_type t;
            edge_equiv_type item(equiv);
            visitor.start_equiv(item,result,t);
            
            # ifndef NDEBUG
            edge_type const& erep = equiv->representative();
            BOOST_FOREACH(edge_type const& ealt, *equiv)
            {
                assert(erep.inside_score() >= ealt.inside_score());
                assert(erep.score() >= ealt.score());
            }
            # endif
            if (visitor_type::require_sorted) equiv->sort();
            
            // Each edge in equiv (best to worst order):
            BOOST_FOREACH(edge_type const& e, *equiv)
            {
                if (e.has_first_child()) {
                    result_type &first_result=compute(e.first_children().DFS_FOREST_GET_PTR());
                    if (e.has_second_child()) { // binary
                        result_type &second_result=compute(e.second_children().DFS_FOREST_GET_PTR());
                        // BINARY visitor hook
                        visitor.visit_edge(e,result,first_result,second_result,t);
                    } else { // unary
                        // UNARY visitor hook
                        visitor.visit_edge(e,result,first_result,t);
                    }
                } else { // nullary
                    SBMT_PEDANTIC_STREAM(forest_domain,"Nullary edge visit for item "<<item);
                    // NULLARY visitor hook
                    visitor.visit_edge(e,result,t);
                }
            }

            /// visitor hook
                visitor.finish_equiv(item,result,t);
        }
        return result;
    }

    bool was_visited(edge_equiv_ptr equiv) const
    {
        return results.find(equiv)!=results.end();
    }

    class was_visited_functor
    {
        was_visited_functor(const dfs_forest_type &dfs) : pdfs(&dfs) {}
        was_visited_functor(const was_visited_functor &o) : pdfs(o.pdfs) {}
        bool operator()(edge_equiv_ptr equiv) const
        {
            return pdfs->was_visited(equiv);
        }
     private:
        const dfs_forest_type *pdfs;
    };
    was_visited_functor get_was_visited_functor() const
    {
        return was_visited_functor(*this);
    }

    /// returns NULL if equiv wasn't visited
    result_type *result(edge_equiv_ptr equiv)
    {
        typename result_map_type::iterator f=results.find(equiv);
        if (f==results.end())
            return NULL;
        return &f->second;
    }

    result_type *result(edge_equiv_type const& eq)
    {
        return result(eq.DFS_FOREST_GET_PTR());
    }


    result_type &operator[](edge_equiv_ptr equiv)
    {
        assert(was_visited(equiv));
        return results.find(equiv)->second;
    }

    result_type &operator[](edge_equiv_type const& eq)
    {
        return (*this)[eq.DFS_FOREST_GET_PTR()];
    }


 private:
    /// holds copy of value-object Visitor if one passed to constructor
//    visitor_type const_visitor;
    /// actual visitor to be called (may be ref to const_visitor)
    visitor_type &visitor;

    result_map_type results;

};

/** Helper for holding/performing a dfs **/
template <class Visitor,class Edge>
struct visited_forest : public Visitor
{
    dfs_forest<Visitor,Edge> dfs;
    typedef dfs_forest<Visitor,Edge> DFS;
    typedef typename Visitor::result_type result_type;
    typedef typename DFS::edge_equiv_type edge_equiv_type;
//    visited_forest(visited_forest<Visitor,Edge> const& o) : Visitor(o), dfs(o.dfs) {}
    visited_forest() : Visitor(),dfs(*this)
    { }
    template <class T1>
    visited_forest(T1 &t1) : Visitor(t1),dfs(*this)
    { }
    template <class T1>
    visited_forest(T1 const&t1) : Visitor(t1),dfs(*this)
    { }
    result_type &compute(edge_equiv_type const& e)
    {
        return dfs.compute_log_oom(e);
    }
    result_type &operator()(edge_equiv_type const& e)
    { return compute(e); }
    result_type &operator[](edge_equiv_type const& e)
    { return dfs[e]; }
};

template <class Visitor,class Edge>
typename Visitor::result_type visit_forest(Visitor &v,edge_equivalence<Edge> const& eq)
{
    dfs_forest<Visitor,Edge> dfs(v);
    return dfs.compute(eq);
}


/** base class for dfs visitors.  MustSort means we want the dfs to give us
  equivalent edges best-first **/
template <class Result=dummy_result,class Temp=dummy_temp,bool MustSort=false>
struct dfs_forest_visitor_base
{
    typedef Result result_type;
    typedef Temp temp_type;
//    enum { require_sorted=MustSort };
    BOOST_STATIC_CONSTANT(bool,require_sorted=MustSort);
    static Result initial_result()
    { return Result(); }
    template <class Equiv>
    void start_equiv(Equiv const&equiv,Result &result,Temp & t) {}
    template <class Equiv>
    void finish_equiv(Equiv const&equiv,Result &result,Temp & t) {}
    template <class Edge>
    void visit_edge(Edge const& edge,Result &r,Temp & t)
    {}
    template <class Edge>
    void visit_edge(Edge const& edge,Result &r,Result &c1,Temp & t)
    {}
    template <class Edge>
    void visit_edge(Edge const& edge,Result &r,Result &c1,Result &c2,Temp & t)
    {}
};


/** base class for dfs visitors where you want to use the same
 visit_edge_ptr(edge,result,c1,c2), where c1 and c2 may be null for
 unary/nullary edges.  Self should be the class that is inheriting (CRTP) **/
template <class Self,class Result,class Temp,bool MustSort=false>
struct dfs_forest_visitor_base_ptr_local_state :
        public dfs_forest_visitor_base<Result,Temp,MustSort>
{
    Self &self()
    { return static_cast<Self&>(*this); }

    template <class Edge>
    void visit_edge(Edge const& edge,Result &r,Temp &t)
    { self().visit_edge_ptr(edge,r,0,0,t); }
    template <class Edge>
    void visit_edge(Edge const& edge,Result &r,Result &c1,Temp &t)
    { self().visit_edge_ptr(edge,r,&c1,0,t); }
    template <class Edge>
    void visit_edge(Edge const& edge,Result &r,Result &c1,Result &c2,Temp &t)
    { self().visit_edge_ptr(edge,r,&c1,&c2,t); }

    /*
    /// non-const
    template <class Edge>
    void visit_edge(Edge & edge,Result &r,Temp &t)
    { self().visit_edge_ptr(edge,r,0,0,t); }
    template <class Edge>
    void visit_edge(Edge & edge,Result &r,Result &c1,Temp &t)
    { self().visit_edge_ptr(edge,r,&c1,0,t); }
    template <class Edge>
    void visit_edge(Edge & edge,Result &r,Result &c1,Result &c2,Temp &t)
    { self().visit_edge_ptr(edge,r,&c1,&c2,t); }
    */
};

template <class Self,class Result=dummy_result,bool MustSort=false>
struct dfs_forest_visitor_base_ptr :
        public dfs_forest_visitor_base<Result,dummy_temp,MustSort>
{
    Self &self()
    { return static_cast<Self&>(*this); }

#if 0
    ///FIXME: this doesn't work!  (not found in dfs_forest)
    template <class Equiv>
    void start_equiv(Equiv const&equiv,Result &result,dummy_temp & t)
    {self().start_equiv(equiv,result);}
    template <class Equiv>
    void finish_equiv(Equiv const&equiv,Result &result,dummy_temp & t)
    {self().finish_equiv(equiv,result);}

/*
    /// non-const
    template <class Equiv>
    void start_equiv(Equiv &equiv,Result &result,dummy_temp & t)
    {self().start_equiv(equiv,result);}
    template <class Equiv>
    void finish_equiv(Equiv &equiv,Result &result,dummy_temp & t)
    {self().finish_equiv(equiv,result);}
*/
/// optional overrides
    template <class Equiv>
    void start_equiv(Equiv const&equiv,Result &result)
    {}
    template <class Equiv>
    void finish_equiv(Equiv const&equiv,Result &result)
    {}
#endif

    /// mandatory override: visit_edge_ptr(edge,R &r,R *c1,R *c2)
    template <class Edge>
    void visit_edge(Edge const& edge,Result &r,dummy_temp & t)
    { self().visit_edge_ptr(edge,r,(Result *)0,(Result *)0); }
    template <class Edge>
    void visit_edge(Edge const& edge,Result &r,Result &c1,dummy_temp & t)
    { self().visit_edge_ptr(edge,r,&c1,(Result *)0); }
    template <class Edge>
    void visit_edge(Edge const& edge,Result &r,Result &c1,Result &c2,dummy_temp & t)
    { self().visit_edge_ptr(edge,r,&c1,&c2); }
/*
    /// non-const
    template <class Edge>
    void visit_edge(Edge & edge,Result &r,dummy_temp const& t)
    { self().visit_edge_ptr(edge,r,0); }
    template <class Edge>
    void visit_edge(Edge & edge,Result &r,Result &c1,dummy_temp const& t)
    { self().visit_edge_ptr(edge,r,&c1,0); }
    template <class Edge>
    void visit_edge(Edge & edge,Result &r,Result &c1,Result &c2,dummy_temp const& t)
    { self().visit_edge_ptr(edge,r,&c1,&c2); }
*/
};

template <class F>
struct visit_equivs : dfs_forest_visitor_base<dummy_result,dummy_temp>
{
    typedef visit_equivs<F> self_type;
    F f;
    visit_equivs(F const& f=F()) : f(f) {}
    template <class Equiv>
    void start_equiv(Equiv const& equiv,dummy_result &result,dummy_temp &temp) {
        f(equiv);
    }
};


///\}

} //sbmt


#endif
