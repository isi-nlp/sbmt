# include <sbmt/search/lattice_reader.hpp>
# include <sbmt/search/block_lattice_tree.hpp>
# include <gusc/lattice/grammar.hpp>
# include <boost/graph/transitive_closure.hpp>
# include <sbmt/token.hpp>
# include <boost/spirit/include/classic_core.hpp>
# include <boost/spirit/include/classic_flush_multi_pass.hpp>
# include <boost/spirit/include/classic_multi_pass.hpp>
# include <boost/spirit/include/classic_position_iterator.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_comparison.hpp>
# include <boost/foreach.hpp>
# include <boost/graph/copy.hpp>
# include <iostream>
# include <sstream>
# include <stdexcept>
# include <vector>
# include <sbmt/hash/hash_map.hpp>
# include <boost/functional/hash.hpp>

using namespace sbmt;
using namespace boost;
using namespace boost::tuples;
using namespace boost::spirit::classic;
using namespace boost::graph;
using namespace ::phoenix;
using namespace std;

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
//
//  create the transitive closure graph, with "x" as the label for each edge,
//  and union it with the original graph
//
////////////////////////////////////////////////////////////////////////////////
graph_t skip_lattice(graph_t const& g, indexed_token_factory& dict)
{
    using boost::make_tuple;

    wildcard_array wc(dict);
    graph_t tc = graph_t(get_property(g));
    //tc.m_property = g.m_property;
    copy_graph(g,tc);

    typedef graph_traits<graph_t>::vertex_descriptor vertex_t;
    std::vector<vertex_t> topo;
    topological_sort(tc,std::back_inserter(topo));

    typedef std::pair<vertex_t,vertex_t> edge_t;
    typedef stlext::hash_map< edge_t, size_t, boost::hash<edge_t> > distance_map;
    distance_map distance;

    for (size_t y = 0; y != topo.size(); ++y) {
        for (size_t x = 0; x != topo.size(); ++x) {
            distance.insert(std::make_pair(edge_t(topo[x],topo[y]),size_t(0)));
        }
    }

    for (size_t y = 0; y != topo.size(); ++y) {
        vertex_t ty = topo[y];
        for (size_t x = y + 1; x != topo.size(); ++x) {
            vertex_t tx = topo[x];
            graph_traits<graph_t>::out_edge_iterator itr, end;
            tie(itr,end) = out_edges(tx,tc);

            size_t dist = distance[edge_t(tx,ty)];
            //std::cerr << "distance " << tc[tx] << " -> " << tc[ty] << "\n";
            //std::cerr << "max:\n";
            for (; itr != end; ++itr) {
                vertex_t tz = target(*itr,tc);
                //std::cerr << "\t" << tc[tx] << " -> " << tc[tz] << std::endl;
                distance[edge_t(tx,tz)] = std::max(size_t(1),distance[edge_t(tx,tz)]);
                size_t dzy = distance[edge_t(tz,ty)];
                if (dzy > 0 or (tz == ty)) {
                    //std::cerr << "\t\t1 + distance(" << tc[tz]<<","<<tc[ty]<<")\n";
                    dist = std::max(dist,1 + dzy);
                }
            }
            distance[edge_t(tx,ty)] = dist;
        }
    }
    //std::cerr << std::flush;

    for(distance_map::iterator p = distance.begin(); p != distance.end(); ++p) {
        size_t d = p->second;
        if (d > 0) {
            vertex_t from,to;
            tie(from,to) = p->first;
            add_edge(from,to,wc[d],tc);
        }
    }

    return tc;
}

////////////////////////////////////////////////////////////////////////////////

typedef boost::tuple<string,string> string_pair_t;
typedef boost::tuple<unsigned,unsigned> uint_pair_t;
typedef vector<string_pair_t> feature_vec_t;

struct dag_construct {

