/*  $Id: throw.hpp 1280 2006-09-26 03:38:52Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file throw.hpp
 *  $LastChangedDate: 2006-09-25 20:38:52 -0700 (Mon, 25 Sep 2006) $
 *  $Revision: 1280 $
 *
 *  \todo Make this header a part of the sbmt_decoder library or graehl.
 *
 */

#ifndef __COMMON_THROW_HPP__
#define __COMMON_THROW_HPP__

#include <string>
#include <stdexcept>

namespace common{


static inline void throw_if(bool cond, std::string const& reason)
{ if (cond) throw std::runtime_error(reason); }

static inline void throw_unless(bool cond, std::string const& reason)
{ throw_if(!cond,reason); }


}	// namespace argmin

#endif	// __COMMON_THROW_HPP__
