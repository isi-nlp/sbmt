/*  $Id: stats.cpp 1308 2006-10-06 04:37:37Z jturian $ 
 *  Copyright (c) 2004-2006, New York University. All rights reserved. */
/*!
 *  \file stats.cpp
 *  $LastChangedDate: 2006-10-05 21:37:37 -0700 (Thu, 05 Oct 2006) $
 *  $Revision: 1308 $
 */

#include "gpl/stats.hpp"
#include "Derivation.hpp"

#include <sys/resource.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

#include <boost/date_time/posix_time/posix_time.hpp>

#ifndef DOXYGEN
using namespace boost::posix_time;
using namespace boost::gregorian;
#endif

namespace stats {
	static ptime m_start_time(microsec_clock::universal_time());
}

std::string stats::current_time() {
	ptime t(microsec_clock::local_time());
	return to_simple_string(t);
}

std::string stats::resource_usage() {
	rusage r;
	unsigned ret;
	ret = getrusage(RUSAGE_SELF, &r);
	assert(ret==0);
	std::ostringstream o;
	o << "Resource usage at " << stats::current_time() << ": ";

	o.precision(4);
	float ty = (r.ru_utime.tv_sec + r.ru_stime.tv_sec) + 1./1000000 * (r.ru_utime.tv_usec + r.ru_stime.tv_usec);
	if (ty < 60) {
		o << ty << "s";
	} else {
		ty /= 60;
		if (ty < 60) {
			o << ty << "m";
		} else {
			ty /= 60;
			if (ty < 24) {
				o << ty << "h";
			} else {
				ty /= 24;
				o << ty << "d";
			}
		}
	}
	o << " user+sys";

	ptime cur_time(microsec_clock::universal_time());
	double elapsed = 1.*(cur_time - m_start_time).total_microseconds()*1e-6;
	double user_sys_ty = (r.ru_utime.tv_sec + r.ru_stime.tv_sec) + 1./1000000 * (r.ru_utime.tv_usec + r.ru_stime.tv_usec);
	o << " (" << 100.*user_sys_ty/elapsed << "% usage of " << elapsed << "s elapsed)";

//	o << "\tmaximum resident set size = " << r.ru_maxrss * getpagesize() / 1024. / 1024 << " MB\n";
//	o << "\tintegral shared memory size = " << r.ru_ixrss * getpagesize() / 1024. / 1024 << " MB\n";
//	o << "\tintegral unshared data size = " << r.ru_idrss * getpagesize() / 1024. / 1024 << " MB\n";
//	o << "\tintegral unshared stack size = " << r.ru_isrss * getpagesize() / 1024. / 1024 << " MB\n";
//	o << "\tpage reclaims = " << r.ru_minflt << "\n";
//	o << "\tpage faults = " << r.ru_majflt << "\n";

	std::ostringstream pidf;
	pidf << "/proc/" << getpid() << "/status";
	std::ifstream i(pidf.str().c_str());
	assert(i.good());
	std::string s;
	do {
		i >> s;
	} while(s != "VmSize:");
	unsigned vmsize;
	i >> vmsize;
	o.precision(4);
	o << ", " << vmsize/1024. << " MB vmsize ";

	if (r.ru_nswap) o << ", " << r.ru_nswap << " SWAPS\n";

	return o.str();
}

double stats::usersys_time() {
	rusage r;
	unsigned ret;
	ret = getrusage(RUSAGE_SELF, &r);
	assert(ret==0);
	return (r.ru_utime.tv_sec + r.ru_stime.tv_sec) + 1./1000000 * (r.ru_utime.tv_usec + r.ru_stime.tv_usec);
}
