/*  $Id: CyclingAgenda.cpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file CyclingAgenda.cpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */

#include "CyclingAgenda.hpp"

#include "gpl/Debug.hpp"

namespace argmin {

CyclingAgenda::CyclingAgenda(boost::function<unsigned (const Derivation& d)> get_index, unsigned max_index) :
	_qs(max_index + 1),
	_current_index(0),
	_max_index(max_index),
	_get_index(get_index)
{
}

bool CyclingAgenda::empty() const {
	std::vector<SetAgenda>::const_iterator q;
	//std::vector<PriorityQueueAgenda>::const_iterator q;
	for (q = _qs.begin(); q != _qs.end(); q++)
		if (!q->empty()) return false;
	return true;
}

void CyclingAgenda::pop(Derivation& d) {
	assert(!this->empty());

	while (current_agenda().empty()) advance_index();
	Debug::log(5) << "Popped from agenda #" << _current_index << "\n";
	current_agenda().pop(d);
	advance_index();
}

void CyclingAgenda::add(const Derivation& i) {
	/// \todo Make sure we don't insert duplicate elements
	_qs.at(_get_index(i)).add(i);

	while (this->size() > MAX_AGENDA_SIZE)
		this->remove_worst();
}

/// The number of Derivation%s in the Agenda.
unsigned CyclingAgenda::size() const {
	unsigned s = 0;
	std::vector<SetAgenda>::const_iterator q;
	//std::vector<PriorityQueueAgenda>::const_iterator q;
	for (q = _qs.begin(); q != _qs.end(); q++)
		s += q->size();
	return s;
}

void CyclingAgenda::remove_worst() {
	assert(!this->empty());

	std::vector<SetAgenda>::iterator qworst = _qs.begin();
	//std::vector<PriorityQueueAgenda>::iterator qworst = _qs.begin();
	Tiebreak<unsigned> qworst_size(qworst->size());

	// Find the longest Agenda
	std::vector<SetAgenda>::iterator q;
	//std::vector<PriorityQueueAgenda>::iterator q;
	for (q = _qs.begin(); q != _qs.end(); q++) {
		Tiebreak<unsigned> qsize(q->size());
		if (qsize > qworst_size) {
			qworst_size = qsize;
			qworst = q;
		}
	}
	// ...and remove the worst element from it.
	assert(!qworst->empty());
	qworst->remove_worst();
}

/*
void CyclingAgenda::prune(Double threshold) {
	unsigned old = this->size();
	std::vector<SetAgenda>::iterator q;
	for (q = _qs.begin(); q != _qs.end(); q++)
		q->prune(threshold);

	unsigned d = old - this->size();
	Debug::log(3) << "CyclingAgenda::prune at threshold " << threshold << " pruned " << 100.*d/old << "% (" << d << "/" << old << ") of derivations, leaving " << this->size() << "\n";
}
*/

}	// namespace argmin

