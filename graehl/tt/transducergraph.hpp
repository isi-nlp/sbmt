// tree transducer interpreted as a hypergraph
#ifndef _transducergraph_HPP
#define _transducergraph_HPP

#define GRAEHL_EXTENDED_HYPERGRAPH_TRAITS 0

#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/graph_traits.hpp>
#include <graehl/shared/byref.hpp>
#include <graehl/shared/tails_up_hypergraph.hpp>
#include <graehl/tt/transducer.hpp>
#include <graehl/shared/property.hpp>
#include <graehl/shared/property_factory.hpp>
#include <graehl/shared/genio.h>
#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <graehl/shared/graph.hpp>
#include <graehl/shared/outedges.hpp>
#endif
//! visit_hyperarcs_nested - should we indicate empty tail sets somehow?

// BGL for treexdcr as graph (really, hypergraph)
// added hyperarc, tail descriptor.  tail=target, head=source(hyperarc).  edge = hyperarc+tail
// rationale: even though this means the direction of arrows is reversed (traditionally head = head of arrow), BGL adjacency graph assumes out_edges, out_degree, bfs,dfs use out_edges ... in_edges is a supplemental concept (you never have in_edges w/o out_edges).  would have to use reverse_graph adaptor all the time if we did this.  reverse_graph requires bidirectional = both indices, too.
// tail_iterator
// out_hyperarc_iterator


namespace boost {
#ifndef TT_TRAITS
  template <class R>
  struct graph_traits<graehl::Treexdcr<R> > {
    typedef graehl::Treexdcr<R> graph;
    typedef unsigned vertex_descriptor;
    //typedef unsigned hyperarc_descriptor;
    //typedef typename graph::hyperarc_descriptor hyperarc_descriptor;
    typedef typename graph::Rule * hyperarc_descriptor;
    typedef Source *tail_descriptor;
    //  typedef OffsetMap<hyperarc_descriptor) offset_map;
    //template<class E> class edge_offset_map<E>;
    struct hyperarc_index_map : public OffsetMap<hyperarc_descriptor> {
      //typedef hyperarc_descriptor edge_descriptor;
      hyperarc_index_map(graph &g) : OffsetMap<hyperarc_descriptor>(g.rules.begin()) {}
      hyperarc_index_map(const hyperarc_index_map &o) : OffsetMap<hyperarc_descriptor>(o) {}
    };
    //typedef OffsetMap<hyperarc_descriptor> hyperarc_index_map;
    typedef boost::identity_property_map vertex_index_map;
#ifdef TT_TRAITS
    typedef typename graph::edge_descriptor edge_descriptor;
#else
    struct edge_descriptor {
      hyperarc_descriptor arc;
      tail_descriptor tail; // pointer into rules[rule].sources
      bool operator==(edge_descriptor r) {
        return arc==r.arc && tail==r.tail;
      }
      edge_descriptor & operator *() { return *this; }
      template <class charT, class Traits>
      std::ios_base::iostate
      print(std::basic_ostream<charT,Traits>& o=std::cerr) const
      {
        o << '"' << *arc << " :: " << *tail << '"';
        return GENIOGOOD;
      }


      //    operator hyperarc_descriptor() { return arc; }
    };
#endif
    //typedef unsigned edge_descriptor;
    typedef directed_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category;
    //      typedef edge_list_graph_tag traversal_category;
    typedef unsigned vertices_size_type;
    typedef unsigned edges_size_type;
    typedef unsigned degree_size_type;
    static vertex_descriptor null_vertex() {
      return (unsigned)-1;
    }
    typedef boost::counting_iterator<tail_descriptor> tail_iterator;
    typedef boost::counting_iterator<hyperarc_descriptor> hyperarc_iterator;
    typedef boost::counting_iterator<vertex_descriptor> vertex_iterator;
    typedef hyperarc_iterator out_hyperarc_iterator;
    /*struct traversal_category {
      operator edge_list_graph_tag() const { return edge_list_graph_tag(); }
      operator vertex_list_graph_tag() const { return vertex_list_graph_tag(); }
      };
    */

