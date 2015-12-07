# include <sbmt/edge/any_info.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <vector>
# include <string>
# include <boost/program_options.hpp>

struct composite_edge_interface {
    typedef sbmt::composite_info info_type;
    typedef sbmt::edge<info_type> edge_type;
    typedef sbmt::grammar_in_mem gram_type;
    typedef sbmt::concrete_edge_factory<edge_type,gram_type> ecs_type;
    
    std::string info_names_str;
    
    boost::program_options::options_description options()
    {
        boost::program_options::options_description 
            od = sbmt::info_registry_options();
        od.add_options()
            ( "use-info,u"
            , boost::program_options::value(&info_names_str)
            , "comma-seperated list of which info types to use in decoding"
            )
            ;
        return od;
    }
    
    std::vector<std::string> info_names() const
    {
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        boost::char_separator<char> sep(",");
        tokenizer tokens(info_names_str, sep);
        std::vector<std::string> retval;
        std::copy(tokens.begin(),tokens.end(),std::back_inserter(retval));
        if (retval.size() == 0) retval.push_back("null");
        return retval;
    }
    
    sbmt::property_constructors<> grammar_properties() const
    {

        return sbmt::info_registry_create_property_constructors(info_names());
    }
    
    ecs_type create_factory(gram_type& grammar, sbmt::lattice_tree const& lat) const
    {
        //SBMT_INFO_MSG(xrs_info_decoder, "using info types: %s", sstr.str());

        typedef sbmt::edge_factory<sbmt::composite_info_factory> edge_factory_type;
        sbmt::composite_info_factory 
            info_fctry = info_registry_create_composite_info_factory( info_names()
                                                                    , grammar
                                                                    , lat
                                                                    , grammar_properties() );
        ecs_type ecs(boost::in_place<edge_factory_type>(info_fctry));
        return ecs;    
    }
};
