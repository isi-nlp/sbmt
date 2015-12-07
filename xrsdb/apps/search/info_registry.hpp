# ifndef XRSDB__SEARCH__INFO_REGISTRY_HPP
# define XRSDB__SEARCH__INFO_REGISTRY_HPP

# include <search/grammar.hpp>
# include <sbmt/edge/any_info.hpp>
# include <string>
# include <map>
# include <boost/program_options.hpp>
# include <gusc/varray.hpp>

namespace xrsdb { namespace search {

typedef sbmt::any_type_info_factory_constructor<grammar_facade> 
        any_xinfo_factory_constructor;

typedef std::map< std::string
        , sbmt::any_type_info_factory_constructor<grammar_facade> 
        > info_registry_type;

typedef sbmt::any_type_info_factory<grammar_facade> any_xinfo_factory;

typedef sbmt::any_type_info<grammar_facade> any_xinfo;
typedef gusc::shared_varray<any_xinfo> info_vec;

boost::program_options::options_description get_info_options();

void set_info_option(std::string info, std::string key, std::string val);

void tee_info_option(std::string info, std::string key, std::string val);

gusc::shared_varray<any_xinfo_factory> 
get_info_factories( std::string info_names_str
                  , grammar_facade& gram
                  , sbmt::lattice_tree& ltree 
                  , sbmt::property_map_type& pmap );

void init_info_factories(sbmt::indexed_token_factory& dict);
void make_fullmap(sbmt::in_memory_dictionary& d);

sbmt::property_map_type& get_property_map();
}} // namespace xrsdb::search

# endif // XRSDB__SEARCH__INFO_REGISTRY_HPP