    //      typedef typename graph::edge_iterator edge_iterator;
#ifdef TT_TRAITS
    typedef typename graph::edge_iterator edge_iterator;
    typedef typename graph::traversal_category traversal_category;
#else
    struct traversal_category: public edge_list_graph_tag, vertex_list_graph_tag {};
    struct edge_iterator {
      edge_descriptor ed;
      hyperarc_descriptor hend; // hend would almost be unnecessary if we could defer initializing t<-begin

      //          tail_descriptor t;

      //! only works for i!=end()
      bool operator !=(const edge_iterator i2) const { // only works for detecting end - sorry
        return ed.arc!=i2.ed.arc;
      }
      bool operator ==(const edge_iterator i2) const {
        return !(*this!=i2);
      }
      void operator ++() {
        ++ed.tail;
        if (!goodTail()) {
          ++ed.arc;
          find_tail();
        }
      }
      bool goodTail() {
        return ed.tail != ed.arc->sources.end();
      }
      void find_tail() {
        for (; ed.arc != hend; ++ed.arc) {
          ed.tail=ed.arc->sources.begin();
          if (goodTail())
            return;
        }
      }
      edge_descriptor operator *() const {
        return ed;
      }
      /*
        edge_iterator(hyperarc_descriptor h,hyperarc_descriptor end) :hend(end){
        ed.head=h;
        --ed.head(); advance_head();
        }*/
    };
#endif
    typedef std::pair<
      typename graph_traits<graehl::Treexdcr<R> >::vertex_iterator,
      typename graph_traits<graehl::Treexdcr<R> >::vertex_iterator> pair_vertex_it;

    typedef std::pair<
      typename graph_traits<graehl::Treexdcr<R> >::tail_iterator,
      typename graph_traits<graehl::Treexdcr<R> >::tail_iterator> pair_tail_it;
    typedef std::pair<
      typename graph_traits<graehl::Treexdcr<R> >::hyperarc_iterator,
      typename graph_traits<graehl::Treexdcr<R> >::hyperarc_iterator> pair_hyperarc_it;
    typedef std::pair<
      typename graph_traits<graehl::Treexdcr<R> >::out_hyperarc_iterator,
      typename graph_traits<graehl::Treexdcr<R> >::out_hyperarc_iterator> pair_out_hyperarc_it;
    typedef std::pair<
      typename graph_traits<graehl::Treexdcr<R> >::edge_iterator,
      typename graph_traits<graehl::Treexdcr<R> >::edge_iterator> pair_edge_it;
    typedef FLOAT_TYPE cost_type;
    /*  struct hyperarc_cost_map {
        typedef boost::read_property_map_tag category;
        typedef cost_type value_type;
        typedef hyperarc_descriptor key_type;
        cost_type operator [](key_type p) {
        return p->getCost();
        }
        }*/
  };
#endif

  template<class R>
  struct property_map<graehl::Treexdcr<R>, boost::edge_weight_t>
  {
    typedef typename graehl::Treexdcr<R>::cost_type cost_type;
    typedef boost::readable_property_map_tag category;
    typedef cost_type value_type;
    typedef typename graehl::Treexdcr<R>::hyperarc_descriptor key_type;
    template <class K>
    cost_type operator [](K p) const {
      return p->getCost();
    }
    typedef property_map<graehl::Treexdcr<R>, boost::edge_weight_t> type;
  };

  template<class R,class K>
  typename property_map<graehl::Treexdcr<R>, boost::edge_weight_t>::type::value_type
  get(typename property_map<graehl::Treexdcr<R>, boost::edge_weight_t>::type map, K k) {
    return map[k];
  }


//FIXME: what am I trying to represent here?  treexdcr have a built-in edge weight propertymap, but why get it with boost::get()???  property factory interface used for creating empty pmaps
  template<class R>
  typename property_map<graehl::Treexdcr<R>, boost::edge_weight_t>::type
  get(boost::edge_weight_t unused,graehl::Treexdcr<R> &unused_g)
  {
    return typename property_map<graehl::Treexdcr<R>, boost::edge_weight_t>::type();
    //return dummy[k];
  }

} // boost::


