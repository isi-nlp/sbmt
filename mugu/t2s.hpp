# ifndef MUGU__T2S_HPP
# define MUGU__T2S_HPP

# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/token.hpp>
# include <iostream>
# include <sstream>
# include <boost/tuple/tuple.hpp>
# include <boost/tr1/unordered_map.hpp>
# include <gusc/generator.hpp>
# include <gusc/iterator.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/search/lazy/indexed_varray.hpp>
# include <sbmt/feature/feature_vector.hpp>
# include <sbmt/forest/implicit_xrs_forest.hpp>

namespace sbmt { namespace t2s {

bool preterminal(indexed_syntax_rule::tree_node const& nd);

struct grammar_state {
    grammar_state() : root(0), virt(false) {}
    grammar_state(indexed_syntax_rule::tree_node const& root, bool virt = false)
    : root(&root), virt(virt) {}
    indexed_syntax_rule::tree_node const* root;
    bool virt;
};

struct internal_state {
    internal_state() : root(0), lex(false) {}
    internal_state(indexed_syntax_rule::tree_node const& root, bool lex)
    : root(&root), lex(lex) {}
    indexed_syntax_rule::tree_node const* root;
    bool lex;
};

size_t hash_value(internal_state const& s);

bool operator == (internal_state const& s1, internal_state const& s2);

void print( std::ostream& out
          , grammar_state const& state
          , in_memory_dictionary const& dict );

std::ostream& operator << (std::ostream& out, grammar_state const& state);

size_t hash_value(grammar_state const& s);

bool operator == (grammar_state const& s1, grammar_state const& s2);

bool operator != (grammar_state const& s1, grammar_state const& s2);

bool operator == (indexed_token const& s1, grammar_state const& s2);

bool operator != (indexed_token const& s1, grammar_state const& s2);

bool operator == (grammar_state const& s1, indexed_token const& s2);

bool operator != (grammar_state const& s1, indexed_token const& s2);

struct rule {
    grammar_state state;
    indexed_syntax_rule syntax;
    feature_vector scores;
    score_t score;
    score_t heur;
    rule( grammar_state const& state
        , indexed_syntax_rule const& syntax
        , feature_vector const& scores
        , score_t score )
    : state(state)
    , syntax(syntax)
    , scores(scores)
    , score(score) {}
    rule() {}
};

bool operator > (rule const& r1, rule const& r2);

struct repstate {
    typedef grammar_state result_type;
    grammar_state operator()(gusc::shared_varray<rule> const& r) const 
    {
        return r[0].state;
    }
};

typedef lazy::shared_indexed_varray<gusc::shared_varray<rule>,repstate> 
        rulemap_entry;
typedef gusc::varray<grammar_state> hyperkey;

typedef std::tr1::unordered_map<
          hyperkey
        , rulemap_entry
        , boost::hash<hyperkey>
        > rulemap_t;

typedef std::tr1::unordered_map<
          hyperkey
        , rule
        , boost::hash<hyperkey>
        > supp_rulemap_t;

typedef gusc::varray<internal_state> source_hyperkey;
typedef std::tr1::unordered_map<source_hyperkey, internal_state> lexstatemap_t;

typedef std::tr1::unordered_map<
          hyperkey
        , std::tr1::unordered_map<
            grammar_state
          , std::vector<rule>
          , boost::hash<grammar_state>
          > 
        , boost::hash<hyperkey>
        > rulepremap_t;

typedef std::tr1::unordered_map<hyperkey,grammar_state,boost::hash<hyperkey> >
        virtmap_t;
typedef std::pair<hyperkey, boost::tuple<grammar_state,indexed_syntax_rule,feature_vector> > ruledata_t;
typedef boost::tuple<grammar_state,indexed_syntax_rule,feature_vector> prerule_t;

void add_supps( indexed_syntax_rule const& syntax
              , indexed_syntax_rule::tree_node const& root
              , supp_rulemap_t& supp_rulemap
              , in_memory_dictionary& dict
              , feature_dictionary& fdict
              , weight_vector const& weights
              , size_t& rid
              );

void level_order_decompose( indexed_syntax_rule const& rule
                          , feature_vector const& fv
                          , rulepremap_t& rulemap
                          , virtmap_t& virtmap
                          , weight_vector const& wv );                         

/// forest
struct forest;

struct hyp {
    grammar_state state() const { return r.state; }
    score_t transition_score() const;
    score_t score() const { return scr; }
    indexed_syntax_rule syntax() const { return r.syntax; }
    feature_vector scores() const 
    {   if (r.state.virt) return source;
        else return r.scores * source; 
    }
    gusc::varray<forest> children;
    template <class Range> 
    hyp( rule const& r
       , indexed_syntax_rule const& src
       , indexed_syntax_rule::tree_node const& nd
       , feature_vector const& srcscrs
       , score_t srctrans
       , Range const& children );
    hyp(){}
private:
    rule r;
    feature_vector source;
    score_t scr;
};

bool operator < (hyp const& h1, hyp const& h2);

bool operator > (hyp const& h1, hyp const& h2);

typedef gusc::any_generator<hyp,gusc::iterator_tag> hyp_generator;

struct forest {
private:
    typedef hyp_generator generator;
public:
    typedef gusc::iterator_from_generator<generator> iterator;
    typedef iterator const_iterator;

