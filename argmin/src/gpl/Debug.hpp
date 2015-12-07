/*  $Id: Debug.hpp 1287 2006-09-26 23:01:36Z jturian $ 
 *  Copyright (c) 2004-2006, New York University. All rights reserved. */
/*!
 *  \file Debug.hpp
 *  $LastChangedDate: 2006-09-26 16:01:36 -0700 (Tue, 26 Sep 2006) $
 *  $Revision: 1287 $
 */
/*
 *  \class Debug
 *  \brief A simple debug/logging library.
 *
 *  Debug objects are singletons. Debug::log(n) is an ostream at debuglevel n.
 *  The higher the debuglevel, the less crucial is the debug output.
 *
 *  It is used globally to handle Debug output (output it to a tee'd
 *  stream), and its behavior is controlled by DebugOptions, which are
 *  passed in the constructor. So, it is typical to use Debug static methods.
 *
 *  \todo Re-enable use of parameter
 *
 *  \todo Short-circuit any debug output that is at too high a debuglevel? i.e.\ such
 *  that no time is wasted string-converting the objects that would have been output.
 *  We can do this at compile-time, by setting an upper-bound on the run-time debuglevel.
 *  \todo Make these static methods?
 *  \todo Split logstream s.t. output also goes to cerr.
 *  \todo Log to several logs simultaneously, one for each error-level?
 *  \todo Add Debug::increase_depth(unsigned debuglevel) and
 *  Debug::decrease_depth(unsigned debuglevel) to allow us to globally
 *  control indentation depth.
 *  \todo Maybe rewrite Debug as a namespace.
 *
 */

#ifndef __DEBUG_HPP_
#define  __DEBUG_HPP_


#include "gpl/stats.hpp"      // to save you some typing

#include "gpl/onullstream.hpp"

#include <boost/iostreams/filtering_stream.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#ifndef DOXYGEN
using namespace std;
#endif /* DOXYGEN */

/// Prefer Debug::warning() over TRACE.
#define TRACE \
	{ std::cerr << "TRACE " << __FILE__ << ":" << __LINE__ << "\n"; }
//	{ Debug::log(1) << "TRACE " << __FILE__ << ":" << __LINE__ << "\n"; }

//class ostream;

/// Debug class options.
/// \internal If you modify this, you may also want to modify
/// argmin::parse_args, where DebugOpts are parsed.
struct DebugOptions {
	/// Global debug level.
	/// Only output debug messages that are logged not above this debug level.
	/// [default: 3]
	unsigned debuglevel;

	/// File to log debug output (in addition to cerr).
	/// [default: "", i.e. none]
	string logfile;

	/// Buffer log output?
	/// [default: false]
	bool buffer;

	/// Dump volatile diagnostics in debug output?
	/// These include time and memory usage.
	/// [default: true]
	bool dump_volatile_diagnostics;

	DebugOptions() : debuglevel(3), logfile(""), buffer(false), dump_volatile_diagnostics(true) { }
};

class Debug {
public:
/*
	/// Open the Debug object for tee output.
	/// \param logfile Tee Debug output to stderr and this file.
	/// \pre !_opened
	/// \post _opened
	Debug(string logfile);

	/// Open the Debug object for cerr output.
	/// \pre !_opened
	/// \post _opened
	Debug();
	*/

	/// Open the Debug object, using the given DebugOptions.
	/// \pre !_opened
	/// \post _opened
	Debug(const DebugOptions& options);

	/// Output all the warnings (in order), and then close the Debug stream.
	/// \todo Sort the warnings by number of occurrences.
	~Debug();

	/// Log a message.
	/// Specifically, return an ostream for a particular debuglevel.
	/// \param debuglevel The debuglevel of the message to be logged.
	/// \return cerr iff debuglevel <= parameter::debuglevel()
	/// A stream to /dev/null otherwise
	/// \pre _opened
	static ostream& log(unsigned debuglevel);

	/// Will we logged at some debuglevel?
	/// \param debuglevel Some debuglevel.
	/// \return True iff a message will be logged at this debuglevel.
	static bool will_log(unsigned debuglevel);

	/// Output a warning message.
	/// Typical usage:
	///	Debug::warning(__FILE__, __LINE__, "foo");
	/// \note We will output each warning only once.
	static void warning(const char* file, int line, string message="");

	static bool dump_volatile_diagnostics();

private:
	static onullstream _null;
	static boost::iostreams::filtering_ostream _out;
	static FILE* _outf;
	static bool _opened;

	static map<string, unsigned> warnings_used;
	static vector<string> warnings_used_list;

	static DebugOptions _options;
};

#endif /* __DEBUG_HPP__ */
