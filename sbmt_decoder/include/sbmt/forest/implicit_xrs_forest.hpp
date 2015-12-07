# if ! defined(SBMT__FOREST__XFOREST_HPP)
# define       SBMT__FOREST__XFOREST_HPP

# include <sbmt/hash/hash_map.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/foreach.hpp>
# include <gusc/generator.hpp>
# include <gusc/iterator.hpp>
# include <gusc/varray.hpp>
# include <gusc/string/escape_string.hpp>
# include <sbmt/feature/feature_vector.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <boost/tr1/unordered_map.hpp>

namespace sbmt {

template<class Edge, class Gram, class StopCondition>
void collect_tm_scores( feature_vector& scores_accum
                      , Edge const& e
                      , Gram& gram
                      , StopCondition const& stop )
{
    if (e.has_rule_id()) {
      scores_accum *= gram.rule_scores(gram.rule(e.rule_id()));
    }

    if (e.has_first_child()) {
        if (not stop(e.first_child())) {
            collect_tm_scores(scores_accum,e.first_child(),gram,stop);
        }
        if (e.has_second_child() and not stop(e.second_child())) {
            collect_tm_scores(scores_accum,e.second_child(),gram,stop);
        }
    }
}

template<class Edge, class Gram>
void collect_tm_scores( feature_vector& scores_accum
                      , Edge const& e
                      , Gram const& gram )
{
  collect_tm_scores(scores_accum,e,gram,gusc::always_false());
}

template <class Info, class Gram, class Factory>
feature_vector
component_scores( edge<Info> const& e
                , Gram& gram
                , Factory const& ef
                )
{
    feature_vector scores;
    collect_tm_scores(scores,e,gram,gusc::always_true());
    feature_vector info_scores;
    ef.component_info_scores(e,gram,info_scores,gusc::always_true());
    scores *= info_scores;
    return scores;
}


////////////////////////////////////////////////////////////////////////////////

class xforest;
class xhyp;
typedef boost::tuple<feature_vector, std::vector<xforest>, score_t> xplode_t;
typedef boost::tuple<score_t, std::vector<xforest>, score_t> xplode_t_no_component;

inline
bool lesser_xplode_score(xplode_t const& x1, xplode_t const& x2)
{
    return boost::get<2>(x1) < boost::get<2>(x2);
}

inline
bool lesser_xplode_score_no_component( xplode_t_no_component const& x1
                                     , xplode_t_no_component const& x2 )
{
    //return false;
    return boost::get<2>(x1) < boost::get<2>(x2);
}

inline
xplode_t mult_xplodes(xplode_t x1, xplode_t const& x2)
{
    boost::get<0>(x1) *= boost::get<0>(x2);
    std::vector<xforest>& f1 = boost::get<1>(x1);
    std::vector<xforest> const& f2 = boost::get<1>(x2);
    f1.insert(f1.end(), f2.begin(), f2.end());
    boost::get<2>(x1) *= boost::get<2>(x2);
    return x1;
}

inline
xplode_t_no_component 
mult_xplodes_no_component(xplode_t_no_component x1, xplode_t_no_component const& x2)
{
    boost::get<0>(x1) *= boost::get<0>(x2);
    std::vector<xforest>& f1 = boost::get<1>(x1);
    std::vector<xforest> const& f2 = boost::get<1>(x2);
    f1.insert(f1.end(), f2.begin(), f2.end());
    boost::get<2>(x1) *= boost::get<2>(x2);
    return x1;
}

typedef gusc::any_generator<xhyp,gusc::iterator_tag> xhyp_generator;
typedef gusc::any_generator<xplode_t,gusc::iterator_tag> xplode_generator;
typedef gusc::any_generator<xplode_t_no_component,gusc::iterator_tag> 
        xplode_no_component_generator;

xhyp xhyp_from_xplode( weight_vector const& weights
                     , indexed_syntax_rule const& rule
                     , feature_vector const& scores
                     , xplode_t const& x );
                     
xhyp xhyp_from_xplode_no_component( indexed_syntax_rule const& rule
                                  , score_t const& s
                                  , xplode_t_no_component const& x );
                     
xplode_t mult_feats(weight_vector const& w, feature_vector const& f, xplode_t x);
xplode_t_no_component mult_feats_no_component(score_t const& f, xplode_t_no_component x);

template <class Edge, class Grammar, class Factory>
xplode_generator
xplode_ornode(edge_equivalence<Edge> eq, Grammar& gram, Factory const& ef);

template <class Edge, class Grammar, class Factory>
xplode_no_component_generator
xplode_ornode_no_component( edge_equivalence<Edge> eq
                          , Grammar const& gram
                          , Factory const& ef );

template <class Edge, class Grammar, class Factory>
xplode_generator
xplode_andnode(Edge const& e, Grammar& gram, Factory const& ef);

template <class Edge, class Grammar, class Factory>
xplode_no_component_generator
xplode_andnode_no_component( Edge const& e
                           , Grammar const& gram
                           , Factory const& ef );

template <class Edge, class Grammar, class Factory>
xplode_generator
xplode(Edge const& e, Grammar& gram, Factory const& ef);

template <class Edge, class Grammar, class Factory>
xplode_no_component_generator
xplode_no_component(Edge const& e, Grammar const& gram, Factory const& ef);

template <class Edge, class Grammar, class Factory>
xhyp_generator
xplode_hyps(Edge const& e, Grammar& gram, Factory const& ef);

template <class Edge, class Grammar, class Factory>
xhyp_generator
xplode_hyps_no_component(Edge const& e, Grammar const& gram, Factory const& ef);

template <class Edge, class Grammar, class Factory>
xhyp_generator
xforest_children( edge_equivalence<Edge> eq
                , Grammar& gram
                , Factory const& ef );
                
template <class Edge, class Grammar, class Factory>
xhyp_generator
xforest_children_no_component( edge_equivalence<Edge> eq
                             , Grammar const& gram
                             , Factory const& ef );

struct xhyp_impl;

class xhyp {
public:
    typedef stlext::hash_map<size_t, xforest> children_map;
    template <class Children>
    xhyp( score_t s
        , indexed_syntax_rule const& r
        , feature_vector const& v
        , Children const& c );
    xhyp() {}
    feature_vector const& scores() const;
    score_t transition_score() const;
    indexed_syntax_rule const& rule() const;
    xforest const& child(size_t x) const;
    size_t num_children() const;
    score_t score() const;
    
private:
    //xhyp();
    boost::shared_ptr<xhyp_impl> p;
};


inline
bool lesser_xhyp_score(xhyp const& h1, xhyp const& h2)
{
    return h1.score() < h2.score();
}

class xforest {
private:
    typedef xhyp_generator generator;
public:
    typedef gusc::iterator_from_generator<generator> iterator;
    typedef iterator const_iterator;

