#include <iostream> // std::cout
#include <utility> // std::pair
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <sbmt/span.hpp>
#include <vector>
#include <map>
#include <ext/hash_set>
#include <string>
#include "ww/ww_utils.hpp"
#include "constituent_list.hpp"
#include <boost/regex.hpp>

using namespace std;
using namespace __gnu_cxx;
using namespace boost;
using namespace source_structure;

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

class A
{
public:

    sbmt::span_t s;

};


bool overlap(sbmt::span_t s1, sbmt::span_t s2)
{
    if((s1.left() < s2.left() && s1.right() > s2.left() && s1.right() < s2.right()) || s1.left() > s2.left() && s1.left() < s2.right() && s1.right() > s2.right()){
        return true;
    } else {
        return false;
    }

}

int main()
{
    // remove the splits.
    boost::regex e("(-\\d+\\s*$)|(-\\d+-BAR\\s*$)");
    const char* fmt="(?1)(?2-BAR)";
    string constituent_e_label="NP-C-44-BAR";
    string sss="NP-C-44";
    ostringstream olabel;
    boost::regex_replace(ostream_iterator<char,char>(olabel), constituent_e_label.begin(), constituent_e_label.end(), e, 
                    fmt, boost::match_default | boost::format_all);
    cout<<olabel.str()<<endl;
    boost::regex_replace(ostream_iterator<char,char>(olabel), sss.begin(), sss.end(), e, 
                    fmt, boost::match_default | boost::format_all);
    cout<<olabel.str()<<endl;
    return 0;


    constituent_list cl("NP[1,3] NPB[1,3] NPB[2,6]");
    cl.dump();
    return 0;


    sbmt::span_t ss1(1,5);
    sbmt::span_t ss2(0,5);
    std::cout<<ss1<<" "<<ss2<<" ";
    if(overlap(ss1,ss2)){
        std::cout<<"Overlap"<<std::endl;
    } else {
        std::cout<<"NO-Overlap"<<std::endl;

    }
    return 0;

    A aa, bb;
    aa.s=sbmt::span_t(1, 5);
    bb=aa;
    std::cout<<bb.s.left()<<" "<<bb.s.right()<<std::endl;


    sbmt::span_t s1, s2(1, 6);

    s1=s2;

    std::cout<<s1<<std::endl;

    __gnu_cxx::hash_set<std::string> ss;

    ss.insert("a");

    map<sbmt::span_t, int> mm;
    mm[sbmt::span_t(9, 17)] = 1;
    mm[sbmt::span_t(3, 6)] = 1;
    mm[sbmt::span_t(0, 5)] = 1;
    mm[sbmt::span_t(1, 6)] = 1;
    mm[sbmt::span_t(1, 5)] = 1;
    mm[sbmt::span_t(6, 7)] = 1;

    map<sbmt::span_t, int>::const_iterator it;
    for(it = mm.begin(); it != mm.end(); ++it){
        std::cout<<it->first<<endl;

    }

    return 0;

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
