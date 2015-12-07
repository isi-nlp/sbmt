// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/*! \file
 *  The Trace class implementation
 */
//*****************************************************************************
// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved




#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   ifndef NOMINMAX
#   define NOMINMAX
#   endif
#   include <windows.h>
#   undef min
#   undef max
#else
/// \attention kludge to compile in pthread_mutexattr_settype prototype
#if defined(linux) || defined(__APPLE__)
#   define _XOPEN_SOURCE 500
#   include <sys/time.h>
#endif
#   include <pthread.h>
#endif // _WIN32

//#include <qdatetime.h>
#include "Common/Trace.h"
#include "Common/Log.h"


#include <iostream>
#include <iomanip>
#include <string>

#include <cstdlib>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

using namespace std;

namespace {

#ifdef _MSC_VER
	int VSNPRINTF(char *_DstBuf, size_t _DstSize, const char *_Format, va_list ArgList) {
		return _vsnprintf_s(_DstBuf, _DstSize, _DstSize-1, _Format, ArgList);
	}
#else // _MSC_VER
# define VSNPRINTF vsnprintf
#endif // _MSC_VER

std::ostream&
vform(std::ostream& os, const char* fmt, va_list vl)
{
    char buf[5120];
    char* p = buf, *p1 = 0;
    int size = sizeof(buf);

    while (-1 == VSNPRINTF(p, size, fmt, vl)) {
        size += sizeof(buf);
        p1 = static_cast<char*>(realloc(p1, size));
        if (0 == p1)
            break;
        p = p1;
    }
    os << p;

    if (0 != p1)
        free(p1);

    return os;
}

std::ostream&
form(std::ostream& os, const char* fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    return vform(os, fmt, vl);
}

} // end of anonymous namespace

static int flags_;
# define SET_FLAG(F)   flags_ |=  (F)
# define UNSET_FLAG(F) flags_ &= ~(F)
# define CHECK_FLAG(F) ((flags_ & (F)) != 0)

namespace LW {


#ifdef MULTI_THREADED

struct Trace_cur_ind_ {

    UNIX_OR_NT(pthread_key_t, int) key_;

    Trace_cur_ind_() { UNIX_OR_NT(pthread_key_create(&key_, 0), key_ = TlsAlloc()); }

    ~Trace_cur_ind_() { UNIX_OR_NT(pthread_key_delete(key_), TlsFree(key_)); }

    unsigned long get() { return (unsigned long) UNIX_OR_NT(pthread_getspecific(key_), TlsGetValue(key_)); }