    iterator begin() const
    {
        return iterator(pimpl->children());
    }
    iterator end() const
    {
        return iterator();
    }
    void const* id() const { return pimpl->id(); }
    template <class H>
    explicit xforest(H const& h) : pimpl(new holder<H>(h)) {}
    xforest(xforest const& xf) : pimpl(xf.pimpl) {}
    xforest() {}

    bool operator==(xforest const& o) const { return pimpl->id() == o.pimpl->id(); }
    bool operator!=(xforest const& o) const { return !(*this == o); }
    size_t hash_self() const { return boost::hash<void const*>()(pimpl->id()); }
    std::string hash_string() const { return pimpl->id_string(); }
    score_t score() const { return pimpl->score(); }
    indexed_token root() const { return pimpl->root(); }
private:
    struct placeholder {
        virtual void const* id() const = 0;
        virtual std::string id_string() const = 0;
        virtual generator children() const = 0;
        virtual score_t score() const = 0;
        virtual indexed_token root() const = 0;
        virtual ~placeholder() {}
    };

    template <class H>
    struct holder : placeholder {
        explicit holder(H const& h) : h(h) {}
        virtual void const* id() const { return h.id(); }
        virtual std::string id_string() const { return h.id_string(); }
        virtual generator children() const { return h.children(); }
        virtual score_t score() const { return h.score(); }
        virtual indexed_token root() const { return h.root(); }
    private:
        H h;
    };
    boost::shared_ptr<placeholder> pimpl;
};

template <class Edge, class Grammar, class Factory>
struct equiv_as_xforest {
    equiv_as_xforest(edge_equivalence<Edge> const& eq, Grammar& gram, Factory const& ef)
    : eq(eq)
    , gram(&gram)
    , ef(&ef) {}

