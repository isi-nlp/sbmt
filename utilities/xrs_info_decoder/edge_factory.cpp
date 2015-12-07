/* 
# include <decoder.hpp>
# include <sstream>
# include <iterator>

using namespace sbmt;
using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////////////////

ecs_type create_edge_factory( vector<string> const& info_names
                            , grammar_in_mem& grammar
                            , property_constructors<> const& pc )
{
    stringstream sstr;
    copy( info_names.begin()
        , info_names.end()
        , ostream_iterator<string>(sstr," ")
        );
    SBMT_INFO_MSG(xrs_info_decoder, "using info types: %s", sstr.str());
    
    typedef edge_factory<composite_info_factory> edge_factory_type;
    composite_info_factory 
        info_fctry = info_registry_create_composite_info_factory( info_names
                                                                , grammar
                                                                , pc );
    ecs_type ecs(in_place<edge_factory_type>(info_fctry));
    return ecs;    
}

////////////////////////////////////////////////////////////////////////////////
*/
