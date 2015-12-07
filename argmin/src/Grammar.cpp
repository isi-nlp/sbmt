
/*  $Id: Grammar.cpp 1803 2007-11-14 19:33:21Z graehl $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file Grammar.cpp
 *  $LastChangedDate: 2007-11-14 11:33:21 -0800 (Wed, 14 Nov 2007) $
 *  $Revision: 1803 $
 */

#include "Grammar.hpp"
#include "Derivation.hpp"
#include "Chart.hpp"
#include "gpl/Debug.hpp"

#include <cmath>

#include "../sbmt_decoder/include/sbmt/forest/derivation.hpp"		// REMOVEME
#include "../sbmt_decoder/include/sbmt/grammar/grammar.hpp"		// REMOVEME

namespace argmin {

bool Grammar::is_goal(const Statement& s) const {
	bool ret = (s.root() == _g->get_token_factory().toplevel_tag());
	if (ret) assert(this->is_target_span(s.span()));
	return ret;
}

void Grammar::derive_unary(const Derivation& d, const Chart& chart,
		const GrammarTemplate::rule_range& rules, vector<Derivation>& derived) const {
	for (GrammarTemplate::rule_iterator r = rules.begin();
			r != rules.end(); r++) {
		const Rule& rule = (*r)->rule;

		assert(rule.rhs_size() == 1);
		assert(rule.rhs(0) == d.root());

		//cout << __FILE__ << ":" << __LINE__ << " " << "Rule score: " << _g->rule_score(*r) << " " << _g->rule_score_estimate(*r) << "\n";

		/// \todo REMOVE THIS KLUDGE. We shouldn't need an edge_equivalence (Pust)
		Statement s = statement_factory().create_edge(*_g, *r, d.equivalence());

		/// \todo Is this correct? Use score or inside_score? (Pust)
		//Derivation d(s, s.score(), s.score());
		Double p = s.score();
		p ^= 1./(s.span().size());
//		Derivation d(s, p);
		Derivation dnew(s, s.score(), statement_equivalence_pool());

		if (dnew.score() > d.score())
			Debug::warning(__FILE__, __LINE__, "Unary rule consequent scores may increase over the antecedent scores!");
		/// \todo UNCOMMENTME! Sanity-check for scores being non-increasing.
/*
		// The following should hold assuming the score is non-increasing.
		assert(dnew.score() <= d.score());
*/

		DerivationStatistics::add(dnew, CREATED);

		/// If d wouldn't be pruned, then keep it.
		if (chart.consider(dnew))
			derived.push_back(dnew);

//		BOOST_CHECKPOINT("before create unary edge");
//		edge_type e = ef.create_edge(gram, *rule_itr, eq);
//		BOOST_CHECKPOINT("after create unary edge");
//		BOOST_CHECK(e.span() == eq.representative().span());
//		chart.insert_edge(ef,e);
	}
}

void Grammar::derive_binary(const pair<const Derivation*, const Derivation*>& rhs, const Chart& chart,
		const GrammarTemplate::rule_range2& rules, vector<Derivation>& derived) const {
	for (GrammarTemplate::rule_iterator2 i = rules.begin();
			i != rules.end(); i++) {
		const Rule& rule = (*i)->rule;

		assert(rule.rhs_size() == 2);
		assert(rule.rhs(0) == rhs.first->statement().root());
		assert(rule.rhs(1) == rhs.second->statement().root());

		/// \todo Output the applicable rule using the actual strings, not the indices
		//sbmt::print(cout, *i, *_g);
                //cout << __FILE__ << ":" << __LINE__ << " " << "\n";

		/// \todo REMOVE THIS KLUDGE. We shouldn't need an edge_equivalence (Pust)
		Statement s = statement_factory().create_edge(*_g, *i, rhs.first->equivalence(), rhs.second->equivalence());

		/// \todo Is this correct? Use score or inside_score? (Pust)
		//Derivation d(s, s.score(), s.score());
		Double p = s.score();
		p ^= 1./(s.span().size());
//		Derivation d(s, p);
		Derivation d(s, s.score(), statement_equivalence_pool());

		if (d.score() > rhs.first->score() || d.score() > rhs.second->score())
			Debug::warning(__FILE__, __LINE__, "Binary rule consequent scores may increase over the antecedent scores!");
		/// \todo UNCOMMENTME! Sanity-check for scores being non-increasing.
/*
		// The following should hold assuming the score is non-increasing.
		assert(d.score() <= rhs.first->score());
		assert(d.score() <= rhs.second->score());
*/

		DerivationStatistics::add(d, CREATED);

		/// If d wouldn't be pruned, then keep it.
		if (chart.consider(d))
			derived.push_back(d);
	}
}


void Grammar::get_rhs(const Derivation& d, const Chart& chart,
		vector<pair<const Derivation*, const Derivation*> >& rhs) const
{
	// Find all binary derivations with d as the *first* item on the RHS
	{
		// Find all Derivation%s in the Chart that are right-adjacent to d, i.e. that have d.span().right() as their left-boundary.
		const Chart::dhash& right_rhs = chart.left_boundary(d.span().right());
		rhs.reserve(right_rhs.size());

		/// ...and add (d, new Derivation) as a potential RHS
		for (Chart::dhash::const_iterator i = right_rhs.begin();
				i != right_rhs.end(); i++)
			rhs.push_back(make_pair(&d, &(**i)));
	}

	// Find all binary derivations with d as the *second* item on the RHS
	{
		// Find all Derivation%s in the Chart that are left-adjacent to d, i.e. that have d.span().left() as their right-boundary.
		const Chart::dhash& left_rhs = chart.right_boundary(d.span().left());
		rhs.reserve(rhs.size() + left_rhs.size());

		/// ...and add (new Derivation, d) as a potential RHS
		for (Chart::dhash::const_iterator i = left_rhs.begin();
				i != left_rhs.end(); i++)
			rhs.push_back(make_pair(&(**i), &d));
	}
}


/// \todo There are faster ways to write this!
/// \todo Collapse binary RHSes with same labels, such that we don't
/// need to do rule-lookup any more than is necessary.
vector<Derivation> Grammar::derive(const Derivation& d, const Chart& chart) const {
	vector<Derivation> derived;

	// Debug output. Make more verbose.
	if (Debug::will_log(4))
		Debug::log(4) << "Deriving using edge: " << d.str(*_g, 0) << "\n";
	if (Debug::will_log(5))
		Debug::log(5) << "Deriving using edge: " << d.str(*_g, 9999) << "\n";

	/// \todo Find all unary derivations
	{
		GrammarTemplate::rule_range rules = _g->unary_rules(d.root()); 
		this->derive_unary(d, chart, rules, derived);

		// If the Derivation is over the entire target span,
		// try applying top-level rules
		if (this->is_target_span(d.span())) {
			rules = _g->toplevel_unary_rules(d.root()); 
			this->derive_unary(d, chart, rules, derived);
		}
	}

	// For binary derivations, store the possible RHS (antecedents)
	vector<pair<const Derivation*, const Derivation*> > rhs;
	this->get_rhs(d, chart, rhs);

	// Try deriving, using each binary-rule RHS
	/// \todo Collapse RHSes with same labels, such that we don't
	/// need to do rule-lookup any more than is necessary.
	for (vector<pair<const Derivation*, const Derivation*> >::const_iterator r = rhs.begin();
			r != rhs.end(); r++) {
		const Statement& rhs1 = r->first->statement(), rhs2 = r->second->statement();
		GrammarTemplate::rule_range2 rules = _g->binary_rules(rhs1.root(), rhs2.root());
		this->derive_binary(*r, chart, rules, derived);

		const Span combined_span = sbmt::combine(rhs1.span(), rhs2.span());
		if (this->is_target_span(combined_span)) {
			rules = _g->toplevel_binary_rules(rhs1.root(), rhs2.root());
			this->derive_binary(*r, chart, rules, derived);
		}
	}

	// Output all new Derivation%s
	/// \todo Include the other antecedent Derivation and the rule
	/// used to derive this new Statement
	for(vector<Derivation>::const_iterator i = derived.begin();
			i != derived.end(); i++) {
		if (Debug::will_log(6))
			Debug::log(6) << "Derived new edge: " << i->str(*_g, 9999) << "\n";

	}

	return derived;
}

}	// namespace argmin