    iterator begin() const { return iterator(pimpl->children()); }
    iterator end() const { return iterator(); }
    score_t score() const { return pimpl->score(); }
    grammar_state state() const { return pimpl->state(); }
    void const* id() const { return pimpl->id(); }

    template <class H>
    explicit forest(H const& h) : pimpl(new holder<H>(h)) {}
    forest(forest const& f) : pimpl(f.pimpl) {}
    forest() {}
private:
    struct placeholder {
        virtual void const* id() const = 0;
        virtual score_t score() const = 0;
        virtual grammar_state state() const = 0;
        virtual generator children() const = 0;
        virtual ~placeholder() {}
    };

    template <class H>
    struct holder : placeholder {
        explicit holder(H const& h) : h(h) {}
        virtual void const* id() const { return h.id(); }
        virtual grammar_state state() const { return h.state(); }
        virtual score_t score() const { return h.score(); }
        virtual generator children() const { return h.children(); }
    private:
        H h;
    };
    boost::shared_ptr<placeholder> pimpl;
};

template <class Range> 
hyp::hyp( rule const& r
        , indexed_syntax_rule const& src
        , indexed_syntax_rule::tree_node const& nd
        , feature_vector const& srcscrs
        , score_t srctrans
        , Range const& children )
: children(children)
, r(r)
, scr(1.0)
{
    if (not r.state.virt) {
        scr = r.score;
    }
    if (&nd == src.lhs_root()) {
        scr *= srctrans;
        source = srcscrs;
    }
    BOOST_FOREACH(forest const& f, children)
    {
        scr *= f.score();
    }
}

bool operator < (forest const& f1, forest const& f2);

bool operator > (forest const& f1, forest const& f2);

xplode_generator
tgt_xplode(hyp const& h, weight_vector const& weights);

xplode_generator
tgt_xplode_andnode(hyp const& h, weight_vector const& weights);

xplode_generator
tgt_xplode_ornode(forest f, weight_vector const& weights);

xhyp_generator
tgt_xforest_children(forest const& f, weight_vector const& weights);

xhyp_generator
tgt_xplode_hyps(hyp const& h, weight_vector const& weights);

struct forest_as_xforest {
    forest_as_xforest(forest const& f,weight_vector const& weights)
    : f(f)
    , weights(&weights) {}
    forest f;
    weight_vector const* weights;

    void const* id() const { return f.id(); }
    std::string id_string() const
    {
        return boost::lexical_cast<std::string>(id());
    }
    xhyp_generator children() const
    {
        return tgt_xforest_children(f,*weights);
    }
    score_t score() const { return f.score(); }