    edge_equivalence<Edge> eq;
    Grammar* gram;
    Factory const* ef;

    void const* id() const { return eq.get_raw(); }
    std::string id_string() const
    {
        return ef->hash_string(*gram,eq.representative());
    }
    xhyp_generator children() const
    {
        return xforest_children(eq,*gram,*ef);
    }
    score_t score() const { return eq.representative().inside_score(); }

    indexed_token root() const { return eq.representative().root(); }
};

template <class Edge, class Grammar, class Factory>
struct equiv_as_xforest_no_component {
    equiv_as_xforest_no_component( edge_equivalence<Edge> const& eq
                                 , Grammar const& gram
                                 , Factory const& ef )
    : eq(eq)
    , gram(&gram)
    , ef(&ef) {}

    edge_equivalence<Edge> eq;
    Grammar const* gram;
    Factory const* ef;

    void const* id() const { return eq.get_raw(); }
    std::string id_string() const
    {
        return ef->hash_string(*gram,eq.representative());
    }
    xhyp_generator children() const
    {
        return xforest_children_no_component(eq,*gram,*ef);
    }
    score_t score() const { return eq.representative().inside_score(); }

    indexed_token root() const { return eq.representative().root(); }
};

template <class Edge, class Grammar, class Factory>
equiv_as_xforest_no_component<Edge,Grammar,Factory>
make_equiv_as_xforest_no_component(edge_equivalence<Edge> const& eq, Grammar const& gram, Factory const& ef)
{
    return equiv_as_xforest_no_component<Edge,Grammar,Factory>(eq,gram,ef);
}

template <class Edge, class Grammar, class Factory>
equiv_as_xforest<Edge,Grammar,Factory>
make_equiv_as_xforest(edge_equivalence<Edge> const& eq, Grammar& gram, Factory const& ef)
{
    return equiv_as_xforest<Edge,Grammar,Factory>(eq,gram,ef);
}

struct xhyp_impl {
    typedef std::tr1::unordered_map<size_t, xforest> children_map;
    //typedef std::vector<xforest> children_map;
    indexed_syntax_rule rule;
    feature_vector scores;
    score_t score;
    children_map children;
    
