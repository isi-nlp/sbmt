# if ! defined(SBMT__EDGE__COMPOSITE_INFO_HPP)
# define       SBMT__EDGE__COMPOSITE_INFO_HPP

# include <sbmt/edge/any_info.hpp>
# include <sbmt/edge/edge_info.hpp>
# include <gusc/varray.hpp>
# include <boost/functional/hash.hpp>
# include <vector>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

class composite_info : public info_base<composite_info> {
public:
    composite_info() {}
    
    composite_info(size_t order,edge_info<any_info> const& ei) 
      : infos(order, ei) {}
    
    size_t size() const { return infos.size(); }
    
    bool equal_to(composite_info const& other) const;
    size_t hash_value() const;
    
    template <class AnyInfoIterator>
    composite_info(AnyInfoIterator begin, AnyInfoIterator end) 
      : infos(begin,end) {}
      
    edge_info<any_info> const& info(size_t index) const { return infos.at(index); }
    edge_info<any_info>& info(size_t index) { return infos.at(index); }
private:
    gusc::shared_varray< edge_info<any_info> > infos;
};

inline size_t composite_info::hash_value() const
{
    size_t x(0);
    boost::hash_range(infos.begin(),infos.end());
    return x;
}

inline bool composite_info::equal_to(composite_info const& other) const
{
    if (other.size() != size()) return false;
    for (size_t x = 0; x != size(); ++x) {
        if (other.info(x) != info(x)) return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

class composite_info_factory {
public:
    typedef composite_info info_type;
    size_t size() const { return factories.size(); }
    template <class AnyInfoFactoryIterator>
    composite_info_factory( AnyInfoFactoryIterator begin
                          , AnyInfoFactoryIterator end )
    : factories(begin,end) {}
      
    edge_info_factory<any_info_factory>& factory(size_t index) 
    { 
        return factories.at(index); 
    }
    
    std::string hash_string(grammar_in_mem const& gram, composite_info const& c)
    {
        std::string retval;
        for (size_t x = 0; x != c.size(); ++x) {
            retval += "[" + factories.at(x).hash_string(gram,c.info(x)) + "]";
        }
        return retval;
    }
    
    score_t rule_heuristic(grammar_in_mem const& gram, grammar_in_mem::rule_type r);
private:
    std::vector< edge_info_factory<any_info_factory> > factories;
};

inline score_t
composite_info_factory::rule_heuristic( grammar_in_mem const& gram
                                      , grammar_in_mem::rule_type r )
{
    score_t ret = 1.0;
    for (size_t x = 0; x != size(); ++x) {
        ret *= factory(x).rule_heuristic(gram,r);
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

property_constructors<>
info_registry_create_property_constructors(std::vector<std::string> const&);

composite_info_factory 
info_registry_create_composite_info_factory( std::vector<std::string> const&
                                           , grammar_in_mem&
                                           , lattice_tree const&
                                           , property_constructors<> const& );

} // namespace sbmt

# endif //     SBMT__EDGE__COMPOSITE_INFO_HPP
