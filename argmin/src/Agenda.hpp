/*  $Id: Agenda.hpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file Agenda.hpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */
/*!
 *  \class argmin::Agenda
 *  \brief An abstract Agenda.
 *
 *  The Agenda stores all Derivation%s that have been scored and are
 *  being considered to be added to the Chart. Agenda allows efficient
 *  retrieval of the highest-score Derivation stored.
 *
 *  \todo Make this into an n-best list, so that we cannot exceed memory?
 *  \todo Make the elements a (Derivation, priority) tuple?
 *  \todo Sanity check that priorities are monotonically decreasing as we pop them,
 *  (assuming the priority is based upon an admissable heuristic)
 *  \todo Rewrite the accounting (using DerivationStatistics) to make
 *  usage less prone to error. Maybe use non-virtual methods of Agenda
 *  to do accounting?
 *
 */

#ifndef __ARGMIN_AGENDA_HPP__
#define __ARGMIN_AGENDA_HPP__

#include <vector>	/// \todo REMOVEME, and just forward declare 'template<typename T> class std::vector;'

#include "types.hpp"	/// \todo REMOVEME, and just forward declare Derivation and Double
#include "gpl/Optional.hpp"

namespace argmin {

class Derivation;

class Agenda {
public:
	/// Is the agenda empty?
	virtual bool empty() const = 0;

	/// Remove and store the highest-priority element
	/// \pre !this->empty()
	/// \post d will be overwritten by that element.
	virtual void pop(Derivation& d) = 0;

	/// Add a Derivation to the Agenda.
	/// The Derivation will not be added if it falls below _score_threshold.
	/// \todo Make sure we don't insert duplicate elements
	virtual void add(const Derivation& i) = 0;

/*
	/// Prune using the current _score_threshold.
	virtual void prune(Double threshold) = 0;
*/

	/// The number of Derivation%s in the Agenda.
	virtual unsigned size() const = 0;

	virtual ~Agenda() { }

private:
	/// Remove the worst Derivation from the Agenda.
	virtual void remove_worst() = 0;

};	// class Agenda


}	// namespace argmin

#endif	// __ARGMIN_AGENDA_HPP__