    indexed_token root() const { return f.state().root->get_token(); }
};

xplode_generator
tgt_xplode_andnode(hyp const& h, weight_vector const& weights);

xplode_generator
tgt_xplode_ornode(forest f, weight_vector const& weights);

xplode_generator
tgt_xplode(hyp const& h, weight_vector const& weights);

xhyp_generator
tgt_xplode_hyps(hyp const& h, weight_vector const& weights);

xhyp_generator
tgt_xforest_children(forest const& f, weight_vector const& weights);

/// \forest

// given a child vector construct hyperedges
// will be ordered correctly.
struct hyp_gen 
 : gusc::peekable_generator_facade<hyp_gen,hyp> {
    struct op {
        typedef hyp result_type;
        explicit op( gusc::varray<forest> const& children
                   , indexed_syntax_rule const& srule
                   , indexed_syntax_rule::tree_node const& snd
                   , feature_vector const& sv
                   , score_t s )
        : children(children)
        , srule(&srule)
        , snd(&snd)
        , sv(&sv)
        , s(s) {}
        hyp operator()(rule const& r) const
        {
            return hyp(r,*srule,*snd,*sv,s,children);
        }
        gusc::varray<forest> children;
        indexed_syntax_rule const* srule;
        indexed_syntax_rule::tree_node const* snd;
        feature_vector const* sv;
        score_t s;
    };
    template <class Range>
    hyp_gen( Range const& rng
           , indexed_syntax_rule const& srule
           , indexed_syntax_rule::tree_node const& snd
           , feature_vector const& sv
           , score_t s
           , gusc::varray<forest> const& c )
    : gen(
        hyp_generator(gusc::make_peekable(
          gusc::generate_transform(
            gusc::generate_from_range(rng)
          , op(c,srule,snd,sv,s)
          )
        ))
      ) {}
private:
    bool more() const { return bool(gen); }
    void pop() { ++gen; }
    hyp const& peek() const { return *gen; }
    friend class gusc::generator_access;
    gusc::varray<forest> children;
    hyp_generator gen;
};


struct hyp_gen_gen
 : gusc::peekable_generator_facade<
     hyp_gen_gen
   , hyp
   > {
public:
    struct op {
        typedef hyp_gen result_type;
        explicit op( gusc::varray<forest> const& children
                   , indexed_syntax_rule const& srule
                   , indexed_syntax_rule::tree_node const& snd
                   , feature_vector const& sv
                   , score_t s )
        : children(children)
        , srule(&srule)
        , snd(&snd)
        , sv(&sv)
        , s(s) {}
        hyp_gen operator()(gusc::shared_varray<rule> const& rr) const
        {
            return hyp_gen(rr,*srule,*snd,*sv,s,children);
        }
        gusc::varray<forest> children;
        indexed_syntax_rule const* srule;
        indexed_syntax_rule::tree_node const* snd;
        feature_vector const* sv;
        score_t s;
    };
    template <class Range>
    hyp_gen_gen( Range const& rng
               , indexed_syntax_rule const& srule
               , indexed_syntax_rule::tree_node const& snd
               , feature_vector const& sv
               , score_t s
               , gusc::varray<forest> const& c )
    : gen( 
        gusc::generate_union_heap(
          gusc::make_peekable(gusc::generate_transform(
            gusc::generate_from_range(rng)
          , op(c,srule,snd,sv,s)
          ))
        , gusc::less()
        )
      ) {}      
private:
    bool more() const { return bool(gen); }
    void pop() { ++gen; }
    hyp const& peek() const { return *gen; }
    friend class gusc::generator_access;
    hyp_generator gen;
};

struct varray_as_forest {
    template <class T>
    explicit varray_as_forest(T const& h)
    : hyps(h) {}
    hyp_generator children() const
    {
        return gusc::generate_from_range(hyps);
    }
    void const* id() const { return hyps.get(); }
    grammar_state state() const { return hyps[0].state(); }
    score_t score() const { return hyps[0].score(); }
    gusc::shared_varray<hyp> hyps;
};

struct to_varray {
    explicit to_varray(size_t sz) : sz(sz) {}
    size_t sz; 
    typedef gusc::varray<forest> result_type;
    result_type operator()(forest const& f) const
    {
        result_type v(sz,forest());
        v[0] = f;
        return v;
    }
};

struct prodvec {
    typedef gusc::varray<forest> result_type;
    size_t sz;
    prodvec(size_t sz) : sz(sz) {}
    result_type operator()(result_type v, forest const& f) const
    {
        v[sz] = f;
        return v;
    }
};

struct lesser_children {
    size_t x;
    explicit lesser_children(size_t x) : x(x) {}
    bool operator()( gusc::varray<forest> const& vf1
                   , gusc::varray<forest> const& vf2 ) const
    {
        score_t s1,s2;
        for (size_t y = 0; y != x; ++y) s1 *= vf1[y].score();
        for (size_t y = 0; y != x; ++y) s2 *= vf2[y].score();
        return s1 < s2;
    }
};

template <class Children>
gusc::any_generator<gusc::varray<forest>, gusc::iterator_tag>
generate_children(Children const& children);

struct hyps_from_children
: gusc::peekable_generator_facade<
    hyps_from_children
  , hyp
  > {
    struct op {
        rulemap_t const* rmap;
        grammar_state root;
        indexed_syntax_rule const* srule;
        indexed_syntax_rule::tree_node const* snd;
        feature_vector const* sv;
        score_t s;
        op( rulemap_t const& rmap
          , grammar_state root
          , indexed_syntax_rule const& srule
          , indexed_syntax_rule::tree_node const& snd
          , feature_vector const& sv
          , score_t s ) 
        : rmap(&rmap)
        , root(root)
        , srule(&srule)
        , snd(&snd)
        , sv(&sv)
        , s(s) {}
        typedef hyp_generator result_type;
        result_type operator()(gusc::varray<forest> const& children) const
        {
            hyperkey hkey(boost::size(children) + 1);
            hkey[0] = root;
            size_t sz = 1;
            BOOST_FOREACH(forest f, children) {
                hkey[sz] = f.state();
                ++sz;
            }
            rulemap_t::const_iterator pos = rmap->find(hkey);
            if (pos != rmap->end()) {
                return hyp_gen_gen(pos->second,*srule,*snd,*sv,s,children);
            } else {
                return hyp_generator();
            }
        }
    };
    template <class Children>
    hyps_from_children( Children const& children
                      , grammar_state root
                      , rulemap_t const& rmap
                      , indexed_syntax_rule const& srule
                      , indexed_syntax_rule::tree_node const& snd
                      , feature_vector const& sv
                      , score_t s )
    {
        //std::cerr << "hyps_from_children::hyps_from_children("<< root << ")\n";
        gusc::any_generator<gusc::varray<forest>, gusc::iterator_tag> 
            cgen = generate_children(children);
        gusc::any_generator<hyp_generator, gusc::iterator_tag> 
            hgen = gusc::make_peekable(generate_transform(cgen,op(rmap,root,srule,snd,sv,s)));
        gen = gusc::generate_union_heap(hgen,gusc::less());
    }
private:
    bool more() const { return bool(gen); }
    void pop() { ++gen; }
    hyp const& peek() const { return *gen; }
    friend class gusc::generator_access;
    hyp_generator gen;
};

struct first_greater {
    template <class T>
    bool operator()(T const& t1, T const& t2) const
    {
        return t1[0] > t2[0];
    }
};

typedef std::tr1::unordered_map<size_t,gusc::shared_varray<forest> > child_forests;
typedef std::pair<indexed_syntax_rule,child_forests> source_data;
typedef gusc::any_generator<source_data,gusc::iterator_tag> source_generator;

gusc::shared_varray<forest>
forests( indexed_syntax_rule const& syntax
       , indexed_syntax_rule::tree_node const& nd
       , feature_vector const& sourcetscores
       , score_t sourcetscore
       , std::tr1::unordered_map<size_t,gusc::shared_varray<forest> >& cmap
       , rulemap_t const& rulemap 
       , supp_rulemap_t const& supp_rulemap );

weight_vector 
weights_from_file(std::string filename, feature_dictionary& dict);

fat_weight_vector weights_from_file(std::string filename);

struct xtree;
typedef boost::shared_ptr<xtree> xtree_ptr;
typedef std::vector<xtree_ptr> xtree_children;
struct xtree {
    score_t scr;
    xhyp root;
    typedef std::tr1::unordered_map<int,xtree_ptr> children_type;
    children_type children;
    template <class RNG>
    xtree(xhyp const& root, RNG c)
    : scr(root.transition_score())
    , root(root)
    {
        typename boost::range_const_iterator<RNG>::type 
             itr = boost::begin(c),
             end = boost::end(c);
        indexed_syntax_rule::rhs_iterator
             rhsitr = root.rule().rhs_begin(),
             rhsend = root.rule().rhs_end();
        size_t x = 0;
        for (; rhsitr != rhsend; ++rhsitr, ++x) {
            if (rhsitr->indexed()) {
                children.insert(std::make_pair(x,*itr));
                scr *= (*itr)->scr;
                ++itr;
             }
         }
     }
};

typedef gusc::any_generator<boost::shared_ptr<xtree>,gusc::iterator_tag> xtree_generator;
xtree_generator xtrees_from_xforest(xforest const& forest);

struct less_xtree_score {
    bool operator()( xtree_ptr const& t1, xtree_ptr const& t2 ) const
    {
        return t1->scr < t2->scr;
    }
};

struct less_xtree_children_score {
    bool operator()(xtree_children const& c1, xtree_children const& c2) const
    {
        score_t s1;
        BOOST_FOREACH(xtree_ptr const& t1, c1) { s1 *= t1->scr; }
        score_t s2;
        BOOST_FOREACH(xtree_ptr const& t2, c2) { s2 *= t2->scr; }
        return s1 < s2;
    }
};

struct append_xtree_children {
    typedef xtree_children result_type;
    xtree_children operator()(xtree_children c, xtree_ptr const& p) const
    {
        c.push_back(p);
        return c;
    }
};

struct make_xtree_children {
    typedef xtree_children result_type;
    xtree_children operator()(xtree_ptr const& p) const
    {
        return xtree_children(1,p);
    }
};


std::ostream& operator << (std::ostream& out, xtree_ptr const& t);


typedef gusc::any_generator<std::vector< boost::shared_ptr<xtree> >,gusc::iterator_tag> xtree_children_generator;
typedef gusc::shared_lazy_sequence<xtree_children_generator> xtree_children_list;
typedef gusc::shared_lazy_sequence<xtree_generator> xtree_list;
typedef std::tr1::unordered_map<void const*,xtree_list> ornode_map;

xtree_list
xtrees_from_xforest(xforest const& forest, ornode_map& omap);
xtree_generator 
xtrees_from_xhyp(xhyp const& hyp, ornode_map& omap);

xtree_children_generator 
generate_xtree_children(xhyp const& hyp, ornode_map& omap);

struct make_xtree {
    xhyp hyp;
    make_xtree(xhyp h)
    : hyp(h){}
    
