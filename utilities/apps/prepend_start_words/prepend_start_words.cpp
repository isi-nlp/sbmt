# include <gusc/lattice/grammar.hpp>
# include <gusc/lattice/ast.hpp>
# include <gusc/lattice/ast_construct.hpp>
# include <boost/spirit.hpp>
# include <boost/graph/adjacency_list.hpp>
# include <boost/graph/properties.hpp>
# include <boost/tuple/tuple_comparison.hpp>
# include <boost/program_options.hpp>

using namespace gusc;
using namespace boost;
using namespace boost::spirit;
using namespace std;
namespace po = boost::program_options;


struct dag_tree_node;
typedef boost::shared_ptr<dag_tree_node> dag_tree_node_ptr;

typedef tuple<int,int> mdpt_t;

mdpt_t justafter(int x) { return mdpt_t(x, 1); }
mdpt_t at(int x) { return mdpt_t(x, 0); }
mdpt_t justbefore(int x) { return mdpt_t(x - 1, 1); }

string escape_quotes(string const& str)
{
    string retval;
    string::const_iterator i = str.begin(), e = str.end();
    for (; i != e; ++i) {
        if(*i == '"' || *i == '\\') {
            retval.push_back('\\');
        }
        retval.push_back(*i);
    }
    return retval;
}

typedef property<edge_name_t, dag_tree_node_ptr> dag_tree_edge_property;
typedef property<vertex_name_t, mdpt_t> dag_tree_vertex_property;

typedef boost::adjacency_list<
          boost::listS
        , boost::listS
        , boost::bidirectionalS
        , dag_tree_vertex_property
        , dag_tree_edge_property
        > dag_tree_graph;

struct dag_tree_node {
    lattice_ast::const_line_iterator rt;
    mdpt_t from;
    mdpt_t to;
    std::string label;
    bool itrvalid;
    dag_tree_node(lattice_ast::const_line_iterator rt)
      : rt(rt)
      , from(at(rt->span().from()))
      , to(at(rt->span().to()))
      , label(rt->label())
      , itrvalid(true) {}
    dag_tree_node(mdpt_t from, mdpt_t to, string label)
      : from(from)
      , to(to)
      , label(label)
      , itrvalid(false) {}
    dag_tree_node() : itrvalid(false) {}
    dag_tree_node(lattice_ast::const_line_iterator rt, mdpt_t from, mdpt_t to)
      : rt(rt)
      , from(from)
      , to(to)
      , label(rt->label())
      , itrvalid(true) {}
    dag_tree_graph graph;
};

////////////////////////////////////////////////////////////////////////////////

template <class Mp, class G, class X>
typename graph_traits<G>::vertex_descriptor
memoized_vertex(Mp& mp, G& g, X const& x)
{
    typename Mp::iterator pos = mp.find(x);
    if (pos == mp.end()) {
        typename boost::graph_traits<G>::vertex_descriptor v = add_vertex(x,g);
        mp.insert(std::make_pair(x,v));
        return v;
    }
    else return pos->second;
}

////////////////////////////////////////////////////////////////////////////////

dag_tree_node_ptr dag_tree(lattice_ast::const_line_iterator ln, string startword);

////////////////////////////////////////////////////////////////////////////////

dag_tree_graph dag_tree( lattice_ast::const_line_iterator i
                       , lattice_ast::const_line_iterator e
                       , string startword )
{
    typedef graph_traits<dag_tree_graph>::vertex_descriptor
            vertex_t;
    typedef graph_traits<dag_tree_graph>::edge_descriptor
            edge_t;
    map<mdpt_t,vertex_t> mp;
    dag_tree_graph graph;
    for (; i != e; ++i) {
        dag_tree_node_ptr n = dag_tree(i,startword);
        //clog << "add_edge "<<at(from)<<" -> "<<at(to) << "\n";
        add_edge( memoized_vertex(mp,graph,at(i->span().from()))
                , memoized_vertex(mp,graph,at(i->span().to()))
                , n
                , graph
                )
                ;
    }
    return graph;
}

////////////////////////////////////////////////////////////////////////////////

