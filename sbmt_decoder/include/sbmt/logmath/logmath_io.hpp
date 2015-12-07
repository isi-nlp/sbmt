#ifndef   SBMT_LOGMATH_LOGMATH_IO_HPP
#define   SBMT_LOGMATH_LOGMATH_IO_HPP

#include <sbmt/logmath/lognumber.hpp>
#include <iostream>

namespace sbmt { namespace logmath {

enum format_scale { fmt_default_scale=0, fmt_log_scale= 1, fmt_linear_scale= 2, fmt_neglog10_scale=3 };
enum format_base { fmt_default_base=0, fmt_base_e=1, fmt_base_10=2 };

// both return previous.  default means set to original default :)
format_scale set_default_scale(format_scale fmt_scale=fmt_default_scale);
format_base set_default_base(format_base fmt_base=fmt_default_base);



////////////////////////////////////////////////////////////////////////////////
///
/// just type:
/// \code
///     os >> log_scale >> lp;
///     os << linear_scale << lp;
/// \endcode
///
/// or vice-versa
///
////////////////////////////////////////////////////////////////////////////////
std::ios_base& 
neglog10_scale(std::ios_base& ios); // if you use this on output, you must also use it on input or you'll be wrong.  on input, still recognized e^123 and 10^123 as 123, but treats 0 and 1 as 10^0 and 10^1

std::ios_base& 
linear_scale(std::ios_base& ios); // print as double - easy to lose precision!

std::ios_base& 
log_scale(std::ios_base& ios); // is default - important for lexical cast

std::ios_base&
default_scale(std::ios_base& ios);

std::ios_base& 
log_base_e(std::ios_base& ios);

std::ios_base& 
log_base_10(std::ios_base& ios); // is default

std::ios_base&
log_base_default(std::ios_base& ios);

template <class O>
inline void set_neglog10(O &out,bool scores_neglog10=false)
{
    if (scores_neglog10)
        out<<neglog10_scale;
    else {
        out<<log_scale;
        out<<log_base_10;
    }
}


/*
template <class BaseT, class F,class CharT, class TraitsT>            
std::basic_ostream<CharT,TraitsT>& 
operator << (std::basic_ostream<CharT,TraitsT>& out, basic_lognumber<BaseT,F> const& p);

template <class BaseT, class F,class CharT, class TraitsT>
std::basic_istream<CharT,TraitsT>&
operator >> (std::basic_istream<CharT,TraitsT> &in, basic_lognumber<BaseT,F>& p);
*/

} } // namespace sbmt::logmath

#include <sbmt/logmath/impl/logmath_io.ipp>

namespace sbmt { namespace logmath {
typedef set_format_scale::save save_scale;
typedef set_format_base::save save_base;
struct save_format : protected save_scale, protected save_base
{
    save_format(std::ios_base &ios) : save_scale(ios), save_base(ios) {}
    void restore() 
    {
        save_scale::restore();
        save_base::restore();
    }
};

    
} } // namespace sbmt::logmath

#endif // SBMT_LOGMATH_LOGMATH_IO_HPP
