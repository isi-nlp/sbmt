// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/**
 * Definitions for the Log class
 */
//*****************************************************************************
// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved


#ifndef _LW_LOG_H_
#define _LW_LOG_H_

#include "Common/ErrorCode.h"

namespace LW {

// usage: LW_LOG((FATAL, "Fatal error %s", err));
#define LOG(x) Logger::i()->log x


/**
 * Initial revision of logging API - to be extended
 */
class Logger 
{
public:

    /// returns singleton of default logger
    static Logger* i();

    /// sets ouput and sync options
    void setOutput(std::ostream* output) { output_ = output; }

    /// sets minimum severity
    void setMinSeverity(int severity) { min_severity_ = severity; }

    /// writes message
    void log(Severity severity, const char* fmt,...);

private:

    Logger() { output_ = &std::cerr; min_severity_ = SVT_WARNING; }

    int min_severity_;

    std::ostream* output_;
    /// default logger
    //static Logger* logger_;
};


}; // namespace LW

#endif // _LW_LOG_H_