    graph_traits<graph_t>::vertex_descriptor
    memoize_vtx(unsigned x)
    {
        map_t::iterator pos = vmap.find(x);
        if (pos == vmap.end()) {
            pos = vmap.insert(make_pair(x,add_vertex(graph))).first;
            graph[pos->second] = x;
            //put(vertex_index_t(),graph,pos->second,x);
        }
        return pos->second;
    }

public:
    dag_construct(boost::tuple<indexed_token_factory&, size_t&> init)
    : tf(&init.get<0>())
    , empty_block(true)
    {
        get_property(graph).id = init.get<1>();
        //graph.m_property.id = init.get<1>();
        //set_property(graph,graph_name_t(),init.get<1>());
    }

    template <class Itr>
    void properties(Itr feat_beg, Itr feat_end)
    {
        Itr i = feat_beg, e = feat_end;
        for (; i != e; ++i) {
            if (i->get<0>() == "id") {
                size_t id = lexical_cast<size_t>(i->get<1>());
	            get_property(graph).id = id;
                //graph.m_property.id = id;
                //set_property(graph,graph_name_t(),id);
                break;
            }
        }
    }

    template <class Itr>
    void begin_block(Itr beg, Itr end)
    {
        empty_block = true;
    }

    void end_block()
    {
        if (not empty_block) {
	    //graph.m_property.brackets.insert(sbmt::span_t(m,M));
	    get_property(graph).brackets.insert(sbmt::span_t(m,M));
        }
    }

    template <class Itr>
    void new_vertex(unsigned x, string lbl, Itr feat_beg, Itr feat_end)
    {
        memoize_vtx(x);
    }
    
    void new_edge(uint_pair_t t, string lbl)
    {
        if (empty_block) {
            m = t.get<0>();
            M = t.get<1>();
            empty_block = false;
        } else {
            m = std::min(m,t.get<0>());
            M = std::max(M,t.get<1>());
        }
        graph_traits<graph_t>::vertex_descriptor src = memoize_vtx(t.get<0>());
        graph_traits<graph_t>::vertex_descriptor tgt = memoize_vtx(t.get<1>());

        graph_traits<graph_t>::edge_descriptor e = add_edge(src,tgt,graph).first;
        graph[e] = tf->foreign_word(lbl);
    }
    
    template <class Itr>
    void new_edge(uint_pair_t t, string lbl, Itr feat_begin, Itr feat_end)
    {
        
        if (empty_block) {
            m = t.get<0>();
            M = t.get<1>();
            empty_block = false;
        } else {
            m = std::min(m,t.get<0>());
            M = std::max(M,t.get<1>());
        }
        graph_traits<graph_t>::vertex_descriptor src = memoize_vtx(t.get<0>());
        graph_traits<graph_t>::vertex_descriptor tgt = memoize_vtx(t.get<1>());

        graph_traits<graph_t>::edge_descriptor e = add_edge(src,tgt,graph).first;
        graph[e] = tf->foreign_word(lbl);
        for (Itr feat_itr = feat_begin; feat_itr != feat_end; ++feat_itr) {
            if (feat_itr->get<0>() == "type") {
                if (feat_itr->get<1>() == "virt") {
                    graph[e] = tf->virtual_tag(lbl);
                }
            }
        }
        //put(edge_name_t(),graph,e,tf->foreign_word(lbl));
    }

    indexed_token_factory* tf;
    graph_t graph;
    typedef map< unsigned int
               , graph_traits<graph_t>::vertex_descriptor
               > map_t;
    map_t vmap;

    bool empty_block;
    unsigned int m, M;
};


struct to_dag_visitor {
    size_t dummy;
    dag_construct* dcon;
    to_dag_visitor(indexed_token_factory& tf) 
    : dummy(0)
    , dcon(new dag_construct(boost::tie(tf,dummy))) {}
    ~to_dag_visitor() { delete dcon; }
    void operator()(lattice_edge const& le)
    {
        dcon->new_edge( uint_pair_t(le.span.left(),le.span.right())
                      , dcon->tf->label(le.source)
                      );
    }
};


void to_dag(gusc::lattice_ast::line const& line, dag_construct& dc)
{
    if (line.is_block()) {
        BOOST_FOREACH(gusc::lattice_ast::line const& line, line.lines()) {
            to_dag(line,dc);
        }
    } else {
        std::vector<boost::tuple<std::string,std::string> > v;
        BOOST_FOREACH(gusc::property_container_interface::key_value_pair kv, line.properties()) {
            v.push_back(boost::make_tuple(kv.key(),kv.value()));
        }
        dc.new_edge( boost::make_tuple(line.span().from(),line.span().to())
                   , line.label()
                   , v.begin()
                   , v.end() );
    }
}

