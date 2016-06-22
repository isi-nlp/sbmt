# if ! defined(SBMT__GRAMMAR__GRAMMAR_PROPERTY_CONSTRUCT_HPP)
# define       SBMT__GRAMMAR__GRAMMAR_PROPERTY_CONSTRUCT_HPP

# include <string>
# include <gusc/const_any.hpp>
# include <boost/range.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_comparison.hpp>
# include <vector>
# include <map>
# include <sbmt/token.hpp>
# include <boost/function.hpp>

namespace sbmt {
    
typedef std::map<std::string,size_t> property_map_type;


////////////////////////////////////////////////////////////////////////////////
///
/// different info types may require that specific features be attached to 
/// binary rules.  lm_string, dlm_string, etc.
/// up until now, we have hacked grammar_in_mem to have those
/// features encoded as member functions.
///
/// we have to get away from that to really support extendable info types.
/// instead, each info type will register which fields it expects to find in 
/// the binarized rule file, and a function for constructing itself from a string
/// the grammar will then store the field as a gusc::const_any.
///
/// example:
/// \code
///  // write a functor to create an lm-string
///  struct construct_lm_string {
///       typedef indexed_lm_string result_type;
///       template <class Dictionary>
///       indexed_lm_string operator()(Dictionary& dict, std::string)
///       { ... }
///  };
///
///  // register your functor with a grammar_property_map
///  property_construct<indexed_token_factory> gpmap;
///  size_t lm_string_id = gmap.register_property("lm_string",construct_lm_string());
///
///  // ...
///  // a grammar_in_mem gram gets constructed with gpmap...
///  // ...
///  // we retrieve an lm-string ...
///  indexed_lm_string const& str = gram.get_property<indexed_lm_string>(rule,lm_string_id);
///
////////////////////////////////////////////////////////////////////////////////
template <class TokenMap = sbmt::indexed_token_factory>
class property_constructors {
public:
    typedef boost::function<gusc::const_any (TokenMap&,std::string)> constructor_type;
    template <class F>
    struct any_wrapper {
        F f;
        any_wrapper(F const& f) : f(f) {}
        gusc::const_any operator()(TokenMap& tm, std::string const& c)
        {
            return gusc::const_any(f(tm,c));
        }
    };
    
private:
    struct data {
        constructor_type constructor;
        size_t id;
        std::string info;
        data(constructor_type const& c, size_t id, std::string const& info) 
          : constructor(c)
          , id(id)
          , info(info) {}
    };
    typedef std::multimap< std::string
                         , data
                         > property_map_t;
    property_map_t pmap;

public:
    typedef typename property_map_t::const_iterator iterator;
    iterator begin() const { return pmap.begin(); }
    iterator end() const { return pmap.end(); }
    std::pair<iterator,iterator> find(std::string const& name) const { return pmap.equal_range(name); }
    template <class F>
    size_t register_constructor(std::string propname, F construct, std::string info = "");
    
    size_t register_constructor(std::string propname, constructor_type construct, std::string info = "");

    template <class KVI, class PD>
    std::vector<gusc::const_any>
    construct_properties(TokenMap& tm, KVI const& range, PD const& pdict) const;
    template <class KVRange>
    std::vector<gusc::const_any> 
    construct_properties(TokenMap& tm, KVRange const& range ) const;
                        
    std::map<std::string,size_t> property_map(std::string info = "") const;
};

////////////////////////////////////////////////////////////////////////////////

template <class D>
std::map<std::string,size_t>
property_constructors<D>::property_map(std::string info) const
{
    std::map<std::string,size_t> retval;
    typename property_map_t::const_iterator itr = pmap.begin(), end = pmap.end();
    for (; itr != end; ++itr) if (itr->second.info == info) {
        retval[itr->first] = itr->second.id;
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class D> template <class F> size_t 
property_constructors<D>::register_constructor( std::string propname
                                              , F construct
                                              , std::string info )
{
    constructor_type c = any_wrapper<F>(construct);
    return register_constructor(propname,c,info);
}

template <class D> size_t 
property_constructors<D>::register_constructor( std::string propname
                                              , constructor_type c
                                              , std::string info )
{
    size_t sz = pmap.size();
    pmap.insert(std::make_pair(propname,data(c,sz,info)));
    return sz;
}

////////////////////////////////////////////////////////////////////////////////

template <class D> template <class KVI> 
std::vector<gusc::const_any> 
property_constructors<D>::construct_properties(D& tm, KVI const& range) const
{
    std::vector<gusc::const_any> retval(pmap.size());
    typename boost::range_const_iterator<KVI const>::type itr;
    for (itr = boost::begin(range); itr != boost::end(range); ++itr) {
        typename property_map_t::const_iterator i, e;
        boost::tie(i,e) = pmap.equal_range(itr->first);
        //std::cerr << "  examine props for "<< itr->first << "\n";
        for (; i != e; ++i) {
            //std::cerr << "    retval["<<i->second.id<<"] constructed\n"; 
            retval[i->second.id] = i->second.constructor(tm,itr->second);
        }
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class D> template <class KVI, class PD> 
std::vector<gusc::const_any>
property_constructors<D>::construct_properties( D& tm
                                              , KVI const& range
                                              , PD const& pdict ) const
{
    std::vector<gusc::const_any> retval(pmap.size());
    typename boost::range_const_iterator<KVI const>::type itr;
    for (itr = boost::begin(range); itr != boost::end(range); ++itr) {
        typename property_map_t::const_iterator i, e;
        boost::tie(i,e) = pmap.equal_range(pdict.get_token(itr->first));
        //std::cerr << "  construct props for "<< pdict.get_token(itr->first) << "\n";
        for (; i != e; ++i) {
            //std::cerr << "    retval["<<i->second.id<<"] constructed\n";
            retval[i->second.id] = i->second.constructor(tm,itr->second);
        }
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__GRAMMAR__GRAMMAR_PROPERTY_CONSTRUCT_HPP
