// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/**
 * Code for tracing/debug printing
 */
//*****************************************************************************
// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

/**
 * @page TRACING The tracing (foundation library)
 *
 * Usage:
 *
 * 1) An application must define the list of traceable tags in an include
 *    file as following:
 *
 *      #ifndef _TRACE_module_name_H_
 *      #define _TRACE_module_name_H_
 *
 *      #define TRACE_TAGS   TAG(tag_id1) TAG(tag_id2) TAG(tag_id3)
 *
 *      #define TRACE_MODULE  MOD(module_name)
 *
 *      #include "common/Trace.h"
 *
 *      #endif // _TRACE_module_name_H_
 *
 *    TAG_ID will be  module_name.tag_id
 *
 *    If several modules have to be defined, the above sequence can be repeated
 *    as needed.
 *
 * 2) In the main source file, after including all definitions of traceable flags,
 *    insert the following, for each module:
 *
 *    TRACE_DEF(module_name);
 *
 * 3) In main() call TRACE_INIT(arg) to initialize tag indicators. arg should
 *    be a pointer to a string that contains comma-delimited tag identifiers
 *    that should be turned on
 *
 *    TRACE_OUTPUT(stream) can be used to change the trace output stream from
 *    the default "cerr"
 *
 * 4) in order to print a diagnostic message, associated with a tag,
 *    use call TRACE(TAG_ID, (format,...)). It will produce a given message
 *    if the tag was turned on by TRACE_INIT
 *
 * 5) TRACE_CALL(TAG_ID, (format,...)) increases identation and prints a message.
 *    Identation will be decreased when control leaves the block
 *    where TRACE_CALL(()) was inserted.
 *
 * 6) TRACE_EXEC(TAG_ID, expr) executes expression expr if tag is enabled
 *    If expr needs a message to be printed, it should use Trace::println()
 *    (or other Trace class print functions)
 *
 * 7) TRACE_PRINT(TAG_ID, obj) can be used to print object obj having a print
 *    method with ostream& argument
 *
 */
#ifndef LW_TRACE_H
#define LW_TRACE_H 1

#include <stdarg.h>
#include <iosfwd>

namespace LW {

#ifndef MULTI_THREADED
#	define MULTI_THREADED
#endif


#ifdef MULTI_THREADED
#  ifndef UNIX_OR_NT
#    if (defined(__unix__) || defined(__MACH__))
#      define UNIX_OR_NT(x,y) x
#    elif (defined (_MSC_VER) || defined(__MINGW32__))
#      define UNIX_OR_NT(x,y) y
#    endif
#  endif
#  define TRACE_THREADID (long long)(UNIX_OR_NT(pthread_self(), GetCurrentThreadId()))
#else
#  define TRACE_THREADID 0
#endif

#ifdef _MSC_VER
#   pragma warning (disable : 4251 4786)
#endif

#ifdef WIN32
#   define _T_PTR(x) __int64(x)
#else
#   define _T_PTR(x) (long long)(x)
#endif

/// Global flags
enum {
    TRACE_THREAD   = 0001, // +/-thread
    TRACE_TIME     = 0002, // +/-time
    TRACE_LOG      = 0004, //+/-log (severity,time,thread)
    TRACE_FILE     = 0010  //+/-file
};


/**
 * An individual trace tag
 */
class TraceTag 
{
public:

    /// Tag constructor
    TraceTag(const char* tagname);

    /// Returns true if tracing is enabled for this flag
    bool is_enabled() const { return enabled_; }

    /// Enable this tag
    void enable()  { enabled_ = true; }

    /// Disable this tag
    void disable() { enabled_ = false; }

    /// Return the name of this tag
    const char* getName() const { return id_; }

    /// Print a one-line message with tag, indentation and newline appended
    void println(const char* fmt, ...) const;

    /// Print a one-line message with newline appended
    void vprintln(const char* fmt, va_list ap) const;

    /// Print a one-line message for function call
    void call_message(const char* fmt, ...) const;

    /// Print a one-line message for function return
    void ret_message() const;

    /// Print current indentation and tag name
    void ind() const;

protected:
    void print_head_() const;

private:
    friend class Trace;

    bool        enabled_;   ///< Is printing enabled for this tag?
    const char* id_;        ///< The tag's identifier
    TraceTag*   next_;      ///< The next tag in the list of all tags
};

/**
 * The basic class for trace definitions
 */
class Trace 
{
public:
    
    enum {
        _INDENT_SIZE = 2     ///< Indent, spaces
    };

    /// Increases indentation on function call
    Trace(TraceTag& tag)
    {
        cur_tag_ = &tag;
        temp_tag_ = 0;
        old_ind_ = cur_ind();
    }

    /// Print message on function return (restores indent)
    ~Trace()
    {
        set_cur_ind_(old_ind_);
        if (cur_tag_->enabled_)
            cur_tag_->ret_message();
        if (temp_tag_)
            temp_tag_->enabled_ = temp_save_;
    }

    /// Checks if the current trace tag is enabled
    bool is_enabled() const
    {
        return cur_tag_->enabled_;
    }

    /// Temporarily enable individual tag (until return from subroutine)
    void enable(TraceTag& tag)
    {
        if (temp_tag_)
            temp_tag_->enabled_ = temp_save_;
        temp_tag_ = &tag;
        temp_save_ = tag.enabled_;
        tag.enabled_ = true;
    }

    /// Temporarily disable individual flag (until return from subroutine)
    void disable(TraceTag& tag)
    {
        if (temp_tag_)
            temp_tag_->enabled_ = temp_save_;
        temp_tag_ = &tag;
        temp_save_ = tag.enabled_;
        tag.enabled_ = false;
    }

