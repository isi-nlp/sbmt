/*  $Id: Chart.cpp 1298 2006-10-03 04:43:58Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file Chart.cpp
 *  $LastChangedDate: 2006-10-02 21:43:58 -0700 (Mon, 02 Oct 2006) $
 *  $Revision: 1298 $
 */

#include "Chart.hpp"

#include "gpl/Debug.hpp"

/// \todo REMOVEME, make this a parameter
//#define MAX_VIRTUAL_EDGES_PER_SPAN	60
//#define MAX_NONVIRTUAL_EDGES_PER_SPAN	10
#define MAX_VIRTUAL_EDGES_PER_SPAN	600
#define MAX_NONVIRTUAL_EDGES_PER_SPAN	100

namespace argmin {

NBest<DerivationP>& Chart::cell(const Derivation& d) const {
	pair<Span, bool> key(d.statement().span(), d.is_virtual_tag());
	if (_cells.find(key) == _cells.end()) {
		if (d.is_virtual_tag())
			_cells.insert(make_pair(key, NBest<DerivationP>(MAX_VIRTUAL_EDGES_PER_SPAN)));
		else
			_cells.insert(make_pair(key, NBest<DerivationP>(MAX_NONVIRTUAL_EDGES_PER_SPAN)));
	}
	assert(_cells.find(key) != _cells.end());
	return _cells.find(key)->second;
}

void Chart::add(const Derivation& d) {
	// Make sure we would consider this derivation.
	assert(this->consider(d));

	Debug::log(4) << "Adding Derivation to Chart: " << d.str(g(), 9999) << "\n";

	/// Clobber old Derivation with newer, *higher-priority* Derivation.
	/// \todo Only allow us to clobber old Derivation%s with
	/// newer, *higher-priority* Derivation%s if a particular
	/// parameter is set.
	/// \todo Debug output about what just happened
	if (this->exists_but_worse(d)) {
		Debug::log(4) << "Removing Derivation from Chart: " << this->get(d.statement()).str(g(), 9999) << "\n";
		Debug::log(4) << "which is worse than Derivation to be added: " << d.str(g(), 9999) << "\n";
		this->remove(this->getp(d.statement()));
	}
	assert(!this->exists(d.statement()));

	DerivationP dp(new Derivation(d));
	_map.insert(make_pair(d.statement(), dp));

	SpanIndex l = d.statement().span().left();
	SpanIndex r = d.statement().span().right();
	assert(_left_boundary_map[l].find(dp) == _left_boundary_map[l].end());
	assert(_right_boundary_map[r].find(dp) == _right_boundary_map[r].end());
	_left_boundary_map[l].insert(dp);
	_right_boundary_map[r].insert(dp);

	NBest<DerivationP>& nbest = cell(d);

	// If we would be the bottom item of the cell, then remove it from the chart.
	if (nbest.full()) {
		assert(d > *nbest.bottom());
		this->remove(nbest.bottom());
	}

	// Make sure the cell is not full.
	assert(!nbest.full());

	// Insert into cell.
	bool ret = nbest.push(dp);
	assert(ret);
}

void Chart::remove(DerivationP d) {
	Debug::warning(__FILE__, __LINE__, "SLOW! Removing a Derivation from the Chart. This should only happen if the FOM is not admissable.");

	hash_map<Statement, DerivationP>::iterator i = _map.find(d->statement());
	assert(i->second == d);
	_map.erase(i);

	{
		dhash& l = _left_boundary_map[d->statement().span().left()]; 
		dhash::iterator i = l.find(d);
		assert(i != l.end());
		l.erase(i);
	}

	{
		dhash& r = _right_boundary_map[d->statement().span().right()]; 
		dhash::iterator i = r.find(d);
		assert(i != r.end());
		r.erase(i);
	}

	NBest<DerivationP>& nbest = cell(*d);
	nbest.erase(d);
}


bool Chart::exists(const Statement& s) const {
	return _map.find(s) != _map.end();
}

/// Does this Derivation's Statement exist in the Chart with a worse Priority ?
bool Chart::exists_but_worse(const Derivation& d) const {
	if (!this->exists(d.statement())) return false;
	const Derivation& dold = this->get(d.statement());
	return d.priority() > dold.priority();
}

const Derivation& Chart::get(const Statement& s) const {
	return *this->getp(s);
}
DerivationP Chart::getp(const Statement& s) const {
	assert(this->exists(s));
	return _map.find(s)->second;
}

/// Increase _score_threshold to new_threshold, and prune against the new threshold.
void Chart::set_score_threshold(const Double& new_threshold) {
	assert(_score_threshold.empty() || _score_threshold() < new_threshold);

	ostringstream o;
	o << "Chart::_score_threshold " << _score_threshold.str() << " -> " << new_threshold << ", ";

	_score_threshold = new_threshold;

	vector<DerivationP> to_prune;
	for (hash_map<Statement, DerivationP>::const_iterator i = _map.begin();
			i != _map.end(); i++)
		if (i->second->score() < new_threshold)
			to_prune.push_back(i->second);

	Debug::log(3) << o.str()
		<< "pruning " << 100.*to_prune.size()/_map.size() << "% (" << to_prune.size() << "/" << _map.size() << ") of derivations in chart\n";

	for (vector<DerivationP>::const_iterator i = to_prune.begin();
			i != to_prune.end(); i++)
		this->remove(*i);
}

bool Chart::consider(const Derivation& d) const {
	// Don't consider this Derivation if its score falls below the threshold
	if (!_score_threshold.empty() && d.score() < _score_threshold()) return false;

	// Don't consider this Derivation if it doesn't fit into its cell
	NBest<DerivationP>& nbest = cell(d);
	//if (!nbest.can_push(d)) return false;
	if (nbest.full() && *nbest.bottom() > d) return false;

	// If we have done work with this Statement before...
	if (this->exists(d.statement())) {
		const Derivation& dold = this->get(d.statement());

/*
		// As a sanity check, make sure the two Derivation%s aren't equivalent (besides their scores and priorities)
		assert(!d.derivation_equal_to(dold))
		*/

		// Sanity-check that the score and priority are either
		// both higher or both lower than that of the old Derivation
		assert((dold.score() >= d.score() && dold.priority() >= d.priority())
			|| (dold.score() <= d.score() && dold.priority() <= d.priority()));

		// ...make sure the score was higher last time
		/// \todo score AND priority, or just score?
		/*
		cout << __FILE__ << ":" << __LINE__ << " " << "Edge old: " << dold.str(_grammar.g(), 0) << "\n";
		cout << __FILE__ << ":" << __LINE__ << " " << "Edge new: " << d.str(_grammar.g(), 0) << "\n";
		cout << __FILE__ << ":" << __LINE__ << " " << "Skipping " << dold.score() << " " << d.score() << "\n";
		cout << __FILE__ << ":" << __LINE__ << " " << "Skipping " << dold.priority()() << " " << d.priority()() << "\n";
		assert(dold.score() >= d.score() && dold.priority() >= d.priority());
		*/

		/// Clobber old Derivation with newer, *higher-priority* Derivation.
		/// \todo Only allow us to clobber old Derivation%s with
		/// newer, *higher-priority* Derivation%s if a particular
		/// parameter is set.
		/// \todo Debug output about what just happened
		if (this->exists_but_worse(d)) return true;

		/// Skip processing this worse Derivation of that Statement
		return false;
	}
	return true;
}

}	// namespace argmin
