/*  $Id: Chart.hpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file Chart.hpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */
/*!
 *  \class argmin::Chart
 *  \brief Live Derivation%s, which participate in deriving future Derivation%s.
 *
 *  Chart keeps track of each Statement along with the minimum cost known for that Statement.
 *
 *  \todo Allow at least one GLUE edge per cell (regardless of virtual max and non-virtual max),
 *  to ensure we can find a complete hypothesis.
 *  \todo Optimization: Store boost::smart_ptr<Derivation>, not Derivation
 *
 */

#ifndef __ARGMIN_CHART_HPP__
#define __ARGMIN_CHART_HPP__

#include "Derivation.hpp"
#include "gpl/NBest.hpp"
#include "gpl/Optional.hpp"

#include "types.hpp"	///< REMOVEME

#include <ext/hash_map>
#include <ext/hash_set>
#ifndef DOXYGEN
using namespace std;
using namespace __gnu_cxx;
#endif

namespace argmin {

class cell_hash {
public: std::size_t operator() (const pair<Span, bool>& p) const {
		std::size_t retval = 0;
		boost::hash_combine(retval, p.first);
		boost::hash_combine(retval, p.second);
		return retval;
	}
};

class Chart {
public:
	typedef hash_set<DerivationP, hash_DerivationP_object, equal_to_DerivationP_object > dhash;

	/// \todo Don't pass in the GrammarTemplate (Pust)
	/// \todo Or, at the very least, store it as a shared_ptr, not a raw pointer
	Chart(const GrammarTemplate& gram) : _g(&gram) { }

	/// Add the Derivation to the Chart.
	/// \param d Derivation to be added.
	/// \note Currently, we will clobber old Derivation%s with newer,
	/// *higher-priority* Derivation%s. (Rather than throwing an assertion)
	/// \todo Only allow us to clobber old Derivation%s with
	/// newer, *higher-priority* Derivation%s if a particular
	/// parameter is set.
	/// \todo Pass in a DerivationP, don't create a duplicate Derivation object.
	void add(const Derivation& d);

	/// Consider exploring some Derivation, or determine that it would be wasted effort.
	/// Currently, we determine that exploring a derivation would be wasted effort if
	/// the Statement already exists in the Chart.
	/// \param d The Derivation to be considered for inference.
	/// \return True iff we should explore this Derivation.
	/// \todo What about if the new Derivation is better than the old one?
	/// \todo Only allow us to clobber old Derivation%s with
	/// newer, *higher-priority* Derivation%s if a particular
	/// parameter is set.
	/// \todo More debug output about why a Derivation is rejected.
	/// \bug We may incorrectly implement pruning, if the FOM is not admissable!
	/// \note This function is monotonic, insofar as if a Derivation is not to be considered,
	/// then it will *never* be considered. This allows us to optimize by pruning Derivation%s
	/// as early as popular.
	/// \todo Any way to sanity check that this function is monotic?
	bool consider(const Derivation& d) const;

	/// Find all Derivation%s in the Chart that have SpanIndex left as their left-boundary.
	const dhash& left_boundary(const SpanIndex& left) const { return _left_boundary_map[left]; }
	/// Find all Derivation%s in the Chart that have SpanIndex right as their right-boundary.
	const dhash& right_boundary(const SpanIndex& right) const { return _right_boundary_map[right]; }

	/// Increase _score_threshold to new_threshold, and prune against the new threshold.
	void set_score_threshold(const Double& new_threshold);

	unsigned size() const { return _map.size(); }

private:
	/// Does this Statement exist in the Chart?
	bool exists(const Statement& s) const;

	/// Does this Derivation's Statement exist in the Chart with a worse Priority ?
	bool exists_but_worse(const Derivation& d) const;

	/// Get the Derivation associated with a particular Statement.
	/// \pre ::exists(s)
	const Derivation& get(const Statement& s) const;
	DerivationP getp(const Statement& s) const;

	/// Completely remove a particular Derivation from the Chart.
	/// \todo Disallow this function from being called if we're working with
	/// an admissable heuristic.
	void remove(DerivationP d);

	/// Prune all Derivation%s with score below this threshold.
	Optional<Double> _score_threshold;

	/// The best known Derivation for a given Statement.
	/// \todo Best means highest score or highest priority?
	/// \todo Make this a multi-map, such that Statements can have more than one Derivation?
	hash_map<Statement, DerivationP> _map;
	//hash_multimap<Statement, Derivation> _map;

	/// Map from SpanIndex to all Derivation%s in the Chart with this SpanIndex as its
	/// left-boundary.
	/// \todo Use a table type, e.g. vector or google table
	/// \todo REMOVE mutable qualifier
	mutable hash_map<SpanIndex, dhash> _left_boundary_map;

	/// Map from SpanIndex to all Derivation%s in the Chart with this SpanIndex as its
	/// right-boundary.
	/// \todo Use a table type, e.g. vector or google table
	/// \todo REMOVE mutable qualifier
	mutable hash_map<SpanIndex, dhash> _right_boundary_map;

	NBest<DerivationP>& cell(const Derivation& d) const;
	/// For each Span and is_virtual, the Derivation%s in the Chart with that information.
	/// \todo Use a table type, e.g. vector or google table
	/// \todo REMOVE mutable qualifier
	mutable hash_map<pair<Span, bool>, NBest<DerivationP>, cell_hash > _cells;

	/// \todo REMOVEME
	const GrammarTemplate& g() const { return *_g; }
	mutable const GrammarTemplate* _g;
};	// class Chart

}	// namespace argmin

#endif	// __ARGMIN_CHART_HPP__
