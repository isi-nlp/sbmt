# include <sbmt/edge/any_info.hpp>
# include <sbmt/edge/composite_info.hpp>
# include <map>
# include <string>
# include <boost/regex.hpp>


namespace sbmt {



boost::regex namedata("^([^\\]\\]]*)(\\[(greedy|split)\\])?$");

boost::tuple<std::string,bool>
procname(std::string const& infoname)
{
    boost::smatch m;
    if (not regex_match(infoname,m,namedata)) {
        throw std::runtime_error("info-registry: bad info name "+infoname);
    }
    std::string name = m.str(1);
    bool greedy = false;
    if (m.size() > 3 and m.str(3) == "greedy") greedy = true;
    return boost::make_tuple(name,greedy);
}

////////////////////////////////////////////////////////////////////////////////

typedef property_constructors_type::constructor_type property_constructor_type;

////////////////////////////////////////////////////////////////////////////////

class info_factory_registry : boost::noncopyable {
    struct constructors {
        typedef std::map<std::string,property_constructor_type>
                property_constructor_map;
        property_constructor_map property_constructors;
        any_info_factory_constructor info_factory_constructor;

        constructors(any_info_factory_constructor const& c)
          : info_factory_constructor(c) {}
    };

    typedef std::map<std::string, constructors>
            registry_map_type;
public:
    void set_option(std::string info, std::string optname, std::string optval)
    {
        registry_map_type::iterator pos = registry.find(info);
        options_map opts = pos->second.info_factory_constructor.get_options();
        options_map::iterator p = opts.find(optname);
        if (p == opts.end()) {
            bool s = pos->second.info_factory_constructor.set_option(optname,optval);
            if (not s) {
                throw std::runtime_error(
                    "option " + optname + " does not exist for info-type " + info
                );
            }
        } else {
            p->second.opt(optval);
        }
    }
    void
    register_constructor( std::string name
                        , any_info_factory_constructor const& ifc )
    {
        registry_map_type::iterator pos = registry.find(name);
        if (pos != registry.end()) {
            throw std::runtime_error("info_factory_registry name conflict");
        }

        registry.insert(std::make_pair(name,constructors(ifc)));
    }
    
    void unregister_property(std::string iname, std::string pname)
    {
        registry_map_type::iterator pos = registry.find(iname);
        if (pos == registry.end()) {
            throw std::runtime_error("info_factory_constructor missing");
        }
        pos->second.property_constructors.erase(pname);
    }

    void register_property_constructor( std::string info_factory_name
                                      , std::string property_name
                                      , property_constructor_type const& pc )
    {
        registry_map_type::iterator pos = registry.find(info_factory_name);
        if (pos == registry.end()) {
            throw std::runtime_error("info_factory_constructor missing");
        }
        pos->second.property_constructors.insert(std::make_pair(property_name,pc));
    }

    boost::program_options::options_description get_options()
    {
        boost::program_options::options_description opts(":: info-type options :");

        registry_map_type::iterator itr = registry.begin(), end = registry.end();

        for (; itr != end; ++itr) {
            namespace po = boost::program_options;
            options_map omap = itr->second.info_factory_constructor.get_options();
            po::options_description o(omap.title() + " [" + itr->first + "]");
            for (options_map::iterator i = omap.begin(); i != omap.end(); ++i) {
                o.add_options()
                  ( i->first.c_str()
                  , po::value<std::string>()->notifier(i->second.opt)
                  , i->second.description.c_str()
                  )
                  ;
            }
            opts.add(o);
        }

        return opts;
    }

    property_constructors<>
    create_property_constructors(std::vector<std::string> const& info_names)
    {
        std::vector<std::string>::const_iterator i = info_names.begin(),
                                                 e = info_names.end();

        property_constructors<> pmap;
        for (; i != e; ++i) {
            std::string name;
            bool ignore;
            boost::tie(name,ignore) = procname(*i);
            constructors::property_constructor_map::const_iterator
                ii = registry.find(name)->second.property_constructors.begin(),
                ee = registry.find(name)->second.property_constructors.end();
            for (; ii != ee; ++ii) {
                pmap.register_constructor(ii->first, ii->second,name);
            }
        }

        return pmap;
    }

    any_info_factory create_info_factory( std::string name
                                        , grammar_in_mem& grammar
                                        , lattice_tree const& lattice
                                        , property_map_type const& pmap )
    {
        return registry.find(name)->second.info_factory_constructor.construct(grammar,lattice,pmap);
    }
    static info_factory_registry& instance();

private:
    info_factory_registry() {}

    registry_map_type registry;
};

////////////////////////////////////////////////////////////////////////////////

void info_registry_set_option( std::string info_name
                             , std::string option_name
                             , std::string option_value )
{
    info_factory_registry::instance().set_option(info_name,option_name,option_value);
}

////////////////////////////////////////////////////////////////////////////////

info_factory_registry& info_factory_registry::instance()
{
    static info_factory_registry self;
    return self;
}

////////////////////////////////////////////////////////////////////////////////

void register_info_factory_construct( std::string name
                                    , any_info_factory_constructor const& ifc )
{
    info_factory_registry::instance().register_constructor(name,ifc);
}

////////////////////////////////////////////////////////////////////////////////

void reg_rule_prop_construct( std::string ifname
                            , std::string pname
                            , property_constructor_type pc )
{
    info_factory_registry::instance().register_property_constructor( ifname
                                                                   , pname
                                                                   , pc );
}

void unregister_rule_property_constructor( std::string iname
                                         , std::string pname )
{
    info_factory_registry::instance().unregister_property(iname,pname);
}

////////////////////////////////////////////////////////////////////////////////

boost::program_options::options_description info_registry_options()
{
    return info_factory_registry::instance().get_options();
}

////////////////////////////////////////////////////////////////////////////////

property_constructors<>
info_registry_create_property_constructors(std::vector<std::string> const& names)
{
    return info_factory_registry::instance().create_property_constructors(names);
}

composite_info_factory
info_registry_create_composite_info_factory( std::vector<std::string> const& names
                                           , grammar_in_mem& grammar
                                           , lattice_tree const& lattice
                                           , property_constructors<> const& pc )
{
    std::vector< edge_info_factory<any_info_factory> > factories;
    std::vector<std::string>::const_iterator i = names.begin(),
                                             e = names.end();

    for (; i != e; ++i) {
        std::string name;
        bool greedy;
        boost::tie(name,greedy) = procname(*i);
        factories.push_back(
          edge_info_factory<any_info_factory>(
            info_factory_registry::instance().create_info_factory( 
              name
            , grammar
            , lattice
            , pc.property_map(*i) 
            )
          , greedy                                   
          )
        );
    }

    composite_info_factory cif(factories.begin(),factories.end());

    return cif;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