    typedef xtree_ptr result_type;
    xtree_ptr operator()(xtree_children const& c) const
    {
        return xtree_ptr(new xtree(hyp,c));
    }
};

xtree_generator 
xtrees_from_xhyp(xhyp const& hyp, ornode_map& omap);

struct make_xtree_generator_from_xhyp {
    make_xtree_generator_from_xhyp(ornode_map& omap) 
    : omap(&omap) {}
    ornode_map* omap;
    typedef xtree_generator result_type;
    xtree_generator operator()(xhyp const& hyp) const
    {
        return xtrees_from_xhyp(hyp,*omap);
    }
};

xtree_list 
xtrees_from_xforest(xforest const& forest, ornode_map& omap);

struct toplevel_generator {
    typedef xtree_ptr result_type;
    boost::shared_ptr<ornode_map> omap;
    xtree_generator gen;
    toplevel_generator(xforest const& forest)
    : omap(new ornode_map())
    , gen(gusc::generate_from_range(xtrees_from_xforest(forest,*omap)))
    {
        
    }
    operator bool() const { return bool(gen); }
    xtree_ptr operator()() { return gen(); }
};

xtree_generator xtrees_from_xforest(xforest const& forest);

feature_vector features(rule_data const& rd, feature_dictionary& fdict);

feature_vector accum(xtree_ptr const& t);

std::string hyptree(xtree_ptr const& t, in_memory_dictionary& dict);

std::string nbest_features(feature_vector const& f, feature_dictionary& dict);

struct dictdata {
    indexed_token_factory d;
    feature_names_type f;
    feature_names_type& feature_names() { return f; }
    indexed_token_factory& dict() { return d; }
    weight_vector weights;
};

rulemap_t read_grammar( std::istream& in
                      , in_memory_dictionary& dict
                      , feature_dictionary& fdict
                      , weight_vector const& wv );
} }

# endif // MUGU__T2S_HPP
