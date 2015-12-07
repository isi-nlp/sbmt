/*  $Id: Minimizer.hpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file Minimizer.hpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */
/*!
 *  \class argmin::Minimizer
 *  \brief Find the minimum-cost structure for some Input given some Grammar.
 *
 *  \todo Should methods that only update the Chart and Agenda be
 *  considered 'const' Minimizer methods?
 *
 */

#ifndef __ARGMIN_MINIMIZER_HPP__
#define __ARGMIN_MINIMIZER_HPP__

#include "Derivation.hpp"
//#include "Agenda.hpp"
//#include "PriorityQueueAgenda.hpp"
#include "CyclingAgenda.hpp"
#include "Chart.hpp"
#include "Grammar.hpp"
#include "gpl/NBest.hpp"

#include <boost/shared_ptr.hpp>

#include <graehl/shared/time_space_report.hpp>

namespace argmin {

class Minimizer {
public:
	/// WRITEME
	/// \param input Input over which to search for a minimum-cost global structure.
	/// \param gram Grammatical rules constraining Derivation%s (Statement%s and their costs).
	/// This is a Grammar \e template because it is not associated with any particular Input (yet).
	/// \todo Instantiate the Grammar (integrate Input and GrammarTemplate into one object)
	/// \todo initialize agenda Q (push all statements that have no antecedents)
	/// AS WELL AS the chart should be initialized. Maybe use a dummy 'nothing' statement to start
	Minimizer(unsigned sent_number, const Input& input, const GrammarTemplate& gram,
		boost::shared_ptr<StatementFactory> statement_factory,
		boost::shared_ptr<Interp> interp, boost::shared_ptr<graehl::stopwatch> timer);

	/// Find the lowest-cost Derivation of the goal Statement over the Input.
	/// \post _have_best == true
	/// \return A k-best list of the lowest-cost Derivation%s found of the goal Statement.
	/// \todo Make this const?
	/// \todo Don't redo work if we've already called argmin
	const NBest<Derivation>& argmin();

private:
	const Grammar& grammar() const { return *_grammar; }

	/// See if we have a new k-best Derivation for _argmin
	void update_argmin(const Derivation& d);
	void update_argmin(const vector<Derivation>& derived);

	/// Call update_argmin and _agenda.add on every new Derivation
	void agenda_add(const vector<Derivation>& derived);

	void diagnostics(unsigned debuglevel) const;

	/// We can prune any Derivation below this threshold.
	Optional<Double> threshold() const;

	bool consider(const Derivation& d) const;

	/// True if this Derivation beats the threshold.
	bool beats_threshold(const Derivation& d) const;

	string sentstr() const;

	/// \todo REMOVEME (Pust)
	StatementEquivalencePool& statement_equivalence_pool() const { return *_statement_equivalence_pool; }

	/// WRITEME
	/// \note We *always* stop if ::_agenda is empty
	bool stopping_condition() const;

	/// Sentence number
	unsigned _sent_number;

	/// # of derivations popped from Agenda
	unsigned _popped;

	/// WRITEME
	/// \todo initialize Chart C to have cost infinity for every statement
	Chart _chart;

	/// WRITEME
	//PriorityQueueAgenda _agenda;
	CyclingAgenda _agenda;
	//Agenda _agenda;

	/// Have we found some ``best'' (lowest cost i.e. highest score) Derivation of the goal?
	bool _have_argmin;

	NBest<Derivation> _argmin;

	/// \todo Do we have to use an edge equivalence pool? (Pust)
	/// And why does this have to be mutable?
	mutable boost::shared_ptr<StatementEquivalencePool> _statement_equivalence_pool;

	/// Weighted inference rules.
	/// \note Instantiation of the GrammarTemplate for this Input.
	boost::shared_ptr<Grammar> _grammar;

	boost::shared_ptr<Interp> _interp;
	boost::shared_ptr<graehl::stopwatch> _timer;
};	// class Minimizer

}	// namespace argmin

#endif	// __ARGMIN_MINIMIZER_HPP__
