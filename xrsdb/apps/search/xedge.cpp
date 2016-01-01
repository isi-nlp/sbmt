# include <search/xedge.hpp>

namespace xrsdb { namespace search {

void const* id(xequiv const& xeq) { return xeq.edges.get(); }

sbmt::indexed_token root(xedge const& xe) { return xe.rule->rule.lhs_root()->get_token(); }

sbmt::indexed_token root(xequiv const& xeq) { return xeq.root; }

sbmt::indexed_token root(xcell const& xc) { return root(xc[0]); }

float cost(xedge const& xe) { return xe.cost; }

float cost(xequiv const& xeq) { return xeq.begin() != xeq.end() ? xeq.begin()->cost : 0; }

float cost(xcell const& xc) { return cost(xc[0]); }

float cost(xspan const& x) {return x.size() > 0 ? cost(x[0]) : 0.; }

float heur(xedge const& xe) { return xe.heur; }

float heur(xequiv const& xeq) { return xeq.heur; }

float heur(xcell const& xc) { return heur(xc[0]); }

float heur(xspan const& x) {return x.size() > 0 ? heur(x[0]) : 0.; }

info_vec const& infos(xedge const& xe) { return xe.infos; }

info_vec const& infos(xequiv const& xeq) { return xeq.infos; }

xequiv::proxy::operator xedge() const
{
    return xedge(ptr->rule,ptr->children,infos,ptr->cost,heur);
}

xequiv::proxy::proxy(xedge_partial* ptr, float heur, gusc::shared_varray<any_xinfo> infos)
: ptr(ptr)
, heur(heur)
, infos(infos) {}

xequiv::proxy& xequiv::proxy::operator=(xedge const& xe)
{
    if (heur != xe.heur or infos.get() != xe.infos.get()) {
        throw std::logic_error("xequiv::proxy assignment must be from equivalent xedge");
    }
    *ptr = xedge_partial(xe);
    return *this;
}

xequiv::proxy& xequiv::proxy::operator=(proxy const& xe)
{
    if (heur != xe.heur or infos.get() != xe.infos.get()) {
        throw std::logic_error("xequiv::proxy assignment must be from equivalent xedge");
    }
    *ptr = *(xe.ptr);
    return *this;
}

xedge_partial::xedge_partial(xedge const& xe)
: rule(xe.rule)
, children(xe.children)
, cost(xe.cost)
{}

xedge_partial::xedge_partial() : rule(0), cost(0) {}

xequiv::xequiv() : heur(0) {}

xequiv::xequiv(sbmt::span_t span, sbmt::indexed_token root, size_t info_size, rule_application const* rule)
: edges(rule == 0 ? 0 : 1)
, infos(info_size)
, heur(0)
, span(span)
, root(root)
{
    if (rule != 0) {
        edges[0].rule = rule;
        edges[0].cost = rule->cost;
    }
}


xedge xequiv::const_iterator::dereference() const
{
    return xedge( base_reference()->rule
                , base_reference()->children
                , infos
                , base_reference()->cost
                , heur );
}

xequiv::const_iterator::const_iterator( 
  xequiv const* equiv
, gusc::shared_varray<xedge_partial>::const_iterator itr 
)
: xequiv::const_iterator::iterator_adaptor_(itr)
, heur(equiv->heur)
, infos(equiv->infos) {}

xequiv::proxy xequiv::iterator::dereference() const
{
    return proxy( &(*(base_reference()))
                , heur
                , infos );
}

xequiv::iterator::iterator( 
  xequiv* equiv
, gusc::shared_varray<xedge_partial>::iterator itr 
)
: xequiv::iterator::iterator_adaptor_(itr)
, heur(equiv->heur)
, infos(equiv->infos) {}

xequiv::const_iterator xequiv::begin() const 
{ 
    return const_iterator(this,edges.begin()); 
}

xequiv::const_iterator xequiv::end() const 
{ 
    return const_iterator(this,edges.end()); 
}

xequiv::iterator xequiv::begin()
{ 
    return iterator(this,edges.begin()); 
}

xequiv::iterator xequiv::end()
{ 
    return iterator(this,edges.end()); 
}

size_t xequiv::size() const 
{ 
    return edges.size(); 
}

bool xequiv::empty() const 
{ 
    return edges.empty(); 
}

bool operator == (xedge const& xe1, xedge const& xe2)
{
    if (root(xe1) != root(xe2)) return false;
    for (size_t x = 0; x != xe1.infos.size(); ++x) if (xe1.infos[x] != xe2.infos[x]) return false;
    return true;
}

size_t hash_value(xedge const& xe) {
    size_t ret = boost::hash<sbmt::indexed_token>()(root(xe));
    boost::hash_combine(ret,boost::hash_range(xe.infos.begin(),xe.infos.end()));
    return ret;
}

bool operator != (xedge const& xe1, xedge const& xe2)
{
    return !(xe1 == xe2);
}

}} // namespace xrsdb::search


