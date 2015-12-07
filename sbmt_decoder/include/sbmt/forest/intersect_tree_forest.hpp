#ifndef SBMT__FOREST__INTERSECT_TREE_FOREST_HPP
#define SBMT__FOREST__INTERSECT_TREE_FOREST_HPP

#include <sbmt/forest/derivation_tree.hpp>
#include <sbmt/edge/edge_equivalence.hpp>
#include <graehl/shared/maybe_update_bound.hpp>
#include <sbmt/search/sausage.hpp>
#include <boost/intrusive_ptr.hpp>

namespace sbmt {

// only works when forest has no cycles. very similar to dfs_forest except we add a tree parameters that we descend with and add to memoization key state
//FIXME: add some interface so force decode edge transforms, given grammar, binary rule id into underlying grammar rule id (which is what you will see on binary deriv. tree
template <class Edge>
struct intersect_tree_forest
{
  typedef Edge edge_type;
  typedef typename edge_type::edge_equiv_type edge_equiv_type;
  typedef typename edge_equiv_type::impl_type edge_equiv_impl;
  typedef typename edge_equiv_type::impl_shared_ptr forest_p; //FIXME: is this necessary? maybe only if deletion happens during visits. get_shared

  typedef binary_derivation_tree const* tree_p;
  struct intersect_key
  {
    forest_p forest;
    tree_p tree;
    friend std::size_t hash_value(intersect_key const& m)
    {
      std::size_t seed = 0;
      boost::hash_combine(seed, m.forest);
      boost::hash_combine(seed, m.tree);
      return seed;
    }
    intersect_key(forest_p forest,tree_p tree) : forest(forest), tree(tree) {}
    bool operator==(intersect_key const& o) const
    {
      return forest==o.forest && tree==o.tree;
    }
  };



  struct best_match : public graehl::intrusive_refcount<best_match>
  {
    typedef boost::intrusive_ptr<best_match> match_p; //FIXME: make sure deriv forest is acyclic or you get leaks
    bool complete;
    unsigned n_match;
// top down binary derivation label matching can succeed at the top but fail toward the bottom. n_match=0 implies that the below members are uninit/meaningless:
    edge_equiv_type equiv;
    edge_type edge;
    unsigned rank_in_equiv; // 0 is best, 1 is 2nd best if intersect competed w/ sorting
    unsigned child_max_rank; // over all children, recursively
    match_p child[2]; // recurse to build single edge_equiv tree later before destroying intersect_tree_forest state which owns all these pointers
// FIXME: need to use more indirection for stable pointers, or store key, because hash table growth (for oa_hashtable) can invalidate refs ... graehl hash guarantees otherwise (chaining w/ linked list) but we don't really use child[] yet, anyway
// intersect_key child_key[2]; // safer than child pointers in case we don't have a guarantee that stlext::hash_map uses static node

    best_match()
      : complete(false),n_match(0),rank_in_equiv(0),child_max_rank(0)
    {}

    best_match(forest_p f,edge_type const& e,unsigned r)
      : complete(true)
      , n_match(1)
      , equiv(f)
      , edge(e)
      , rank_in_equiv(r)
      , child_max_rank(0)
    {
    }

    void improve(best_match const& o)
    {
      if (!complete && o.complete || o.n_match > n_match)
        *this=o;
    }


    void operator=(best_match const& o)
    {
      // had to write this because intrusive_refcount isn't copyable
      complete=o.complete;
      n_match=o.n_match;
      equiv=o.equiv;
      edge=o.edge;
      rank_in_equiv=o.rank_in_equiv;
      child_max_rank=o.child_max_rank;
      child[0]=o.child[0];
      child[1]=o.child[1];
    }


    void improve(match_p const& o)
    {
      improve(*o);
    }

    void join_child(match_p const& o,unsigned child_i)
    {
      child[child_i]=o;
      n_match += o->n_match;
      complete = complete && o->complete;
      graehl::maybe_increase_max(child_max_rank,o->child_max_rank);
      graehl::maybe_increase_max(child_max_rank,o->rank_in_equiv);
    }
    template <class O>
    void print (O &o) const
    {
      o <<(complete?"found":"not found")<<" in forest: matched "<<n_match<<" binary derivation nodes, going at most "<<child_max_rank<<" deep in alternate edges for a forest equivalence item ("<<rank_in_equiv<<" deep at the top)";
    }