    score_t transition_score() const
    {
        score_t s = score;
        BOOST_FOREACH(children_map::const_reference r, children) {
            s /= r.second.score();
        }
        return s;
    }
    template <class Children>
    xhyp_impl( score_t s
             , indexed_syntax_rule const& r
             , feature_vector const& v
             , Children const& c )
      : rule(r)
      , scores(v)
      , score(s)
    {
        typename boost::range_const_iterator<Children>::type
            itr = boost::begin(c),
            end = boost::end(c);
        indexed_syntax_rule::rhs_iterator
            rhsitr = rule.rhs_begin(),
            rhsend = rule.rhs_end();
        size_t x = 0;
        for (; rhsitr != rhsend; ++rhsitr, ++x) {
            if (rhsitr->indexed()) {
                assert(itr != end);
                //children.push_back(*itr);
                children.insert(std::make_pair(x,*itr));
                ++itr;
            }
        }
        assert(itr == end);
    }
private:
    xhyp_impl();
};

template <class Children>
xhyp::xhyp( score_t s
          , indexed_syntax_rule const& r
          , feature_vector const& v
          , Children const& c )
: p(new xhyp_impl(s,r,v,c)) {}

inline
score_t xhyp::transition_score() const { return p->transition_score(); }

inline
feature_vector const& xhyp::scores() const { return p->scores; }

inline
indexed_syntax_rule const& xhyp::rule() const { return p->rule; }

inline
xforest const& xhyp::child(size_t x) const { return p->children[x]; }

inline
size_t xhyp::num_children() const { return p->children.size(); }

inline
score_t xhyp::score() const { return p->score; }

inline
size_t hash_value(xforest const& xf)
{
    return xf.hash_self();
}

inline
xhyp xhyp_from_xplode( weight_vector const& weights
                     , indexed_syntax_rule const& rule
                     , feature_vector const& scores
                     , xplode_t const& x )
{
    return xhyp( geom(scores, weights) * boost::get<2>(x)
               , rule
               , scores * boost::get<0>(x)
               , boost::get<1>(x)
               );
}

inline
xhyp xhyp_from_xplode_no_component( indexed_syntax_rule const& rule
                                  , score_t const& score
                                  , xplode_t_no_component const& x )
{
    feature_vector fv;
    fv[0] = score * boost::get<0>(x);
    return xhyp( score * boost::get<2>(x)
               , rule
               , fv
               , boost::get<1>(x)
               );
}

inline xplode_t mult_feats(weight_vector const& w, feature_vector const& f, xplode_t x)
{
    boost::get<0>(x) *= f;
    boost::get<2>(x) *= geom(f, w);
    return x;
}

inline
xplode_t_no_component mult_feats_no_component(score_t const& s, xplode_t_no_component x)
{
    boost::get<0>(x) *= s;
    boost::get<2>(x) *= s;
    return x;
}

template <class Edge, class Grammar, class Factory>
xplode_no_component_generator
xplode_ornode_no_component( edge_equivalence<Edge> eq
                          , Grammar const& gram
                          , Factory const& ef )
{
    
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    xplode_no_component_generator gene;
    
    //return gene;
    
    if (!eq.representative().has_syntax_id(gram)) {
        eq.sort();
        assert(eq.size() > 0);

        gene = gusc::generate_union_heap(
                 gusc::generator_as_iterator(
                   gusc::generate_transform(
                     gusc::generate_from_range(eq)
                   , boost::bind( xplode_andnode_no_component<Edge,Grammar,Factory>
                                , boost::lambda::_1
                                , boost::cref(gram)
                                , boost::cref(ef)
                                )
                   )
                 )
               , &lesser_xplode_score_no_component
               );
        assert(gene);

    } else {
        vector<xforest> f(1,xforest(make_equiv_as_xforest_no_component(eq,gram,ef)));
        gene = gusc::make_single_value_generator(
                 xplode_t_no_component(score_t(),f,f[0].score())
               );
        assert(gene);
    }
    return gene;
    
}


template <class Edge, class Grammar, class Factory>
xplode_generator
xplode_ornode(edge_equivalence<Edge> eq, Grammar& gram, Factory const& ef)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;


    xplode_generator gene;

    if (!eq.representative().has_syntax_id(gram)) {
        eq.sort();
        assert(eq.size() > 0);

        gene = gusc::generate_union_heap(
                 gusc::generator_as_iterator(
                   gusc::generate_transform(
                     gusc::generate_from_range(eq)
                   , boost::bind( xplode_andnode<Edge,Grammar,Factory>
                                , boost::lambda::_1
                                , boost::ref(gram)
                                , boost::cref(ef)
                                )
                   )
                 )
               , &lesser_xplode_score
               );
        assert(gene);
    } else {
        vector<xforest> f(1,xforest(equiv_as_xforest<Edge,Grammar,Factory>(eq,gram,ef)));
        gene = gusc::make_single_value_generator(
                 xplode_t(feature_vector(),f,f[0].score())
               );
        assert(gene);
    }
    return gene;
}

template <class Edge, class Grammar, class Factory>
xplode_no_component_generator
xplode_andnode_no_component(Edge const& e, Grammar const& gram, Factory const& ef)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    assert(not e.has_syntax_id(gram));
    score_t score = e.delta_inside_score();

    xplode_no_component_generator gene;
    
    if (e.child_count() == 0) {
        gene = gusc::make_single_value_generator(
                xplode_t_no_component(score,vector<xforest>(),score)
               );
        assert(gene);
    } else {
        gene = gusc::generator_as_iterator(
                 gusc::generate_transform(
                   xplode_no_component(e,gram,ef)
                 , boost::bind( mult_feats_no_component
                              , score
                              , boost::lambda::_1
                              )
                 )
               );
        assert(gene);
    }
    
    return gene;
}

template <class Edge, class Grammar, class Factory>
xplode_generator
xplode_andnode(Edge const& e, Grammar& gram, Factory const& ef)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    assert(not e.has_syntax_id(gram));
    feature_vector feats = component_scores(e,gram,ef);

    xplode_generator gene;

    if (e.child_count() == 0) {
        gene = gusc::make_single_value_generator(
                xplode_t(feats,vector<xforest>(),geom(feats, gram.get_weights()))
               );
        assert(gene);
    } else {
        gene = gusc::generator_as_iterator(
                 gusc::generate_transform(
                   xplode(e,gram,ef)
                 , boost::bind( mult_feats
                              , boost::cref(gram.get_weights())
                              , feats
                              , boost::lambda::_1
                              )
                 )
               );
        assert(gene);
    }
    return gene;
}

