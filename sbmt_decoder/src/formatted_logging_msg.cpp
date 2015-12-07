#include <sbmt/io/formatted_logging_msg.hpp>
#include <sbmt/io/detail/logging_public_stream.hpp>
#include <sbmt/io/logging_stream.hpp>
#include <string>

namespace sbmt { namespace io {

////////////////////////////////////////////////////////////////////////////////

formatted_logging_msg::formatted_logging_msg( std::string const& fmt_string
                                            , logging_level loglevel
                                            , bool ignore )
: ignore(ignore)
, loglevel(loglevel)
, fmt("")
{
    if (!ignore) fmt = boost::format(fmt_string);
}        

////////////////////////////////////////////////////////////////////////////////

detail::logging_public_stream& 
operator << (logging_stream& str, formatted_logging_msg const& fmt)
{
    detail::logging_public_stream& pstr = (str << fmt.loglevel);
    pstr << fmt.fmt;
    return pstr;
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::io   
                              
