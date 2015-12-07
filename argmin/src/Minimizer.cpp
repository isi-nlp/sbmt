/*  $Id: Minimizer.cpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file Minimizer.cpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */

#include "Minimizer.hpp"
#include "throw.hpp"
#include "gpl/Debug.hpp"

/// \todo REPLACEME
#define ARGMIN	1
//#define ARGMIN	1000

/// Max total user+sys seconds per sentence
/// \todo REMOVEME
#define MAX_TOTAL_SEC	1000
//#define MAX_TOTAL_SEC	100
/// Max wall seconds per sentence
/// \todo REMOVEME
#define MAX_WALL_SEC	5000
//#define MAX_WALL_SEC	500

namespace argmin {

/// \todo DESCRIBEME
/// \todo Move me elsewhere?
/*
unsigned length_virtual_index(const Derivation& d) {
	return (d.statement().span().size() - 1) * 2 + d.is_virtual_tag();
}
unsigned max_length_virtual_index(const Input& input) {
	return input.size() * 2 - 1;
}
*/
class span_virtual_index {
public:
	span_virtual_index(unsigned length) : _length(length) { }
	unsigned operator()(const Derivation& d) const {
		return ((d.statement().span().right() - 1) * _length + d.statement().span().left()) * 2 + d.is_virtual_tag();
	}
private:
	unsigned _length;
};
unsigned max_span_virtual_index(const Input& input) {
	return input.size() * input.size() * 2 - 1;
}

/// \todo WRITEME
Minimizer::Minimizer(unsigned sent_number, const Input& input, const GrammarTemplate& gram,
		boost::shared_ptr<StatementFactory> statement_factory,
		boost::shared_ptr<Interp> interp, boost::shared_ptr<graehl::stopwatch> timer)
	:
		_sent_number(sent_number),
		_popped(0),
		_chart(gram),
		//_agenda(length_virtual_index, max_length_virtual_index(input)),
		_agenda(span_virtual_index(input.size()), max_span_virtual_index(input)),
		_have_argmin(false), _argmin(ARGMIN),
		_statement_equivalence_pool(new StatementEquivalencePool),
		_grammar(new Grammar(input, gram, statement_factory, _statement_equivalence_pool)),
		_interp(interp), _timer(timer)
{
	/// Given the Input, create an initial Derivation for each word and add it to the Agenda.
	/// These Derivation%s have no antecedents, zero cost, and zero (highest) priority.
	unsigned idx = 0;
	vector<Derivation> to_add;
	for (sbmt::fat_sentence::iterator w = input.begin(); w != input.end(); w++, idx++) {
		/// \todo Why is gram argument necessary? (Pust)
		Statement e = grammar().statement_factory().create_edge(gram, w->label(), sbmt::span_t(idx, idx+1));
		Statement s(e);
		/// \todo Use negative-log probability as cost?
		//Derivation d(s, 0, 0);
		//Derivation d(s, 1, 1, statement_equivalence_pool());
		Derivation d(s, 1, statement_equivalence_pool());
		to_add.push_back(d);
	}

	this->agenda_add(to_add);
}

string Minimizer::sentstr() const {
	ostringstream o;
	o << "sent=" << _sent_number << " ";
	return o.str();
}

/// \todo Other stopping conditions
bool Minimizer::stopping_condition() const {
	// Always stop if the agenda is exhausted.
	if (_agenda.empty()) return true;

	// Stop if we have exceeded the maximum user+sys time or maximum wall time.
	// (only check every 1000 pops)
	/// \todo Make this 1000 a parameter
	/// \todo Throw for either clock overrun?
	if (_popped % 1000 == 0) {
		if (_timer->total_time(graehl::stopwatch::USER_TIME) + _timer->total_time(graehl::stopwatch::SYSTEM_TIME) > MAX_TOTAL_SEC) return true;
		/// \todo Put wall time in message
		common::throw_unless(_timer->total_time(graehl::stopwatch::WALL_TIME) <= MAX_WALL_SEC,
			"Exceed maximum wall time");
	}

//	return _have_argmin;		// Stop if we have some complete hypothesis
//	return _argmin.size() == _argmin.max_size();		// Stop if the n-best list is full.
	return _agenda.empty();		// Stop if the agenda is empty
}

void Minimizer::diagnostics(unsigned debuglevel) const {
	Debug::log(debuglevel) << sentstr() << _chart.size() << " derivations in Chart...\n";
	Debug::log(debuglevel) << sentstr() << _popped << " derivations popped from Agenda...\n";
	Debug::log(debuglevel) << sentstr() << _agenda.size() << " derivations remaining in Agenda...\n";
	Debug::log(debuglevel) << DerivationStatistics::str(sentstr());
	if (Debug::dump_volatile_diagnostics()) {
		Debug::log(debuglevel) << stats::resource_usage() << "\n";
		Debug::log(debuglevel) << *_timer << "\n";
	}
	Debug::log(debuglevel) << "\n";
}

/// We can prune any Derivation below this threshold.
Optional<Double> Minimizer::threshold() const {
	if (!_argmin.full()) return Optional<Double>();
	else return Optional<Double>(_argmin.bottom().score());
}

/// True if this Derivation beats the threshold.
bool Minimizer::beats_threshold(const Derivation& d) const {
	if (this->threshold().empty()) return true;
	if (d.score() >= this->threshold()()) return true;
	return false;
}

bool Minimizer::consider(const Derivation& d) const {
	if (!this->beats_threshold(d)) return false;
	if (!_chart.consider(d)) return false;
	return true;
}

/// See if we have a new k-best Derivation for _argmin
void Minimizer::update_argmin(const Derivation& d) {
	// Return if this Derivation is not of the goal Statement...
	if (!grammar().is_goal(d.statement())) return;

	Debug::log(4) << sentstr() << " " << (d.score() > _argmin.top().score()) << " old score= " << _argmin.top().score() << " vs. new score " << d.score() << "\n";

	if (!_argmin.can_push(d)) return;

	// We have a new best
	_have_argmin = true;
	Debug::log(3) << "\n";
	Debug::log(3) << sentstr() << "NEW N-BEST DERIVATION OF GOAL: " << d.str(grammar().g(), 9999) << "\n";
	bool ret = _argmin.push(d);
	assert(ret);
	/// \todo Print *entire* n-best list
	if (Debug::dump_volatile_diagnostics())
		Debug::log(3) << "(intermediate) " << *_timer << " " << _interp->nbest(_sent_number, sbmt::best_derivation(_argmin.top().equivalence()), 0) << flush;
	else
		Debug::log(3) << "(intermediate) " <<_interp->nbest(_sent_number, sbmt::best_derivation(_argmin.top().equivalence()), 0) << flush;
	Debug::log(4) << "N-best size = " << _argmin.size() << " (max size = " << _argmin.max_size() << ")\n";
	if (_argmin.full()) {
		// Prune the Chart and the Agenda according to the worst n-best hypothesis stored
		_chart.set_score_threshold(this->threshold()());
//		_agenda.prune(this->threshold()());
	}
	diagnostics(3);
}

void Minimizer::update_argmin(const vector<Derivation>& derived) {
	for (vector<Derivation>::const_iterator d = derived.begin();
			d != derived.end(); d++)
		this->update_argmin(*d);
}

void Minimizer::agenda_add(const vector<Derivation>& derived) {
	this->update_argmin(derived);
	for (vector<Derivation>::const_iterator d = derived.begin();
			d != derived.end(); d++)
		if (this->consider(*d)) {
			_agenda.add(*d);
			DerivationStatistics::add(*d, ADDED_TO_AGENDA);
		}
}

/// \todo Don't redo work if we've already called argmin
const NBest<Derivation>& Minimizer::argmin() {
	// if (_have_argmin) return _argmin;

	// while we have not achieved the stopping condition (e.g. passed
	// global threshold on # of statements to score, or Q is empty,
	// or we have exceed some time threshold)
	Derivation d;
	while (!this->stopping_condition()) {
		// Pop lowest priority Derivation from the Agenda.
		_agenda.pop(d);
		DerivationStatistics::add(d, POPPED_FROM_AGENDA);
		_popped++;
		if (_popped % 100000 == 0)
			diagnostics(3);
		else if (_popped % 10000 == 0)
			diagnostics(4);
		else if (_popped % 1000 == 0)
			diagnostics(5);

		// If this Derivation is of the goal Statement...
		if (grammar().is_goal(d.statement())) {
			// Try again to keep it in _argmin (This may redo work)
			this->update_argmin(d);

			// ...then just continue
			continue;
		}

		// If we should not explore this Derivation, just continue
		// on to the next Derivation in the Agenda
		if (!this->consider(d)) continue;

		// Find all assignments (Derivation%s) derivable from this Statement and the current Chart
		vector<Derivation> derived = grammar().derive(d, _chart);

		// Add the current Derivation to the Chart
		_chart.add(d);

		// See if we can update _argmin
		this->agenda_add(derived);
	}

	Debug::log(2) << "Minimizer::argmin() done\n";
	diagnostics(2);

	common::throw_unless(_have_argmin, "No complete structure found by Minimizer::argmin()");
	assert(_argmin.size() >= 1);
	return _argmin;
}

}	// namespace argmin