void cut_around( graph_traits<dag_tree_graph>::vertex_descriptor v
               , dag_tree_graph& g
               , string startword
               )
{
    typedef graph_traits<dag_tree_graph>::edge_descriptor edge_t;
    typedef graph_traits<dag_tree_graph>::vertex_descriptor vertex_t;

    graph_traits<dag_tree_graph>::out_edge_iterator ei, ee;
    tie(ei,ee) = out_edges(v,g);

    int from = get(vertex_name_t(),g,v).get<0>();
    mdpt_t after = justafter(from);
    vertex_t ja;
    bool added = false;

    for (;ei != ee; ++ei) {
        dag_tree_node_ptr nptr_o = get(edge_name_t(),g,*ei);
        dag_tree_node_ptr nptr(new dag_tree_node(nptr_o->rt,after,nptr_o->to));
        if (!nptr->itrvalid || !nptr->rt->is_block()) {
            if (!added) {
                ja = add_vertex(after,g);
                //clog<<"add_start_edge "<<at(from)<<" -> "<<after<<"\n";
                added = true;
            }
            add_edge(ja,target(*ei,g),nptr,g);
            //clog<<"add_edge_from_start "<<get(vertex_name_t(),g,source(*ei,g))<<" -> "<<get(vertex_name_t(),g,target(*ei,g))<<"\n";
        }
    }
    if (added) {
        dag_tree_node_ptr nd(new dag_tree_node(at(from),after,startword));
        add_edge(v,ja,nd,g);
    }
}

////////////////////////////////////////////////////////////////////////////////

