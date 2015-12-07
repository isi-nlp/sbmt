#ifndef   SBMT_IO_LOGGING_MACROS_HPP
#define   SBMT_IO_LOGGING_MACROS_HPP

////////////////////////////////////////////////////////////////////////////////
///
///   \defgroup Logging Logging
///
///  logging like this:
///
///  string bob("bob");
///  token red("red");
///  SBMT_INFO_MSG( my_domain
///               , "this is an info message from %1% about the color %2%"
///               , bob % red );
///
///  conditions:  you must have declared a logging domain using
///               SBMT_REGISTER_LOGGING_DOMAIN(my_domain);
///
///  you can also use the logging stream naked:
///  logging_stream& str logfile_registry::instance().log(my_domain);
///    (or for short: logs().log(my_domain)
///
///  SBMT_LOGGING_MSG_TO( str
///                     , SBMT_WARNING_MSG
///                     , "this is a warning from %1% about %2%"
///                     , bob % red );
///
///  why formatter % syntax instead of streaming << syntax?
///  because I want to use parentheses to be able to macro-eliminate logging
///  messages, which leads to printf-like syntax.  but i also wanted to be able
///  to print c++ objects to logs easily, so that let me to the boost::format
///  library.
///
///  to eliminate logs with lower priority than a given level, define
///  SBMT_LOGGING_MINIMUM_LEVEL before including this file
///
///  formatted logging can be more expensive than regular streaming, so
///  you can also use normal streaming macros:
///
///  SBMT_INFO_STREAM( my_domain
///                 , "this is a warning from " <<bob<< " about " << red );
///
///  or if you dont care about eliminating messages at compile time:
///  str << io::lvl_info << "this is a warning from " << bob
///      << " about " << red;
///
///  note that even if you dont eliminate at compile time, the logging
///  streams and formatters still short-circuits evaluations that are below
///  their runtime logging level (though not temporary evaluations).
///
////////////////////////////////////////////////////////////////////////////////

///\{

# include <sbmt/io/logging_stream.hpp>
# include <sbmt/io/logfile_registry.hpp>
# include <sbmt/io/logging_level.hpp>

# include <boost/config/suffix.hpp>

////////////////////////////////////////////////////////////////////////////////

# define SBMT_LOGGING_LEVEL(x) BOOST_JOIN(::sbmt::io::lvl_,x)
# define SBMT_LOGGING_MANIP(x) \
::sbmt::io::logging_level_manip(SBMT_LOGGING_LEVEL(x))

////////////////////////////////////////////////////////////////////////////////

# define SBMT_REGISTER_LOGGING_DOMAIN_NAME( D, N )  \
namespace {  \
static ::sbmt::io::logging_domain D(N); \
}

