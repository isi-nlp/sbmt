# if ! defined(SBMT__SEARCH__UNARY_APPLICATIONS_HPP)
# define       SBMT__SEARCH__UNARY_APPLICATIONS_HPP

# include <sbmt/edge/edge.hpp>
# include <sbmt/hash/hash_set.hpp>
# include <sbmt/search/unary_filter_interface.hpp>
# include <sbmt/search/logging.hpp>
# include <sbmt/io/log_auto_report.hpp>
# include <sbmt/grammar/sorted_rhs_map.hpp>
# include <sbmt/hash/ref_array.hpp>


# include <boost/graph/adjacency_list.hpp>
# include <boost/graph/depth_first_search.hpp>
# include <boost/graph/topological_sort.hpp>
# include <boost/graph/graphviz.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/shared_ptr.hpp>

# include <algorithm>
# include <vector>
# include <cassert>

namespace sbmt {
SBMT_REGISTER_CHILD_LOGGING_DOMAIN(unary_apps,search);
}

////////////////////////////////////////////////////////////////////////////////
//
// workaround for graphviz printing:  apple gcc really doesnt like 
//  conversions from void* to std::size_t
//
////////////////////////////////////////////////////////////////////////////////
template <class CT>
struct to_idx_property_map
: public boost::put_get_helper< std::size_t, 
                                to_idx_property_map<CT> >
{
    typedef CT key_type;
    typedef std::size_t value_type;
    typedef std::size_t reference;
    typedef boost::readable_property_map_tag category;

    value_type operator[](const key_type& v) const { return (value_type)(v); }
};

namespace sbmt {

template <class PropertyMap, class DictT> 
struct eq_printer {
    eq_printer(PropertyMap pmap, DictT& dict) 
    : pmap(pmap)
    , dict(&dict) {}
    
    template <class V>
    void operator()(std::ostream& out, V const& v)
    {
        out << "[ label = \"" << print(pmap[v].representative().root(),*dict) << "\" ]";
    }
    PropertyMap pmap;
    DictT* dict;
};

template <class PropertyMap,class Gram> 
struct id_printer {
    id_printer(PropertyMap pmap, Gram const& gram) 
    : pmap(pmap) 
    , gram(&gram){}
    
