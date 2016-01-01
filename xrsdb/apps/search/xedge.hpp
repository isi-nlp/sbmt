# ifndef XRSDB__SEARCH__XEDGE_HPP
# define XRSDB__SEARCH__XEDGE_HPP

# include <filesystem.hpp>
# include <search/info_registry.hpp>
# include <gusc/varray.hpp>
# include <sbmt/span.hpp>
# include <sbmt/token/indexed_token.hpp>
# include <boost/iterator/iterator_adaptor.hpp>

namespace xrsdb { namespace search {

struct xedge;
typedef gusc::shared_varray<xedge> xequiv_pre;
struct xedge_partial;

struct xequiv;

struct xedge_partial {
    rule_application const* rule;
    gusc::shared_varray<xequiv> children;
    float cost;
    xedge_partial(xedge const& xe);
    xedge_partial();
};

struct xedge {
    rule_application const* rule;
    gusc::shared_varray<xequiv> children;
    gusc::shared_varray<any_xinfo> infos;
    float cost;
    float heur;
    template <class Range, class InfoRange>
    xedge( rule_application const* rule
         , Range const& rng
         , InfoRange const& inforange
         , float cost
         , float heur );
    xedge() : rule(0), cost(0), heur(0) {}
};

struct xequiv {
    struct proxy {
        xedge_partial* ptr;
        float heur;
        gusc::shared_varray<any_xinfo> infos;
        operator xedge() const;
        proxy(xedge_partial* ptr, float heur, gusc::shared_varray<any_xinfo> infos);
        proxy& operator=(xedge const& xe);
        proxy& operator=(proxy const& xe);
    };
    struct iterator 
    : boost::iterator_adaptor<
        iterator
      , gusc::shared_varray<xedge_partial>::iterator
      , xedge
      , boost::random_access_traversal_tag
      , proxy
      > {
        float heur;
        gusc::shared_varray<any_xinfo> infos;
        proxy dereference() const;
        iterator(xequiv* equiv, gusc::shared_varray<xedge_partial>::iterator itr);
    };
    struct const_iterator 
    : boost::iterator_adaptor<
        const_iterator
      , gusc::shared_varray<xedge_partial>::const_iterator
      , xedge
      , boost::random_access_traversal_tag
      , xedge
      > {
        float heur;
        gusc::shared_varray<any_xinfo> infos;
        xedge dereference() const;
        const_iterator(xequiv const* equiv, gusc::shared_varray<xedge_partial>::const_iterator itr);
    };
    typedef xedge value_type;
    typedef xedge const_reference;
    typedef proxy reference;
    gusc::shared_varray<xedge_partial> edges;
    gusc::shared_varray<any_xinfo> infos;
    float heur;
    template <class Range>
    explicit xequiv(Range const& rng);
    xequiv(sbmt::span_t span, sbmt::indexed_token root, size_t info_size, rule_application const* rptr = 0);
    xequiv();
    const_iterator begin() const;
    const_iterator end() const;
    iterator begin();
    iterator end();
    size_t  size() const;
    bool empty() const;
    sbmt::span_t span;
    sbmt::indexed_token root;
};

typedef gusc::shared_varray<xequiv> xcell;
typedef gusc::shared_varray<xcell> xspan;

sbmt::indexed_token root(xedge const& xe);
sbmt::indexed_token root(xequiv const& xeq);
sbmt::indexed_token root(xcell const& xc);

float cost(xedge const& xe);
float cost(xequiv const& xeq);
float cost(xcell const& xc);
float cost(xspan const& x);

float heur(xedge const& xe);
float heur(xequiv const& xeq);
float heur(xcell const& xc);
float heur(xspan const& x);

info_vec const& infos(xedge const& xe);
info_vec const& infos(xequiv const& xeq);

void const* id(xequiv const& xeq);

bool operator == (xedge const& xe1, xedge const& xe2);
size_t hash_value(xedge const& xe);
bool operator != (xedge const& xe1, xedge const& xe2);

// template implementation methods

template <class Range, class InfoRange>
xedge::xedge( rule_application const* rule
            , Range const& rng
            , InfoRange const& inforng
            , float cost
            , float heur )
: rule(rule)
, children(rng)
, infos(inforng)
, cost(cost)
, heur(heur) 
{
    #if 0
    size_t x = 0;
    BOOST_FOREACH(fixed_rule::rule_node const& nd, this->rule->rule.rhs()) {
        //if (nd.indexed()) {
            assert(nd.get_token() == root(children[x]));
            ++x;
        //}
    }
    assert(x == children.size());
    #endif
}

template <class Range>
xequiv::xequiv(Range const& rng)
: edges(rng.size())
, infos(rng[0].infos)
, heur(rng[0].heur)
, root(::xrsdb::search::root(rng[0]))
{
    int m = 10000000;
    int M = -9999999;
    BOOST_FOREACH(xequiv ce, rng[0].children) {
        m = std::min(int(ce.span.left() ), m);
        M = std::max(int(ce.span.right()), M);
    }
    span = sbmt::span_t(m,M);
    for (size_t x = 0; x != edges.size(); ++x) edges[x] = xedge_partial(rng[x]);
}

} } // namespace xrsdb::search

# endif // XRSDB__SEARCH__XEDGE_HPP
