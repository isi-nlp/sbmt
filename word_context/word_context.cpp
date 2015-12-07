#include <iostream> // std::cout
#include <utility> // std::pair
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <vector>
#include <map>
#include <string>
using namespace std;
using namespace __gnu_cxx;
using namespace boost;
struct name_t {
  typedef boost::edge_property_tag kind;
};

typedef tuple<double, string, string, string> tuptype;

tuple<double, string, string, string>  tup(0.1, string("a"), string("b"), string("c"));
void sub2(tuptype& t){
    get<0>(t) = 2;
}
void sub1 (tuptype t){
    sub2(t);
    std::cout<<t<<endl;
}

int main()
{

    //get<0>(tup) = 2;

    sub1(tup);
    std::cout<<tup<<endl;
    return 0;

    // BEGIN convert the lattice tree (spans) into the the lattice graph.
  typedef std::pair<int, int> Edge;
  Edge used_by[] = {
    Edge(0, 1), 
    Edge(1, 2), 
    Edge(0, 2),
    Edge(2, 3),
  };


  typedef boost::property<name_t, std::string> ep;

  typedef adjacency_list<vecS, vecS, bidirectionalS, no_property, ep> Graph;
  Graph g(used_by, used_by + sizeof(used_by) / sizeof(Edge), 4);

  boost::graph_traits<Graph>::vertex_descriptor uu, vv;
  boost::graph_traits<Graph>::edge_descriptor ed ;
  bool b;
  uu=vertex(0, g);
  vv=vertex(3,g);
  tie(ed, b) = add_edge(uu,vv,  g);
  put(name_t(), g, ed, "wei");

  property_map<Graph, name_t> :: type  name = get(name_t(), g);




  // END convert


  // get the list of left edges and right edges of vertext i
  int i = 2;

  typedef graph_traits<Graph>::vertex_descriptor Vertex;

  Vertex v = *(vertices(g).first+i);

  typedef property_map<Graph, vertex_index_t>::type IndexMap;
  IndexMap index = get(vertex_index, g);


  // In edges
  typedef graph_traits<Graph> :: in_edge_iterator in_edge_iterator;
  in_edge_iterator edgei1, edgei2;
  for(tie(edgei1, edgei2) = in_edges(v, g); edgei1 != edgei2; ++edgei1){
      std::cout<<index[source(*edgei1,g)]<<std::endl;
      // use the index pairs to access to the original lattice edge so 
      // as to get the source word.
  }

  // Out edges
  typedef graph_traits<Graph> :: out_edge_iterator out_edge_iterator;
  out_edge_iterator edgeo1, edgeo2;
  for(tie(edgeo1, edgeo2) = out_edges(uu, g); edgeo1 != edgeo2; ++edgeo1){
      std::cout<<index[target(*edgeo1,g)]<<std::endl;
      std::cout<<"AA: "<<name[*edgeo1]<<endl;
      // use the index pairs to access to the original lattice edge so 
      // as to get the source word.
  }

}