template <class Edge, class Grammar, class Factory>
xplode_no_component_generator
xplode_no_component(Edge const& e, Grammar const& gram, Factory const& ef)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    xplode_no_component_generator ret;
    assert(e.has_first_child());

    ret = xplode_ornode_no_component(e.first_children(),gram,ef);
    assert(ret);
    //prechildren.push_back(xplode_ornode(e.first_children(),gram,ef));
    if (e.has_second_child()) {
        typedef gusc::shared_lazy_sequence<xplode_no_component_generator>
                xplode_sequence;
        xplode_no_component_generator g = xplode_ornode_no_component(e.second_children(),gram,ef);
        assert(g);
        xplode_sequence v2(g);
        xplode_sequence v1(ret);
        ret = gusc::generate_product_heap(
                &mult_xplodes_no_component
              , &lesser_xplode_score_no_component
              , v1
              , v2
              );
    }
    return ret;
}

template <class Edge, class Grammar, class Factory>
xplode_generator
xplode(Edge const& e, Grammar& gram, Factory const& ef)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    xplode_generator ret;
    assert(e.has_first_child());

    ret = xplode_ornode(e.first_children(),gram,ef);
    assert(ret);
    //prechildren.push_back(xplode_ornode(e.first_children(),gram,ef));
    if (e.has_second_child()) {
        typedef gusc::shared_lazy_sequence<xplode_generator> xplode_sequence;
        xplode_generator g = xplode_ornode(e.second_children(),gram,ef);
        assert(g);
        xplode_sequence v2(g);
        xplode_sequence v1(ret);
        ret = gusc::generate_product_heap(
                &mult_xplodes
              , &lesser_xplode_score
              , v1
              , v2
              );
    }
    return ret;
}

template <class Edge, class Grammar, class Factory>
xhyp_generator
xplode_hyps_no_component(Edge const& e, Grammar const& gram, Factory const& ef)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    indexed_syntax_rule rule = gram.get_syntax(gram.rule(e.rule_id()));
    score_t score = e.delta_inside_score();

    if (e.child_count() == 0) {
        feature_vector fv;
        fv[0] = score;
        xhyp hyp(score, rule, fv, vector<xforest>());
        return gusc::make_single_value_generator(hyp);
    } else {
        return gusc::generator_as_iterator(
                 gusc::generate_transform(
                   xplode_no_component(e,gram,ef)
                 , boost::bind( xhyp_from_xplode_no_component
                              , rule
                              , score
                              , boost::lambda::_1
                              )
                 )
               );
    }
}

template <class Edge, class Grammar, class Factory>
xhyp_generator
xplode_hyps(Edge const& e, Grammar& gram, Factory const& ef)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    indexed_syntax_rule rule = gram.get_syntax(gram.rule(e.rule_id()));
    feature_vector scores = component_scores(e,gram,ef);

    if (e.child_count() == 0) {
        xhyp hyp(geom(scores, gram.get_weights()), rule, scores, vector<xforest>());
        return gusc::make_single_value_generator(hyp);
    } else {
        return gusc::generator_as_iterator(
                 gusc::generate_transform(
                   xplode(e,gram,ef)
                 , boost::bind( xhyp_from_xplode
                              , boost::cref(gram.get_weights())
                              , rule
                              , scores
                              , boost::lambda::_1
                              )
                 )
               );
    }
}

template <class Edge, class Grammar, class Factory>
xhyp_generator
xforest_children_no_component( edge_equivalence<Edge> eq
                             , Grammar const& gram
                             , Factory const& ef )
{
    eq.sort();
    return
        gusc::generate_union_heap(
          gusc::generator_as_iterator(
            gusc::generate_transform(
              gusc::generate_from_range(eq)
              , boost::bind( xplode_hyps_no_component<Edge,Grammar,Factory>
                           , boost::lambda::_1
                           , boost::cref(gram)
                           , boost::cref(ef)
                           )
            )
          )
        , &lesser_xhyp_score
        );
}

