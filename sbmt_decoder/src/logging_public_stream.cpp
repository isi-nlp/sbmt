#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/device/file.hpp>
#include <sbmt/io/detail/logging_public_stream.hpp>
#include <sbmt/io/formatted_logging_msg.hpp>
#include <sbmt/io/logging_level.hpp>
#include <boost/ref.hpp>

namespace sbmt { namespace io { namespace detail {

////////////////////////////////////////////////////////////////////////////////

logging_public_stream::logging_public_stream( sync_logging_buffer& b
                                            , std::string const& nm
                                            , logging_level ignore_above )
  : ignore_above(ignore_above)
  , current_level(ignore_above)
  , my_name(nm)
  , buf(&b)
{
    push(logging_filter(ignore_above,&current_level));
    push(boost::ref(b));
    assert(is_complete());
}

////////////////////////////////////////////////////////////////////////////////

logging_public_stream::logging_public_stream( std::streambuf& b
                                            , std::string const& nm
                                            , logging_level ignore_above )
  : ignore_above(ignore_above)
  , current_level(ignore_above)
  , my_name(nm)
{
    sync_logging_buffer slb(&b);
    push(logging_filter(ignore_above,&current_level));
    push(slb);
    buf = component<sync_logging_buffer>(1);
    assert(is_complete());
}

////////////////////////////////////////////////////////////////////////////////

void logging_public_stream::set_logging_level(logging_level lvl)
{
    bool ignore = (lvl > ignore_above);
    current_level = lvl;
    strict_sync();
    buf->flush();
    if (!ignore) {
        *this << '[' << my_name << "][" << lvl << "]: ";
    }
}

////////////////////////////////////////////////////////////////////////////////

void logging_public_stream::end_msg()
{
    buf->flush();
}

////////////////////////////////////////////////////////////////////////////////

formatted_logging_msg
logging_public_stream::formatted_msg( logging_level lvl
                                    , std::string const& fmt) const
{
    return formatted_logging_msg(fmt,lvl,lvl > ignore_above);
}

////////////////////////////////////////////////////////////////////////////////

} } } // namespace sbmt::io::detail
