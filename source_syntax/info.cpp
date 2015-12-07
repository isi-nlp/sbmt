//TODO: actually use indicator.hpp?
#define USE_OLD 0
#if USE_OLD
# include "info_old.cpp"
#else
# include <graehl/shared/intrusive_refcount.hpp>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <source_syntax/source_syntax.hpp>

using namespace sbmt;

namespace ssyn {

sbmt::options_map constructor::get_options()
{
    sbmt::options_map opts("root-variable agreement");
    opts.add_option( name + "-prefix-name"
                   , sbmt::optvar(ps)
                   , "prefix for feature names, ie ${prefix}_match, $prefix[x][y], etc.\n"
                     "default=" + name
                   );
    opts.add_option( name + "-root-name"
                   , sbmt::optvar(rs)
                   , "name of root feature attached to rules.\n"
                     "default=${prefix}_root"
                   );
    opts.add_option( name + "-vars-name"
                   , sbmt::optvar(vs)
                   , "name of variable list feature attached to rules.\n"
                     "default=${prefix}_vars"
                   );
    opts.add_option( name + "-match-only"
                   , sbmt::optvar(mo)
                   , "only generate and utilize *_match *_nomatch and *_missing features.\n"
                     "default=false"
                   );
    return opts;
}

constructor::constructor( std::string name
                        , std::string rootname
                        , std::string varsname ) 
: name(name), root(rootname), vars(varsname), mo(false)
{
    ps.cons = this; ps.t = setter::prefix;
    vs.cons = this; vs.t = setter::vars;
    rs.cons = this; rs.t = setter::root;
    prefix = name;
    if (rootname == "") root = name + "_root";
    if (varsname == "") vars = name + "_vars";
    std::cerr << "cons: root:" << root << "    vars:" << vars << '\n';
}

void constructor::swap_self(constructor& o)
{
    swap(name,o.name);
    swap(prefix,o.prefix);
    swap(root,o.root);
    swap(vars,o.vars);
    std::swap(ps.is_set,o.ps.is_set);
    std::swap(rs.is_set,o.rs.is_set);
    std::swap(vs.is_set,o.vs.is_set);
    std::swap(mo,o.mo);
}

constructor::constructor(constructor const& o)
: name(o.name)
, prefix(o.prefix)
, root(o.root)
, vars(o.vars)
, ps(o.ps)
, rs(o.rs)
, vs(o.vs)
, mo(o.mo)
{
    ps.cons = this;
    vs.cons = this;
    rs.cons = this;
    //std::cerr << "ccons: root:" << root << "    vars:" << vars << '\n';
}

constructor& constructor::operator=(constructor const& o)
{
    constructor(o).swap_self(*this);
    return *this;
    std::cerr << "=cons: root:" << root << "    vars:" << vars << '\n';
}

std::ostream& operator << (std::ostream& out, setter const& s)
{
    if (s.t == setter::prefix) return out << s.cons->prefix;
    if (s.t == setter::root) return out << s.cons->root;
    if (s.t == setter::vars) return out << s.cons->vars;
    return out;
}

std::istream& operator >> (std::istream& in, setter& s)
{
    std::string str;
    in >> str;
    s.is_set = true;

    unregister_rule_property_constructor(s.cons->name,s.cons->root);
    unregister_rule_property_constructor(s.cons->name,s.cons->vars);

    if (s.t == setter::prefix) {
        s.cons->prefix = str;
        if (s.cons->rs.is_set == false) s.cons->root = str + "_root";
        if (s.cons->vs.is_set == false) s.cons->vars = str + "_vars";
    } else if (s.t == setter::root) {
        s.cons->root = str;
    } else if (s.t == setter::vars) {
        s.cons->vars = str;
    }

    register_rule_property_constructor(s.cons->name,s.cons->root,read_froot());
    register_rule_property_constructor(s.cons->name,s.cons->vars,read_fvars());

    return in;
}

struct init {
    init()
    {
        # define BOOST_PP_LOCAL_LIMITS (0,9)
        # define BOOST_PP_LOCAL_MACRO(N) \
        register_info_factory_constructor( "rva" BOOST_PP_STRINGIZE(N) \
                                         , constructor("rva" BOOST_PP_STRINGIZE(N)) \
                                         ); \
        register_rule_property_constructor( "rva" BOOST_PP_STRINGIZE(N) \
                                          , "rva" BOOST_PP_STRINGIZE(N) "_root" \
                                          , read_froot() \
                                          ); \
        register_rule_property_constructor( "rva" BOOST_PP_STRINGIZE(N) \
                                          , "rva" BOOST_PP_STRINGIZE(N) "_vars" \
                                          , read_fvars() \
                                          );
        # include BOOST_PP_LOCAL_ITERATE()
    }
};

init init_;

} // namespace ssyn
#endif