template <class Edge, class Grammar, class Factory>
xhyp_generator
xforest_children( edge_equivalence<Edge> eq
                , Grammar& gram
                , Factory const& ef )
{
    eq.sort();
    return
        gusc::generate_union_heap(
          gusc::generator_as_iterator(
            gusc::generate_transform(
              gusc::generate_from_range(eq)
              , boost::bind( xplode_hyps<Edge,Grammar,Factory>
                           , boost::lambda::_1
                           , boost::ref(gram)
                           , boost::cref(ef)
                           )
            )
          )
        , &lesser_xhyp_score
        );
}

////////////////////////////////////////////////////////////////////////////////

template <class Edge,class Grammar>
class brf_forest_printer {
    typedef stlext::hash_map<
              edge_equivalence<Edge>
            , size_t
            , boost::hash< edge_equivalence<Edge> > 
            > idmap_t;
    idmap_t idmap;
public:
    void print_forest(std::ostream& os, edge_equivalence<Edge> eq, Grammar& gram)
    {
        typename idmap_t::iterator pos = idmap.find(eq);
        
        if (pos != idmap.end()) {
            os << '#' << pos->second;
            return;
        } else {
            size_t id = idmap.size() + 1;
            idmap.insert(std::make_pair(eq,id));
            os << '#' << id;
        }

        typename edge_equivalence<Edge>::iterator 
            itr = eq.begin(), 
            end = eq.end();
        assert(itr != end);

        os << "(OR";
        BOOST_FOREACH(Edge e, std::make_pair(itr,end)) {
            os << " ";
            print_hyperedge(os,e,gram);
        }
        os << " )";
    }
private:
    void print_hyperedge(std::ostream& os, Edge const& e, Grammar& gram)
    {
        if (e.child_count() != 0) {
            os << "(" << e.span();
            //size_t x = 0;
            for (size_t x = 0; x != e.child_count(); ++x) {
                os << " ";
                print_forest(os,e.get_children(x),gram);
            }
            os << " )";
        } else {
            os << e.span();
        }
    }
};

template <class Grammar>
class xrs_forest_printer {
public:
    struct hyperedge_printer_interface {
        virtual bool supply_parens(xhyp hyp) const = 0;
        virtual void print_hyperedge(std::ostream&, xhyp, Grammar&) = 0;
        virtual ~hyperedge_printer_interface(){}
    };

    struct forest_printer_interface {
        virtual void print_forest(std::ostream&, xforest, Grammar&) = 0;
        virtual ~forest_printer_interface(){}
    };
    boost::shared_ptr<hyperedge_printer_interface> h;
    boost::shared_ptr<forest_printer_interface> f;
    typedef stlext::hash_map<xforest,size_t, boost::hash<xforest> > idmap_t;

    idmap_t idmap;

    void print_features(std::ostream& os, feature_vector const& m, feature_dictionary& dict)
    {
        os << logmath::neglog10_scale;
        os << '<';
        os << print(m,dict);
        os << '>';
    }

    void print_forest(std::ostream& os, xforest forest, Grammar& gram)
    {
        f->print_forest(os,forest,gram);
    }

    void print_hyperedge(std::ostream& os, xhyp hyp, Grammar& gram)
    {
        h->print_hyperedge(os,hyp,gram);
    }

    bool supply_parens(xhyp hyp) const
    {
        return h->supply_parens(hyp);
    }

    void operator()(std::ostream& os, xforest forest, Grammar& grammar)
    {
        idmap.clear();
        print_forest(os,forest,grammar);
        idmap.clear();
    }
};