    template <class V>
    void operator()(std::ostream& out, V const& e)
    {
        out << "[ label = \"" << gram->get_syntax(pmap[e]).id() << "\" ]";
    }
    PropertyMap pmap;
    Gram const* gram;
};

template <class PropertyMap, class DictT>
eq_printer<PropertyMap,DictT> make_eq_printer( PropertyMap pmap
                                             , DictT& dict )
{
    return eq_printer<PropertyMap,DictT>(pmap,dict);
}

template <class PropertyMap, class DictT>
id_printer<PropertyMap,DictT> make_id_printer( PropertyMap pmap
                                             , DictT& dict )
{
    return id_printer<PropertyMap,DictT>(pmap,dict);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
class unary_applications {
public:
    typedef ET edge_type;
    typedef GT grammar_type;
    typedef typename GT::rule_type rule_type;
    typedef edge_equivalence<ET> edge_equiv_type;
    typedef boost::shared_ptr<unary_filter_interface<ET,GT> > filter_p;
    typedef boost::shared_ptr<unary_filter_factory<ET,GT> > factory_p;
    
    template <class ItrT>
    unary_applications( ItrT, ItrT
                      , GT&, sorted_rhs_map<GT> const&
                      , concrete_edge_factory<ET,GT>&
                      , factory_p
                      , span_t
                      , bool top = false);

private:    
    typedef oa_hashtable< edge_equivalence<ET>, representative_edge<ET> > 
            equiv_set_t;

public:                            
    typedef typename equiv_set_t::const_iterator const_iterator;
    typedef typename equiv_set_t::iterator iterator;
    
    iterator begin() { return equiv_set.begin(); }
    iterator end() { return equiv_set.end(); }
    const_iterator begin() const { return equiv_set.begin(); }
    const_iterator end() const { return equiv_set.end(); }
private:
    typedef boost::adjacency_list<
        boost::listS, boost::listS, boost::directedS
      , boost::property<boost::vertex_name_t,edge_equiv_type> // vertex property 
      , boost::property<boost::edge_name_t,rule_type> // edge property
    > graph_type;
    
    typedef typename graph_type::vertex_descriptor vertex_t;
    typedef typename graph_type::edge_descriptor graph_edge_t;
    
    typedef std::map<vertex_t, boost::default_color_type> 
            color_hash_map_t;
    typedef boost::associative_property_map<color_hash_map_t> color_map_t;
    
    typedef stlext::hash_map<ET,vertex_t,boost::hash<ET> > vertex_map_t; 
    
    /// dfs-visitor event.  abusing boost::depth_first_search a bit to 
    /// create the graph in dfs order while eliminating any back edges in
    /// the process.
    class creation_visitor {
    public:
        typedef boost::on_discover_vertex event_filter;
        
        creation_visitor( color_map_t& color_map
                        , GT& gram
                        , sorted_rhs_map<GT> const& rhs_map
                        , concrete_edge_factory<ET,GT>& ef
                        , filter_p filter
                        , vertex_map_t& vertex_map
                        , bool toplevel
                        , size_t& depth
                        , size_t maxdepth = 2)
        : color_map(&color_map)
        , gram(&gram)
        , rhs_map(&rhs_map)
        , ef(&ef)
        , filter(filter)
        , vertex_map(&vertex_map)
        , toplevel(toplevel)
        , depth(&depth)
        , maxdepth(maxdepth) {}
        
        void operator()(vertex_t v, graph_type const& g);
        
    private:
                            
        void apply_rules( edge_equiv_type eq 
                        , vertex_t v
                        , graph_type const& g
                        , typename sorted_rhs_map<GT>::iterator
                        , typename sorted_rhs_map<GT>::iterator );
        color_map_t* color_map;
        GT* gram;
        sorted_rhs_map<GT> const* rhs_map;
        concrete_edge_factory<ET,GT>* ef;
        boost::shared_ptr< unary_filter_interface<ET,GT> > filter;
        vertex_map_t* vertex_map;
        bool toplevel;
        size_t* depth;
        size_t maxdepth;
        static vertex_t vertex_for_et( edge_type const& e
                                     , graph_type& g
                                     , vertex_map_t& vmap
                                     , color_map_t& cmap );
        friend class unary_applications;
    };
    
    class finish_visitor {
    public:
        typedef boost::on_finish_vertex event_filter;
        finish_visitor(size_t& depth) : depth(&depth) {}
        void operator()(vertex_t v, graph_type const& g) { --(*depth); }
    private:
        size_t* depth;
    };
    
    rule_type const& getrule(typename boost::graph_traits<graph_type>::edge_descriptor e) const
    {
        typename boost::property_map<graph_type, boost::edge_name_t>::const_type 
            pmap = get(boost::edge_name_t(),graph);
        return pmap[e];
    }
    
    void apply_rules( GT& gram
                    , concrete_edge_factory<ET,GT>& ef
                    , boost::shared_ptr<unary_filter_interface<ET,GT> > filter );
    
    graph_type graph;
    vertex_map_t vertex_map;
    equiv_set_t equiv_set;
    color_hash_map_t m;
    color_map_t color_map;
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
template <class ItrT>
unary_applications<ET,GT>::unary_applications( ItrT b, ItrT e
                                             , GT& gram
                                             , sorted_rhs_map<GT> const& rhs_map
                                             , concrete_edge_factory<ET,GT>& ef
                                             , boost::shared_ptr<unary_filter_factory<ET,GT> > filts
                                             , span_t span
                                             , bool toplevel )
: color_map(m)
{
    using namespace boost;
    using namespace std;
    
    vector<edge_equivalence<ET> > start_edges(b,e);
    sort(start_edges.begin(),start_edges.end(),greater_edge_score<ET>());
    
    typename vector<edge_equivalence<ET> >::iterator itr = start_edges.begin(),
                                                     end = start_edges.end();
                                                 
    for(; itr != end; ++itr) {
        creation_visitor::vertex_for_et( itr->representative()
                                       , graph
                                       , vertex_map
                                       , color_map );
        equiv_set.insert(*itr);
    }
    if (not equiv_set.empty()) {

    io::logging_stream& log = io::registry_log(unary_apps);
    {
        io::log_time_report report(log,io::lvl_verbose,"dag-creation: ");
        size_t depth(0);
        depth_first_search( graph
                          , make_dfs_visitor( 
                                std::make_pair( 
                                    creation_visitor( color_map
                                                    , gram
                                                    , rhs_map
                                                    , ef
                                                    , filts->create_shared(span,gram,ef)
                                                    , vertex_map
                                                    , toplevel
                                                    , depth ) 
                                  , finish_visitor(depth)
                                )
                            )
                          , color_map );
        
        if (io::logging_at_level(log,io::lvl_verbose)) {
            to_idx_property_map<vertex_t> pm;
            write_graphviz( log << io::verbose_msg
                          , graph
                          , make_eq_printer(get(vertex_name_t(),graph),gram)
                          , make_id_printer(get(edge_name_t(),graph),gram)
                          , default_writer()
                          , pm );
        }
    }
    SBMT_VERBOSE_MSG_TO(
        log
      , "dag has %s nodes and %s edges"
      , num_vertices(graph)
      % num_edges(graph)
    ); // careful, num_vertices can be linear cost.
    
    {
        io::log_time_report report(log,io::lvl_verbose,"apply-rules: ");
        apply_rules(gram,ef,filts->create_shared(span,gram,ef));
    }
    }
}



template <class ET, class GT>
void unary_applications<ET,GT>::apply_rules( GT& gram
                                           , concrete_edge_factory<ET,GT>& ef
                                           , boost::shared_ptr<unary_filter_interface<ET,GT> > filter )
{
    using namespace boost;
    using namespace std;
    
    edge_equivalence_pool<ET> dummy_pool;
    
    vector<vertex_t> sorted_vertices;
    topological_sort( graph
                    , back_inserter(sorted_vertices)
                    , boost::color_map(color_map) );

    typename vector<vertex_t>::reverse_iterator vitr = sorted_vertices.rbegin(),
                                                vend = sorted_vertices.rend();

    for (; vitr != vend; ++vitr) {
        edge_equiv_type eq = get(vertex_name_t(),graph,*vitr);
        typename equiv_set_t::iterator pos = equiv_set.find(eq.representative());
        if (pos == equiv_set.end()) {
            pos = equiv_set.insert(eq).first;
        }
        typedef typename graph_traits<graph_type>::edge_descriptor graph_edge_t;
        typedef typename graph_traits<graph_type>::out_edge_iterator 
                         out_edge_iterator;
        pair<out_edge_iterator, out_edge_iterator> p = out_edges(*vitr,graph);
        
        typedef concrete_forward_iterator<rule_type const> rule_iterator;
        typedef boost::function<rule_type const& (graph_edge_t)> transform_t;
        typedef unary_applications<ET,GT> self_t;
        transform_t trans = boost::bind(&self_t::getrule,this,_1);
        rule_iterator ritr = transform_iterator<transform_t,out_edge_iterator,rule_type const&>(p.first,trans),
                      rend = transform_iterator<transform_t,out_edge_iterator,rule_type const&>(p.second,trans);
                      
        filter->apply_rules(*pos,boost::make_tuple(ritr,rend));
        //cerr << "from " << print(eq,gram) << endl; 
        while (not filter->empty()) {
            edge_type const& e = filter->top();
            //cerr << "\t to " << print(e,gram) << endl;
            //vertex_t vv = target(*p.first,graph);
            //edge_equiv_type toeq = get(vertex_name_t(),graph,vv);
            //assert(toeq.representative() == e);
            typename equiv_set_t::iterator epos = equiv_set.find(e);
            if (epos == equiv_set.end()) {
                //cerr << "\t\tno seen it\n";
                epos = equiv_set.insert(edge_equiv_type(e)).first;
            } else {
                //cerr << "\t\tseen it\n";
                const_cast<edge_equiv_type&>(*epos).insert_edge(e);
            }
            filter->pop();
        }
    }
    typename equiv_set_t::iterator eqitr = equiv_set.begin(),
                                   eqend = equiv_set.end();
    for (; eqitr != eqend; ++eqitr) {
        //cerr << "*** " << print(*eqitr,gram) << endl;
    }
}                                               

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
typename unary_applications<ET,GT>::vertex_t
unary_applications<ET,GT>::creation_visitor::vertex_for_et( edge_type const& e
                                                          , graph_type& g
                                                          , vertex_map_t& vmap
                                                          , color_map_t& cmap )
{
    using namespace boost;
    
    typename vertex_map_t::iterator pos = vmap.find(e);
    if (pos != vmap.end()) return pos->second;
    
    vertex_t v = add_vertex(g);
    put(vertex_name_t(),g,v,edge_equiv_type(e));
    put(cmap,v,color_traits<default_color_type>::white());
    insert_into_map(vmap, e, v);
    return v;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT> void
unary_applications<ET,GT>::creation_visitor::operator()( vertex_t v
                                                       , graph_type const& g )
{
    using namespace boost;
    
    if (*depth < maxdepth) {
        /// we create transition vertices before the rest of the dfs algorithm
        /// sees them.  apply_rules will rule out cycles from being created.
        edge_equiv_type eq = get(vertex_name_t(),g,v);
        assert (eq.size() == 1);
        typename sorted_rhs_map<GT>::iterator ritr, rend;
        indexed_token rt = eq.representative().root();
        boost::tie(ritr,rend) = rhs_map->rules(ref_array(rt));

        apply_rules(eq,v,g,ritr,rend);
        if (toplevel) {
            boost::tie(ritr,rend) = rhs_map->toplevel_rules(ref_array(rt));
            apply_rules(eq,v,g,ritr,rend);
        }
        assert (eq.size() == 1);
    }
    ++(*depth);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
struct tuple_greater_edge_score {
    typedef bool result_type;
    bool operator()( boost::tuple<ET,typename GT::rule_type> const& p1
                   , boost::tuple<ET,typename GT::rule_type> const& p2 ) const
    {
        return op(p1.template get<0>(),p2.template get<0>());
    }
    greater_edge_score<ET> op;
};

template <class ET, class GT> void
unary_applications<ET,GT>::creation_visitor::apply_rules( 
                                               edge_equiv_type eq
                                             , vertex_t v
                                             , graph_type const& g
                                             , typename sorted_rhs_map<GT>::iterator begin
                                             , typename sorted_rhs_map<GT>::iterator end
                                             )
{
    using namespace boost;
    using namespace std;
    
    filter->apply_rules( 
                eq
              , typename unary_filter_interface<ET,GT>::rule_range(begin,end)
            );

    while(not filter->empty()) {
        edge_type const& e = filter->top(); 
        rule_type r = gram->rule(e.rule_id());
        vertex_t vv = vertex_for_et( e 
                                   , const_cast<graph_type&>(g) 
                                   , *vertex_map 
                                   , *color_map );
        /// only add transition edges if they are not back-edges
        if (get(*color_map,vv) != color_traits<default_color_type>::gray()) {
            graph_edge_t ge = add_edge( v
                                      , vv
                                      , r
                                      , const_cast<graph_type&>(g)
                                      ).first;
            put( edge_name_t()
               , const_cast<graph_type&>(g)
               , ge
               , r );
        }
        filter->pop();
    }
}

///////////////////////////////////////////////////////////////////////////////

} // namespace sbmt


# endif //     SBMT__SEARCH__UNARY_APPLICATIONS_HPP
