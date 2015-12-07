/*  $Id: Grammar.hpp 1295 2006-10-02 01:51:27Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file Grammar.hpp
 *  $LastChangedDate: 2006-10-01 18:51:27 -0700 (Sun, 01 Oct 2006) $
 *  $Revision: 1295 $
 */
/*!
 *  \class argmin::Grammar
 *  \brief AKA rules
 *
 *  The implementation of the grammar.
 *  Grammatical rules determine how to derive new Statement%s (Derivation%s ?) from existing ones.
 * 
 *  \warning We assume all the rules in the Grammar are unary or binary
 *  (i.e. have at most two variables).
 *
 *  \internal An important design question is how to implement abstraction
 *  hierarchies (different grammar levels).
 *
 */

#ifndef __ARGMIN_GRAMMAR_HPP__
#define __ARGMIN_GRAMMAR_HPP__

#include <cassert>		///< REMOVEME
#include <vector>
#ifndef DOXYGEN
using namespace std;
#endif

#include "types.hpp"		///< REMOVEME

namespace argmin {

class Derivation;
class Chart;

class Grammar {
public:
	/// \todo Are we initializing _target_span correctly, or are we off-by-one? (Pust)
	/// \todo Actually store the GrammarTemplate here, not just a pointer!
	Grammar(const Input& input, const GrammarTemplate& g,
			boost::shared_ptr<StatementFactory> statement_factory,
			boost::shared_ptr<StatementEquivalencePool> statement_equivalence_pool)
		: _g(&g), _statement_factory(statement_factory),
			_statement_equivalence_pool(statement_equivalence_pool),
			_target_span(0, input.size())
		{ assert(_g); }

	/// Find all assignments (Derivation%s) derivable from this Statement and the current Chart.
	/// \param d WRITEME
	/// \param chart WRITEME
	/// \return WRITEME
	/// \todo MAKE sure to check that the Statement is not derived in Chart with a higher cost than previously.
	/// \todo Or just assert that the Statement in Derivation d is not already in the Chart.
	/// \todo We can make this function faster by short-circuiting Derivation%s that we
	/// know will be pruned.
	vector<Derivation> derive(const Derivation& d, const Chart& chart) const;

	/// \todo REMOVEME (Pust)
	StatementFactory& statement_factory() const { return *_statement_factory; }

	/// Is this Statement a goal statement?
	/// In the SBMT world, this means that the root token is the toplevel token
	/// \todo Make this method a member of Statement, independent of the Grammar
	bool is_goal(const Statement& s) const;

	/// Is this the target span?
	/// If it is, we can fire top-level rules.
	bool is_target_span(const Span& s) const { return s == _target_span; }

	/// \todo REMOVEME
	const GrammarTemplate& g() const { return *_g; }

private:
	/// \todo REMOVEME (Pust)
	StatementEquivalencePool& statement_equivalence_pool() const { return *_statement_equivalence_pool; }

	/// Get all binary RHSs based on which Derivation%s are adjacent to d.
	void get_rhs(const Derivation& d, const Chart& chart,
			vector<pair<const Derivation*, const Derivation*> >& rhs) const;

	/// Find all Derivation%s derivable from d using the given set of applicable unary rules.
	/// \post New Derivation%s are appended to derive.
	/// \todo Sanity-check for scores being non-increasing.
	void derive_unary(const Derivation& d, const Chart& chart,
			const GrammarTemplate::rule_range& rules,
			vector<Derivation>& derived) const;

	/// Find all Derivation%s derivable from rhs using the given set of applicable binary rules.
	/// \todo Don't post-prune binary rules, just assert that they are all applicable!
	/// \post New Derivation%s are appended to derive.
	/// \todo Sanity-check for scores being non-increasing.
	void derive_binary(const pair<const Derivation*, const Derivation*>& rhs, const Chart& chart,
			const GrammarTemplate::rule_range2& rules,
			vector<Derivation>& derived) const;

	/// \todo Actually store the GrammarTemplate here, not just a pointer!
	/// \todo Or, at least use a boost smart pointer so we know if the object is invalid!
	const GrammarTemplate* _g;

	/// \todo Do we have to use an edge factory? (Pust)
	/// And why does this have to be mutable?
	mutable boost::shared_ptr<StatementFactory> _statement_factory;

	/// \todo Do we have to use an edge equivalence pool? (Pust)
	/// And why does this have to be mutable?
	mutable boost::shared_ptr<StatementEquivalencePool> _statement_equivalence_pool;

	Span _target_span;
};	// class Grammar

}	// namespace argmin

#endif	// __ARGMIN_GRAMMAR_HPP__
