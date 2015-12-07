# include <sbmt/edge/ngram_constructor.hpp>
# include <boost/program_options.hpp>
# include <sbmt/grammar/rule_feature_constructors.hpp>
# include <sbmt/search/block_lattice_tree.hpp>

template <unsigned int N>
struct ngram_edge_interface {
    typedef sbmt::edge_info_factory< 
              sbmt::ngram_info_factory<N,sbmt::dynamic_ngram_lm> 
            > info_factory_type;
    typedef sbmt::edge_factory< 
              info_factory_type
            > edge_factory_type;
    
    typedef typename info_factory_type::info_type info_type;
    typedef typename edge_factory_type::edge_type edge_type;
    typedef sbmt::grammar_in_mem gram_type;
    typedef sbmt::concrete_edge_factory<edge_type,gram_type> ecs_type;
    
    sbmt::ngram_constructor constructor;
    
    ecs_type create_factory(gram_type& grammar,sbmt::lattice_tree const& lat)
    {
        boost::shared_ptr<info_factory_type> 
            ifact(new info_factory_type(constructor.construct<N>(grammar,grammar_properties().property_map())));
            
        return ecs_type(boost::in_place<edge_factory_type>(ifact));
    }
    
    sbmt::property_constructors<> grammar_properties() const
    {
        sbmt::property_constructors<> pc;
        pc.register_constructor("lm_string",sbmt::lm_string_constructor());
        return pc;
    }
    
    boost::program_options::options_description options()
    {
        namespace po = boost::program_options;
        
        sbmt::options_map omap = constructor.get_options();
        po::options_description opts(omap.title());
        for (sbmt::options_map::iterator i = omap.begin(); i != omap.end(); ++i) {
            opts.add_options()
                ( i->first.c_str()
                , po::value<std::string>()->notifier(i->second.opt)
                , i->second.description.c_str()
                )
                ;
        }
        return opts;
    }
};