    void set(int value) { UNIX_OR_NT(pthread_setspecific(key_, (void*)(long)value), TlsSetValue(key_, (void*)(long)value)); }

};

/// this is for safe static initialisation, do not remove!
static Trace_cur_ind_&
cur_ind_value()
{
    static Trace_cur_ind_ cur_ind;
    return cur_ind;
}

/// The current indentation, per thread var
unsigned short
Trace::cur_ind()
{
    return cur_ind_value().get();
}

void
Trace::set_cur_ind_(unsigned short cur_ind)
{
    cur_ind_value().set(cur_ind);
}

#else // !MULTI_THREADED

static unsigned short cur_ind_value = 0;

unsigned short
Trace::cur_ind()
{
    return cur_ind_value;
}

void
Trace::set_cur_ind_(unsigned short cur_ind)
{
    cur_ind_value = cur_ind;
}

#endif

/// The current output stream
std::ostream* Trace::output_ = &cout;

/// The list of all tags
TraceTag* Trace::tags_ = 0;

/**
 * Tag constructor - adds a tag to the list
 *
 * @arg tagname the symbolic name for the tag
 *
 * NOTE: tagname is NOT duplicated
 */
TraceTag::TraceTag(const char* tagname)
{
    id_ = tagname;
    enabled_ = false;
    next_ = Trace::tags_;
    Trace::tags_ = this;
}

/**
 * Print the message line, when tracing is enabled for the tag
 *
 * @arg fmt the format string
 * @arg ... the argument list
 */
void
TraceTag::println(const char* fmt, ...) const
{
    va_list ap;

    if (enabled_) { // some optimisation
        va_start(ap, fmt);
        vprintln(fmt, ap);
        va_end(ap);
    }
}

/**
 * Print thread information
 */
void
TraceTag::print_head_() const
{
    int sec;
    int msec;


#if (defined(__unix__) || defined(__MACH__))
    struct timeval tv;
    gettimeofday(&tv, 0);
    sec = tv.tv_sec;
    msec = tv.tv_usec;
#else
    unsigned __int64 time;
    GetSystemTimeAsFileTime((FILETIME*)&time);
    #ifdef _MSC_VER
    time -= 116444736000000000i64;
    #else
    time -= 116444736000000000ll;
    #endif
    time /= 10;
    
    sec = int(time / 1000000);
    msec = int(time / 10000 % 100);
#endif
    

    if (CHECK_FLAG(TRACE_TIME|TRACE_THREAD|TRACE_LOG)) {
        form(*Trace::output_, "[");

#ifdef MULTI_THREADED
        if (CHECK_FLAG(TRACE_THREAD) || CHECK_FLAG(TRACE_LOG))
            form(*Trace::output_, "%05d", TRACE_THREADID );
#endif
        if (CHECK_FLAG(TRACE_TIME) || CHECK_FLAG(TRACE_LOG))
            form(*Trace::output_, ",%d.%2.2d", sec, msec);
        if (CHECK_FLAG(TRACE_LOG))
            form(*Trace::output_, ",NON");
        form(*Trace::output_, "] ");
    }
}

/**
 * Print the message line, when tracing is enabled for the tag
 *
 * @arg fmt the format string
 * @arg ap the argument list
 */
void
TraceTag::vprintln(const char* fmt, va_list ap) const
{
    if (enabled_) {
        print_head_();
        form(*Trace::output_, "%*s%s: ", Trace::cur_ind() * Trace::_INDENT_SIZE, "", id_);
        vform(*Trace::output_, fmt, ap);
        *Trace::output_ << endl;
//        Trace::output_->flush();
    }
}

/**
 * Print the message for function call
 *
 * @arg fmt the format string
 * @arg ... the argument list
 */
void
TraceTag::call_message(const char* fmt, ...) const
{
    va_list ap;
    int cur_ind = Trace::cur_ind();
    print_head_();
    form(*Trace::output_, "%*s%s >%u> ", cur_ind * Trace::_INDENT_SIZE, "", id_,
                                        cur_ind + 1);
    va_start(ap, fmt);
    vform(*Trace::output_, fmt, ap);
    va_end(ap);

    *Trace::output_ << endl;
    Trace::output_->flush();
    Trace::set_cur_ind_(cur_ind + 1); // increment
}

/**
 * Print the message on function return
 */
void
TraceTag::ret_message() const
{
    int cur_ind = Trace::cur_ind();
    print_head_();
    form(*Trace::output_, "%*s%s <%u<\n", cur_ind * Trace::_INDENT_SIZE, "", id_,
                                         cur_ind + 1);
    Trace::output_->flush();
}

/**
 * Print a current indent with a tag
 */
void
TraceTag::ind() const
{
    form(*Trace::output_, "%*s%s: ", Trace::cur_ind() * Trace::_INDENT_SIZE, "", id_);
}

/**
 * Set the list of enabled trace flags by their names
 *
 * By default, all tags are disabled
 *
 * Syntax:
 *
 *  taglist := tagspec { "," tagspec }
 *
 *  tagspec := [ "!" ] ( "*" | module_name [ "." tagid ] )
 *
 *  tagid := "*"  |  tag_name
 *
 * @arg taglist the list of all tags to enable
 */
void
Trace::init(const char* taglist)
{
    const char* b;
    size_t len;
    bool enb;
    bool found;
    TraceTag* t;
    int cnt = 0;

    // Disable all tags
    for (t = tags_; t; t = t->next_) {
        cnt++;
        if (cnt++ > 512) {
            print("Trace tags are broken... Possible reason - few MODULES declared with the same name\n");
            tags_ = 0;
            break;
        }
        t->enabled_ = false;
    }

    // Scan by taglist elements
    while (*taglist) {
        // Skip commas
        b = taglist;
        while (*b == ',')
            b++;

        // Find end of the tag spec
        taglist = strchr(b, ',');
        if (!taglist)
            taglist = strchr(b, 0); // Must _always_ be non-zero

        // Check for negation
        enb = true;
        if (*b == '+')
            b++;

        if (*b == '-' || *b == '!') {
            enb = false;
            b++;
        }

        // Empty tag spec?
        len = taglist - b;
        if (len == 0)
            continue; 

        /************* check for special cases ********************/
        if (strncmp(b, "THREAD", len) == 0) {
            if (enb)
                SET_FLAG(TRACE_THREAD);
            else
                UNSET_FLAG(TRACE_THREAD);
            continue;
        }
        // check for special cases
        if (strncmp(b, "TIME", len) == 0) {
            if (enb)
                SET_FLAG(TRACE_TIME);
            else
                UNSET_FLAG(TRACE_TIME);
            continue;
        }
        // check for special cases
        if (strncmp(b, "LOG", len) == 0) {
            if (enb)
                SET_FLAG(TRACE_LOG);
            else
                UNSET_FLAG(TRACE_LOG);
            continue;
        }
        // check for special cases
        if (strncmp(b, "FILE", len) == 0) {
            if (enb)
                SET_FLAG(TRACE_FILE);
            else
                UNSET_FLAG(TRACE_FILE);
            continue;
        }
        /************* end  ********************/

        // All modules & tags?
        if (len == 1 && *b == '*') {
            for (t = tags_; t; t = t->next_)
                t->enabled_ = enb;
            continue;
        }

        // A specific tag?
        if (memchr(b, '.', len)) {
            if (*b == '.' || b[len-1] == '.') {
                print("*** invalid trace tag name '%.*s'\n", len, b);
                continue;
            }

            // check that this is NOT modname.*
            if (b[len-1] != '*' || b[len-2] != '.') {
                // search for the exact tag name
                found = false;
                for (t = tags_; t; t = t->next_) {
                    if (!memcmp(b, t->id_, len) && t->id_[len] == 0) {
                        t->enabled_ = enb;
                        found = true;
                        break;
                    }
                }

                if (!found)
                    print("*** unknown trace tag '%.*s'\n", len, b);
                continue;
            }

            len -= 2;     // chop off .*
        }

        // All tags in a module
        found = false;
        for (t = tags_; t; t = t->next_) {
            if (!memcmp(b, t->id_, len) && t->id_[len] == '.') {
                t->enabled_ = enb;
                found = true;
            }
        }

        if (!found)
            print("*** unknown trace module name '%.*s'\n", len, b);
    }
}

/**
 * Print a current indent
 */
void Trace::ind()
{
    form(*output_, "%*s", cur_ind() * _INDENT_SIZE, "");
}

/**
 * Print an end of line
 */
void
Trace::nl()
{
    *output_ << endl;
    output_->flush();
}

/**
 * Print the message line (w/o tag name)
 *
 * @arg fmt the format string
 * @arg ... the argument list
 */
void
Trace::println(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    form(*output_, "%*s", cur_ind() * _INDENT_SIZE, "");
    vform(*output_, fmt, ap);
    *output_ << endl;
    va_end(ap);
    output_->flush();
}




/**
 * Print the message line (w/o tag name)
 *
 * @arg fmt the format string
 * @arg ap the argument list
 */
void
Trace::vprintln(const char* fmt, va_list ap)
{
    form(*output_, "%*s", cur_ind() * _INDENT_SIZE, "");
    vform(*output_, fmt, ap);
    *output_ << endl;
    output_->flush();
}

/**
 * Print the message w/o newline or indentation
 *
 * @arg fmt the format string
 * @arg ... the argument list
 */
void
Trace::print(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vform(*output_, fmt, ap);
    va_end(ap);
    output_->flush();
}

/**
 * Print the message w/o newline or indentation
 *
 * @arg fmt the format string
 * @arg ap the argument list
 */
void
Trace::vprint(const char* fmt, va_list ap)
{
    vform(*output_, fmt, ap);
    output_->flush();
}

/**
 * Print the hexdump of a memory buffer w/newline and w/o indentation
 *
 * @arg buffer the buffer start address
 * @arg bufsize the buffer length
 */
void
Trace::dump(const void* buffer, unsigned int bufsize)
{
    if (bufsize == 0)
        return;
    char text[17];
    const unsigned char* buf = reinterpret_cast<const unsigned char*>(buffer);
    int width = output_->width(2);
    char fillch = output_->fill('0');
    *output_ << "Dumping " << bufsize;
    ostream::fmtflags flags = output_->setf(ostream::hex, ostream::basefield);
    *output_ << " byte(s) @ 0x" << buffer << endl;

    unsigned int i = 0;
    for (; i < bufsize; i++) {
      unsigned int j = i % 16;
      if (j == 0 && i > 0) {
        text[16] = 0;
        *output_ << ' ' << text << endl;
      }
      unsigned char c = buf[i];
      *output_ << setw(2) << unsigned(c) << ' ';
      text[j] = isprint(c) ? c : '.';
    }
    if( (i%=16) == 0) {\
      i = 16;
    }
    text[i] = 0;
    *output_ << string((16 - i)*3+1, ' ') << text << endl;
    *output_ << setw(width) << setfill(fillch);
    output_->setf(flags);
    output_->flush();
}

/**
 * Print status of all tracing tags
 */
void
Trace::print_tag_status()
{
    TraceTag* t;
    int l = 0;

    for (t = tags_; t; t = t->next_) {
        int ll = int(strlen(t->id_));

        if (ll > l)
            l = ll;
    }
    for (t = tags_; t; t = t->next_)
        form(*output_, "%*s: %s\n", l, t->id_, (t->enabled_ ? "on" : "off"));
}
/**
* check the global flag(s), true if present, false if not
*/
bool
Trace::checkFlag(long int flag)
{
    return CHECK_FLAG(flag);
}


/**
 * Print the message line (w/o tag name)
 *
 * @arg fmt the format string
 * @arg ... the argument list
 */
void
Logger::log(Severity severity, const char* fmt, ...)
{
}

 /// returns singleton of default logger
Logger* 
Logger::i() 
{ 
	static Logger l; return &l; 
} 


} // namespace
