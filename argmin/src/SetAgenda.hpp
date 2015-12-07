/*  $Id: SetAgenda.hpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file SetAgenda.hpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */
/*!
 *  \class argmin::SetAgenda
 *  \brief An Agenda based upon an stl::set
 *
 *  \todo Make this into an n-best list, so that we cannot exceed memory?
 *  \warning Don't use me, pops are relatively slow compared to PriorityQueueAgenda.
 *
 */

#ifndef __ARGMIN_SET_AGENDA_HPP__
#define __ARGMIN_SET_AGENDA_HPP__

#include "Agenda.hpp"
#include "Derivation.hpp"

#include <set>

namespace argmin {


class SetAgenda : public Agenda {
	friend class CyclingAgenda;	///< \todo REMOVEME?
public:
	/// Is the agenda empty?
	bool empty() const { return _set.empty(); }

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

	const Derivation& top() const;
	const Derivation& bottom() const;
	void sanity_check() const;

	std::set<Derivation, std::greater<Derivation> > _set;
};	// class SetAgenda


}	// namespace argmin

#endif	// __ARGMIN_SET_AGENDA_HPP__
