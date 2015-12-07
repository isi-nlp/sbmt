/*  $Id: stats.hpp 1308 2006-10-06 04:37:37Z jturian $ 
 *  Copyright (c) 2004-2006, New York University. All rights reserved. */
/*!
 *  \file stats.hpp
 *  $LastChangedDate: 2006-10-05 21:37:37 -0700 (Thu, 05 Oct 2006) $
 *  $Revision: 1308 $
 */
/*!
 *  \namespace stats
 *  \brief Statistics maintained by the system.
 *
 */

#ifndef __STATS_HPP__
#define  __STATS_HPP__

#include <string>

namespace stats {
	std::string current_time();
	std::string resource_usage();

	/// Get the user+sys time.
	double usersys_time();
}

#endif /* __STATS_HPP__ */
