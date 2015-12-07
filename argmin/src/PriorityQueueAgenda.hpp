/*  $Id: PriorityQueueAgenda.hpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file PriorityQueueAgenda.hpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */
/*!
 *  \class argmin::PriorityQueueAgenda
 *  \brief An Agenda based upon a stl::priority_queue
 *
 *  \todo Make this into an n-best list, so that we cannot exceed memory?
 *
 */

#ifndef __ARGMIN_PRIORITY_QUEUE_AGENDA_HPP__
#define __ARGMIN_PRIORITY_QUEUE_AGENDA_HPP__

#include "Agenda.hpp"
#include "Derivation.hpp"

#include <queue>	// For priority_queue

namespace argmin {


class PriorityQueueAgenda : public Agenda {
public:
	/// Is the agenda empty?
	bool empty() const { return _q.empty(); }

	/// Remove and store the highest-priority element
	/// \pre !this->empty()
	/// \post d will be overwritten by that element.
	void pop(Derivation& d);

	/// The number of Derivation%s in the Agenda.
	unsigned size() const;

	/// Add a Derivation to the Agenda.
	/// The Derivation will not be added if it falls below _score_threshold.
	/// \todo Make sure we don't insert duplicate elements
	void add(const Derivation& i);

/*
	/// Prune using the current _score_threshold.
	void prune(Double threshold);
*/

private:
	/// Remove the worst Derivation from the Agenda.
	void remove_worst();

	std::priority_queue<Derivation> _q;
};	// class PriorityQueueAgenda


}	// namespace argmin

#endif	// __ARGMIN_PRIORITY_QUEUE_AGENDA_HPP__
