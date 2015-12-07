/*  $Id: Derivation.hpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file Derivation.hpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 */
/*!
 *  \class argmin::Derivation
 *  \brief A concrete derivation of a Statement.
 *
 *  Essentially, a Statement token.
 *  This Statement includes a score and a priority, as well as its antecedent Derivation%s.
 *
 *  \todo Derivation%s in a Chart do not need a priority, only a score?
 *
 */

#ifndef __ARGMIN_DERIVATION_HPP__
#define __ARGMIN_DERIVATION_HPP__

#include "gpl/Tiebreak.hpp"
#include "types.hpp"

#include <string>
#include <vector>
#include <boost/operators.hpp>
#include <boost/multi_array.hpp>
#include <boost/shared_ptr.hpp>

namespace argmin {

class Grammar;

class Derivation : public Statement,
	boost::totally_ordered< Derivation >
{
public:
	/// \todo REMOVEME, or add assertions such that this empty item can't be used.
	Derivation() : _priority(0) { }

	Derivation(const Statement& statement, const double& priority, StatementEquivalencePool& pool)
		: Statement(statement), _priority(priority),
			_equivalence(new StatementEquivalence(pool.create(statement)))
	{
		assert(equivalence().representative() == *this);
	}
	Derivation(const Statement& statement, const Double& priority, StatementEquivalencePool& pool)
		: Statement(statement), _priority(priority),
			_equivalence(new StatementEquivalence(pool.create(statement)))
	{
		assert(equivalence().representative() == *this);
	}

	/// Is this a virtual Derivation?
	/// i.e. is the Statement's root type virtual_tag_token or not?
	/// \todo Move this to Statement
	/// \todo The default case should throw an assertion, or at least make a warning!
	bool is_virtual_tag() const {
		switch (this->statement().root().type()) {
			case sbmt::tag_token: return false;
			case sbmt::virtual_tag_token: return true;
			//default: assert(0);
			default: return false;
		}
	}

//	const Statement& statement() const { return _statement; }
	const Statement& statement() const { return *this; }
	Double score() const { return statement().score(); }
	const Tiebreak<Double>& priority() const { return _priority; }

	/// Comparisons are based upon priority.
	/// \todo Does high priority mean first in the ordering or last?
	bool operator<(const Derivation& d) const { return this->priority() < d.priority(); }
	bool operator==(const Derivation& d) const { return this->priority() == d.priority(); }

/*
	/// Compare two Derivation%s for Derivation equality.
	/// Derivation equality includes Statement information equality and antecedent equality,
	/// but not score or priority equality.
	/// \todo WRITEME
	bool derivation_equal_to(const Derivation& d) const;
*/

	/// \bug We only check for Statement equality, not entire Derivation equality. i.e. we don't compare antecedents.
	/// \todo Can we do true equality testing?
	bool object_equal_to(const Derivation& d) const {
		return this->statement() == d.statement() && this->score() == d.score() && this->priority() == d.priority();
	}

	/// \todo Describe maxdepth
	/// \todo Allow maxdepth = -1 to go arbitrarily deep?
	/// \todo Remove the dependence on Grammar parameter (Pust)
	/// \todo Include other edge information (LM info, i.e. boundary words) (Pust)
	std::string str(const GrammarTemplate& g, unsigned maxdepth=0) const;

	/// \todo REMOVEME (Pust)
	const StatementEquivalence& equivalence() const { return *_equivalence; }

private:
//	/// The Statement (item type) derived by this object.
//	Statement _statement;

	/// The agenda priority of this derivation.
	/// \todo WRITEME more documentation on how this can be an FOM
	Tiebreak<Double> _priority;

	/// \todo REMOVEME? (Pust)
	boost::shared_ptr<StatementEquivalence> _equivalence;

};	// class Derivation

class DerivationP : public boost::shared_ptr<Derivation const>,
	boost::totally_ordered< DerivationP >,
	boost::totally_ordered< DerivationP, Derivation >
{
public:
	DerivationP(const Derivation* d) : boost::shared_ptr<Derivation const>(d) { }
	bool operator<(DerivationP d) const { return **this < *d; }
	bool operator==(DerivationP d) const { return **this == *d; }
	bool operator==(const Derivation& d) const { return **this == d; }
};

class hash_DerivationP_object {
	/// \todo Include score() and priority() hash?
	/// \todo Make a hash value function for basic_lognumber
	public: std::size_t operator() (const DerivationP& d) const {
		std::size_t retval = 0;
		boost::hash_combine(retval, d->statement().hash_value());
		boost::hash_combine(retval, d->score().log());
		boost::hash_combine(retval, d->priority()().log());
		return retval;
	}
};
class equal_to_DerivationP_object {
	public: std::size_t operator() (const DerivationP& d1, const DerivationP& d2) const {
		return d1->object_equal_to(*d2);
	}
};

/// DerivationStatistics category
typedef enum { CREATED, ADDED_TO_AGENDA, POPPED_FROM_AGENDA, DUMMY_LAST_CAT } DStatCat;

/// Statistics about Derivation%s.
/// \todo Make this a namespace.
class DerivationStatistics {
public:
	/// Initialize values stored.
	static void clear();
	/// Maintain statistics about some Derivation.
	static void add(const Derivation& d, DStatCat category);
	/// Return a string summarizing statistics about Derivation%s.
	static std::string str(std::string prefix="");
private:
	/// \todo Put this limit in a more global location.
	/// \todo Don't use enum?
	enum {MAX_LENGTH = 1000};

	typedef boost::multi_array<unsigned, 3> Array;
	/// \todo WRITEME
	/// Dimensions are: DStatCat x Length x IsVirtual (DUMMY_LAST_CAT x max_length x 2)
	static Array m_counts;

	/// Total number of Derivations, one for each DStatCat
	static std::vector<unsigned> m_tot;
};

}	// namespace argmin

#endif	// __ARGMIN_DERIVATION_HPP__
