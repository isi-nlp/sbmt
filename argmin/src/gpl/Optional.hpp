/*  $Id: Optional.hpp 1308 2006-10-06 04:37:37Z jturian $
 *  Copyright (c) 2004-2006, New York University. All rights reserved. */
/*!
 *  \file Optional.hpp
 *  $LastChangedDate: 2006-10-05 21:37:37 -0700 (Thu, 05 Oct 2006) $
 *  $Revision: 1308 $
 */
/*!
 *  \class Optional
 *  \brief An Optional data-type.
 *
 *  Optional objects can be either empty, or have some particular value.
 *
 *  \warning This class should not be used if the data-type is UNICODE!
 *  \todo Remove dependence on T::T() constructor (use 'new' and store a pointer?)
 *
 */

#ifndef __Optional_H__
#define  __Optional_H__

#include <iostream>
#include <sstream>
//using namespace std;

template<class T>
class Optional {
public:
	/// Create an empty object.
	Optional();

	/// Create a non-empty object with a particular value.
	Optional(const T& t);

	/// Return the object's value.
	/// \note An assertion will be thrown if the object is empty.
	const T& operator()() const;

	/// Is the object empty?
	bool empty() const;

	/// Make the object empty.
	void clear();

	bool operator==(const T& t) const;
	void operator=(const T& t);

	std::string str() const;

	/*
	// Disabled cuz there's no Debug:: methods

	//template<class U> friend istream& operator>>(istream& i, Optional<U>& t);
	//template<class U> friend ostream& operator<<(ostream& o, const Optional<U>& t);
	/// \warning This input operator will SPEW if we try to read UNICODE.
	friend istream& operator>>(istream& i, Optional<T>& t) {
		string s;
		i >> s;
		if (s == "_") {
			t.clear();
		} else {
			istringstream is(s);
			is >> t._t;
			assert(is.eof());
		}
		return i;
	}
	friend ostream& operator<<(ostream& o, const Optional<T>& t) {
		if (t.empty()) o << "_";
		else o << t();
		return o;
	}
	*/

private:
	T _t;
	bool _empty;
};

template<class T> Optional<T>::Optional() : _t(T()), _empty(true) { }

template<class T> Optional<T>::Optional(const T& t) : _t(t), _empty(false) { }

template<class T> const T& Optional<T>::operator()() const { assert(!this->empty()); return _t; }

template<class T> bool Optional<T>::empty() const { return _empty; }

template<class T> void Optional<T>::clear() { _empty = true; }

template<class T> bool Optional<T>::operator==(const T& t) const { assert(!this->empty()); return (*this)() == t; }

template<class T> void Optional<T>::operator=(const T& t) { _empty = false; _t = t; }

/*
template<class T> istream& operator>>(istream& i, Optional<T>& t) {
}

template<class T> ostream& operator<<(ostream& o, const Optional<T>& t) {
	if (t.empty()) o << "_";
	else o << t();
	return o;
}
*/

template<class T> std::string Optional<T>::str() const {
	std::ostringstream o;
	if (this->empty()) o << "_";
	else o << (*this)();
	return o.str();
}

#endif /* __Optional_H__ */
