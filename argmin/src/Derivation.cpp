/*  $Id: Derivation.cpp 1298 2006-10-03 04:43:58Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file Derivation.cpp
 *  $LastChangedDate: 2006-10-02 21:43:58 -0700 (Mon, 02 Oct 2006) $
 *  $Revision: 1298 $
 */

#include "Derivation.hpp"
#include "Grammar.hpp"

#include <vector>

#ifndef DOXYGEN
using namespace std;
#endif

namespace argmin {

string Derivation::str(const GrammarTemplate& g, unsigned maxdepth) const {
	ostringstream o;
	this->statement().print_state(o, g, maxdepth);
	o << " at span " << this->span() << " with score " << this->score() << " and priority " << this->priority()();
	return o.str();
}

DerivationStatistics::Array DerivationStatistics::m_counts(boost::extents[DUMMY_LAST_CAT][MAX_LENGTH][2]);
vector<unsigned> DerivationStatistics::m_tot(DUMMY_LAST_CAT, 0);

void DerivationStatistics::clear() {
	Array::index cat, size, virt;
	for(cat = 0; cat < DUMMY_LAST_CAT; cat++) {
		m_tot[cat] = 0;
		for(size = 0; size < MAX_LENGTH; size++)
			for(virt = 0; virt < 2; virt++)
				m_counts[cat][size][virt] = 0;
	}
}

void DerivationStatistics::add(const Derivation& d, DStatCat category) {
	switch (category) {
		case CREATED:
		case ADDED_TO_AGENDA:
		case POPPED_FROM_AGENDA:
				break;
		default:	assert(0);
	}
	assert(d.span().size() < MAX_LENGTH);

	m_counts[category][d.span().size()][d.is_virtual_tag()]++;
	m_tot[category]++;
}

std::string DerivationStatistics::str(std::string prefix) {
	Array::index cat, size;

	ostringstream o;
	o.precision(5);
	for(cat = 0; cat < DUMMY_LAST_CAT; cat++) {
		switch (cat) {
			case CREATED:			o << prefix << "Derivations created: "; break;
			case ADDED_TO_AGENDA:		o << prefix << "Derivations added to agenda: "; break;
			case POPPED_FROM_AGENDA:	o << prefix << "Derivations popped from agenda: "; break;
			default:			assert(0);
		}
		o << m_tot[cat] << " total\n";

		for(size = 0; size < MAX_LENGTH; size++) {
			if (m_counts[cat][size][0] || m_counts[cat][size][1]) {
				o << "\tsize " << setw(4) << size;
				o << " not virtual: " << m_counts[cat][size][0] * 100. / m_tot[cat] << "%,";
				o << " virtual: " << m_counts[cat][size][1] * 100. / m_tot[cat] << "%\n";
			}
		}
	}

	return o.str();
}

}	// namespace argmin

