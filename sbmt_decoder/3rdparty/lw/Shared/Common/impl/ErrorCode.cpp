// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/**
 * Implementation for ErrorCode class
 */
//*****************************************************************************
// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved


#include <Common/stldefs.h>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include "Common/ErrorCode.h"

using namespace std;

namespace LW {

/** 
 * default (SUCCESS) constructor
 */
ErrorCode::ErrorCode()
{
    code_ = 0;
    prev_ = 0;
}


/** 
 * copy constructor; can add file/line to the message
 */
ErrorCode::ErrorCode(const ErrorCode& err, const char* file, int line)
{
    set(err, file, line);
}


/** 
 * constructs from const char*
 */
ErrorCode::ErrorCode(int ec, const char* msg, const ErrorCode* prev)
{
    set(ec, msg, prev);
}


/** 
 * constructs from a formatted string
 */
ErrorCode::ErrorCode(int ec, const ErrorCode* prev, const char* fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf_s(buf, 1024, 1023, fmt, ap);
    va_end(ap);

    set(ec, buf, prev);
}

/**
 * constructs from an STL-string
 */
ErrorCode::ErrorCode(int ec, const string& msg, const ErrorCode* prev)
{
    set(ec, msg, prev);
}



/** 
 * sets content
 */
void
ErrorCode::set(const ErrorCode& err, const char* file, int line)
{
    code_ = err.code_;

    // optimization
    if (code_ == 0) {
        prev_ = 0;
        return;
    }

    if (!file)
        msg_ = err.msg_;

    else {
        char buf[256];
        lw_snprintf(buf, 255, " at (%s:%d)", file, line);
        msg_ = err.msg_ + string(buf);
    }

    // loc_ = err.loc_;
    if (err.prev_ )
        prev_ = new ErrorCode(*err.prev_);

    else
        prev_ = 0;
}

/**
 * sets content, message as const char*
 */
void
ErrorCode::set(int ec, const char* msg, const ErrorCode* prev)
{
     code_ = ec;
    if (msg)
        msg_ = string(msg);

    if (prev && prev->code_ != 0)
        prev_ = new ErrorCode(*prev);
    else
        prev_ = 0;
}

/**
 * sets contents, formatted message
 */
void
ErrorCode::set(int ec, const ErrorCode* prev, const char* fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf_s(buf, 1024, 1023, fmt, ap);
    va_end(ap);

    set(ec, buf, prev);
}


/**
 * sets content, message as STL-string
 */
void
ErrorCode::set(int ec, const std::string& msg, const ErrorCode* prev)
{
    code_ = ec;
    msg_ = msg;
   
    if (prev)
        prev_ = new ErrorCode(*prev);
    else
        prev_ = 0;
}


/** 
  * assignment operator
  */
ErrorCode&
ErrorCode::operator=(const ErrorCode& that)
{
    if (this == &that)
        return *this;

    if (prev_ ) {
        delete prev_;
        prev_ = 0;
    }

    code_ = that.code_;
    msg_ = that.msg_;

    if (that.prev_)
        prev_ = new ErrorCode(*that.prev_);

    return *this;
}

void
ErrorCode::cut()
{
    int cnt = 0;
    for (ErrorCode* err = prev_; err; err = err->prev_) {
        if (++cnt > 10) {
            delete err->prev_;
            err->prev_ = 0;
            break;
        }
    }
}

/**
 * Merges two errors: result is the more severe
 */
ErrorCode&
ErrorCode::operator|=(const ErrorCode& that)
{
    cut();

    if (this == &that || that.code_ == 0)
        return *this; // x |= x returns x

    if (this->code_ == 0) {
        *this = that;
        return *this;
    }

    if (this->code_ > that.code_) {
        *this += that;
    }
    else {
        ErrorCode e = *this;
        *this = that;
        *this += e;
    }

    return *this;
}

/**
 * Add error to the chain of errors (copy if LHS is 0)
 */
ErrorCode&
ErrorCode::operator+=(const ErrorCode& that)
{
    cut();

    if (this == &that || that.code_ == 0)
        return *this;

    if (code_ == 0 && that.code_ != 0)
        return *this = that;

    {
       int n=0;
       ErrorCode* p = this;
       while (n++ < ERROR__MAXCHAIN && p->prev_) {
           p = p->prev_;
       }
       if (p->prev_ == 0)
           p->prev_ = new ErrorCode(that);
    }

    return *this;
}

/**
 * return message for the last error
 */
const char* 
ErrorCode::getErrorMessage(bool recursive) const
{
    if (code_ == 0)
        return "OK";

    if (msg_.size() == 0 || msg_[0] != '[')
    {
        char buf[128];
        
        lw_snprintf(buf, 128, "[%ld] ", code_);
      
        if (prev_ && *prev_ != 0) {
            if (recursive)
                msg_ += string(" {") + string(prev_->getErrorMessage(false)) + string("}");
            else
                msg_ += string(",...");
        }
    }
    return msg_.c_str();
}

/**
 * Returns full error message. Adds 'delim' to the end and 'head' to the beginning
 */
string
ErrorCode::getFullErrorMessage(const char* delim, const char* head) const
{
    string s;
    string omsg;
    const ErrorCode* a = this;
    while (a ) {
        omsg = a->msg_;
        s += string(head) + string(a->getErrorMessage(false)) + string(delim);
        a->msg_ = omsg;
        a = a->prev_;
    }
    return s;
}


/**
 * Clear message buffer, without dispatching the messages
 */
void
ErrorCode::clear()
{
    code_ = 0;
    msg_ = string();
    if (prev_) {
        delete prev_;
        prev_ = 0;
    }
}


/**
 * throws fatal error and abort process
 */
void
FatalError::abort(const FatalError& err)
{
    std::cerr << "\nfatal error: {\n " << err.getFullErrorMessage("\n","\t-->") << "}\n";
    throw err;
}

}; // namespace LW
