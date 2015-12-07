// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/**
 * Definitions for the ErrorCode class
 */
//*****************************************************************************
// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved


#ifndef _LW_ERROR_H_
#define _LW_ERROR_H_

//#include <stdio.h>
#include <iostream>
#include <string>
#include <Common/Platform.h>
#include <stdexcept>

#ifndef _WIN32
#define _vsnprintf vsnprintf
#endif

class QDataStream;

namespace LW {

//#define LW_ERROR(_errcode, _text) ErrorCode(_errcode, _text)
#define LW_ERROR(_errcode, _severity, _text) ErrorCode(_LW_ERR_CODE(_errcode, _severity), _text)
#define LW_ABORT(_err) FatalError::abort(FatalError(_err, __FILE__, __LINE__))
#define LW_OTHER_FATAL_ERROR  FatalError(ERR_UNKNOWN)
#define _LW_ERR_CODE(_err, _severity) ((_err) | (((_severity) & 0xf)) << 20)
#define _LW_GET_ERR(_errcode) ((_errcode) & 0x0fffffff)
#define _LW_GET_SEVERITY(_errcode) (((_errcode) >> 20) & 0xf)


/// severity levels
enum Severity {
    SVT_UNKNOWN = 0,
    SVT_FATAL = 1,
    SVT_ERROR = 2,
    SVT_WARNING = 3,
    SVT_INFO = 4,
    SVT_DEBUG = 5
};


/// Numeric error codes
enum eErrorCode {
    OK = 0,
    ERR_XML_PARSE,
    ERR_IO,   
    ERR_EOF,
	ERR_PARSE,
    ERR_INVALID_PARAM,
    ERR_COMM,
    ERR_TERMINATED,
    ERR_USER_INPUT,
	ERR_INVALID_INPUT,
	ERR_CONFIG,
	ERR_CHAR_ENCODING,
	ERR_NOT_INITIALIZED,
	ERR_TOO_MANY,
	ERR_SESSION_ID_USED,
	ERR_CRYPTO,
	ERR_HTML_TEMPLATE_NOT_FOUND,
	ERR_FILE_TOO_LARGE,
	ERR_FILE_TOO_SMALL,
	ERR_AUTH,
	ERR_NOT_FOUND,
	ERR_LICENSE,
	ERR_NOT_IMPLEMENTED,
	ERR_DICTIONARY_NOT_FOUND,
	ERR_CUSTOMIZER_NOT_FOUND,
	ERR_DB_ACCESS,
	ERR_INTERNAL_ERROR,
    ERR_UNKNOWN = -1
};

/// simple error without text message
#define _LW_ERR(_errcode) ErrorCode(_errcode)


/**
 * ErrorCode accumulates error codes with the associated text messages.
 * ErrorCode carry <b>error code</b> and <b>error message</b>
 */
class ErrorCode  : public std::exception
{
public:


    enum {
        MIN_SEVERITY = 0,
        MAX_SEVERITY = 15
    };

    /// default constructor (for 'SUCCESS')
    ErrorCode();

    /// copy constructor
    ErrorCode(const ErrorCode& err, const char* file = 0, int line = 0);

    /// constructor, message as const char*
    ErrorCode(int ec, const char* msg = 0, const ErrorCode* prev = 0);

    /// constructor, formatted message
    ErrorCode(int ec, const ErrorCode* prev, const char* fmt, ...);

    /// constructor, message as STL-string
    ErrorCode(int ec, const std::string& msg, const ErrorCode* prev = 0);

    /// destructor clears all chain of errors
    ~ErrorCode() throw() { clear(); }

    /// casting to int
    operator int() const { return code_; };


    /// sets content
    void set(const ErrorCode& err, const char* file = 0, int line = 0);

    /// sets content, message as const char*
    void set(int ec, const char* msg = 0, const ErrorCode* prev = 0);

    /// sets contents, formatted message
    void set(int ec, const ErrorCode* prev, const char* fmt, ...);

    /// sets content, message as STL-string
    void set(int ec, const std::string& msg, const ErrorCode* prev = 0);

    /// clears this, copy that to this
    ErrorCode& operator=(const ErrorCode& that);

    /// merges two errors
    ErrorCode& operator|=(const ErrorCode& that);

    /// appends error
    ErrorCode& operator+=(const ErrorCode& that);

    /// returns the error message
    const char* getErrorMessage(bool recursive  = true) const;

    const char * what () const throw()
    {
        return getErrorMessage();
    }
    
    /// returns an int32 error code
    int getErrorCode() const { return code_; }

    /// returns the error code
    int getError() const { return _LW_GET_ERR(code_); }

    /// returns the severity code
    int getSeverity() const { return _LW_GET_SEVERITY(code_); }

    /// creates and returns full error message
    std::string getFullErrorMessage(const char* delim = "\n", const char* head = "") const;

    /// print message
    void print(std::ostream& out) { out << getFullErrorMessage(); }

    /// throw fatal error
    static void abort(const ErrorCode& reason);

	// marshalling
	void read(QDataStream& s);
	void write(QDataStream& s);

protected:

    /// Clear message buffer, without dispatching the messages
    void clear();

    void cut();

private:

    enum {
        ERROR__MAXCHAIN = 8
    };

    /// error code
    int code_;

    /// error message
    mutable std::string msg_;

    /// previous error in the chain
    ErrorCode* prev_;
};

typedef class ErrorCode Exception;

/// Fatal ErrorCode
typedef class ErrorCode FatalError;

inline std::ostream&
operator<< (std::ostream& str, const ErrorCode& err)
{
    return str << err.getErrorMessage();
}

class AbortException : public Exception
{
public:
	AbortException() 
		: ErrorCode(ERR_TERMINATED, "Operation aborted on request.") {};
};


}; // namespace LW

#endif // _LW_ERROR_H_
