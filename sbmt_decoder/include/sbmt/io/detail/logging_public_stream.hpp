#ifndef   SBMT_IO_DETAIL_LOGGING_PUBLIC_STREAM
#define   SBMT_IO_DETAIL_LOGGING_PUBLIC_STREAM

#ifdef _WIN32
#include <iso646.h>
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <boost/iostreams/filtering_stream.hpp>

#include <sbmt/io/logging_fwd.hpp>
#include <boost/iostreams/device/file.hpp>
#include <sbmt/io/detail/logging_filter.hpp>
#include <sbmt/io/detail/sync_logging_buffer.hpp>

namespace sbmt { namespace io { 

namespace detail {

class logging_finish_stream {};

////////////////////////////////////////////////////////////////////////////////
///
///  uses the chain 
///     sink <-- sync <-- filt <-- log
///  where 
///   - sink is a file or stdout/stderr (the logged to file)
///   - sync buffers messages, and only flushes them when it is told the 
///     message is complete.
///   - filt skips over any data while the current logging_level 
///     is below the ignore threshold 
///
///  the reason logging_public_stream exists in addition to logging_stream, is 
///  to enforce at compile time the fact that you should put a logging level
///  into your stream before using it, and that inserting an endmsg prevents
///  further messages from being sent until a new logging level is inserted.
/// 
////////////////////////////////////////////////////////////////////////////////
class logging_public_stream 
: public boost::iostreams::filtering_ostream
, public logging_finish_stream
{
public:
    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  allows you to turn any standard buffer into a logging stream.
    ///  used primarily in test code.  your buffer should be alive as long as
    ///  the logging stream.
    ///
    ///////////////////////////////////////////////////////////////////////////
    logging_public_stream( std::streambuf& buf
                         , std::string const& label
                         , logging_level ignore_above );
                         
    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  to share a streambuf safely between multiple logs on multiple threads,
    ///  wrap the streambuf in a sync_logging_buffer along with a mutex
    ///  used primarily in the logfile_registry.
    ///
    ////////////////////////////////////////////////////////////////////////////
    logging_public_stream( sync_logging_buffer& buf
                         , std::string const& label
                         , logging_level ignore_above );
    
    virtual ~logging_public_stream(){}
    void set_logging_level(logging_level lvl);
    logging_level minimal_level() const { return ignore_above; }
    std::string const& name() const { return my_name; }
    bool ignoring() const { return current_level > ignore_above; }
protected:
    formatted_logging_msg 
        formatted_msg(logging_level lvl, std::string const& msg) const;
private:
    void end_msg();
    logging_level ignore_above;
    logging_level current_level;
    std::string   my_name;
    sync_logging_buffer* buf;
    friend logging_finish_stream& ::sbmt::io::endmsg(logging_public_stream&);
};

////////////////////////////////////////////////////////////////////////////////
///
///  short-circuits messages to the stream if they arent above an acceptable
///  logging level.  so even if you dont wrap your log in a macro or an if 
///  statement, you get good efficiency as long as the compiler can deduce its
///   working with a logging_public_stream.
///
///  \note: this template may conflict with lazily produced templates of the
///  form
///  \code
///  template<class O> O& operator<<(O&, myclass const&);
///  \endcode
///
///  to fix, simply convert to
///  \code
///  template<class C,class T> std::basic_ostream<C,T>&
///  operator<<(std::basic_ostream<C,T>&, myclass const&);
///  \endcode
///
///  or whatever ios base-class makes the most sense for your classes printing
///  capabilities.
///
///  casting to std::ostream defeats this optimization, though the stream still
///  behaves correctly (ignores messages below a given threshold).
///
////////////////////////////////////////////////////////////////////////////////
template <class T>
logging_public_stream& operator << (logging_public_stream& log, T& t)
{
    if (not log.ignoring()) {
        static_cast<boost::iostreams::filtering_ostream&>(log) << t;
    }
    return log;
}

template <class T>
logging_public_stream& operator << (logging_public_stream& log, T const& t)
{
    if (not log.ignoring()) {
        static_cast<boost::iostreams::filtering_ostream&>(log) << t;
    }
    return log;
}

inline logging_finish_stream& 
operator<< ( logging_public_stream& log
           , logging_finish_stream& (*f)(logging_public_stream&) )
{
    return f(log);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt::io::detail


////////////////////////////////////////////////////////////////////////////////
///
///  log << info_msg << "some message" << endmsg;
///  signals the end of a message.  not truly necessary, since the start of a
///  new message also signals the end of the old message.
///  but it can be useful if you are expecting to see a message appear, but it
///  doesnt because you havent started a new message yet.
///
///  trying to continue logging after sending endmsg, but before setting a new
///  message level, results in a runtime error
///
///  \code
///  log << info_msg << "some message" << endmsg;
///  continue_log(log) << "more of the message"; // runtime error!
///  \endcode
///
///  \code
///  log << info_msg << "some message" << endmsg 
///      << "more message"; // compile time error!
///  \endcode
///
///  note: endl and flush will not necessarily flush your message, because
///  logging streams refuse to flush a log message until they are sure that
///  the message is over.  this prevents multiple threads from corrupting a
///  shared output stream.
///  
////////////////////////////////////////////////////////////////////////////////
inline
detail::logging_finish_stream& endmsg(detail::logging_public_stream& log)
{
    if (not log.ignoring()) {
        log << std::endl;
    }
    log.end_msg();
    return log;
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::io

#endif // SBMT_IO_DETAIL_LOGGING_PUBLIC_STREAM
