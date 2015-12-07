#ifndef   SBMT_LOGMATH_LEXICAL_CAST_HPP
#define   SBMT_LOGMATH_LEXICAL_CAST_HPP

#include <string>

namespace sbmt { namespace logmath {

////////////////////////////////////////////////////////////////////////////////
///
///  can convert the following types of strings into a lognumber of your choice
///  e^-1.2334
///  10^-4567
///  0.9345
///  base conversions to the base of your lognumber are handled automatically,
///  and hopefully, correctly.
///
////////////////////////////////////////////////////////////////////////////////
template<typename LogNumber>
basic_lognumber<typename LogNumber::base_type,typename LogNumber::float_type> 
lexical_cast(std::string const& s);

} } // namespace sbmt::logmath

#include <sbmt/logmath/impl/lexical_cast.ipp>

#endif // SBMT_LOGMATH_LEXICAL_CAST_HPP
