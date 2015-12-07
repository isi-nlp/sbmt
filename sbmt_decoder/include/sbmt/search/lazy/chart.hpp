# if ! defined(SBMT__SEARCH__LAZY__CHART_HPP)
# define       SBMT__SEARCH__LAZY__CHART_HPP

# include <algorithm>
# include <boost/foreach.hpp>
# include <boost/range.hpp>
# include <sbmt/search/lazy/indexed_varray.hpp>
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/search/edge_queue.hpp>
# include <sbmt/grammar/grammar.hpp>

namespace sbmt { namespace lazy {
    
template <class Chart>
struct chart_cell_sequence {
    typedef typename Chart::data_type type;
};

template <class Chart>
struct chart_cell {
    typedef 
        typename boost::range_value<
                    typename chart_cell_sequence<Chart>::type
                 >::type
        type;
};

template <class Chart>
struct chart_edge_equivalence {
    typedef 
        typename boost::range_value<typename chart_cell<Chart>::type>::type
        type;
};

template <class Chart>
struct chart_edge {
    typedef typename chart_edge_equivalence<Chart>::type::edge_type type;
};

////////////////////////////////////////////////////////////////////////////////

template <class Compare>
struct compare_0th {
    compare_0th(Compare const& compare) : compare(compare) {}
    
    template <class A, class B>
    bool operator()(A const& a, B const& b) const
    {
        return compare(a[0],b[0]);
    }
    
    Compare const compare;
};

template <class C> 
compare_0th<C> make_compare_0th(C const& c) { return compare_0th<C>(c); }

////////////////////////////////////////////////////////////////////////////////
//
//  given an array of arrays, sort each internal array, and then sort
//  arrays by best leading element.
//
////////////////////////////////////////////////////////////////////////////////
template <class Ranges, class Compare>
void sort_sort(Ranges& ranges, Compare const& compare)
{
    typedef 
        typename boost::iterator_reference<
                     typename boost::range_iterator<Ranges>::type
                 >::type
        reference;
    BOOST_FOREACH(reference rng, ranges) {
        std::sort(boost::begin(rng),boost::end(rng),compare);
    }
    std::sort(boost::begin(ranges), boost::end(ranges), make_compare_0th(compare));
}

////////////////////////////////////////////////////////////////////////////////

template <class Edge> 
class cell_construct {
    
    typedef
        boost::multi_index_container<
          edge_equivalence<Edge>
        , boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique< 
              representative_edge<Edge> 
            , boost::hash<Edge>
            , std::equal_to<Edge>
            >
          >  
        >
        equiv_container;
    
    struct get_root {
        typedef indexed_token result_type;
        indexed_token operator()(equiv_container const& c) const
        {
            return c.begin()->representative().root();
        }
    };
    
    typedef 
        boost::multi_index_container<
          equiv_container
        , boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<get_root>
          >
        > 
        container;
        
    container c;
    span_t spn;
    
public:
    typedef equiv_container value_type;
    typedef value_type* pointer;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;  
    typedef value_type& reference;
    typedef value_type const& const_reference;
    typedef typename container::const_iterator const_iterator;
    typedef typename container::iterator iterator;
    
    cell_construct(span_t const& s) : spn(s) {}
    
    const_iterator find(indexed_token const& e) const { return c.find(e); }
    const_iterator begin() const { return c.begin(); }
    const_iterator end() const { return c.end(); }
    size_type size() const { return c.size(); }
    span_t span() const { return spn; }
    void insert(edge_equivalence<Edge> const& eq)
    {
        assert(spn == eq.representative().span());

        typename container::iterator pos = c.find(eq.representative().root());
        if (pos == c.end()) {
            equiv_container eqc;
            eqc.insert(eq);
            c.insert(eqc);
        } else {
            typename equiv_container::iterator p = pos->find(eq.representative());
            if (p == pos->end()) {
                const_cast<equiv_container&>(*pos).insert(eq);
            } else {
                const_cast<edge_equivalence<Edge>&>(*p).merge(eq);
            }
        }
    }
    