    template <class Chart>
    void fill_sausage(sausage &s,Chart const& chart) const
    {
      s.insert(delta(chart,edge));
      for (unsigned i=0;i<=1;++i)
        if (child[i])
          child[i]->fill_sausage(s,chart);
    }

    template <class O,class Chart>
    void print_with_sausage(O &o,Chart const& chart) const
    {
      print(o);
      o << " sausage={{{";
      print_sausage(o,chart);
      o << "}}}";
    }

    template <class O,class Chart>
    void print_sausage(O &o,Chart const& chart) const
    {
      sausage s;
      fill_sausage(s,chart);
      o << s;
    }

  };
  typedef boost::intrusive_ptr<best_match> match_p;

  typedef oa_hash_map<intersect_key,match_p> memo_t;
// typedef stlext::hash_map<intersect_key,match_p,boost::hash<intersect_key> > memo_t;
  memo_t memo;
// typedef typename memo_t::value_type memo_v;

  // if sorted is false, then the rank stats collected may be nonsense after rank=0
  match_p intersect(edge_equiv_type const& e,tree_p t,bool sorted=true)
  {
    return intersect(e.get_shared(),t,sorted);
  }

  // use return by value right away, before you ask any other intersect questions, because may be invalidated by further intersection. could drop ref. from return to make it safer
  match_p intersect(forest_p f,tree_p t,bool sorted=true) {
    grammar_rule_id root_rule=t->label.id();
    intersect_key key(f,t);
// typename memo_t::insert_ret
    std::pair<typename memo_t::iterator,bool>
      w=memo.insert(std::make_pair(key,match_p()));
    match_p &ret=const_cast<match_p&>(w.first->second);
    if (w.second) { // compute new
      typedef typename edge_equiv_type::edge_range ER;
      ER edges=sorted ? f->edges_sorted() : f->edges();
      unsigned rank_in_equiv=0;
      for (typename ER::iterator &i=edges.begin(),&end=edges.end();i!=end;++i,++rank_in_equiv) {
        edge_type const& e=*i;
        match_p m=new best_match(f,e,rank_in_equiv);
        if (root_rule==NULL_GRAMMAR_RULE_ID && e.root().type()==foreign_token) { // tree has null id for foreign words; but lattices may have a rule id for initial feat on foreign words. in non-lattice case, the e.rule_id() will be null as well
          ret=m;
        } else if (e.rule_id() == root_rule) {
          // note: all the edges with a matching rule will have the same # of children. so we could iterate again inside each branch and end the original iteration, for efficiency (waste of programmer effort, though)
          if ((unsigned)t->size() != e.child_count()) {
            throw_bad_syntax_derivation_tree("matching binary derivation tree against forest: binary derivation tree has different # of children than an edge with the same binary rule. is this binary derivation built on the right grammar?");
          }
          if (e.has_first_child()) { //1-2 ary
            match_p const& m0=intersect(e.first_children(),t->child(0),sorted);
            m->join_child(m0,0);
            if (e.has_second_child()) { // binary
              match_p const& m1=intersect(e.second_children(),t->child(1),sorted);
              m->join_child(m1,1);
            }
            // need to lookup key again because "ret" could have been invalidated by recursive intersects:
            match_p ret2=memo.find(key)->second; // always exists because we inserted it at start of intersect()
            ret2->improve(m);
            return ret2;
          } else { //nullary
            ret=m;
          }
        } // else no match, so leave ret as default
      }
    }
    return ret;
  }


};

template <class O,class Edge>
void print_tree_in_forest(O &o,edge_equivalence<Edge> const &eq,binary_derivation_tree const& t)
{
  intersect_tree_forest<Edge> i;
  i.intersect(eq,&t,true)->print(o);
  o << " for derivation tree of " << t.count_nodes() << " nodes.";
}


template <class O,class Edge,class Chart>
void print_tree_in_chart(O &o,edge_equivalence<Edge> const& eq,Chart const& chart,binary_derivation_tree const& t,unsigned sentid)
{
  typedef typename Chart::edge_type edge_type;
  typedef typename Chart::edge_equiv_type edge_equiv_type;
  typedef intersect_tree_forest<edge_type> X;
  typedef typename X::match_p Match;

  X i;
  Match m=i.intersect(eq,&t,true);
  m->print(o);
  o << "sausage_find_derivation["<<sentid<<"]={{{";
  m->print_sausage(o,chart);
  o << "}}}";
  o << " for derivation tree of " << t.count_nodes() << " nodes.";
}



}

#endif