namespace graehl {


/*
  template <class R,>
  typename graph_traits<graehl::Treexdcr<R> >::cost_type
  get(graehl::Treexdcr<R> &g,
  unsigned get(OffsetMap k,K p) {
  return k[p];
  }

  };
*/

// default property factories for a graph (PropertyFactoryGraph)
template <class R>
struct property_factory
<Treexdcr<R>,hyperarc_tag> :
  public ArrayPMapFactory<typename Treexdcr<R>::hyperarc_index_map>
{
  typedef typename Treexdcr<R>::hyperarc_index_map index_map;
  typedef ArrayPMapFactory<index_map> Imp;
  typedef Treexdcr<R> G;
  property_factory(G&g) : Imp(num_hyperarcs(g),index_map(g)) {}
  property_factory(const property_factory &o) : Imp(o) {}
  /*  <Treexdcr<R>,typename graph_traits<typename Treexdcr<R> >::hyperarc_descriptor> {
      typedef property_factory
      <Treexdcr<R>,typename graph_traits<typename Treexdcr<R> >::hyperarc_descriptor> Self;

      typedef ArrayPMapFactory<typename graph_traits<typename Treexdcr<R> >::hyperarc_index_map> type;

      static inline typename Self::type get(const Treexdcr<R> &g) {
      return
      typename Self::type(num_hyperarcs(g),g);
      }
  */
};

//choice: either require propertyfactories to take a single argument (graph &g) i.e. just make subclasses of impls that interpret g and pass on to parent constructor (also need copy), or use the external get below to build an impl and return it by value


template <class R>
struct
property_factory
<Treexdcr<R>,vertex_tag> :
  public ArrayPMapFactory<typename Treexdcr<R>::vertex_index_map>
{
  typedef ArrayPMapFactory<typename Treexdcr<R>::vertex_index_map> Imp;
  typedef Treexdcr<R> G;
  property_factory(const G&g) : Imp(num_vertices(g)) {}
  property_factory(const property_factory &o) : Imp(o) {}
};

/*{

typedef property_factory
<Treexdcr<R>,typename graph_traits<typename Treexdcr<R> >::vertex_descriptor> Self;
typedef ArrayPMapFactory<typename graph_traits<typename Treexdcr<R> >::vertex_index_map> type;

static inline typename Self::type get(const Treexdcr<R> &g) {
return
typename Self::type(num_vertices(g));
}

};
*/
template <class R,class F>
inline void visit
//<Treexdcr<R>,typename graph_traits<typename Treexdcr<R> >::vertex_descriptor,F>
(vertex_tag unused,Treexdcr<R> &g,F f) {
  typedef Treexdcr<R> G;
  typedef typename hypergraph_traits<G>::vertex_iterator vi;
  for (vi i=0,end=g.n_rules();i!=end;++i)
    deref(f)(*i);
}


template <class R,class F>
void visit
//<typename hypergraph_traits<typename Treexdcr<R> >::hyperarc_descriptor>
(hyperarc_tag unused,Treexdcr<R> &g,F f) {
  typedef Treexdcr<R> G;
  typedef typename hypergraph_traits<G>::hyperarc_iterator ei;
  for (ei i=g.rules.begin(),end=g.rules.end();i!=end;++i)
    deref(f)(*i);
}

template <class R,class F>
void visit
//<typename hypergraph_traits<typename Treexdcr<R> >::edge_descriptor>
(edge_tag unused,Treexdcr<R> &g,F f) {
  typedef Treexdcr<R> G;
  typedef typename hypergraph_traits<G>::hyperarc_iterator ei;
  typedef typename hypergraph_traits<G>::tail_iterator ti;
  typedef typename hypergraph_traits<G>::edge_descriptor ed;
  ed edge;
  for (ei i=g.rules.begin(),end=g.rules.end();i!=end;++i) {
    edge.arc=*i;
    for (ti j=edge.arc->sources.begin(),jend=edge.arc->sources.end();j!=jend;++j) {
      edge.tail=*j;
      deref(f)(edge);
    }
  }
}

/*
  template <class G,class F>
  inline void visit<typename hypergraph_traits<G>::edge_descriptor>(G &g,F f) {
  visit_edges(g,f);
  }

  template <class G,class F>
  inline void visit<typename hypergraph_traits<G>::hyperarc_descriptor>(G &g,F f) {
  visit_hyperarcs(g,f);
  }
*/

template <class R,class F>
inline void visit_hyperarcs_nested(Treexdcr<R> &g,F f) {
  typedef Treexdcr<R> G;
  typedef typename hypergraph_traits<G>::hyperarc_iterator ei;
  typedef typename hypergraph_traits<G>::tail_iterator ti;
  typedef typename hypergraph_traits<G>::edge_descriptor ed;
  for (ei i=g.rules.begin(),end=g.rules.end();i!=end;++i) {
    deref(f)(*i);
    for (ti j=i->sources.begin(),jend=i->sources.end();j!=jend;++j) {
      deref(f)(*j);
    }
  }
}


template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::vertex_descriptor
source(
       typename Treexdcr<R>::hyperarc_descriptor e,
       const Treexdcr<R> &g
       )
{
  //g.rules[e].
  return e->state;
}

/*
  template <class R>
  inline typename hypergraph_traits<Treexdcr<R> >::pair_hyperarc_it
  out_hyperarcs(
  typename hypergraph_traits<Treexdcr<R> >::vertex_descriptor v,
  const Treexdcr<R> &g
  )
  {
  typename hypergraph_traits<Treexdcr<R> >::pair_hyperarc_it ret;
  ret.first=g.rules.begin();
  ret.second=g.rules.end();

  return ret;
  }
*/

template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::pair_hyperarc_it
hyperarcs(
          Treexdcr<R> &g
          )
{
  typename hypergraph_traits<Treexdcr<R> >::pair_hyperarc_it ret;
  ret.first=g.rules.begin();
  ret.second=g.rules.end();

  return ret;
}

template <class R>
inline unsigned num_vertices(const Treexdcr<R> &g) {
  return g.n_states();
}

template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::pair_vertex_it
vertices(
         const Treexdcr<R> &g
         )
{
  typedef typename hypergraph_traits<Treexdcr<R> >::pair_vertex_it Ret;
  return Ret(0,g.n_states());
}

template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::pair_edge_it
edges(
      Treexdcr<R> &g
      )
{
  typename hypergraph_traits<Treexdcr<R> >::pair_edge_it ret;
  ret.first.hend=
    ret.second.ed.arc=
    g.rules.end();
  ret.first.ed.arc=g.rules.begin();
  ret.first.find_tail();
  return ret;
}

template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::pair_hyperarc_it
hyperarcs(
         const Treexdcr<R> &g
         )
{
  typedef typename hypergraph_traits<Treexdcr<R> >::pair_hyperarc_it Ret;
  return Ret(g.rules.begin(),g.rules.end());
}


template <class R>
inline unsigned num_hyperarcs(const Treexdcr<R> &g) {
  //FIXED: reader counts Sum(#sources) or else iterator over all rules here.
  return g.n_rules();
}

template <class R>
inline unsigned num_edges(const Treexdcr<R> &g) {
  //FIXED: reader counts Sum(#sources) or else iterator over all rules here.
  return g.total_sources;
}


template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::degree_size_type
tail_degree(
            typename hypergraph_traits<Treexdcr<R> >::hyperarc_descriptor e,
            const Treexdcr<R> &g
            )
{
  return e->sources.size();
}

template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::tail_iterator
tails_begin(
            typename hypergraph_traits<Treexdcr<R> >::hyperarc_descriptor e,
            const Treexdcr<R> &g
            )
{
  return e->sources.begin();
}

template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::tail_iterator
tails_end(
          typename hypergraph_traits<Treexdcr<R> >::hyperarc_descriptor e,
          const Treexdcr<R> &g
          )
{
  return e->sources.end();
}


/*
  template <class R>
  inline typename hypergraph_traits<Treexdcr<R> >::hyperarc_iterator
  begin
  //<typename hypergraph_traits<Treexdcr<R> >::hyperarc_iterator>
  (
  hyperarc_tag unused,
  const Treexdcr<R> &g)
  {
  return g.rules.begin();
  }

  template <class R>
  inline typename hypergraph_traits<Treexdcr<R> >::vertex_iterator
  begin
  //<typename hypergraph_traits<Treexdcr<R> >::vertex_iterator>
  (
  vertex_tag unused,
  const Treexdcr<R> &g)
  {
  return 0;
  }

  template <class R>
  inline typename hypergraph_traits<Treexdcr<R> >::hyperarc_iterator
  end
  //<typename hypergraph_traits<Treexdcr<R> >::hyperarc_iterator>
  (
  hyperarc_tag unused,
  const Treexdcr<R> &g)
  {
  return g.rules.end();
  }

  template <class R>
  inline typename hypergraph_traits<Treexdcr<R> >::vertex_iterator
  end
  //<typename hypergraph_traits<Treexdcr<R> >::vertex_iterator>
  (
  vertex_tag unused,
  const Treexdcr<R> &g)
  {
  return g.n_states();
  }
*/

template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::pair_tail_it
tails(
      typename hypergraph_traits<Treexdcr<R> >::hyperarc_descriptor e,
      const Treexdcr<R> &g
      )
{
  typedef typename hypergraph_traits<Treexdcr<R> >::pair_tail_it Ret;
  return Ret(
             e->sources.begin(),
             e->sources.end()
             );
  //g.rules[e].
  //return e->state;
}


template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::hyperarc_descriptor
hyperarc(
         typename hypergraph_traits<Treexdcr<R> >::edge_descriptor e,
         const Treexdcr<R> &g
         )
{
  return e.arc;
}


template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::vertex_descriptor
source(
       typename hypergraph_traits<Treexdcr<R> >::edge_descriptor e,
       const Treexdcr<R> &g
       )
{
  return source(hyperarc(e,g),g);
}


template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::tail_descriptor
tail(
     typename hypergraph_traits<Treexdcr<R> >::edge_descriptor e,
     const Treexdcr<R> &g
     )
{
  //g.rules[e.rule].
  //e.rule->sources[e.tail];
  return e.tail;
}


template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::vertex_descriptor
target(
       typename hypergraph_traits<Treexdcr<R> >::tail_descriptor e,
       const Treexdcr<R> &g
       )
{
  return e->state;
}


template <class R>
inline typename hypergraph_traits<Treexdcr<R> >::vertex_descriptor
target(
       typename hypergraph_traits<Treexdcr<R> >::edge_descriptor e,
       const Treexdcr<R> &g
       )
{
  return target(tail(e,g),g);
}


// template <class R> ostream & ::dbgout(ostream &o,typename Treexdcr<R>::edge_descriptor &arg) {  return gen_inserter(o,arg); }

std::ostream & dbgout(std::ostream &o,
  Treexdcr<Xrule<Symbol, Trhs<Symbol> > >::edge_descriptor &arg)
{ return gen_inserter(o,arg); }

std::ostream & operator <<(std::ostream &o,
  Treexdcr<Xrule<Symbol, Trhs<Symbol> > >::edge_descriptor &arg)
{ return gen_inserter(o,arg); }


#ifdef GRAEHL_TEST


const char *test_rtg=
"{start=0 rules=("
" 0 =>`1"
" 1 => q(`2 e(h `0) `2 `2)"
" 0 .01 => f"
" 1 => q(`3 g `3 `2)"
" 3 => a(`2)"
" 3 .1 => f"
" 2 .8=> f"
" 1 .5=> `3"
" 4 => a(`4 f)"
" 4 => b(`5 `1 e `1)"
" 5 => b(`6 `2)"
" 6 => c(`4 `5)"
")}";


typedef hypergraph_traits<TT> GTT;

BOOST_AUTO_UNIT_TEST( TEST_transducergraph )
{
  TT t(test_rtg);
  TT e(empty_rtg);
  BOOST_REQUIRE(t.valid());
  //BOOST_CHECK(false);
  BOOST_CHECK(num_vertices(t) == 7);
  BOOST_CHECK(num_hyperarcs(t) == 12);
  BOOST_CHECK(num_edges(t) == 18);
  GTT::pair_edge_it pit=edges(t);
  BOOST_REQUIRE(pit.first!=pit.second);
  GTT::edge_descriptor ed=*pit.first;
  BOOST_CHECK(source(ed,t)==0);
  BOOST_CHECK(target(ed,t)==1);
  BOOST_CHECK(source(*(hyperarcs(t).first),t)==0);
  //typename hypergraph_traits<TT>::pair_hyperarc_it=

  BOOST_CHECK(target(*(tails(*hyperarcs(t).first,t).first),t)==1);
  GTT::pair_edge_it epit=edges(e);
  BOOST_CHECK(epit.first == epit.second);
}

typedef OutEdges<TT> S;
typedef boost::graph_traits<S> ST;

BOOST_AUTO_UNIT_TEST( TEST_outedges )
{
  TT t(test_rtg);
  S s(t);
//  DBP(t);
  BOOST_CHECK(num_vertices(t)==num_vertices(s.graph())); // tests conversion
  GTT::pair_edge_it pit=edges(t);
  BOOST_REQUIRE(pit.first!=pit.second);
  GTT::edge_descriptor e=*pit.first;

  BOOST_CHECK(out_degree(0,s)==1);
  BOOST_CHECK(out_degree(1,s)==8);
  BOOST_CHECK(out_degree(2,s)==0);
  BOOST_CHECK(out_degree(3,s)==1);

  ST::pair_out_edge_it p=out_edges(0,s);
  BOOST_REQUIRE(p.first!=p.second);
  BOOST_CHECK(*p.first == e);


  // DBP(**pit.first);
  ++pit.first;BOOST_REQUIRE(pit.first!=pit.second);
  //  GTT::edge_descriptor ed=**pit.first;
  //  dbgout(cerr,ed);
  //  cerr << ed << endl; //FIXME: why doesn't template get made for Blah<R>::type??

  //    DBP(**pit.first);
  ++pit.first;BOOST_REQUIRE(pit.first!=pit.second);
  //DBP(**pit.first);

  p=out_edges(1,s);
  //DBP(**p.first);
  BOOST_REQUIRE(p.first!=p.second);
  ++p.first;BOOST_REQUIRE(p.first!=p.second);
  //DBP(**p.first);
  BOOST_REQUIRE(*p.first==*pit.first); // the `S in 2nd rule: Q=>
  /*    for (;pit.first!=pit.second;++pit.first)
        DBP(**pit.first);
    for (int i=0; i<=3; ++i) {
    ST::pair_out_edge_it j=out_edges(i,s);
    for (;j.first!=j.second;++j.first)
      DBP(**j.first);
      }*/
}

BOOST_AUTO_UNIT_TEST( TEST_hypergraph )
{
typedef hypergraph_traits<TT> GT;
typedef TailsUpHypergraph<TT> R;
 typedef property_factory<TT,vertex_tag> VertF;
  TT t(test_rtg);
  R r(t);

 VertF pfact(t);

typedef R::BestTree<> Best;
typedef R::Reach<> Reach;
 typedef Best::DefaultMu Mu;
typedef Best::DefaultPi Pi;
typedef Reach::DefaultReach Visited;
typedef R::BestTree<> Best;
typedef R::Reach<> Reach;
 Visited visited(pfact);
  Mu mu(pfact);
  Pi pi(pfact);


  //  DBP(t);
  BOOST_CHECK(t.n_rules()==12);

  Reach reach(r,ref(visited));
  Best best(r,ref(mu),ref(pi));

    BOOST_CHECK(r.terminal_arcs.size()==3);

    //    DBP(r.terminal_arcs);
  for (int i=0; i<7; ++i) {
    //    DBP2(i,r[i]);
    BOOST_CHECK(r[i].size() > 0);
  }
  //  DBP(r.unique_tails_pmap());

  //  DBP(reach.tr);
  reach.init();
  //  DBP(reach.tr);
  reach.finish();
  for (int i=0; i <7; ++i) {
    //DBP2(i,visited[i]);
    BOOST_CHECK(visited[i] == i<4);
  }
  best.init();
  best.finish();
  //  DBP(mu);
  //  DBP(pi);
  //  DBP(best.hyperarc_remain_and_cost_map());
  BOOST_CHECK(pi[0]==&t.rules[0]);
  BOOST_CHECK(pi[1]==&t.rules[3]);
  BOOST_CHECK(pi[2]==&t.rules[6]);
  BOOST_CHECK(pi[3]==&t.rules[4]);
  BOOST_CHECK(pi[4]==NULL);
  BOOST_CHECK(pi[5]==NULL);
  BOOST_CHECK(pi[6]==NULL);
  for(int i=0;i<12;++i)
    BOOST_CHECK((i>=8) == (deref(best.hyperarc_remain_and_cost_map())[t.rules.begin()+i].remain() > 0));
}


#endif

}//ns

#endif
