/*  $Id: Debug.cpp 1308 2006-10-06 04:37:37Z jturian $
 *  Copyright (c) 2004-2006, New York University. All rights reserved. */
/*!
 *  \file Debug.cpp
 *  $LastChangedDate: 2006-10-05 21:37:37 -0700 (Thu, 05 Oct 2006) $
 *  $Revision: 1308 $
 */

#include "gpl/Debug.hpp"
//#include "common/parameter.H"

#include <unistd.h>

#include <iostream>

#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/chain.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
//#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/tee.hpp>

onullstream Debug::_null;
boost::iostreams::filtering_ostream Debug::_out;
FILE* Debug::_outf = NULL;
bool Debug::_opened = false;
map<string, unsigned> Debug::warnings_used;
vector<string> Debug::warnings_used_list;
DebugOptions Debug::_options;

//Debug _debug;

/*
/// Open the Debug object for cerr output.
/// \pre !_opened
/// \post _opened
Debug::Debug() {
	assert(!_opened);
	_out.push(boost::iostreams::file_descriptor(STDERR_FILENO));
	if (!_options.buffer)
		_out.setf(std::ios_base::unitbuf);	// Disable buffering for _out
	_opened = true;

	Debug::log(1) << "\n\n";
	Debug::log(1) << "#####################################################################\n";
	Debug::log(1) << "#####################################################################\n";
	Debug::log(1) << "Logging to cerr\n";

	if (_options.dump_volatile_diagnostics)
		Debug::log(2) << stats::resource_usage() << "\n";
	Debug::log(2) << "####################################\n";
	Debug::log(2) << "# Global parameters:\n";
//	Debug::log(2) << parameter::str("\t");
	Debug::log(2) << "\n\n";
}

/// Open the Debug object.
/// \param logfile Tee Debug output to stderr and this file.
/// \pre !_opened
/// \post _opened
Debug::Debug(string logfile) {
	assert(!_opened);
	_out.push(boost::iostreams::tee(boost::iostreams::file_descriptor(STDERR_FILENO)));

	FILE* _outf;
//	if (parameter::resumeTraining())
//	  _outf = fopen(logfile.c_str(), "at");
//	else
	  _outf = fopen(logfile.c_str(), "w");
	assert(_outf);
//	_out.push(boost::iostreams::bzip2::bzip2_compressor());
	_out.push(boost::iostreams::file_descriptor(fileno(_outf)));
	if (!_options.buffer)
		_out.setf(std::ios_base::unitbuf);	// Disable buffering for _out

	_opened = true;

	Debug::log(1) << "\n\n";
	Debug::log(1) << "#####################################################################\n";
	Debug::log(1) << "#####################################################################\n";
	Debug::log(1) << "Logging to cerr + " << logfile << "\n";

	if (_options.dump_volatile_diagnostics)
		Debug::log(2) << stats::resource_usage() << "\n";
	Debug::log(2) << "####################################\n";
	Debug::log(2) << "# Global parameters:\n";
//	Debug::log(2) << parameter::str("\t");
	Debug::log(2) << "\n\n";
}
*/

/// Open the Debug object.
/// \pre !_opened
/// \post _opened
Debug::Debug(const DebugOptions& options) {
	assert(!_opened);

	Debug::_options = options;

	if (_options.logfile == "") {
		_out.push(boost::iostreams::file_descriptor(STDERR_FILENO));
	} else {
		_out.push(boost::iostreams::tee(boost::iostreams::file_descriptor(STDERR_FILENO)));

		FILE* _outf;
//		if (parameter::resumeTraining())
//		  _outf = fopen(_options.logfile.c_str(), "at");
//		else
		  _outf = fopen(_options.logfile.c_str(), "w");
		assert(_outf);
//		_out.push(boost::iostreams::bzip2::bzip2_compressor());
		_out.push(boost::iostreams::file_descriptor(fileno(_outf)));
	}

	if (!_options.buffer)
		_out.setf(std::ios_base::unitbuf);	// Disable buffering for _out

	_opened = true;

	Debug::log(1) << "\n\n";
	Debug::log(1) << "#####################################################################\n";
	Debug::log(1) << "#####################################################################\n";
	Debug::log(1) << "Logging to cerr + " << _options.logfile << "\n";

	if (_options.dump_volatile_diagnostics)
		Debug::log(2) << stats::resource_usage() << "\n";
	Debug::log(2) << "####################################\n";
	Debug::log(2) << "# Global parameters:\n";
//	Debug::log(2) << parameter::str("\t");
	Debug::log(2) << "\n\n";
}

/// Retrieve a logging ostream.
/// \param The debuglevel of the message to be logged.
/// \return cerr iff debuglevel <= _options.debuglevel
/// onullstream otherwise
/// \pre _opened
ostream& Debug::log(unsigned debuglevel) {
	assert(_opened);
	if (debuglevel <= _options.debuglevel) {
		for (unsigned d = 2; d < debuglevel && d < 10; d++) _out << " ";
		return _out;
	} else {
		return _null;
	}
}

/// Will we logged at some debuglevel?
/// \param debuglevel Some debuglevel.
/// \return True iff a message will be logged at this debuglevel.
bool Debug::will_log(unsigned debuglevel) {
	return debuglevel <= _options.debuglevel;
}

/// Output all the warnings (in order), and then close the Debug stream.
/// \todo Sort the warnings by number of occurrences.
Debug::~Debug() {
	if(_opened) {
		if (_outf) fclose(_outf);
	} else {
		assert(!_outf);
		assert(warnings_used.empty());
		return;
	}

	Debug::log(1) << "\n\n";
	if (!warnings_used.empty()) {
		Debug::log(0) << "==================================\n";
		Debug::log(0) << "==   WARNINGS USED              ==\n";
		Debug::log(0) << "==================================\n";
		for(vector<string>::const_iterator s = warnings_used_list.begin();
				s != warnings_used_list.end(); s++) {
			Debug::log(0) << warnings_used[*s] << " " << *s << "\n";
		}
	} else {
		Debug::log(1) << "==================================\n";
		Debug::log(1) << "No warnings :)\n";
	}
	Debug::log(1) << "\n\n";
	Debug::log(1) << "Ending logging.\n";
	if (_options.dump_volatile_diagnostics)
		Debug::log(1) << "Current time is now " << stats::current_time() << "\n";
	Debug::log(1) << "########################################################################################\n";
	Debug::log(1) << "\n\n\n\n";
}

bool Debug::dump_volatile_diagnostics() {
	assert(_opened);
	return _options.dump_volatile_diagnostics;
}

/// Output a warning message.
/// Typical usage:
///	Debug::warning(__FILE__, __LINE__, "foo");
/// \note We will output each warning only once.
void Debug::warning(const char* file, int line, string message) {
	ostringstream o;
	if (_options.dump_volatile_diagnostics)
		o << "WARNING (" << file << ":" << line << "): " << message << "\n";
	else
		o << "WARNING: " << message << "\n";
	string s = o.str();
	if (Debug::warnings_used.find(s) != Debug::warnings_used.end()) {
		Debug::warnings_used[s]++;
		return;
	}
	warnings_used[s] = 0;
	warnings_used_list.push_back(s);
	assert(warnings_used.size() == warnings_used_list.size());
	Debug::log(0) << s;
}
