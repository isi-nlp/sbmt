#ifndef   SBMT_IO_LOGGING_STREAM_HPP
#define   SBMT_IO_LOGGING_STREAM_HPP

#include <iostream>
#include <streambuf>
#include <string>

#include <sbmt/io/logging_fwd.hpp>
#include <sbmt/io/detail/logging_public_stream.hpp>

namespace sbmt { namespace io {
    
class logging_stream;
detail::logging_public_stream& continue_log(logging_stream& str);

////////////////////////////////////////////////////////////////////////////////
///
/// private inheritence is intentional. logging_level_manip gives access rights
/// forcing you to insert a logging_level_manip to the stream before use.
/// like 
/// \code 
///    mylog << io::warning_msg << foo;
/// \endcode
///
/// if you need to continue a log at the same level, and dont wish to insert
/// and additional loglevel notification, you can use continue_log()
/// but in multi-threading environments you should not use a flush or endl
/// between continue_log messages:
/// 
/// \code
///  mylog << io::warning_msg << "this is a warning\n";
///  continue_log(mylog) << "message continues...";
///  continue_log(mylog) << "message ends" << std::endl;
/// \endcode
///
////////////////////////////////////////////////////////////////////////////////
class logging_stream 
  : private detail::logging_public_stream 
{
public:                  
    template <class StreamBuf>
    logging_stream( StreamBuf& s
                  , std::string const& name
                  , logging_level ignore_above );
                  
    virtual ~logging_stream() { }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// the intention with these methods is that you use the 
    /// formatted_logging_message with the stream that provided it to you.
    /// you _could_ put it in another stream, but the results are unpredictable
    ///
    /// of course, you could also make a formatted_logging_message from
    /// scratch, or just use a raw boost::format object,
    /// but you dont get the same efficiency.  namely,
    /// your formatting object will not correctly short-circuit its argument
    /// reading operations.
    ///
    ////////////////////////////////////////////////////////////////////////////
    //\@{
    formatted_logging_msg 
        formatted_msg(logging_level loglevel, std::string const& msg) const;
    //\@}
    
    std::string const& name() const 
    { return detail::logging_public_stream::name(); }
    
    logging_level minimal_level() const 
    { return detail::logging_public_stream::minimal_level(); }
private:
    friend class logging_level_manip;
    friend detail::logging_public_stream& continue_log(logging_stream& str);
    detail::logging_public_stream& stream();
};

////////////////////////////////////////////////////////////////////////////////

template <class StreamBuf>
logging_stream::logging_stream( StreamBuf& buf
                              , std::string const& name
                              , logging_level ignore_below )
: detail::logging_public_stream(buf, name, ignore_below)
{}

////////////////////////////////////////////////////////////////////////////////

inline detail::logging_public_stream& continue_log(logging_stream& str)
{ return str.stream(); }

inline bool logging_at_level(logging_stream const& str, logging_level lvl) 
{ return lvl <= str.minimal_level(); }

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::io

#endif // SBMT_LOGGING_LOGSTREAM_HPP
