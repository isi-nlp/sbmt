#include <sbmt/io/logging_stream.hpp>
#include <sbmt/io/formatted_logging_msg.hpp>
#include <string>

namespace sbmt { namespace io {

////////////////////////////////////////////////////////////////////////////////

detail::logging_public_stream& logging_stream::stream()
{
    return *this;
}

////////////////////////////////////////////////////////////////////////////////

formatted_logging_msg
logging_stream::formatted_msg(logging_level lvl, std::string const& msg) const
{
    return logging_public_stream::formatted_msg(lvl,msg);
} 

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::io