    void insert(Edge const& e)
    {
        if (size() == 0) spn = e.span();
        else {
            assert(spn == e.span());
        }
        
        typename container::iterator pos = c.find(e.root());
        if (pos == c.end()) {
            equiv_container eqc;
            eqc.insert(edge_equivalence<Edge>(e));
            c.insert(eqc);
        } else {
            typename equiv_container::iterator p = pos->find(e);
            if (p == pos->end()) {
                const_cast<equiv_container&>(*pos).insert(edge_equivalence<Edge>(e));
            } else {
                const_cast<edge_equivalence<Edge>&>(*p).insert_edge(e);
            }
        }
    }
};

struct greater_edge_inside_score {
    typedef bool result_type;
    
    template <class Edge>
    bool operator()( edge_equivalence<Edge> const& e1
                   , edge_equivalence<Edge> const& e2 ) const
    {
        return e1.representative().inside_score() >
               e2.representative().inside_score() ;
    }
    
    template <class Info>
    bool operator()( edge<Info> const& e1
                   , edge<Info> const& e2 ) const
    {
        return e1.inside_score() >
               e2.inside_score() ;
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class F>
struct apply_begin : F {
    template <class X>
    struct result {};
    template <class X> 
    struct result<apply_begin<F>(X)> {
        typedef 
            typename boost::result_of<
                         F(typename boost::range_value<X>::type)
                     >::type 
            type;
    };
    
    template <class X>
    typename result<apply_begin<F>(X)>::type
    operator()(X const& x) const { return f(*boost::begin(x)); }
    F f;
    apply_begin(F const& f = F()) : f(f) {}
};

template <class Cell>
indexed_token cell_root(Cell const& c)
{
    return c[0].representative().root();
}

template <class Edge>
class chart {
    struct reproot {
        typedef indexed_token result_type;
        indexed_token 
        operator()(gusc::shared_varray< edge_equivalence<Edge> > const& v) const
        {
            return v[0].representative().root();
        }
    };
    typedef 
        shared_indexed_varray<
          gusc::shared_varray< edge_equivalence<Edge> >
        , reproot
        > cell;
    typedef
        gusc::shared_varray< gusc::shared_varray< edge_equivalence<Edge> > >
        cell_varray;
    
public:
    typedef cell data_type;
    
    span_t span() const { return total_span; }
    
    chart(span_t total_span)
     : total_span(total_span)
    {
        for (span_index_t x = total_span.left(); x != total_span.right(); ++x) {
            for (span_index_t y = x + 1; y <= total_span.right(); ++y) {
                cells.insert(std::make_pair(span_t(x,y),cell()));
            }
        }
    }
    
    data_type const& operator[](span_t spn) const
    {
        
        return cells.find(spn)->second;
    }
    
    void put_cell(cell_construct<Edge> const& cc)
    {
        cell_varray c(cc.size());
        size_t x = 0;
        typedef typename cell_construct<Edge>::const_reference reference;
        BOOST_FOREACH(reference rng, cc) {
            c[x] = gusc::shared_varray< edge_equivalence<Edge> >(rng.size());
            size_t y = 0;
            BOOST_FOREACH(edge_equivalence<Edge> eq, rng) {
                c[x][y] = eq;
                ++y;
            }
            ++x;
        }
        
        sort_sort(c,greater_edge_score<Edge>());
        span_t span = cc.span();
        
        //cells.insert(std::make_pair(span,cell(c)));
        typename cell_map::iterator pos = cells.find(span);
        if (pos == cells.end()) {
            throw std::runtime_error("accessing span out of chart range");
        }
        pos->second = cell(c);
    }
    
private:
    span_t total_span;
    typedef stlext::hash_map< span_t, cell, boost::hash<span_t> > cell_map;
    stlext::hash_map< span_t, cell, boost::hash<span_t> > cells;
};



////////////////////////////////////////////////////////////////////////////////

template <class C, class T, class Edge>
std::basic_ostream<C,T>& 
operator<<(std::basic_ostream<C,T>& os, chart<Edge> const& c)
{
    span_t total_span = c.span();
    for (span_index_t left = total_span.left(); left != total_span.right(); ++left) {
        for (span_index_t right = left + 1; right <= total_span.right(); ++right) {
            span_t span(left,right);
            os << span << ":";
            BOOST_FOREACH(typename chart_cell<chart<Edge> >::type const& v, c[span])
            {
                os << "(" << v[0].representative().root() << " " << v[0].representative().inside_score() << ")";
            }
            os << std::endl;
        }
    }
    return os;
}

////////////////////////////////////////////////////////////////////////////////

template <class Edge, class Dict>
void print(std::ostream& os, chart<Edge> const& c, Dict const& dict)
{
    token_format_saver sav(os);
    os << token_label(dict);
    os << c;
}

////////////////////////////////////////////////////////////////////////////////

template <class Edge>
struct intro_cells {

    typedef 
        stlext::hash_map<span_t, cell_construct<Edge>, boost::hash<span_t> >
        prechart_t;
    prechart_t prechart;
    
    cell_construct<Edge> operator()(span_t const& span) const
    {
        typename prechart_t::const_iterator pos = prechart.find(span);
        if (pos != prechart.end()) return pos->second;
        else return cell_construct<Edge>(span);
    }
    
    template <class Grammar>
    intro_cells( concrete_edge_factory<Edge,Grammar>& ef
               , Grammar& gram
               , lattice_tree const& lat );
private:
    template <class Grammar>
    void seed( concrete_edge_factory<Edge,Grammar>& ef
             , Grammar& gram
             , lattice_tree::node const& lat );
};

template <class Edge>
template <class Grammar>
intro_cells<Edge>::intro_cells( concrete_edge_factory<Edge,Grammar>& ef
                              , Grammar& gram
                              , lattice_tree const& lat)
{
    seed(ef,gram,lat.root());
    
    // unary productions... 
    typedef std::pair<span_t const,cell_construct<Edge> > pair_span_cell_t;
    BOOST_FOREACH(pair_span_cell_t& cc, prechart) {
        std::vector<Edge> newedges;
        BOOST_FOREACH( typename cell_construct<Edge>::value_type const& precell
                     , cc.second ) {
            if (!is_lexical(precell.begin()->representative().root())) continue;
            BOOST_FOREACH(edge_equivalence<Edge> const& eq, precell) {
                BOOST_FOREACH( grammar_in_mem::rule_type rule
                             , gram.unary_rules(eq.representative().root()) ) {
                    gusc::any_generator<Edge> gen = ef.create_edge(gram,rule,eq);
                    while (gen) {
                        newedges.push_back(gen());
                    }
                }
            }
        }
        BOOST_FOREACH(Edge& e, newedges) {
            cc.second.insert(e);
        }
    }
}

template <class Edge>
template <class Grammar>
void intro_cells<Edge>::seed( concrete_edge_factory<Edge,Grammar>& ef
                            , Grammar& gram
                            , lattice_tree::node const& latnode )
{
    lattice_tree::children_iterator itr = latnode.children_begin(),
                                    end = latnode.children_end();

    for (; itr != end; ++itr) {
        if (itr->is_internal()) {
            seed(ef,gram,*itr);
        } else {
            typename Grammar::rule_type br;
  
            span_t spn = itr->span();
            Edge f = ef.create_edge(itr->lat_edge().source, spn, 1.0);
            edge_equivalence<Edge> fq(f);
            br = gram.rule(itr->lat_edge().rule_id);
            gusc::any_generator<Edge> gen = ef.create_edge(gram, br, fq);
            typename prechart_t::iterator pos = prechart.find(spn);
            if (pos == prechart.end()) {
                pos = prechart.insert(
                        std::make_pair(spn, cell_construct<Edge>(spn))
                      ).first;
            }
            cell_construct<Edge>& cc = pos->second;
            if (itr->lat_edge().syntax_rule_id == NULL_GRAMMAR_RULE_ID) {
                while (gen) cc.insert(gen());
            } else {
                edge_queue<Edge> edges;
                edge_equivalence_pool<Edge> epool;
                br = gram.rule(itr->lat_edge().syntax_rule_id);
                while (gen) edges.insert(epool,gen());
                while (not edges.empty()) {
                    gusc::any_generator<Edge> 
                        gen2 = ef.create_edge(gram,br,edges.top());
                    edges.pop();
                    while (gen2) cc.insert(gen2());
                }
            }
        }
    }
}

}} // namespace sbmt::lazy

# endif //     SBMT__SEARCH__LAZY__CHART_HPP
