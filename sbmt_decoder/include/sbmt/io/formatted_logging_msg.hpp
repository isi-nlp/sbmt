#ifndef   SBMT_IO_FORMATTED_MESSAGE
#define   SBMT_IO_FORMATTED_MESSAGE

#include <sbmt/io/logging_fwd.hpp>
#include <sbmt/io/logging_level.hpp>
#include <iostream>
#include <boost/format.hpp>

namespace sbmt { namespace io {

////////////////////////////////////////////////////////////////////////////////

detail::logging_public_stream&
operator<<(logging_stream& str, formatted_logging_msg const& fmt);

inline detail::logging_public_stream&
operator<<(logging_stream& str, formatted_logging_msg& fmt)
{
    return (str << const_cast<formatted_logging_msg const&>(fmt));
}

////////////////////////////////////////////////////////////////////////////////

class formatted_logging_msg {
public:
    formatted_logging_msg( std::string const& fmt_string
                         , logging_level loglevel
                         , bool ignore = false );
     
    template <class T>
    formatted_logging_msg& operator % (T const& arg);
    
private:
    bool                ignore;
    logging_level_manip loglevel;
    boost::format       fmt;
    
    friend detail::logging_public_stream& 
        operator << (logging_stream& str, formatted_logging_msg const& fmt);
    
};

////////////////////////////////////////////////////////////////////////////////

template <class T>
formatted_logging_msg&
formatted_logging_msg::operator % ( T const& arg )
{
    if (!ignore) fmt % arg;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::io
#endif // SBMT_IO_FORMATTED_MESSAGE