template <class Grammar>
struct xrs_traditional_forest_printer
  : xrs_forest_printer<Grammar>::forest_printer_interface {
    xrs_forest_printer<Grammar>* printer;
    const size_t limit;

    xrs_traditional_forest_printer(xrs_forest_printer<Grammar>* printer, size_t limit = 1000)
    : printer(printer)
    , limit(limit) {}

    void print_forest(std::ostream& os, xforest forest, Grammar& gram)
    {
        bool parens = false;

        typename xrs_forest_printer<Grammar>::idmap_t::iterator
            pos = printer->idmap.find(forest);
        if (pos != printer->idmap.end()) {
            os << '#' << pos->second;
            return;
        } else {
            size_t id = printer->idmap.size() + 1;
            printer->idmap.insert(std::make_pair(forest,id));
            os << '#' << id;
            parens = true;
        }

        xforest::iterator itr = forest.begin(), end = forest.end();
        assert(itr != end);
        xhyp hyp = *itr; ++itr;
        size_t x = 0;
        if (itr == end) {
            parens = parens && !printer->supply_parens(hyp);
            if (parens) os << "(";
            printer->print_hyperedge(os,hyp,gram);
            if (parens) os << " )";
        }
        else {
            os << "(OR ";
            printer->print_hyperedge(os,hyp,gram);
            ++x;
            BOOST_FOREACH(xhyp h, std::make_pair(itr,end)) {
                if ((x == limit) and (forest.root().type() != top_token)) break;
                os << " ";
                printer->print_hyperedge(os,h,gram);
                ++x;
            }
            os << " )";
        }
    }
};

template <class Grammar>
struct xrs_mergeable_forest_printer
  : xrs_forest_printer<Grammar>::forest_printer_interface {
    xrs_forest_printer<Grammar>* printer;
    const size_t limit;
    xrs_mergeable_forest_printer(xrs_forest_printer<Grammar>* printer, size_t ornode_limit = 1000)
    : printer(printer)
    , limit(ornode_limit) {}
    virtual void print_forest(std::ostream& os, xforest forest, Grammar& gram)
    {
        //assert(forest->hyperedges.size() > 0);
        typename xrs_forest_printer<Grammar>::idmap_t::iterator
            pos = printer->idmap.find(forest);
        if (pos != printer->idmap.end()) {
            os << '#' << pos->second;
        } else {
            size_t id = printer->idmap.size() + 1;
            printer->idmap.insert(std::make_pair(forest,id));
            os << "(OR #" << id << "=" << forest.hash_string();

            size_t x = 0;
            BOOST_FOREACH(xhyp hyp, forest) {
                if (x == limit) break;
                os << ' ';
                printer->print_hyperedge(os,hyp,gram);
                ++x;
            }
            os << " )";
        }
    }
};



template <class Grammar>
struct xrs_id_hyperedge_printer
  : xrs_forest_printer<Grammar>::hyperedge_printer_interface {
    xrs_forest_printer<Grammar>* printer;

    xrs_id_hyperedge_printer(xrs_forest_printer<Grammar>* printer)
      : printer(printer) {}

    virtual bool supply_parens(xhyp hyp) const
    {
        return hyp.num_children() != 0;
    }

    virtual void print_hyperedge(std::ostream& os, xhyp hyp, Grammar& gram)
    {
        std::stringstream sstr;
        sstr << hyp.rule().id();
        printer->print_features(sstr,hyp.scores(),gram.feature_names());
        if (hyp.num_children() != 0) {
            os << "(" << sstr.str();
            indexed_syntax_rule::rhs_iterator
                ritr = hyp.rule().rhs_begin(),
                rend = hyp.rule().rhs_end();
            size_t x = 0;
            for (; ritr != rend; ++ritr, ++x) {
                if (ritr->indexed()) {
                    os << " ";
                    printer->print_forest(os,hyp.child(x),gram);
                }
            }
            os << " )";
        } else {
            os << sstr.str();
        }
    }
};

template <class Grammar>
struct xrs_target_tree_hyperedge_printer
  : xrs_forest_printer<Grammar>::hyperedge_printer_interface {
    xrs_forest_printer<Grammar>* printer;

    xrs_target_tree_hyperedge_printer(xrs_forest_printer<Grammar>* printer)
      : printer(printer) {}

    bool supply_parens(xhyp hyp) const { return true; }

    void print_internal(std::ostream& os, indexed_syntax_rule::tree_node const& nd, xhyp hyp, Grammar& gram)
    {
        if (nd.indexed()) {
            printer->print_forest(os,hyp.child(nd.index()),gram);
        } else if (nd.lexical()) {
            os << '"' << gusc::escape_c(gram.dict().label(nd.get_token())) << '"';
        } else {
            os << '(' << print(nd.get_token(),gram.dict());
            indexed_syntax_rule::lhs_children_iterator
                itr = nd.children_begin(),
                end = nd.children_end();
            for (; itr != end; ++itr) {
                os << ' ';
                print_internal(os,*itr,hyp,gram);
            }
            os << " )";
        }
    }

    void print_hyperedge(std::ostream& os, xhyp hyp, Grammar& gram)
    {
        std::stringstream sstr;
        printer->print_features(sstr,hyp.scores(),gram.feature_names());
        os << '(' << print(hyp.rule().lhs_root()->get_token(),gram.dict())
           << '~' << hyp.rule().id() << sstr.str();
        indexed_syntax_rule::lhs_children_iterator
            litr = hyp.rule().lhs_root()->children_begin(),
            lend = hyp.rule().lhs_root()->children_end();
        for (; litr != lend; ++litr) {
            os << ' ';
            print_internal(os,*litr,hyp,gram);
        }
        os << " )";
    }
};

