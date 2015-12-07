namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

void
reg_rule_prop_construct( std::string info_name
                       , std::string property_name
                       , property_constructors_type::constructor_type rpc );

template <class RPC>
void register_rule_property_constructor( std::string info_name
                                       , std::string property_name
                                       , RPC const& rpc )
{
    reg_rule_prop_construct( 
        info_name
      , property_name
      , property_constructors_type::template any_wrapper<RPC>(rpc)
    );
}

////////////////////////////////////////////////////////////////////////////////

void 
register_info_factory_construct( std::string name
                               , any_info_factory_constructor const& ifc );

template <class InfoFactoryConstruct>
void 
register_info_factory_constructor( std::string name
                                 , InfoFactoryConstruct const& ifc )
{
    register_info_factory_construct( name
                                   , any_info_factory_constructor(ifc) );
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
