#include <boost/lexical_cast.hpp>
#include <sbmt/logmath/lexical_cast.hpp>

#ifdef OLD_LOGMATH_LEXICAL_CAST
#include <boost/regex.hpp>
#endif

namespace sbmt { namespace logmath { 

#ifdef OLD_LOGMATH_LEXICAL_CAST
extern const boost::regex match_e_carat;
extern const boost::regex match_10_carat;
#endif

template<typename LogNumber>
basic_lognumber<typename LogNumber::base_type,typename LogNumber::float_type> 
lexical_cast(std::string const& str)    
{
    typedef basic_lognumber<typename LogNumber::base_type,typename LogNumber::float_type>
        lognumber_t;

#ifdef OLD_LOGMATH_LEXICAL_CAST
    // cast e^-2.04
    if (boost::regex_search(str,match_e_carat)) {
        return lognumber_t(
            boost::lexical_cast<double>(
                boost::regex_replace(str,match_e_carat,"")
                ),
            as_ln()
            );
    } 
    // cast 10^-1.65
    else if (boost::regex_search(str,match_10_carat)) {
        return lognumber_t(
            boost::lexical_cast<double>(
                boost::regex_replace(str,match_10_carat,"")
                ),
            as_log10()
            );
    } 
    // cast 0.305
    else return boost::lexical_cast<double>(str);
#else
    return boost::lexical_cast<lognumber_t>(str); // NOTE: logmath_io.cpp sets log as default (fortunate for lexical_cast!)
#endif 
}

} } // sbmt::logmath