template <class Grammar>
struct xrs_target_string_hyperedge_printer
  : xrs_forest_printer<Grammar>::hyperedge_printer_interface {
    xrs_forest_printer<Grammar>* printer;

    xrs_target_string_hyperedge_printer(xrs_forest_printer<Grammar>* printer)
      : printer(printer) {}

    bool supply_parens(xhyp hyp) const { return true; }

    void print_hyperedge(std::ostream& os, xhyp hyp, Grammar& gram)
    {
        std::stringstream sstr;
        printer->print_features(sstr,hyp.scores(),gram.feature_names());
        os << '(' << hyp.rule().id() << sstr.str();
        indexed_syntax_rule::lhs_preorder_iterator
            litr = hyp.rule().lhs_begin(),
            lend = hyp.rule().lhs_end();

        for (; litr != lend; ++litr) {
            if (litr->indexed()) {
                os << ' ';
                printer->print_forest(os,hyp.child(litr->index()),gram);
            } else if(litr->lexical()) {
                os << ' ' << '"' << gusc::escape_c(gram.dict().label(litr->get_token())) << '"';
            }
        }
        os << " )";
    }
};

template <class Grammar>
struct xrs_source_string_hyperedge_printer
  : xrs_forest_printer<Grammar>::hyperedge_printer_interface {
    xrs_forest_printer<Grammar>* printer;

    xrs_source_string_hyperedge_printer(xrs_forest_printer<Grammar>* printer)
      : printer(printer) {}

    bool supply_parens(xhyp hyp) const { return true; }

    void print_hyperedge(std::ostream& os, xhyp hyp, Grammar& gram)
    {
        std::stringstream sstr;
        printer->print_features(sstr,hyp.scores(),gram.feature_names());
        os << '(' << hyp.rule().id() << sstr.str();
        indexed_syntax_rule::rhs_iterator
            ritr = hyp.rule().rhs_begin(),
            rend = hyp.rule().rhs_end();

        for (; ritr != rend; ++ritr) {
            if (ritr->indexed()) {
                os << ' ';
                printer->print_forest(os,hyp.child(hyp.rule().index(*ritr)),gram);
            } else if(ritr->lexical()) {
                os << ' ' << '"' << gusc::escape_c(gram.dict().label(ritr->get_token())) << '"';
            }
        }
        os << " )";
    }
};

template <class Gram>
void print_target_string_forest(std::ostream& os, xforest ptr, Gram& gram)
{
    xrs_forest_printer<Gram> printer;
    printer.h.reset(new xrs_target_string_hyperedge_printer<Gram>(&printer));
    printer.f.reset(new xrs_traditional_forest_printer<Gram>(&printer));
    printer(os,ptr,gram);
}

template <class Gram>
void print_source_string_forest(std::ostream& os, xforest ptr, Gram& gram)
{
    xrs_forest_printer<Gram> printer;
    printer.h.reset(new xrs_source_string_hyperedge_printer<Gram>(&printer));
    printer.f.reset(new xrs_traditional_forest_printer<Gram>(&printer));
    printer(os,ptr,gram);
}

template <class Gram>
void print_id_forest(std::ostream& os, xforest ptr, Gram& gram)
{
    xrs_forest_printer<Gram> printer;
    printer.h.reset(new xrs_id_hyperedge_printer<Gram>(&printer));
    printer.f.reset(new xrs_traditional_forest_printer<Gram>(&printer));
    printer(os,ptr,gram);
}

} // namespace sbmt

# endif //     SBMT__FOREST__XFOREST_HPP
