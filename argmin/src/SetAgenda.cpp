/*  $Id: SetAgenda.cpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file SetAgenda.cpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */

#include "SetAgenda.hpp"

namespace argmin {

const Derivation& SetAgenda::top() const {
	return *_set.begin();
}

const Derivation& SetAgenda::bottom() const {
	return *_set.rbegin();
}

void SetAgenda::pop(Derivation& d) {
	assert(!this->empty());
	d = this->top();
	_set.erase(_set.begin());
}

void SetAgenda::add(const Derivation& i) {
	// Make sure we don't insert duplicate elements
	assert(_set.find(i) == _set.end());
	_set.insert(i);
	this->sanity_check();
}

void SetAgenda::sanity_check() const {
	if (!this->empty())
		assert(this->top() >= this->bottom());
}

/// Remove the worst Derivation from the Agenda.
void SetAgenda::remove_worst() {
	assert(!this->empty());
	_set.erase(*_set.rbegin());
}


/*
void SetAgenda::prune(Double threshold) {
	this->sanity_check();

	while (!this->empty() && _set.rbegin()->score() < threshold)
		_set.erase(*_set.rbegin());

	this->sanity_check();
}
*/

/// The number of Derivation%s in the Agenda.
unsigned SetAgenda::size() const {
	return _set.size();
}

}	// namespace argmin