    /// Enable tags by names
    static void init(const char* taglist);

    /// Print a one-line message with current indentation & newline appended
    static void println(const char* fmt, ...);

    /// Print a one-line message with current indentation & newline appended
    static void vprintln(const char* fmt, va_list ap);

    /// Print the part of message, w/o indentation or newline
    static void print(const char* fmt, ...);

    /// Print the part of message, w/o indentation or newline
    static void vprint(const char* fmt, va_list ap);

    /// Prints hexadecimal dump of a memory buffer
    static void dump(const void* buf, unsigned int bufsize);

    /// Print current indentation
    static void ind();

    /// Print a newline
    static void nl();

    /// Set the output stream
    static void set_output(std::ostream* os)
    {
        output_ = os;
    }

    /// Returns the output stream
    static std::ostream* get_output()
    {
        return output_;
    }

    /// Print status of all tracing tags
    static void  print_tag_status();

    /// returns currect indentation, thread local var
    static unsigned short cur_ind();

    /// increments current indentation : cur_ind_++
    static void incrIndent() { Trace::set_cur_ind_(cur_ind() + 1); }

    ///check the global flag(s), true if present, false if not
    static bool checkFlag(long int flag);

private:
    friend class TraceTag;

    //------------ PER-CALL DATA ---------

    /// The current tag
    TraceTag*          cur_tag_;

    /// The temporarily enabled tag (if >= 0)
    TraceTag*          temp_tag_;

    /// Old indent
    unsigned short     old_ind_;

    /// Old state of the temporary enabled tag
    bool               temp_save_;

    //------------ PER-PROCESS DATA ------------

    /// The list of known tags
    static TraceTag*        tags_;

    /// returns currect indentation, thread local var
    static unsigned short cur_ind_();

    /// set currect indentation to new value
    static void set_cur_ind_(unsigned short cur_ind);

    /// The current output stream
    static std::ostream*    output_;
};

#ifndef NOTRACE

//
// Trace interfaces
//
#define TRACE_DEF(modname)      class TraceTags_ ## modname modname

#define TRACE_DEF_DLL(modname, expimp) \
                                expimp class TraceTags_ ## modname modname

#define TRACE_INIT(arg)         LW::Trace::init(arg)

#define TRACE_OUTPUT(out)       LW::Trace::set_output(out)

#define TRACE(tag, x)           { \
                                    if (tag.is_enabled()) \
                                        tag.println x; \
                                }

#define TRACE_(tag, x)          { \
                                    if (tag.is_enabled()) \
                                        LW::Trace::print x; \
                                }

#define TRACE_DUMP(tag, x)      { \
                                    if (tag.is_enabled()) \
                                        LW::Trace::dump x; \
                                }

#define TRACE_CALL(tag, x)      LW::Trace _trace_(tag); \
                                { \
                                    if (_trace_.is_enabled()) \
                                        tag.call_message x; \
                                }

#define TRACE_EXEC(tag, expr)   { \
                                    if (tag.is_enabled()) \
                                        { expr; } \
                                }

#define TRACE_ENABLE(tag)       _trace_.enable(tag)

#define TRACE_DISABLE(tag)      _trace_.disable(tag)

#define TRACE_PRINT(tag, obj)   { \
                                    if (tag.is_enabled()) \
                                        (obj)->print(*LW::Trace::get_output()); \
                                }

#else // NOTRACE set

//
// Fake Trace interfaces (trace code completely eliminated)
//
#define TRACE_DEF(modname)
#define TRACE_DEF_DLL(modname, EXPIMP)
#define TRACE_INIT(arg)
#define TRACE_OUTPUT(out)
#define TRACE(tag, x)
#define TRACE_(tag, x)
#define TRACE_DUMP(tag, x)
#define TRACE_CALL(tag, x)
#define TRACE_EXEC(tag, expr)
#define TRACE_ENABLE(tag)
#define TRACE_DISABLE(tag)
#define TRACE_PRINT(tag, obj)

#endif // NOTRACE

} // namespace

#endif // _TRACE_H_

//-----------------------------------------------------------------------------------------
//
// NOTE: The following code uses TRACE_MODULE and TRACE_TAGS
//       to generate trace module declaration
//
#if defined(TRACE_MODULE) || defined(TRACE_TAGS)

#ifndef TRACE_TAGS
#error "TRACE_TAGS is not defined!"
#define TRACE_TAGS
#endif

#ifndef TRACE_MODULE
#error "TRACE_MODULE is not defined!"
#define TRACE_MODULE __dummy_trace__
#endif

#ifndef NOTRACE


#define MOD(modname) TraceTags_ ## modname
class TRACE_MODULE 
{
public:
    /// Constructor
    TRACE_MODULE() :
#undef MOD
#define MOD(modname) #modname
#define TAG(tag_id)  tag_id(TRACE_MODULE "." #tag_id),
        TRACE_TAGS
#undef TAG
#undef MOD
        dummy_(0)
    { }

    // Tag fields
#define TAG(tag_id) LW::TraceTag tag_id;
    TRACE_TAGS
#undef TAG

private:
    int dummy_;     ///< A dummy field, to get around a problem with closing comma
};

//
// The trace module singleton declaration
//
#define MOD(modname) extern TraceTags_ ## modname modname;
TRACE_MODULE
#undef MOD

#endif // !NOTRACE

#undef TRACE_EXPIMP
#undef TRACE_MODULE
#undef TRACE_TAGS

#endif // defined(TRACE_MODULE) || defined(TRACE_TAGS)
