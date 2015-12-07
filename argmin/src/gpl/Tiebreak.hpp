/*  $Id: Tiebreak.hpp 1293 2006-10-01 23:18:34Z jturian $
 *  Copyright (c) 2004-2006, New York University. All rights reserved. */
/*!
 *  \file Tiebreak.hpp
 *  $LastChangedDate: 2006-10-01 16:18:34 -0700 (Sun, 01 Oct 2006) $
 *  $Revision: 1293 $
 */
/*!
 *  \class Tiebreak
 *  \brief Define a canonical tiebreaking.
 *
 *  Given an object with operator< and operator== defined, assign it
 *  random value in the range [0, 1) which is used for tiebreaking.
 *
 *  \note We assume that T is totally-ordered, not just partially-ordered.
 *  See http://boost.org/libs/utility/operators.htm#ordering for more details
 *
 *  \todo Use boost::random and a user-supplied seed, not just plain drand48()
 *
 */

#ifndef __TIEBREAK_HPP__
#define __TIEBREAK_HPP__

#include <boost/operators.hpp>
#include <cassert>

//namespace argmin {

template <typename T>
class Tiebreak
	: boost::totally_ordered< Tiebreak<T> >
{
public:
	/// \todo Use boost::random and a user-supplied seed, not just plain drand48()
	Tiebreak(const T& t) : _t(t), _v(drand48()) {}

//	void set(T t) { _t = t; }
	void set(const T& t) { _t = t; }

	const T& operator()() const { return _t; }

	bool operator<(const Tiebreak<T>& t) const {
//		assert(*this != t);
		if (_t < t._t) return true;
		else if (_t > t._t) return false;
		assert(_t == t._t);
		return _v < t._v;
	}

	//bool operator>(const Tiebreak<T>& t) const { return t < *this; }
	bool operator==(const Tiebreak<T>& t) const { return _t == t._t && _v == t._v; }

private:
	T _t;
	double _v;
};


//}	// namespace argmin

#endif	// __TIEBREAK_HPP__
