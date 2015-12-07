#ifndef __lattice_graph_hpp__
#define __lattice_graph_hpp__

#include <vector>
#include <iostream> // std::cout
#include <utility> // std::pair
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <sbmt/search/block_lattice_tree.hpp>

namespace word_context {


using namespace sbmt;
using namespace boost;
using namespace std;

template<class new_graph>
void visit_lattice_tree_node(const sbmt::lattice_tree::node & n,
        new_graph& new_g)
{
    if(n.is_internal()){
        BOOST_FOREACH(const sbmt::lattice_tree::node& m, make_pair(n.children_begin(), n.children_end())){
            visit_lattice_tree_node(m, new_g);
        }
    } else {
        new_g.add_edge(n);
    }
};

struct foreign_token_t {
  typedef boost::edge_property_tag kind;
};
typedef boost::property<foreign_token_t, std::string> ep;


template<class TF>
class lattice_graph : public adjacency_list<vecS, vecS, bidirectionalS, no_property, ep>
{
    const TF& dict;
public:
   typedef adjacency_list<vecS, vecS, bidirectionalS, no_property , ep> Graph;

   /// construct this lattice graph from the lattice tree.
   lattice_graph(const sbmt::lattice_tree& latree, const TF& tf ): dict(tf)

   {
       visit_lattice_tree_node(latree.root(), *this);
   }

   /// returns the labels (or foreign words) on the in edges of the vertext.
   vector<string> in_edges(graph_traits<Graph>::vertex_descriptor v)
   {
      property_map<Graph, foreign_token_t>  :: type  foreign_tokens = get(foreign_token_t(), *this);
       vector<string> vec;
      typedef graph_traits<Graph> :: in_edge_iterator in_edge_iterator;
      in_edge_iterator edgei1, edgei2;
      for(tie(edgei1, edgei2) = boost::in_edges(v, *this); edgei1 != edgei2; ++edgei1){
           vec.push_back(foreign_tokens[*edgei1]);
       }
       return vec;
   }

   /// returns the labels (or foreign words) on the out edges of the vertext.
   vector<string> out_edges(graph_traits<Graph>::vertex_descriptor v)
   {
       vector<string> vec;
       property_map<Graph, foreign_token_t> :: type  foreign_tokens = get(foreign_token_t(), *this);
       boost::graph_traits<Graph>::edge_descriptor ed;
      typedef graph_traits<Graph> :: out_edge_iterator out_edge_iterator;
      out_edge_iterator edge1, edge2;
      for(tie(edge1, edge2) = boost::out_edges(v, *this); edge1 != edge2; ++edge1){
           vec.push_back(foreign_tokens[*edge1]);
       }
       return vec;
   }

   /// add the information in a lattice_tree node into the edge of this
   /// lattice graph.
   void add_edge(const lattice_tree::node& n) {
       boost::graph_traits<Graph>::vertex_descriptor u,v;
       string tok = dict.label(n.lat_edge().source);
       u=vertex(n.lat_edge().span.left(), *this);
       v=vertex(n.lat_edge().span.right(), *this);
       boost::graph_traits<Graph>::edge_descriptor ed;
       bool b;
       tie(ed,b)= boost::add_edge(u, v, *this);
       // put the foreign token on the edge.

       put(foreign_token_t(), *this, ed,dict.label(n.lat_edge().source));
       //std::cout<<"edge label: "<<n.lat_edge().source.label()<<endl;
   }
};

} // namespace word_context;

#endif
