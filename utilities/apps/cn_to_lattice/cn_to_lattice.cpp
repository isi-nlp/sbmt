//# define BOOST_SPIRIT_DEBUG 1
#define  PHOENIX_LIMIT 6
#define  PHOENIX_CONSTRUCT_LIMIT 6
#define  BOOST_SPIRIT_CLOSURE_LIMIT 6
# include <boost/graph/adjacency_list.hpp>
# include <boost/graph/properties.hpp>
# include <boost/spirit.hpp>
# include <gusc/phoenix_helpers.hpp>
# include <boost/tuple/tuple.hpp>

using namespace boost;
using namespace boost::spirit;
using namespace std;

struct scored_alt {
    string label;
    double score;
    scored_alt() {}
    scored_alt(string label, double score) : label(label), score(score) {}
};

struct cngram_c : closure<cngram_c, list< list<scored_alt> > > {
    member1 val;
};

struct cngram : grammar<cngram, cngram_c::context_t> {

    struct alt_c : closure<alt_c, scored_alt, string, double> {
        member1 val;
        member2 label;
        member3 score;
    };

    struct alt_list_c : closure< alt_list_c, list<scored_alt> > {
        member1 val;
    };

    template <class ST> struct definition {
        rule<ST> seq;//, featvec, primary, alt, alts;
        typedef typename lexeme_scanner<ST>::type LT;
        rule<LT> featvec;
        rule<LT,alt_c::context_t> primary, alt;
        rule<LT,alt_list_c::context_t> alts;

        definition(cngram const& g)
        {
            using namespace phoenix;
            featvec
                = '('
                >>  list_p(real_p,',')
                >> ')'
                ;

            primary
                =
                (
                    (
                        (+(anychar_p - '='))
                        [
                            primary.label = construct_<string>(arg1,arg2)
                        ]
                    )
                    >> '='
                    >> real_p[ primary.score = arg1 ]
                )
                [
                    primary.val = construct_<scored_alt>( primary.label
                                                        , primary.score
                                                        )
                ]
                ;

            alt
                = primary [ alt.val = arg1 ]
                >> ','
                >> featvec
                ;

            alts
                = alt[ gusc::push_back_(alts.val,arg1) ]
                >> *( '|' >> alt [ gusc::push_back_(alts.val,arg1) ] )
                ;
            seq = + lexeme_d[ alts[ gusc::push_back_(g.val,arg1) ] ];
            # if BOOST_SPIRIT_DEBUG
            BOOST_SPIRIT_DEBUG_RULE(seq);
            BOOST_SPIRIT_DEBUG_RULE(featvec);
            BOOST_SPIRIT_DEBUG_RULE(primary);
            BOOST_SPIRIT_DEBUG_RULE(alt);
            BOOST_SPIRIT_DEBUG_RULE(alts);
            # endif
        }

        rule<ST> start() const { return seq; }
    };
};



////////////////////////////////////////////////////////////////////////////////

typedef property<vertex_name_t,unsigned int> vertex_property_t;
typedef property<edge_name_t,scored_alt> edge_property_t;
typedef adjacency_list< listS
                      , listS
                      , bidirectionalS
                      , vertex_property_t
                      , edge_property_t > graph_t;

////////////////////////////////////////////////////////////////////////////////

graph_t dag_from_cn( list<list<scored_alt> > const& cn
                   , string eps = "*DELETE*"
                   , string fs = "<foreign-sentence>"
                   )
{
    typedef graph_traits<graph_t>::vertex_descriptor vertex_t;
    typedef graph_traits<graph_t>::edge_descriptor edge_t;
    typedef graph_traits<graph_t>::in_edge_iterator in_edge_iterator;
    graph_t g;
    vertex_t prev = add_vertex(0,g);
    vertex_t curr = add_vertex(1,g);
    add_edge(prev,curr,scored_alt(fs,1.0),g);
    size_t x = 1;
    list<list<scored_alt> >::const_iterator oi = cn.begin(), oe = cn.end();
    for (; oi != oe; ++oi) {
        vertex_t next = add_vertex(++x,g);
        list<scored_alt>::const_iterator i = oi->begin(), e = oi->end();
        for (; i != e; ++i) {
            scored_alt alt = *i;
            if (alt.label != eps) {
                add_edge(curr,next,alt,g);
            } else {
                in_edge_iterator ei, ee;
                tie(ei,ee) = in_edges(curr,g);
                for (;ei != ee; ++ei) {
                    scored_alt a = get(edge_name_t(), g, *ei);
                    add_edge( source(*ei,g)
                            , next
                            , scored_alt(a.label, a.score * alt.score)
                            , g
                            )
                            ;
                }
            }
        }
        curr = next;
    }
    return g;
}

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

void print_d(std::ostream& os, int idx, graph_t const& g, string sep = "\n")
{
    os << "lattice id=\""<< idx << "\" { " <<  sep;
    graph_traits<graph_t>::edge_iterator i, e;
    tie(i,e) = edges(g);
    for (; i != e; ++i) {
        size_t s = get(vertex_name_t(),g,source(*i,g));
        size_t t = get(vertex_name_t(),g,target(*i,g));
        scored_alt a = get(edge_name_t(),g,*i);
        os<<'['<<s<<','<<t<<"] \""<<a.label<<"\" cn_conf=\""<<a.score<<"\"; "<<sep;
    }
    os << "};";
}

struct action_ph {
    template <class O, class I, class G, class S>
    struct result { typedef void type; };

    template <class O, class I, class G, class S>
    void operator()(O& o, I& i, G const& g, S const& s)
    {
        print_d(o,i,dag_from_cn(g),s);
        ++i;
    }
};

phoenix::function<action_ph> action_ = action_ph();

struct options {
    bool multi_line;
    int start_id;
    options() : multi_line(false), start_id(0) {}
};
/*
options parse_options(int argc, char** argv)
{
    options opts;
    return options;
}
*/

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    using namespace phoenix;
    string line;
    int id = 1;
    cngram g;
    while (getline(cin,line)) {
        parse_info<string::iterator> info = parse( line.begin()
                                                 , line.end()
                                                 , g
                                                   [
                                                       action_( var(cout)
                                                              , var(id)
                                                              , arg1
                                                              , string("")
                                                              )
                                                   ] >> eps_p >> flush_multi_pass_p
                                                 , space_p
                                                 );
        if (!info.hit) cerr << "failed to parse " << line << endl;
        else cout << endl;
    }
    return 0;
}