bool docutaround(dag_tree_graph const& g, std::string startword)
{
    graph_traits<dag_tree_graph>::edge_iterator ei, ee;
    tie(ei,ee) = edges(g);
    for (;ei != ee; ++ei) {
        dag_tree_node_ptr np = get(edge_name_t(),g,*ei);
        if (np->label == startword) {
            return false;
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

dag_tree_node_ptr dag_tree(lattice_ast::const_line_iterator ln, string startword)
{
    dag_tree_node_ptr nd(new dag_tree_node(ln));
    if (ln->is_block()) {
        lattice_ast::const_line_iterator i, e;
        tie(i,e) = ln->lines();
        nd->graph = dag_tree(i,e,startword);
    }
    graph_traits<dag_tree_graph>::vertex_iterator vi,ve;
    tie(vi,ve) = vertices(nd->graph);

    if (docutaround(nd->graph,startword)) for (; vi != ve; ++vi) {
        if (in_degree(*vi,nd->graph) == 0) {
            cut_around(*vi,nd->graph,startword);
        }
    }
    return nd;
}


////////////////////////////////////////////////////////////////////////////////

dag_tree_node_ptr dag_tree(lattice_ast const& ast, string startword)
{
    dag_tree_node_ptr nd(new dag_tree_node());
    lattice_ast::const_line_iterator i,e;
    tie(i,e) = ast.lines();
    nd->graph = dag_tree(i,e,startword);
    graph_traits<dag_tree_graph>::vertex_iterator vli,vle;
    tie(vli,vle) = vertices(nd->graph);
    typedef list<graph_traits<dag_tree_graph>::vertex_descriptor> vertex_list_t;
    vertex_list_t vl(vli,vle);
    vertex_list_t::iterator vi = vl.begin(), ve = vl.end();
    if (docutaround(nd->graph,startword)) for (; vi != ve; ++vi) {
        if (in_degree(*vi,nd->graph) == 0) {

            mdpt_t to = get(vertex_name_t(),nd->graph,*vi).get<0>();
            mdpt_t from = justbefore(to.get<0>());
            graph_traits<dag_tree_graph>::vertex_descriptor vfrom =
                add_vertex(from,nd->graph);

            dag_tree_node_ptr nptr(new dag_tree_node(from,to,startword));
            add_edge(vfrom,*vi,nptr,nd->graph);
        }
    }
    return nd;
}

////////////////////////////////////////////////////////////////////////////////

lattice_ast const&
get_ast (lattice_ast_construct const& g)
{
    return g.ast;
};

void vertex_map(dag_tree_node_ptr nd, set<mdpt_t>& s)
{
    graph_traits<dag_tree_graph>::edge_iterator i,e;
    tie(i,e) = edges(nd->graph);
    for (; i != e; ++i) {
        dag_tree_node_ptr n = get(edge_name_t(),nd->graph,*i);
        s.insert(n->from);
        s.insert(n->to);
        vertex_map(n,s);
    }
}

map<mdpt_t,size_t> vertex_map(dag_tree_node_ptr nd)
{
    set<mdpt_t> s;
    map<mdpt_t,size_t> m;
    vertex_map(nd,s);

    set<mdpt_t>::iterator i = s.begin(), e = s.end();
    size_t x = 0;
    for (; i != e; ++i,++x) m.insert(make_pair(*i,x));
    return m;
}

std::ostream& operator<<(std::ostream& out, boost::tuple<string,string> const& t)
{
    return out << t.get<0>() << "=\"" << escape_quotes(t.get<1>()) << "\"";
}

template <class LatOrBlock>
lattice_ast::line_iterator
edge_or_block(LatOrBlock& lb, map<mdpt_t,size_t>& mp, dag_tree_node_ptr nd)
{
    lattice_ast::line_iterator pos;
    if (!nd->itrvalid || !nd->rt->is_block()) {
        pos = lb.insert_edge(lattice_ast::vertex_pair(mp[nd->from],mp[nd->to]),nd->label);
    } else if (nd->itrvalid && nd->rt->is_block()) {
        pos = lb.insert_block();
    }
    return pos;
}

void start_word_dag_internal(lattice_ast::line_iterator pos, map<mdpt_t,size_t>& mp, dag_tree_node_ptr nd)
{
    if (nd->itrvalid) {
        lattice_ast::const_property_iterator pi,pe;
        tie(pi,pe) = nd->rt->properties();
        for (; pi != pe; ++pi) {
            pos->insert_property(pi->key(),pi->value());
        }
    }
    if (nd->itrvalid && nd->rt->is_block()) {
        graph_traits<dag_tree_graph>::edge_iterator ei,ee;
        tie(ei,ee) = edges(nd->graph);
        for (; ei != ee; ++ei) {
            dag_tree_node_ptr n = get(edge_name_t(),nd->graph,*ei);
            lattice_ast::line_iterator p = edge_or_block(*pos,mp,n);
            start_word_dag_internal(p,mp,n);
        }
    }
}

lattice_ast start_word_dag(lattice_ast const& g, string startword)
{
    dag_tree_node_ptr nptr = dag_tree(g,startword);
    lattice_ast ast;
    map<mdpt_t,size_t> mp = vertex_map(nptr);

    lattice_ast::const_property_iterator pi,pe;
    tie(pi,pe) = g.properties();
    for (;pi != pe; ++pi) ast.insert_property(pi->key(),pi->value());

    graph_traits<dag_tree_graph>::edge_iterator ei,ee;
    tie(ei,ee) = edges(nptr->graph);
    for (;ei != ee; ++ei) {
        dag_tree_node_ptr nd = get(edge_name_t(),nptr->graph,*ei);
        lattice_ast::line_iterator pos = edge_or_block(ast,mp,nd);
        start_word_dag_internal(pos,mp,nd);
    }
    return ast;
}


struct print_with_start_symbols {
    template <class O, class G, class S, class S2>
    struct result { typedef void type; };
    template <class O, class G, class S, class S2>
    void operator()(O& o, G const& g, S s, S2 sep) const
    {
        print_dag(o,get_ast(g),s,sep);
    }
};

struct options {
    string start_token;
    bool multi_line;
    options() : start_token("<foreign-sentence>"), multi_line(false) {}
};

options parse_options(int argc, char** argv)
{
    options opts;
    po::options_description desc;
    desc.add_options()
         ( "help,h"
         , "print this message"
         )
         ( "multi-line"
         , po::bool_switch(&opts.multi_line)
         , "if set, write lattice across multiple lines"
         )
         ( "start-token,s"
         , po::value(&opts.start_token)->default_value("<foreign-sentence>")
         , "start word to prepend to blocks and lattice for gluing"
         )
         ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv,desc),vm);
    po::notify(vm);
    if (vm.count("help")) {
        cerr << desc << endl;
        exit(1);
    }
    return opts;
}

int main(int argc, char** argv)
{
    options opts = parse_options(argc,argv);
    clog << "start-token: " << opts.start_token << endl;
    std::string sep = opts.multi_line ? "\n" : "";

    lattice_ast ast;
    while (getlattice(cin,ast)) {
        cout << start_word_dag(ast,opts.start_token) << endl;
    }
    /*
    using namespace phoenix;
    phoenix::function<print_with_start_symbols> p;

    typedef multi_pass< istreambuf_iterator<char> > mp_iterator_t;
    typedef position_iterator<mp_iterator_t> iterator_t;


    mp_iterator_t mp_first = make_multi_pass(istreambuf_iterator<char>(cin));
    mp_iterator_t mp_last  = make_multi_pass(istreambuf_iterator<char>());

    iterator_t first(mp_first,mp_last);
    iterator_t last;

    lattice_grammar<lattice_ast_construct> dag;
    parse_info<iterator_t> info
        = parse(
            first
          , last
          , +(   //dag[ cout << p(arg1) << endl ]
                 dag[ p(var(cout),arg1,opts.start_token,sep) ]
                 >> ';'
                 >> flush_multi_pass_p
             )
          , space_p | comment_p("#")
          )
          ;

    if (!info.full) {
        stringstream sstr;
        sstr << "parse failure at line: "<< info.stop.get_position().line
             << " and column: " << info.stop.get_position().column;
        throw runtime_error(sstr.str());
    } */
}