# define SBMT_REGISTER_LOGGING_DOMAIN( D ) \
SBMT_REGISTER_LOGGING_DOMAIN_NAME( D, #D )

////////////////////////////////////////////////////////////////////////////////

# define SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME( D, N, PARENT ) \
namespace { \
static ::sbmt::io::logging_domain D(N,PARENT); \
}

# define SBMT_REGISTER_CHILD_LOGGING_DOMAIN( D , PARENT ) \
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME( D, #D, PARENT )

////////////////////////////////////////////////////////////////////////////////

# define SBMT_SET_DOMAIN_LOGFILE( name, filename )                             \
namespace {                                                                    \
static ::sbmt::io::domain_setter BOOST_JOIN(name, _file)(name,filename);       \
}

////////////////////////////////////////////////////////////////////////////////

# define SBMT_SET_DOMAIN_LOGGING_LEVEL( name, lvl )                            \
namespace {                                                                    \
static ::sbmt::io::domain_setter BOOST_JOIN(name, _lvl)(                       \
          name                                                                 \
        , SBMT_LOGGING_LEVEL(lvl)                                              \
       );                                                                      \
}

////////////////////////////////////////////////////////////////////////////////

#ifndef SBMT_LOGGING_MINIMUM_LEVEL
# ifdef SBMT_DEBUG
#  define SBMT_LOGGING_MINIMUM_LEVEL SBMT_PEDANTIC_LEVEL
# else
#  define SBMT_LOGGING_MINIMUM_LEVEL SBMT_PEDANTIC_LEVEL
# endif
#endif

////////////////////////////////////////////////////////////////////////////////

# define SBMT_LOGSTREAM(domain) ::sbmt::io::registry_log(domain)
# define SBMT_LOGGING(domain,level) \
    (::sbmt::io::logging_at_level(SBMT_LOGSTREAM(domain),SBMT_LOGGING_LEVEL(level)))

# define SBMT_EXPR_LOG continue_log(str)

# define SBMT_BARE_EXPR_TO(stream,level,str_expr) do { \
         ::sbmt::io::logging_stream& str = stream; \
        if (::sbmt::io::logging_at_level(str,SBMT_LOGGING_LEVEL(level))) { \
            str_expr;  \
        } } while(0)

# define SBMT_MSG_TO(stream, level, message, args) \
SBMT_BARE_EXPR_TO( \
    stream \
  , level \
  , str << str.formatted_msg(SBMT_LOGGING_LEVEL(level),message) \
         % args; \
    continue_log(str) << ::sbmt::io::endmsg \
  )

# define SBMT_STREAM_TO(stream, level, args) \
SBMT_BARE_EXPR_TO( \
    stream \
  , level \
  , str << SBMT_LOGGING_MANIP(level) << args; \
    continue_log(str) << sbmt::io::endmsg \
  )

# define SBMT_EXPR_TO(stream,level,str_expr) \
SBMT_BARE_EXPR_TO( \
    stream \
  , level \
  , str << SBMT_LOGGING_MANIP(level); \
    str_expr \
    continue_log(str) << sbmt::io::endmsg    \
  )

#if      SBMT_PEDANTIC_LEVEL > SBMT_LOGGING_MINIMUM_LEVEL
 #define SBMT_PEDANTIC_EXPR_TO(str, expr)
 #define SBMT_PEDANTIC_MSG_TO(str, msg, expr)
 #define SBMT_PEDANTIC_STREAM_TO(str, args)
#else
 #define SBMT_PEDANTIC_EXPR_TO(str, expr) SBMT_EXPR_TO(str,pedantic,expr)
 #define SBMT_PEDANTIC_MSG_TO(str, msg, args) SBMT_MSG_TO(str,pedantic,msg,args)
 #define SBMT_PEDANTIC_STREAM_TO(str, args)   SBMT_STREAM_TO(str,pedantic,args)
#endif

////////////////////////////////////////////////////////////////////////////////

#if      SBMT_DEBUG_LEVEL > SBMT_LOGGING_MINIMUM_LEVEL
 #define SBMT_DEBUG_EXPR_TO(str, expr)
 #define SBMT_DEBUG_MSG_TO(str, msg, args)
 #define SBMT_DEBUG_STREAM_TO(str, args)
#else
 #define SBMT_DEBUG_EXPR_TO(str, expr) SBMT_EXPR_TO(str,debug,expr)
 #define SBMT_DEBUG_MSG_TO(str, msg, args) SBMT_MSG_TO(str,debug,msg,args)
 #define SBMT_DEBUG_STREAM_TO(str, args)   SBMT_STREAM_TO(str,debug,args)
#endif

////////////////////////////////////////////////////////////////////////////////

#if      SBMT_VERBOSE_LEVEL > SBMT_LOGGING_MINIMUM_LEVEL
 #define SBMT_VERBOSE_EXPR_TO(str, expr)
 #define SBMT_VERBOSE_MSG_TO(str, msg, args)
 #define SBMT_VERBOSE_STREAM_TO(str, args)
#else
 #define SBMT_VERBOSE_EXPR_TO(str, expr) SBMT_EXPR_TO(str,verbose,expr)
 #define SBMT_VERBOSE_MSG_TO(str, msg, args) SBMT_MSG_TO(str,verbose,msg,args)
 #define SBMT_VERBOSE_STREAM_TO(str, args)   SBMT_STREAM_TO(str,verbose,args)
#endif

////////////////////////////////////////////////////////////////////////////////

#if      SBMT_INFO_LEVEL > SBMT_LOGGING_MINIMUM_LEVEL
 #define SBMT_INFO_EXPR_TO(str, expr)
 #define SBMT_INFO_MSG_TO(str, msg, args)
 #define SBMT_INFO_STREAM_TO(str, args)
#else
#define SBMT_INFO_EXPR_TO(str, expr) SBMT_EXPR_TO(str,info,expr)
 #define SBMT_INFO_MSG_TO(str, msg, args) SBMT_MSG_TO(str,info,msg,args)
 #define SBMT_INFO_STREAM_TO(str, args)   SBMT_STREAM_TO(str,info,args)
#endif

////////////////////////////////////////////////////////////////////////////////

#if      SBMT_TERSE_LEVEL > SBMT_LOGGING_MINIMUM_LEVEL
 #define SBMT_TERSE_EXPR_TO(str, expr)
 #define SBMT_TERSE_MSG_TO(str, msg, args)
 #define SBMT_TERSE_STREAM_TO(str, args)
#else
 #define SBMT_TERSE_EXPR_TO(str, expr) SBMT_EXPR_TO(str,terse,expr)
 #define SBMT_TERSE_MSG_TO(str, msg, args) SBMT_MSG_TO(str,terse,msg,args)
 #define SBMT_TERSE_STREAM_TO(str, args)   SBMT_STREAM_TO(str,terse,args)
#endif


////////////////////////////////////////////////////////////////////////////////

#if      SBMT_WARNING_LEVEL > SBMT_LOGGING_MINIMUM_LEVEL
 #define SBMT_WARNING_EXPR_TO(str, expr)
 #define SBMT_WARNING_MSG_TO(str, msg, args)
 #define SBMT_WARNING_STREAM_TO(str, args)
#else
#define SBMT_WARNING_EXPR_TO(str, expr) SBMT_EXPR_TO(str,warning,expr)
 #define SBMT_WARNING_MSG_TO(str, msg, args) SBMT_MSG_TO(str,warning,msg,args)
 #define SBMT_WARNING_STREAM_TO(str, args)   SBMT_STREAM_TO(str,warning,args)
#endif

////////////////////////////////////////////////////////////////////////////////

#if      SBMT_ERROR_LEVEL > SBMT_LOGGING_MINIMUM_LEVEL
 #define SBMT_ERROR_EXPR_TO(str, expr)
 #define SBMT_ERROR_MSG_TO(str, msg, args)
 #define SBMT_ERROR_STREAM_TO(str, args)
#else
#define SBMT_ERROR_EXPR_TO(str, expr) SBMT_EXPR_TO(str,error,expr)
 #define SBMT_ERROR_MSG_TO(str, msg, args) SBMT_MSG_TO(str,error,msg,args)
 #define SBMT_ERROR_STREAM_TO(str, args)   SBMT_STREAM_TO(str,error,args)
#endif

////////////////////////////////////////////////////////////////////////////////

#define SBMT_PEDANTIC_MSG(domain, msg , args) \
SBMT_PEDANTIC_MSG_TO(SBMT_LOGSTREAM(domain), (msg), args)

#define SBMT_DEBUG_MSG(domain, msg , args) \
SBMT_DEBUG_MSG_TO(SBMT_LOGSTREAM(domain), (msg), args)

#define SBMT_VERBOSE_MSG(domain, msg , args) \
SBMT_VERBOSE_MSG_TO(SBMT_LOGSTREAM(domain), (msg), args)

#define SBMT_INFO_MSG(domain, msg , args) \
SBMT_INFO_MSG_TO(SBMT_LOGSTREAM(domain), (msg), args)

#define SBMT_TERSE_MSG(domain, msg , args) \
SBMT_TERSE_MSG_TO(SBMT_LOGSTREAM(domain), (msg), args)

#define SBMT_WARNING_MSG(domain, msg , args) \
SBMT_WARNING_MSG_TO(SBMT_LOGSTREAM(domain), (msg), args)

#define SBMT_ERROR_MSG(domain, msg , args) \
SBMT_ERROR_MSG_TO(SBMT_LOGSTREAM(domain), (msg), args)

////////////////////////////////////////////////////////////////////////////////

#define SBMT_PEDANTIC_EXPR(domain, expr) \
SBMT_PEDANTIC_EXPR_TO(SBMT_LOGSTREAM(domain) , expr )

#define SBMT_DEBUG_EXPR(domain, expr) \
SBMT_DEBUG_EXPR_TO( SBMT_LOGSTREAM(domain) , expr )

#define SBMT_VERBOSE_EXPR(domain, expr) \
SBMT_VERBOSE_EXPR_TO( SBMT_LOGSTREAM(domain) , expr )

#define SBMT_INFO_EXPR(domain, expr) \
SBMT_INFO_EXPR_TO( SBMT_LOGSTREAM(domain) , expr )

#define SBMT_TERSE_EXPR(domain, expr) \
SBMT_TERSE_EXPR_TO( SBMT_LOGSTREAM(domain) , expr )

#define SBMT_WARNING_EXPR(domain, expr) \
SBMT_WARNING_EXPR_TO( SBMT_LOGSTREAM(domain) , expr )

#define SBMT_ERROR_EXPR(domain, expr)  \
SBMT_ERROR_EXPR_TO( SBMT_LOGSTREAM(domain) , expr )

////////////////////////////////////////////////////////////////////////////////

#define SBMT_PEDANTIC_STREAM(domain, args) \
SBMT_PEDANTIC_STREAM_TO(SBMT_LOGSTREAM(domain), args)

#define SBMT_DEBUG_STREAM(domain, args) \
SBMT_DEBUG_STREAM_TO(SBMT_LOGSTREAM(domain), args)

#define SBMT_VERBOSE_STREAM(domain, args) \
SBMT_VERBOSE_STREAM_TO(SBMT_LOGSTREAM(domain), args)

#define SBMT_INFO_STREAM(domain, args) \
SBMT_INFO_STREAM_TO(SBMT_LOGSTREAM(domain), args)

#define SBMT_TERSE_STREAM(domain, args) \
SBMT_TERSE_STREAM_TO(SBMT_LOGSTREAM(domain), args)

#define SBMT_WARNING_STREAM(domain, args) \
SBMT_WARNING_STREAM_TO(SBMT_LOGSTREAM(domain), args)

#define SBMT_ERROR_STREAM(domain, args) \
SBMT_ERROR_STREAM_TO(SBMT_LOGSTREAM(domain), args)

////////////////////////////////////////////////////////////////////////////////

///\}

#define SBMT_LOG_TIME_SPACE(domain,lvlname,header) io::log_time_space_report\
    sbmt_log_time_space ## __LINE__ (SBMT_LOGSTREAM(domain), SBMT_LOGGING_LEVEL(lvlname),header)

#endif // SBMT_IO_LOGGING_MACROS_HPP


