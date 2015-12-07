/*  $Id: types.hpp 1314 2006-10-07 01:24:57Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file types.hpp
 *  $LastChangedDate: 2006-10-06 18:24:57 -0700 (Fri, 06 Oct 2006) $
 *  $Revision: 1314 $
 *
 *  \brief SBMT types used by the system.
 *
 *  \todo RENAMEME to sbmt_types.hpp
 *  \todo REMOVEME! Make our interface with SBMT less brittle, and/or use templates.
 *
 */

#ifndef __ARGMIN_TYPES_HPP__
#define __ARGMIN_TYPES_HPP__

/// \todo Don't use this!!!
#define USING_NGRAM_ORDER	3


#define MAX_AGENDA_SIZE 5000000         /// \todo Make this a parameter
//#define MAX_AGENDA_SIZE 500000         /// \todo Make this a parameter



/// Define NO_LM to override any ngram_info and decode without the language model
//#define	NO_LM

#include "../sbmt_decoder/include/sbmt/edge/edge.hpp"		// TODO: Fix these include location (t/o code)
#include "../sbmt_decoder/include/sbmt/edge/edge_equivalence.hpp"
#include "../sbmt_decoder/include/sbmt/edge/null_info.hpp"
#include "../sbmt_decoder/include/sbmt/edge/ngram_info.hpp"
#include "../sbmt_decoder/include/sbmt/sentence.hpp"
#include "../sbmt_decoder/include/sbmt/grammar/grammar_in_memory.hpp"
#include "../sbmt_decoder/include/sbmt/logmath/lognumber.hpp"
#include "../sbmt_decoder/include/sbmt/forest/derivation.hpp"

namespace argmin {

///  Extra-information stored in a Statement, in addition to the
///  information used by the translation-model rules (i.e. besides
///  the root constituent label)
///  \todo WRITEME
#ifdef NO_LM
	typedef sbmt::null_info				ExtraInformation;
#else /* !NO_LM */
	typedef sbmt::ngram_info<USING_NGRAM_ORDER>	ExtraInformation;
#endif /* NO_LM */

///  \brief A statement about (constraint over) some Input.
///  \note AKA ``Item'' in the parsing literature, or ``Edge''
///
///  The most important aspect of the statement is that it contain all
///  the information used in deriving new Statement%s. It also should
///  not contain any extra information, otherwise we may lose efficiency
///  when trying to determine if two Statement tokens are the same type.
///
///  For example, ``there is an NP from [2, 5] with these two terminal words at each boundary''.
///
///  \note An sbmt::edge contains not only Statement information, but
///  also Derivation information (antecedents, score, &tc.).
///  However, edge equality is only based upon Statement information
///  (specifically, the TM-info and the LM-info).
///
///  \todo Make a genuine Statement class, AKA EdgeType, which doesn't contain derivation information (Pust)
///  \todo Allow different sorts of edge info. (Ask Michael Pust)
///  \note Essentially, a wrapper around SBMT edges, abstracting away stuff we don't need.
///  \todo Make an output function for Statement, which includes the Span
typedef sbmt::edge<ExtraInformation>	Statement;

typedef sbmt::info_factory<ExtraInformation>	InfoFactory;

typedef sbmt::edge_factory<Statement>	StatementFactory;
//typedef sbmt::edge_factory<Edge>	EdgeFactory;


/// Edge equivalences store Statement%s, i.e. edges with the same
/// type. However, they are dominated by the one Statement with the
/// highest score.
/// It's necessary in sbmt, so we kludge around it.
/// \todo Rename these types, to explain better their contract and their uses
/// \todo Ultimately, REMOVE THIS (Pust)
typedef sbmt::edge_equivalence_pool<Statement>	StatementEquivalencePool;
typedef sbmt::edge_equivalence<Statement>	StatementEquivalence;

typedef sbmt::fat_sentence		Input;

/// \todo RENAMEME?
typedef sbmt::grammar_in_mem		GrammarTemplate;

typedef sbmt::indexed_binary_rule	Rule;

/// \todo RENAMEME?
typedef sbmt::span_t			Span;
typedef sbmt::span_index_t		SpanIndex;

typedef sbmt::score_combiner		ScoreCombiner;

typedef sbmt::derivation_interpreter<Statement,GrammarTemplate>	Interp;

typedef boost::shared_ptr<sbmt::LWNgramLM>	LM;

#include <ostream>
/// \todo Merge this into span_t code (Pust)
inline std::ostream& operator<<(std::ostream& o, const Span& s) {
	o << "[" << s.left() << "," << s.right() << "]";
	return o;
}

/// \todo Move this to common?
typedef sbmt::logmath::basic_lognumber<sbmt::logmath::exp_n<1>, float>	Double;

}	// namespace argmin

#ifndef DOXYGEN
namespace __gnu_cxx {
	//! Template specialization of STL's hash object.
	template<> class hash<argmin::Statement> {
		public: size_t operator() (const argmin::Statement& s) const {
			return s.hash_value();
		}
	};
	template<> class hash<argmin::Span> {
		public: size_t operator() (const argmin::Span& s) const {
			return sbmt::hash_value(s);
		}
	};
}
#endif /* DOXYGEN */

#endif	// __ARGMIN_TYPES_HPP__