graph_t to_dag(gusc::lattice_ast const& lat, indexed_token_factory& tf)
{
    size_t id;
    dag_construct dc(boost::tie(tf,id));
    gusc::lattice_ast::const_line_iterator li,le;
    boost::tie(li,le) = lat.lines();
    BOOST_FOREACH(gusc::lattice_ast::line const& line, lat.lines()) {
        to_dag(line,dc);
    }
    return dc.graph;
}

graph_t to_dag(lattice_tree const& ltree, indexed_token_factory& tf)
{
    to_dag_visitor vis(tf);
    ltree.visit_edges(vis);
    return vis.dcon->graph;
}


////////////////////////////////////////////////////////////////////////////////

void found_lattice(dag_construct const& d, lattice_callback f, size_t& id)
{
    //id = d.graph.m_property.id + 1;
    id = get_property(d.graph).id + 1;
    f(d.graph);
}

////////////////////////////////////////////////////////////////////////////////

void lattice_reader( istream& in
                   , lattice_callback f
                   , indexed_token_factory& tf )
{

    typedef multi_pass< istreambuf_iterator<char> > mp_iterator_t;
    typedef position_iterator<mp_iterator_t> iterator_t;

    size_t id = 1;

    mp_iterator_t mp_first = make_multi_pass(istreambuf_iterator<char>(in));
    mp_iterator_t mp_last  = make_multi_pass(istreambuf_iterator<char>());

    iterator_t first(mp_first,mp_last);
    iterator_t last;

    gusc::lattice_grammar<dag_construct> dag;
    parse_info<iterator_t> info
        = parse(
            first
          , last
          , *(
                 dag(tie(tf,id))[ bind(&found_lattice)(arg1,f,var(id)) ]
                 >> !ch_p(';')
                 >> flush_multi_pass_p
             )
          , space_p | comment_p("//") | comment_p("/*","*/")
          )
          ;

    if (not info.hit) {
        stringstream sstr;
        sstr << "parse failure at line: "<< info.stop.get_position().line
             << " and column: " << info.stop.get_position().column
             << "\n" << in.rdbuf();
        throw runtime_error(sstr.str());
    }
}

template <class Graph>
void fill( Graph const& skip_lattice
         , indexed_token_factory& tf
         , lex_outside_distances& lod
         , typename boost::graph_traits<Graph>::vertex_descriptor v )
{
    lod[tf.toplevel_tag()].insert(0);
    BOOST_FOREACH( typename graph_traits<Graph>::edge_descriptor e
                 , out_edges(v,skip_lattice) ) 
    {
        indexed_token lbl = skip_lattice[e];
        if (is_virtual_tag(lbl)) {
            int distance = boost::lexical_cast<int>(tf.label(lbl));
            lod[tf.toplevel_tag()].insert(distance);
            BOOST_FOREACH( typename graph_traits<Graph>::edge_descriptor te
                         , out_edges(target(e,skip_lattice), skip_lattice) )
            {
                indexed_token word = skip_lattice[te];
                if (not is_virtual_tag(word)) {
                    lod[word].insert(distance);
                    
                }
            }
        } else {
            lod[lbl].insert(0);
        }
    }
}

left_right_distance_map
make_left_right_distance_map(graph_t const& skip_lattice, indexed_token_factory& tf)
{
    using boost::graph_traits;
    left_right_distance_map lrdm;
    BOOST_FOREACH( graph_traits<graph_t>::vertex_descriptor v
                 , vertices(skip_lattice) )
    {
        int vertex_id = skip_lattice[v];
        lex_outside_distances& rod = lrdm[vertex_id].get<1>();
        fill(skip_lattice,tf,rod,v);
        lex_outside_distances& lod = lrdm[vertex_id].get<0>();
        fill(boost::make_reverse_graph(skip_lattice),tf,lod,v);
    }
    return lrdm;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
