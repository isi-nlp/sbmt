/*  $Id: CyclingAgenda.hpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file CyclingAgenda.hpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */
/*!
 *  \class argmin::CyclingAgenda
 *  \brief A meta-Agenda that cycles over indexed Agenda%e.
 *
 *  WRITEME
 *
 *  \todo Make this into an n-best list, so that we cannot exceed memory?
 *  \todo Templatize by Agenda type.
 *
 */

#ifndef __ARGMIN_CYCLING_AGENDA_HPP__
#define __ARGMIN_CYCLING_AGENDA_HPP__

#include "Agenda.hpp"
#include "Derivation.hpp"
//#include "PriorityQueueAgenda.hpp"
#include "SetAgenda.hpp"

#include <queue>	// For priority_queue
#include <boost/function.hpp>	/// \todo REMOVEME, and forward declare boost::function?

namespace argmin {

class CyclingAgenda : public Agenda {
public:
	/// WRITEME
	CyclingAgenda(boost::function<unsigned (const Derivation& d)> get_index, unsigned max_index);

	/// Is every agenda empty?
	bool empty() const;

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
	/// For a CyclingAgenda, we remove the worst Derivation
	/// from the longest Agenda.
	void remove_worst();

	void advance_index() {
		_current_index++;
		if (_current_index > _max_index) _current_index = 0;
	}

	SetAgenda& current_agenda() {
	//PriorityQueueAgenda& current_agenda() {
		return _qs.at(_current_index);
	}

	std::vector<SetAgenda> _qs;
	//std::vector<PriorityQueueAgenda> _qs;

	unsigned _current_index;
	unsigned _max_index;

	/// A pointer to an indexing function.
	/// _get_index(d) computes the index of d, starting at 0.
	boost::function<unsigned (const Derivation& d)> _get_index;
};	// class CyclingAgenda


}	// namespace argmin

#endif	// __ARGMIN_CYCLING_AGENDA_HPP__
