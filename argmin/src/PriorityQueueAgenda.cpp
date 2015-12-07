/*  $Id: PriorityQueueAgenda.cpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file PriorityQueueAgenda.cpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */

#include "PriorityQueueAgenda.hpp"

namespace argmin {

void PriorityQueueAgenda::pop(Derivation& d) {
	assert(!this->empty());
	d = _q.top();
	_q.pop();
}

void PriorityQueueAgenda::add(const Derivation& i) {
	/// \todo Make sure we don't insert duplicate elements
	_q.push(i);
}

/// The number of Derivation%s in the Agenda.
unsigned PriorityQueueAgenda::size() const {
	return _q.size();
}

void PriorityQueueAgenda::remove_worst() {
	/// \todo FIXME. This operation is unsupported by stl::priority_queue
	assert(0);
}

/*
/// \todo WRITEME? Can we even implement this efficiently without using
/// the heap functions directly?
void PriorityQueueAgenda::prune(Double threshold) {
	assert(0);
}
*/

}	// namespace argmin

