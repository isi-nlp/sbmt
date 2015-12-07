#ifndef SBMT__FOREST__DERIVATION_HPP
#define SBMT__FOREST__DERIVATION_HPP

#define SBTM_NBEST_VERIFY_EPSILON 6e-4
// relative difference allowed in cost
#define SBTM_NBEST_VERIFY_MIN_SCALE 10.
// totalcost is usually around this

//TODO: save binary rule pointers in derivation and then show binary derivation tree w/ F-E alignments (using lmstring)

//#define JENS_SGMLIZE_LMCOST
//#define GRAEHL__DEBUG_PRINT
//#define GRAEHL__DBG_ASSOC
#include <sbmt/forest/logging.hpp>
#include <sbmt/forest/xrs_forest.hpp>
#include <sbmt/edge/edge_equivalence.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/grammar/alignment.hpp>
#include <sbmt/search/concrete_edge_factory.hpp>
#include <sbmt/printer.hpp>
//#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/tree.hpp>
#include <graehl/shared/assoc_container.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/char_predicate.hpp>
#include <graehl/shared/quote.hpp>
#include <graehl/shared/epsilon.hpp>
#include <graehl/shared/string_match.hpp>
#include <boost/ref.hpp>
#include <boost/function.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <iostream>
#include <vector>
#include <memory>
//#include <sbmt/forest/derivation_tree.hpp>

//#include <sbmt/multipass/cell_heuristic.hpp>

namespace sbmt {

/**
   \defgroup Derivations Derivations and Forests
   For now, a derivation tree is the same type as an edge equivalence (list of edges), aka forest.  Using the best scoring edge for each equivalence, we have a derivation tree.  Since creating non-best derivations may be more expensive because of all the singleton equivalences we're forced to create, we intend to later use a more compact data structure for derivations.  Thus, derivation_traits and associated free functions:
 */

///\{

template <class O>
inline void print_nbest_header(O &o,unsigned sent,unsigned nbest_rank=0,unsigned pass=0)
{
    o<<"NBEST sent="<<sent<<" nbest="<<nbest_rank<<" ";
    if (pass)
        o << "pass="<<pass<<' ';
}

inline std::string nbest_header(unsigned sent,unsigned nbest_rank=0,unsigned pass=0)
{
    std::ostringstream o;
    print_nbest_header(o,sent,nbest_rank,pass);
    return o.str();
}


template <class EdgeT>
class derivation_traits
{
 public:
    typedef EdgeT                                            edge_type;
    typedef edge_equivalence<edge_type>                      edge_equiv_type;
    typedef edge_equiv_type forest_type;
     /// A forest (equivalence) is interpereted as a derivation tree by looking at the
     /// representative edge of each equivalence. nonbest derivation trees
     /// (e.g. from kbest) use at their non-best points singleton equivalences
     /// (allocated from the same edgefactory/ecs).  best derivation subtrees
     /// can just use their as-built-from-chart equiv
    typedef edge_equiv_type derivation_type;
    /// a different derivation type might be declared in the future; for now a
    /// forest is interpreted as a derivation by replacing its children
    /// subforests as their (representative) derivation
    //    static derivation_type best_derivation(forest_type f) const { return f.representative(); }

    /// the following functions: edge, derivation, equivalence, best_derivation, manage the relationship between edge, equiv, forest, and derivation.
};

/// edge at the root of a deriv
template<class E>
inline
typename derivation_traits<E>::edge_type
//E
& deriv_edge(
//    typename derivation_traits<E>::derivation_type
    edge_equivalence<E>
                    const &deriv)
{
    return deriv.representative();
}

template<class E>
inline typename derivation_traits<E>::edge_type
& deriv_edge(edge_equivalence_impl<E> *p)
{
    return p->representative();
}

#if 1
/// (combined info and tm) score of a derivation
template<class E>
inline score_t
inside_score(//typename derivation_traits<E>::derivation_type
    edge_equivalence<E>
    const& deriv)
{
    return deriv_edge(deriv).inside_score();
}

template<class E>
inline score_t&
score(edge_equivalence<E>  const& deriv)
{
    return deriv_edge(deriv).score();
}


template<class E>
inline score_t&
score(edge_equivalence_impl<E> *p)
{
    return deriv_edge(p).score();
}

#else
/// (combined info and tm) score of a derivation
template<class E>
inline score_t
inside_score(E const& deriv)
{
    return deriv_edge(deriv).inside_score();
}

template<class E>
inline score_t&
score(E const& deriv)
{
    return deriv_edge(deriv).score();
}

template<class E>
inline
void
set_heuristic_score(E const& deriv,score_t s)
{
    deriv_edge(deriv).set_heuristic_score(s);
}

#endif

/** due to some bug (or misunderstanding), this method is not actually being found by the compiler.  We define this instead in kbest_derivation_factory */
template <class Edge> inline
bool derivation_better_than(
      edge_equivalence<Edge> const& me,
      edge_equivalence<Edge> const& than) {
    return inside_score(me) > inside_score(than); // bug - not finding
}

/// derivation from edge equivalence - identity for now
template<class E>
inline typename derivation_traits<E>::derivation_type derivation(typename derivation_traits<E>::edge_equiv_type const &equivalence)
{
    return equivalence;
}

/// derivation from an edge - requires an edge factory (aka Edge Construction Strategy) to build the equivalence
template<class E,class ECS>
inline typename derivation_traits<E>::edge_equiv_type singleton_equivalence(typename derivation_traits<E>::edge_type const &edge,ECS &ecs)
{
    return ecs.create(edge);
}

/// get the best derivation of an equivalence (forest)
template<class E>
inline
//typename derivation_traits<E>::derivation_type
edge_equivalence<E>
best_derivation(
    //typename derivation_traits<E>::forest_type
    edge_equivalence<E>
    const& forest
    )
{
    return forest;
}

///\}


/** \defgroup derivation_interp Printing Derivations

A derivation_interpreter wraps a grammar and fixes an edge_type, and can:

 - collect rule feature vectors for a derivation
 -  separate the part of the derivation score due to the info_type of the edge, and that due to the rule (TM) scores
 - collect the multiset of syntax rule ids (and their multiplicity) appearing in a derivation
 - get the yield (english tree or foreign) of a derivation
 - print derivations: their derivation tree (mentioning only syntax rule ids), english tree, foreign string, foreign tree, english string, used rules, feature vectors, alignments, and info/tm score
 - can print a phony (empty) nbest entry when parsing has failed

 \code
       edge_equiv_type top_equiv = chart.top_equiv(grammar);

        typedef derivation_interpreter<edge_type,grammar_in_mem> Interp;
        Interp interp(grammar,deriv_opt);

        typedef typename Interp::template nbest_printer<ostream> N_printer;
        N_printer nprint(interp,nbest_out,sentid);

        kbest_derivation_factory<edge_type> kfact(edge_factory);
        kfact.enumerate_kbest(top_equiv,nbests,nprint);

        std::string best_deriv_printed=interp.nbest(sentid,best_derivation(top_equiv));

        std::string failed_parse_deriv=interp.nbest(sentid);
\endcode

Note: would like names to be: derivation_reader becomes derivation_interpreter; derivation_interpreter becomes derivation_printer.  But don't want to change client code.

Example output:
<code>
NBEST sent=1 nbest=0 totalcost=3.70842 hyp={{{indonesia really reiterated its acceptance to the presence of foreign military forces}}} tree={{{(TOP (S (NP-C (NPB (NNP indonesia)[1,2][0,1]))[1,2][0,1] (ADVP really) (VP (VBD reiterated)[2,3][2,3] (NP-C (NPB (PRP$ its) (NN acceptance)[3,4][4,5]) (PP (TO to) (NP-C (NPB (DT the) (NN presence)[6,7][7,8])[6,7][6,8] (PP (IN of) (NP-C (NPB (JJ foreign) (JJ military) (NNS forces))[4,6][9,12])[4,6][9,12]))[4,7][6,12]))[3,7][3,12]))[1,7][0,12])[0,7][0,12]}}} derivation={{{(1000000001 (163 (137 128) 132 (131 2 (127 (123 122) (126 124)))))}}} foreign={{{<foreign-sentence> c1 c2 c3 c4 c5 c6}}} foreign-tree={{{[0,7](1000000001:TOP <foreign-sentence> [1,7](163:S [1,2](137:NP-C [1,2](128:NNP c1)) [2,3](132:VBD c2) [3,7](131:NP-C [3,4](2:NN c3) [4,7](127:NP-C [4,6](123:NP-C [4,6](122:NPB c4 c5)) [6,7](126:NPB [6,7](124:NN c6))))))}}} used-rules={{{2(0) 122(0) 123(0) 124(0) 126(0) 127(0) 128(0) 131(0) 132(0) 137(0) 163(0) 1000000001(0)}}} sbtm-cost=0 derivation-size=11 text-length=12 count=11 end=47 foreign-length=6 gt_prob=4.01375 start=24 lmcost=3.70842
</code>

Note that [0,7][0,12] is a foreign span followed by an english span, and [3,7] alone is just a foreign span.
*/
///\{

struct derivation_output_options
/**
   All the cosmetic options affecting how we output nbest (derivation) lines are collected here.  In addition, the info_wt, if nonzero, will be used to recover the info score, and *must* be the same as the weight given to the edge factory.
 **/
{
    graehl::char_predicate needs_quote;
    bool print_lattice_align;
    bool check_info_score;
    bool print_used_rules;
    bool print_foreign_tree;
    bool print_foreign_tree_ruleid;
    bool print_foreign_bin_tree;
    bool print_foreign_string;
    bool print_edge_information;
    /// unused for now.  intended to facilitate printing span alignments that refer to the original foreign sentence
    bool foreign_spans_skip_start;
//TODO    bool english_spans_skip_epsilon;
    bool show_spans;
    /// lisp style: (1 (2 3)).  non-lisp style: 1(2(3))
    bool lisp_style_trees;
    /// true -> force printing of 10^-1 as "1" (otherwise, the options controlling output of score_t for the output stream are used)
    bool scores_neglog10;
    /// I had the idea that "estring" and "etree" might be better names than "hyp" and "tree".  if you agree, make this true
    bool new_fieldnames;
    /// quotation of tokens: tokens could contain funny characters that make the output unparsable.  if true, quote_always quotes everything (escaping internal quotes with \")
    bool quote_always;
    /// the old/ugly/risky way: never quote anything.  good luck!  if neither always nor never are true, then tokens will be quoted *as needed* (i.e. the person using the output will probably never realize that unquoting is necessary)
    bool quote_never;
    /// print only the non-unit feature scores (ones that actually *cost* something)
    bool sparse_features;
    /// *must* agree with the weight used in edge factory.  if 0, no info score will be printed
    double info_wt;
    /// used to activate printing ngram score details, now connected to nothing.  nbest_printer takes an extra_english_output_t instead
    bool more_info_details;

    /// new_format=false gives output (mostly) compatible with old syntax-decoder
    void set_default(bool new_format=false) {
        print_lattice_align=true;
        check_info_score=true;
        print_foreign_string=false;
        print_used_rules=true;
        print_foreign_tree=true;
        print_foreign_tree_ruleid=true;
        print_foreign_bin_tree=false;
        print_edge_information=false;
        info_wt=1;
        foreign_spans_skip_start=false;
        sparse_features=true;
        new_fieldnames=new_format;
        show_spans=new_format;
        quote_always=false;
        quote_never=!new_format;
        scores_neglog10=true;
        lisp_style_trees=true;
        more_info_details=false;
        needs_quote.set_in_excl("()} \n\t",true);
    }
    derivation_output_options()
    {
        set_default(false);
    }
};

/**
A derivation_interpreter wraps a grammar and fixes an edge_type.  Everything in derivation_interpreter could have been moved inside grammar, as template members depending on the edge type.  But none of this is used in parsing, so it's separate.  We can compute feature vectors, tm scores, and thus (with an info weight), info scores.  We also print various (english,foreign,derivation) trees.

Everything in derivation_interpreter could have been moved inside grammar, as template members depending on the edge type.  But none of this is used in parsing, so it's separate.

Even though the derivations are binarized, much of the work done by the interpreter is in terms of the non-binarized syntax-rule derivation (as printed in nbests).  We build vectors of children by skipping over the nodes without syntax rules, and then (recursively) visit them (in an order specified by the syntax rule).

Example output:
<code>
NBEST sent=1 nbest=0 totalcost=3.70842 hyp={{{indonesia really reiterated its acceptance to the presence of foreign military forces}}} tree={{{(TOP (S (NP-C (NPB (NNP indonesia)[1,2][0,1]))[1,2][0,1] (ADVP really) (VP (VBD reiterated)[2,3][2,3] (NP-C (NPB (PRP$ its) (NN acceptance)[3,4][4,5]) (PP (TO to) (NP-C (NPB (DT the) (NN presence)[6,7][7,8])[6,7][6,8] (PP (IN of) (NP-C (NPB (JJ foreign) (JJ military) (NNS forces))[4,6][9,12])[4,6][9,12]))[4,7][6,12]))[3,7][3,12]))[1,7][0,12])[0,7][0,12]}}} derivation={{{(1000000001 (163 (137 128) 132 (131 2 (127 (123 122) (126 124)))))}}} foreign={{{<foreign-sentence> c1 c2 c3 c4 c5 c6}}} foreign-tree={{{[0,7](1000000001:TOP <foreign-sentence> [1,7](163:S [1,2](137:NP-C [1,2](128:NNP c1)) [2,3](132:VBD c2) [3,7](131:NP-C [3,4](2:NN c3) [4,7](127:NP-C [4,6](123:NP-C [4,6](122:NPB c4 c5)) [6,7](126:NPB [6,7](124:NN c6))))))}}} used-rules={{{2(0) 122(0) 123(0) 124(0) 126(0) 127(0) 128(0) 131(0) 132(0) 137(0) 163(0) 1000000001(0)}}} sbtm-cost=0 derivation-size=11 text-length=12 count=11 end=47 foreign-length=6 gt_prob=4.01375 start=24 lmcost=3.70842
</code>

Note that [0,7][0,12] is a foreign span followed by an english span, and [3,7] alone is just a foreign span.

 **/
template <class EdgeT,class GrammarT>
class derivation_reader
{
 public:
    typedef GrammarT                                         grammar_type;
    typedef derivation_reader<EdgeT,GrammarT> reader_type;
    typedef reader_type interp_type;
    typedef EdgeT edge_type;
    typedef typename edge_type::info_type info_type;

    // same as below, but working around template bugs
    typedef edge_equivalence<edge_type>                      edge_equiv_type;
    typedef edge_equiv_type forest_type;
    typedef edge_equiv_type derivation_type;

//    typedef typename derivation_traits<edge_type>::edge_equiv_type edge_equiv_type;
//    typedef typename derivation_traits<edge_type>::forest_type forest_type;
//    typedef typename derivation_traits<edge_type>::derivation_type derivation_type;

    typedef typename grammar_type::rule_type rule_type;

    derivation_reader() : grammar_p(NULL) {}
    void set_grammar(grammar_type const&gram) //FIXME: why is pointer non-const?
    {
        grammar_p=const_cast<grammar_type *>(&gram);
    }
    derivation_reader(grammar_type & gram) { set_grammar(gram); }

    derivation_reader(derivation_reader const &o) : grammar_p(o.grammar_p) {}

    grammar_type & grammar() const
    {
        return *grammar_p;
    }

    std::string const& label(indexed_token const& token) const
    {
        return grammar().label(token);
    }

    score_t tm_score(edge_equiv_type const&e, std::vector<feature_vector>* fvec = 0) const
    {
        return tm_score(e.representative(),fvec);
    }

    score_t tm_score( edge_type const& e
                    , std::vector<feature_vector>* fvec ) const // gets scores from binary rules so correct for text-length
    {
        grammar_type const&g=grammar();
        grammar_rule_id id=e.rule_id();
        if (id==NULL_GRAMMAR_RULE_ID) return as_one(); // e.g. foreign word
        score_t s=g.rule_score(g.rule(id));
        if (fvec) fvec->push_back(g.rule_scores(g.rule(id)));
        if (e.has_first_child()) {
	    s *= tm_score(e.first_children(),fvec);
            if (e.has_second_child())
	        s *= tm_score(e.second_children(),fvec);
        }
        return s;

    }


    /// note: version w/ quote options in derivation_interpreter
    template <class Ostream,class Token_String>
    void print_tokens(Ostream &o,Token_String const& tokens) const
    {
        graehl::word_spacer sp;
        for (typename Token_String::const_iterator
                 i=tokens.begin(),e=tokens.end();i!=e;++i) {
            o << sp;
            o << label(*i);
        }
    }

    /// builds a list of the children (or grand*-children) satisfying stop(edge)
    template <class Subderivations,class Stop_At>
    static void collect_children(Subderivations &subderivs,derivation_type deriv,Stop_At const& stop)
    {
        collect_children(subderivs,deriv_edge(deriv),stop);
    }

    /// builds a list of the children (or grand*-children) satisfying stop(edge)
    template <class Subderivations,class Stop_At>
    static void collect_children(Subderivations &subderivs,edge_type const& e,Stop_At const& stop)
    {
        if (e.has_first_child()) {
            collect_frontier(subderivs,e.first_children(),stop);
            if (e.has_second_child()) {
                collect_frontier(subderivs,e.second_children(),stop);
            }
        }
    }

    /// helper for collect_children (will return self as a singleton list if the criteria is satisfied)
    template <class Subderivations,class Stop_At>
    static void collect_frontier(Subderivations &subderivs,derivation_type deriv,Stop_At const& stop)
    {
        edge_type const& e=deriv_edge(deriv);
        if (stop(e))
            subderivs.push_back(deriv);
        else
            collect_children(subderivs,e,stop);
    }

 private:
    struct stop_at_syntax_rule
    {
        grammar_type const& gram;
        stop_at_syntax_rule(stop_at_syntax_rule const& o) : gram(o.gram) {}
        stop_at_syntax_rule(grammar_type const& gram) : gram(gram) {}
        bool operator()(edge_type const &e) const
        {
            return e.has_syntax_id(gram);
        }
    };
    struct stop_at_precondition
    {
        grammar_type const& gram;
        stop_at_precondition(stop_at_precondition const& o) : gram(o.gram) {}
        stop_at_precondition(grammar_type const& gram) : gram(gram) {}
        bool operator()(edge_type const &e) const
        {
            return e.has_syntax_id(gram) || e.is_foreign();
        }
    };
    /*
    struct stop_at_lm_scoreable
    {
        grammar_type const& gram;
        stop_at_lm_scoreable(stop_at_lm_scoreable const& o) : gram(o.gram) {}
        stop_at_lm_scoreable(grammar_type const& gram) : gram(gram) {}
        bool operator()(edge_type const &e) const
        {
            return gram.lm_scoreable(gram.rule(e.rule_id()));
        }
    };
    */

 public:
    static inline bool has_children(derivation_type deriv)
    {
        return deriv_edge(deriv).has_first_child();
    }


    /// vector left->right of (grand*)-children having a syntax rule id
    struct syntax_children : public std::vector<derivation_type>
    {
        syntax_children(derivation_type const& deriv,grammar_type const& gram)
        {
            reader_type::collect_children(*this,deriv,stop_at_syntax_rule(gram));
        }
    };

    /// vector left->right of (grand*)-children: either foreign words or having syntax rule id
    struct precondition_children : public std::vector<derivation_type>
    {
        precondition_children(derivation_type const& deriv,grammar_type const& gram)
        {
            reader_type::collect_children(*this,deriv,stop_at_precondition(gram));
        }
    };

    /// vector left->right of (grand*)-children: only lm scoreable (non-foreign) children
    struct lm_scoreable_children : public std::vector<derivation_type>
    {
        lm_scoreable_children(derivation_type const& deriv,grammar_type const& gram)
        {
            collect(deriv_edge(deriv),gram);
        }
        lm_scoreable_children(edge_type const& e,grammar_type const& gram)
        {
            collect(e,gram);
        }

        void collect(edge_type const& e,grammar_type const& g)
        {
            if (e.has_first_child()) {
                collect_frontier(e.first_children(),g);
                if (e.has_second_child()) {
                    collect_frontier(e.second_children(),g);
                }
            }
        }

        template <class D>
        void collect_frontier(D const& d,grammar_type const& g)
        {
            edge_type const& e=deriv_edge(d);
            grammar_rule_id id=e.rule_id();
            if (id!=NULL_GRAMMAR_RULE_ID) {
                if (g.lm_scoreable(g.rule(id)))
                    this->push_back(d);
                else
                    collect(e,g);
            }
        }

    };



    /// Collect the leaves left->right (edges having tokens with type=of_type).
    /// Doesn't in any way look at english trees; only inspects the foreign tree
    /// (referring to the root token of each edge)
    template <class Lexical_Tokens>
    static void collect_yield(Lexical_Tokens &tokens,derivation_type deriv,token_type_id of_type=foreign_token)
    {
        edge_type const& e=deriv_edge(deriv);
        indexed_token t=e.root();
        if (t.type()==of_type) {
            tokens.push_back(t);
        }
        else
            if (e.has_first_child()) {
                collect_yield(tokens,e.first_children(),of_type);
                if (e.has_second_child()) {
                    collect_yield(tokens,e.second_children(),of_type);
                }
            }
    }

    template <class Tokens>
    void collect_foreign_tokens(Tokens& tokens, derivation_type deriv) const
    {
        edge_type const& e = deriv_edge(deriv);
        if (e.root().type() == foreign_token) tokens.push_back(e.root());
        else if (e.has_syntax_id(grammar()) && !e.has_first_child()) {
            tokens.push_back(grammar().get_syntax(e.syntax_id(grammar())).rhs_begin()->get_token());
        } else if (e.has_first_child()) {
            collect_foreign_tokens(tokens,e.first_children());
            if (e.has_second_child()) {
                collect_foreign_tokens(tokens,e.second_children());
            }
        }
    }

    struct foreign_tokens : public std::vector<indexed_token>
    {
        foreign_tokens(derivation_type deriv,interp_type const& interp)
        {
            interp.collect_foreign_tokens(*this,deriv);
        }
    };

    /// vector of the yield
    struct yield : public std::vector<indexed_token>
    {
        yield(derivation_type deriv,token_type_id of_type=foreign_token)
        {
            reader_type::collect_yield(*this,deriv,of_type);
        }
    };

    /// accumulates ++map[ruleid] for each ruleid in deriv
    template <class IDMap>
    void count_used_rules(IDMap &map,derivation_type deriv) const
    {
        if (!deriv) return;
        edge_type const&e=deriv_edge(deriv);
        //syntax_id_type id=e.syntax_id(grammar());
        if (e.has_rule_id()) ++map[grammar().rule(e.rule_id())]; // default constructor called first time!
        if (e.has_first_child()) {
            count_used_rules(map,e.first_children());
            if (e.has_second_child())
                count_used_rules(map,e.second_children());
        }
    }

    typedef std::map<typename grammar_type::rule_type,unsigned> used_rule_counts_type;

    scored_syntax const& get_scored_syntax(edge_type const& e) const
    {
        return grammar().get_scored_syntax(e.syntax_id(grammar()));
    }

    /// add a derivation's syntax rules' feature vectors to scores_accum
    void collect_scores(feature_vector& scores_accum, derivation_type deriv) const
    {
        edge_type const& e=deriv_edge(deriv);
        if (e.has_rule_id())
            scores_accum *= grammar().rule_scores(grammar().rule(e.rule_id()));
            //scores_accum.accumulate(grammar().rule_scores(grammar().rule(e.rule_id())));
        if (e.has_first_child()) {
            collect_scores(scores_accum,e.first_children());
            if (e.has_second_child())
            collect_scores(scores_accum,e.second_children());
        }
    }

#if 0
    bool has_english_string(derivation_type deriv) const
    {
        return deriv_edge(deriv).has_syntax_id(grammar()); //FIXME: can give english string for lm scoreable binary rules as well (would make duplicate removal faster)
    }
#endif

    /// vector of english string for a derivation (root must be syntax rule)
    struct english_string : public std::vector<indexed_token>
    {
        english_string(derivation_type deriv,reader_type const& reader)
        {
            reader.collect_english_string(*this,deriv);
        }
    };

    /// vector of english strings for a derivation, each separated by native_separator()
    struct english_string_list : public std::vector<indexed_token>
    {
        english_string_list(derivation_type deriv,reader_type const& reader)
        {
            reader.collect_english_string_list(*this,deriv);
        }
    };

    template <class English_Tokens>
    void collect_english_string_list(English_Tokens &tokens,derivation_type deriv) const
    {
        if (deriv_edge(deriv).has_syntax_id(grammar())) {
            collect_english_string(tokens,deriv);
            return;
        }
        if (!has_children(deriv))
            return;
        syntax_children c(deriv,grammar());
        typename syntax_children::const_iterator i=c.begin(),e=c.end();
        if (i==e) return; // shouldn't be possible to have children that aren't syntax children, without yourself having a syntax id ... BUT I'm scared
        for (;;) {
            collect_english_string(tokens,*i);
            if (++i==e)
                break;
            tokens.push_back(native_separator());
        }
    }

    template <class English_Tokens>
    void collect_english_string(English_Tokens &tokens,derivation_type deriv) const
    {
        edge_type const&e=deriv_edge(deriv);
        assert(e.has_syntax_id(grammar()));
        precondition_children children(deriv,grammar()); // because indices in rule trees refer to precondition including foreign words
        indexed_syntax_rule const& syn = e.get_syntax(grammar());
        collect_english_string(tokens,*syn.lhs_root(),children);
    }


    indexed_token native_epsilon() const
    {
        return dict().native_epsilon();
    }

    indexed_token native_separator() const
    {
        return dict().native_separator();
    }

    indexed_token_factory & dict() const
    {
        return grammar().dict();
    }

    template <class English_Tokens>
    void collect_english_string(English_Tokens &tokens,indexed_syntax_rule::tree_node const& node,precondition_children const& children) const
    {
        if (node.indexed())
            collect_english_string(tokens,children[node.index()]);
        else if (node.lexical()) {
            indexed_token t=node.get_token();
            if (t!=native_epsilon())
                tokens.push_back(t);
        } else
            for (indexed_syntax_rule::lhs_children_iterator
                     i=node.children_begin(),e=node.children_end();i!=e;++i)
                collect_english_string(tokens,*i,children);
    }

//    template <class English_Tokens>
//    void collect_english_lmstring(English_Tokens &tokens,derivation_type deriv) const
//    {
//        collect_english_lmstring(tokens,deriv_edge(deriv));
//    }
/*
    // same as collect_english_string, but respects what's in the lmstring rather than the leaves of syntax rules.  principal difference: @UNKNOWN@ w/ empty lmstring.  does *NOT* wrap in <s> </s> tags (as those aren't on rule)
    template <class English_Tokens>
    void collect_english_lmstring(English_Tokens &tokens,edge_type const& e) const
    {
        grammar_type const&g=grammar();
        grammar_rule_id id=e.rule_id();
        if (id==NULL_GRAMMAR_RULE_ID) {
            SBMT_DEBUG_STREAM(forest_domain,"collect_english_lmstring no binary rule id "<<e);
            return; // e.g. foreign word
        }
        lm_scoreable_children ch(e,g);

        typedef typename grammar_type::lm_string_type lm_string_t;
        lm_string_t const& lmstr=g.rule_lm_string(g.rule(id));
        SBMT_DEBUG_STREAM(forest_domain,"collect_english_lmstring lmstr=" << print(lmstr,dict()));
        for (typename lm_string_t::const_iterator i = lmstr.begin(),end= lmstr.end();
             i != end; ++i) {
            // lexical item
            if(i->is_token()){
                SBMT_DEBUG_STREAM(forest_domain,"collect_english_lmstring push_back " << print(i->get_token(),dict()));
                tokens.push_back(i->get_token());
            }
            // variable
            else if(i->is_index()) {
                size_t v = i->get_index();
                collect_english_lmstring(tokens,ch.at(v));
            }
        }
    }
*/

    alignment align(derivation_type const& d) const
    {
        alignment a;
        compute_align(a,d);
        return a;
    }

    void compute_align(alignment &a,derivation_type const& d,std::vector<span_t> & foreign_spans) const
    {
        foreign_spans.clear();
        if (!d) {
            a.set_null();
            return;
        }
        compute_align_rec(a,d,foreign_spans);
    }

    void compute_align_rec(alignment &a,derivation_type const& d,std::vector<span_t> & foreign_spans) const
    {
        edge_type const& e=deriv_edge(d);
        assert(e.has_syntax_id(grammar()));

        precondition_children p(d,grammar());
        unsigned ns=p.size();
        std::vector<alignment> ca(ns);
        typedef alignment::substitution sub;
        std::vector<sub> subs;
        for (unsigned i=0;i<ns;++i) {
            edge_type const& ce=deriv_edge(p[i]);
            if (ce.has_syntax_id(grammar())) {
                compute_align_rec(ca[i],p[i],foreign_spans);
                subs.push_back(sub(ca[i],i));
            } else {
                foreign_spans.push_back(ce.span());
            }
        }

        get_scored_syntax(e).align.substitute_into(a,subs);
    }

    void compute_align(alignment &a,derivation_type const& d) const
    {
        std::vector<span_t> ignore_foreign_spans;
        compute_align(a,d,ignore_foreign_spans);
    }
/*
    std::string headmarker(derivation_type const& d) const
    {
        std::string m;
        compute_headmarker(m,d);
        return m;
    }
*/
    void compute_headmarker(std::string&m, derivation_type const& d) const
    {
        if (!d) {
            m = "";
            //std::cerr << "headmarker: null d\n";
            return;
        }


        edge_type const&e=deriv_edge(d);
        assert(e.has_syntax_id(grammar()));
        precondition_children p(d, grammar());
        text_feature_vector_byid texts = grammar().rule_text(grammar().rule(e.rule_id()));
        text_feature_vector_byid::iterator pos = texts.begin();
        feature_id_type hmid = grammar().feature_names().get_index("headmarker");
        for(; pos != texts.end(); ++pos) if (pos->first == hmid) break;
        if (pos == texts.end()) {
            //std::cerr << "headmarker: no feature\n";
            m = "";
            return;
        }
        std::string hm = pos->second;

        //std::cerr << "headmarker: " << hm << '\n';
        std::ostringstream ost;
        std::istringstream ist(hm);
        indexed_syntax_rule const& s=e.get_syntax(grammar());
        for (indexed_syntax_rule::lhs_preorder_iterator i=s.lhs_begin(),e=s.lhs_end();
             i!=e;++i) {
            if (i->lexical()){
            } else if(i->indexed()){
                std::string mm;
                char ch;
                while(ist>>ch){
                    if (ch == 'R'){
                        throw std::runtime_error("headmarker:cannot have root over indexed");
                    } else if (ch == '(' || ch == ')'){
                        ost<<ch;
                        continue;
                    } else {
                        ost <<ch; //<<"-";
                        break;
                    }
                }
                //std::cerr << "headmarker: enter child "<< i->index() << ". sofar: " << ost.str() << '\n';
                compute_headmarker(mm, p[i->index()]); ////
                ost<<mm;
            } else {
                char ch;
                while(ist>>ch){
                    if(ch == 'R'){
                        if (i != s.lhs_root()) {
                            throw std::runtime_error("headmarker: R seen in internal part of rule");
                        }
                        break;
                    } else if(ch == '(' || ch == ')'){
                        ost<<ch;
                        continue;
                    } else {
                        ost << ch;
                        break;
                    }
                }
            }
        }
        char ch;
        while(ist>>ch){
            ost<<ch;
        }
        m = ost.str();
        //std::cerr << "headmarker: ret "<< m << '\n';
    }

 protected:
    grammar_type *grammar_p;

};


typedef boost::function<void (std::ostream &o,indexed_sentence const& s,score_t)> extra_english_output_t;

struct null_extra_english_output {
    void operator()(std::ostream &o,indexed_sentence const& s,score_t base_score) const
    {}
};

// convert to extra_english_output_t
template <class LM>
struct extra_english_lm
{
    LM *lm;
    bool more_details;
    extra_english_lm(LM *lm,bool more_details=false)
        : lm(lm), more_details(more_details)
    {
    }
    void operator()(std::ostream &o,indexed_sentence const& s,score_t base_score) const
    {
        score_t is=lm->print_sentence_details(o,s.begin(),s.end(),more_details);
        o << " totalcost-with-lm="<<pow(is,lm->own_weight)*base_score;
    }
    extra_english_output_t output() const
    {
        return *this;
    }
};

template <class LM>
extra_english_lm<LM> make_extra_english_lm(LM *lm,bool more_details=false)
{
    return extra_english_lm<LM>(lm,more_details);
}

typedef graehl::tree<grammar_rule_id> derivation_tree;
typedef boost::function<void (std::ostream &o,derivation_tree const& t,score_t)> extra_deriv_output_t;
struct null_extra_deriv_output {
    void operator()(std::ostream &o,derivation_tree const& t,score_t base_score) const
    {}
};

typedef graehl::tree<indexed_token> english_tree;
typedef boost::function<void (std::ostream &o,english_tree const& t,score_t)> extra_etree_output_t;
struct null_extra_etree_output {
    void operator()(std::ostream &o,english_tree const& t,score_t base_score) const
    {}
};


template <class EdgeT,class GrammarT>
class derivation_interpreter : public derivation_reader<EdgeT,GrammarT>
{
    typedef derivation_reader<EdgeT,GrammarT> R;
 public:


    /// the log is only used to complain about disagreements between combined feature_vector sum, and the sum of rule scores used during parsing
    std::ostream *log;
    void set_log(std::ostream &o)
    {
        log=&o;
    }
    void no_log()
    {
        log=0;
    }

    /// if no log has been set, throw an exception (NULL means don't throw, but print to STDERR instead).
    std::ostream &get_log(char const* exception_if_no_log=0) const
    {
        if (exception_if_no_log && !log)
            throw std::runtime_error(exception_if_no_log);
        return log ? *log : std::cerr;
    }



    derivation_interpreter() : log(&std::cerr) {}

    typedef GrammarT                                         grammar_type;
    typedef derivation_interpreter<EdgeT,GrammarT> interp_type;
    typedef EdgeT edge_type;
    typedef typename edge_type::info_type info_type;

    // same as below, but working around template bugs
    typedef edge_equivalence<edge_type>                      edge_equiv_type;
    typedef edge_equiv_type forest_type;
    typedef edge_equiv_type derivation_type;

//    typedef typename derivation_traits<edge_type>::edge_equiv_type edge_equiv_type;
//    typedef typename derivation_traits<edge_type>::forest_type forest_type;
//    typedef typename derivation_traits<edge_type>::derivation_type derivation_type;

//    typedef typename EdgeT::info_factory_type info_factory_type;
    typedef concrete_edge_factory<edge_type,grammar_type> edge_factory_type;


    edge_factory_type &ef;

    derivation_output_options opt;
    typedef typename grammar_type::rule_type rule_type;
    derivation_interpreter(grammar_type & gram
                           , edge_factory_type &ef
                           , derivation_output_options const& opt=derivation_output_options()
                           , std::ostream *log_p=&std::cerr
        )
        : R(gram),ef(ef),opt(opt)
    {
        /*
        io::logging_stream& logstr = io::registry_log(forest_domain);
        BOOST_FOREACH(typename grammar_type::rule_type rule, gram.all_rules()) {
            score_t scr = gram.rule_score(rule);
            feature_vector const& scrvec = gram.rule_scores(rule);
            whine_unless_very_close(scr,geom(scrvec,gram.get_weights()),logstr,"rule","features");
        }
        */
    }


    grammar_type & grammar() const
    {
        return *this->grammar_p;
    }

    R const& reader() const
    {
        return *this;
    }

    typedef typename R::syntax_children syntax_children;

    template <class Ostream>
    void print_derivation_tree(Ostream &o,derivation_type deriv,bool show_spans=false) const
    {
        grammar_type const& gram=grammar();
        if (!deriv) return;
        edge_type const& e=deriv_edge(deriv);
        assert(e.has_syntax_id(gram));
        syntax_children ch(deriv,gram);
        if (!ch.empty()) o << '(';
        if (show_spans) o << e.span();
        o << e.syntax_id(gram);
        if (!ch.empty()) {
            for (typename syntax_children::const_iterator
                     i=ch.begin(),e=ch.end();i!=e;++i) {
                o << ' ';
                print_derivation_tree(o,*i,show_spans);
            }
            o << ')';
        }
    }

    template <class C, class T>
    void print_virt_deriv_tree( std::basic_ostream<C,T>& o, edge_type const& e) const
    {
        grammar_type const& gram=grammar();
        std::vector<edge_type const*> children;
        for (size_t x = 0; x != e.child_count(); ++x) {
            if (not is_foreign(e.get_child(x).root())) {
                children.push_back(&e.get_child(x));
            }
        }
        if (children.size() > 0) o << '(';
        o << '[' << e.score() << ',' << e.inside_score() << ']';
        o << e.span();
        if (e.has_syntax_id(gram)) o << e.syntax_id(gram);
        else o << gram.dict().label(e.root());
        for (size_t x = 0; x != children.size(); ++x) {
            o << ' ';
            print_virt_deriv_tree(o,*children[x]);
        }
        if (children.size() > 0) o << " )";
    }

    template <class C, class T>
    void print_virt_deriv_tree( std::basic_ostream<C,T>& o, derivation_type d) const
    {
        if (!d) return;
        print_virt_deriv_tree(o,deriv_edge(d));
    }

    void combine_scores(feature_vector& feats,derivation_type deriv) const
    {
      collect_tm_scores(feats,deriv.representative(),grammar()); // traverses all children, accum gram.rule_scores(binary rule)
    }

    template <class Ostream>
    void print_scores(Ostream &o,derivation_type deriv) const
    {
        if (!deriv) return;
        feature_vector feats;
        collect_tm_scores(feats,deriv);
        o << feats;
    }

#if 0
  ///TODO: use these (optionally) so that foreign bylines' use is incorporated into span alignments
    span_t fix_foreign_span(span_t const& span) const
    {
        span_t fixed=span;
        if (opt.foreign_spans_skip_start && span.left()>0)
            --span.left();
        return span;
    }
    span_t const& fix_native_span(span_t const& span) const
    {
        return span;
    }
    template <class O>
    void print_foreign_span(O &o,span_t const& span) const
    {
        o << fix_foreign_span(span);
    }
    template <class O>
    void print_native_span(O &o,span_t const& span) const
    {
        o << fix_native_span(span);
    }
#endif

    /// print a space separated series of tokens using the grammar's dictionary
    template <class Ostream,class Token_String>
    void print_tokens(Ostream &o,Token_String const& tokens) const
    {
        graehl::word_spacer sp;
        for (typename Token_String::const_iterator
                 i=tokens.begin(),e=tokens.end();i!=e;++i) {
            o << sp;
            print_maybe_quote(o,*i);
        }
    }

    template <class Ostream>
    void print_english_string(Ostream &o,derivation_type deriv) const
    {
        if (!deriv) return;
        print_tokens(o,typename R::english_string(deriv,*this));
    }

    void print_extra_english_output(std::ostream &o,derivation_type deriv,extra_english_output_t const& f,score_t totalscore) const
    {
        if (!deriv) return;
        logmath::save_format s(o);
        logmath::set_neglog10(o,opt.scores_neglog10);
        indexed_sentence sent; // (native_token) - no, we have tag+native for force-etree decode
        //collect_english_lmstring(sent,deriv);
        f(o,sent,totalscore);
    }

    template <class Ostream>
    void print_foreign_string(Ostream &o,derivation_type deriv) const
    {
        if (!deriv) return;
        print_tokens(o,typename R::foreign_tokens(deriv,reader()));
    }


  //FIXME: no longer useful to consider just the syntax rules since pipeline allows features on bin rules too (e.g. text-length pushing)
    /// holds rule use multiset for a derivation (can print; can return total tm
    /// score).  print() looks like:
    /// <code>12x2(0) 122(0) 123(0) 124(0)</code>
    /// where rule 12 was used twice, and (0) is the cost of the rule
    struct counted_rules
    {
        typedef typename R::used_rule_counts_type used_rule_counts_type;
        //FIXME: put the computing part in derivation_reader, print part here
        struct rule_use
        {
            typename grammar_type::rule_type rule;
            unsigned times;
            score_t score;
            rule_use( typename grammar_type::rule_type rule
                    , unsigned times
                    , grammar_type const& g )
                : rule(rule)
                , times(times)
                , score(g.rule_score(rule))
                {}
        };
        typedef std::vector<rule_use> rule_uses_type;
        rule_uses_type rule_uses;
        grammar_type const* pg;
        counted_rules(derivation_type deriv,interp_type const& interp)
        {
            used_rule_counts_type counts;
            interp.count_used_rules(counts,deriv);
            pg = &interp.grammar();
            for ( typename used_rule_counts_type::const_iterator
                      i=counts.begin(),
                      end=counts.end()
                ; i != end
                ; ++i
                ) rule_uses.push_back(rule_use(i->first,i->second,*pg));
        }
        template <class O>
        void print(O &o,interp_type const& interp,char sep=' ') const
        {
            std::set< boost::tuple<syntax_id_type,int> > sorted;
            graehl::word_spacer sp(sep);
            BOOST_FOREACH(typename rule_uses_type::const_reference c,rule_uses)
            {
                if (pg->is_complete_rule(c.rule)) {
                    sorted.insert(boost::make_tuple(pg->get_syntax_id(c.rule),c.times));
                }
            }
            syntax_id_type synid, times;
            BOOST_FOREACH(boost::tie(synid,times),sorted)
            {
                o << sp << synid;
                if (times > 1)
                    o << 'x'<< times;
            }
        }
      //FIXME: incorrect - skips scores from binary rules
        score_t tm_score() const
        {
            score_t score=as_one();
            for (typename rule_uses_type::const_iterator
                     i=rule_uses.begin(),end=rule_uses.end();i!=end;++i)
                score *= (i->score) ^ (i->times);
            return score;
        }
    };

    template <class O,class Modelname>
    bool whine_unless_very_close( score_t a
                                , score_t b
                                , O &logstr
                                , std::string const& header
                                , Modelname const& model
                                , char const* a_is="decode"
                                , char const* b_is="nbest-rescore"
				, std::string const& extra="" ) const
    {
      if (!graehl::close_enough_min_scale(a.log(),b.log(),SBTM_NBEST_VERIFY_EPSILON,SBTM_NBEST_VERIFY_MIN_SCALE)) { // absolute difference because these are sums of opposite signed things and may coincidentally be near 0
                logstr << io::warning_msg
                       << header << model << " mismatch: "<<a_is<<"="<<a
                       << " "<<b_is<<"="<<b<<" log_"<<a_is<<"("<<b_is<<")="<<a.log_base_me(b)
                       <<" "<<a_is<<"/"<<b_is<<"="<<a/b
                       << extra << io::endmsg;
                return true;
            }
            return false;
    }

    template <class Ostream>
    void print_all_scores(Ostream& o,derivation_type deriv) const
    {
        if (deriv) {
            feature_vector info_scores;
            feature_vector info_heuristics;
            ef.component_info_scores(deriv.representative(),grammar(),info_scores,info_heuristics);

            feature_vector tm_scores;
            combine_scores(tm_scores,deriv);

            feature_vector all_scores = tm_scores * info_scores;

            BOOST_FOREACH(feature_vector::value_type c, const_cast<feature_vector const&>(all_scores))
            {
                o << grammar().feature_names().get_token(c.first) << '=' << c.second << ' ';
            }
            if (deriv.representative().root().type() != top_token) {
                o << "info-heuristics={{{ ";
                BOOST_FOREACH(feature_vector::value_type c, const_cast<feature_vector const&>(info_heuristics))
                {
                    o << grammar().feature_names().get_token(c.first) << '=' << c.second << ' ';
                }
                o << "}}} ";
                o << "tm-heuristic=" << grammar().rule_score_estimate(grammar().rule(deriv.representative().rule_id()));
            }
        }
    }

    /// print all the information about a derivation used in an nbest line
    template <class Ostream>
    score_t print_all(Ostream &o,derivation_type deriv=derivation_type(),std::string const& header="") const
    {
        bool dummy=!deriv;
        feature_vector info_scores;
        if (!dummy)
          ef.component_info_scores(deriv.representative(),grammar(),info_scores); //compute before printing anything, so debug messages don't break up nbest line if sharing same file output

        o<<header;
        o<<logmath::neglog10_scale;
        io::logging_stream& logstr = io::registry_log(forest_domain);
        score_t total_score=dummy ? score_t(as_one()) : inside_score(deriv);
        print_score(o,"total",total_score);
        o<<" ";
        o<<(opt.new_fieldnames ?"estring":"hyp");
        o<<"={{{";
        print_english_string(o,deriv);
        o<<"}}} ";
        if (dummy)
            o << "failed-parse=1 ";
        o<<(opt.new_fieldnames ? "etree":"tree");
        o<<"={{{";
        print_english_tree(o,deriv);
        o<<"}}} ";
        o<<"derivation={{{";
        print_derivation_tree(o,deriv,opt.show_spans);
        o<<"}}} ";
        o<<"binarized-derivation={{{";
        print_virt_deriv_tree(o,deriv);
        o<<"}}} ";
        if (opt.print_foreign_string) {
            o<<"foreign={{{";
            print_foreign_string(o,deriv);
            o<<"}}} ";
        }
        if (opt.print_foreign_tree) {
            o<<"foreign-tree={{{";
            print_foreign_tree(o,deriv,opt.print_foreign_tree_ruleid);
            o<<"}}} ";
        }
        if (opt.print_foreign_bin_tree) {
            o << "foreign-binarized-tree={{{";
            print_foreign_derivation(o,deriv_edge(deriv));
            o << "}}} ";
        }
        counted_rules used_rules(deriv,*this);
        if (opt.print_used_rules) {
            o <<"used-rules={{{";
            used_rules.print(o,*this);
            o<<"}}} ";
        }
        // o now ends without a space. all additions prepend a space.
        if (!dummy) {
	    std::vector<feature_vector> fvecs;
            score_t binary_tm_score=R::tm_score(deriv,&fvecs);

            print_score(o,"sbtm-",binary_tm_score);

            feature_vector tm_scores;
            combine_scores(tm_scores,deriv); // collect_tm_scores

            weight_vector const& weights=grammar().get_weights();
            score_t sbtmw = geom(tm_scores, weights);

	    std::stringstream bvmsg;
            bvmsg << '\n' << print(tm_scores,grammar().feature_names()) << '\n';
            BOOST_FOREACH(feature_vector ff, fvecs) { bvmsg << print(ff,grammar().feature_names()) << '\n'; }
            whine_unless_very_close( binary_tm_score
                                   , sbtmw
                                   , logstr
                                   , header
                                   , "sbtm"
                                   , "prod(bin-rule^weights)"
                                   , "prod(bin-rules)^weights"
                                   , bvmsg.str() );
            feature_vector all_scores = tm_scores * info_scores;
            score_t infow=geom(info_scores,weights);
            score_t allw=geom(all_scores,weights);

            whine_unless_very_close(infow*sbtmw,allw,logstr,header,"total","rules^weights*infos^weights","(rules*infos)^weights");

            check_nonfinite_costs(all_scores,grammar().feature_names());

            BOOST_FOREACH(feature_vector::value_type c, const_cast<feature_vector const&>(all_scores))
            {
              std::string const& f=grammar().feature_names().get_token(c.first);
              o << ' ' <<  f << '=' << c.second;
//              if (!c.second.is_positive_finite()) logstr<<io::warning_msg<<f<<'='<<c.second<<" cost is not finite (i.e. prob is not positive and finite) - tuning and 1best may be ill-defined!"<<io::endmsg;
            }
            //o="... "

            if (whine_unless_very_close(total_score,allw,logstr,header,"total","possibly-greedy-inside","(rules*infos)^weights")) {
                logstr << io::verbose_msg << "info_scores:" << print(info_scores,grammar().feature_names()) << "\n"
                       <<  "tm_scores:  " << print(tm_scores,grammar().feature_names()) << "\n"
                       <<  "all_scores: " << print(all_scores,grammar().feature_names()) << "\n"
                       <<  "all_mixed_score: " << total_score << io::endmsg;
            }

            print_align(o,deriv);
            print_headmarker(o,deriv);
        }

        //o << "\n";
        return total_score;
    }


    struct source_lattice_align_printer
    {
        template <class O>
        void print_source_index(O&o,unsigned s) const
        {
            assert(s<source_spans.size());
            o << source_spans[s];
        }
        template <class O>
        void print_target_index(O&o,unsigned t) const
        {
            o << t;
        }

        typedef std::vector<span_t> spanvec;
        spanvec source_spans; // foreign

        source_lattice_align_printer() {}

        source_lattice_align_printer(spanvec const& source_spans)
            : source_spans(source_spans) {}

        source_lattice_align_printer(source_lattice_align_printer const& o)
            : source_spans(o.source_spans) {}
    };


    template <class O>
    void print_align(O &o,derivation_type const& d) const
    {
        alignment a;
        source_lattice_align_printer lat;
        this->compute_align(a,d,lat.source_spans);
        if (!a.is_null()) {
            o << " align={{{"<< a <<"}}}";
            if (opt.print_lattice_align) {
                o << " source-lattice-align={{{";
                a.print_custom(o,lat);
                o << "}}}";
            }
        }
    }

    template <class O>
    void print_headmarker(O &o,derivation_type const& d) const
    {
        //std::cerr << "headmarker: start\n";
        std::string m;
        this->compute_headmarker(m,d);
        if (m != "")
            // we didnt print the tree root 'R', we thus print it here.
            o << " headmarker={{{R"<< m <<"}}}";
    }

    template <class O>
    friend
    void print(O &o,derivation_type const& d,interp_type const& interp)
    {
        interp.print_all(o,d);
    }

  template <class O>
  void print_nbest(O &out,derivation_type const& deriv,std::string const& header="",extra_english_output_t *eo=0) const {
    SBMT_VERBOSE_STREAM(forest_domain,"Printing "<<header<<" ...");
    score_t s=print_all(out,deriv,header);
    if (opt.print_edge_information) {
      std::ostringstream o;
      o << "\tedge-info " << header << "sentence-length " << deriv_edge(deriv).span().size();
      print_edge_information(out,deriv_edge(deriv), o.str());
    }
    if (eo)
      print_extra_english_output(out,deriv,*eo,s);
    out<<std::endl;
    SBMT_DEBUG_STREAM(forest_domain,"DONE printing "<<header);
  }

//  SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(nbest_domain,"nbest",forest_domain);

    /// callback used for enumerating kbest (kbest.hpp).  you supply interpreter and sentence id.
    template <class Ostream>
    class nbest_printer
    {
     public:
        bool operator()(derivation_type const& deriv,unsigned nbest_rank=0) const
        {
          interp.print_nbest(out,deriv,get_nbest_header(nbest_rank),eo);
          return true;
        }
        std::string get_nbest_header(unsigned nbest_rank) const
        {
            return nbest_header(sent,nbest_rank,pass);
        }

        nbest_printer(const derivation_interpreter &di,Ostream &o,
                      unsigned sent=1,unsigned pass=0,extra_english_output_t *eo=NULL)
            : interp(di), out(o), sent(sent),pass(pass),eo(eo) {}
     protected:
        derivation_interpreter const& interp;
        Ostream &out;
        unsigned sent;
        unsigned pass;
        extra_english_output_t *eo;
    };

    /// intended for use by lazy_forest::enumerate_kbest
    template <class Ostream>
    nbest_printer<Ostream> make_nbest_printer(Ostream &o,unsigned sent_number,unsigned pass=0,extra_english_output_t *eo=NULL) const
    {
        return nbest_printer<Ostream>(*this,o,sent_number,pass,eo);
    }

    template <class Ostream>
    void print_nbest(Ostream &o,unsigned sent_number,unsigned pass,derivation_type d=derivation_type(),unsigned nbest_rank=0,extra_english_output_t *eo=NULL) const
    {
        nbest_printer<Ostream>(*this,o,sent_number,pass,eo)(d,nbest_rank);
    }

    /// returns an nbest string via print_nbest. has a newline on the end already.
    std::string nbest(unsigned sent_number,unsigned pass,derivation_type d=derivation_type(),unsigned nbest_rank=0,extra_english_output_t *eo=NULL) const
    {
        std::ostringstream ret;
        print_nbest(ret,sent_number,pass,d,nbest_rank,eo);
        return graehl::chomp(ret.str());
    }

    template <class Ostream>
    void print_score(Ostream &o,score_t s) const
    {
        if (opt.scores_neglog10)
            o << s.neglog10();
        else
            o << s;
    }

    template <class Ostream,class P>
    void print_score(Ostream &o,P const& prefix,score_t s) const
    {
        o << prefix;
        o << (opt.scores_neglog10 ? "cost=" : "score=");
        print_score(o,s);
    }

    template <class Ostream>
    void print_maybe_tree_quote(Ostream &o,std::string const& word) const
    {
        if (opt.quote_always or not opt.quote_never)
            graehl::out_always_quote(o,word);
        else if (opt.quote_never)
            o << word;
        else
            graehl::out_quote(o,word,graehl::char_predicate_ref(opt.needs_quote));
    }

    template <class Ostream>
    void print_maybe_quote(Ostream &o,std::string const& word) const
    {
        if (opt.quote_always)
            graehl::out_always_quote(o,word);
        else if (opt.quote_never)
            o << word;
        else
            graehl::out_quote(o,word,graehl::char_predicate_ref(opt.needs_quote));
    }

    template <class Ostream>
    void print_maybe_quote(Ostream &o,indexed_token token) const
    {
        print_maybe_quote(o,this->label(token));
    }

    template <class Ostream>
    void print_maybe_tree_quote(Ostream &o,indexed_token token) const
    {
 	print_maybe_tree_quote(o,this->label(token));
    }

    template <class Ostream>
    void print_foreign_tree(Ostream &o,derivation_type deriv,bool show_ruleid=true,bool show_f_spans_pre=false) const
    {
        if (!deriv) return;
        edge_type const&e=deriv_edge(deriv);
        assert(e.has_syntax_id(grammar()));
        precondition_children ch(deriv,grammar());
        indexed_syntax_rule const& s=e.get_syntax(grammar());
        graehl::word_spacer sp;
        if (show_f_spans_pre)
            o << e.span();
        if (is_nonterminal(e.root()))
            if (opt.lisp_style_trees)
                o << '(' << sp;
        assert(e.has_syntax_id(grammar()));
        if (show_ruleid)
            o << e.syntax_id(grammar()) << ':';
        print_maybe_quote(o,s.lhs_root_token());
        if (!opt.lisp_style_trees)
            o << '(';
        assert(ch.size()==s.rhs_size());
        typename precondition_children::const_iterator i_deriv=ch.begin();
        for (indexed_syntax_rule::rhs_iterator i=s.rhs_begin(),end=s.rhs_end();
             i!=end;++i,++i_deriv) {
            o << sp;
            if (i->indexed())
                print_foreign_tree(o,*i_deriv,show_ruleid,show_f_spans_pre);
            else
                print_maybe_quote(o,i->get_token());
        }
        if (is_nonterminal(e.root()))
            o << ')';
    }

    template <class Ostream>
    void print_foreign_derivation(Ostream& o, edge_type const& e) const
    {
        if (e.is_null()) return;
        graehl::word_spacer sp;
        if (e.root().type() != foreign_token) {
            if (opt.lisp_style_trees) o << '(' << sp;
            if ( opt.print_foreign_tree_ruleid
                 && e.root().type() != virtual_tag_token )
                o << e.syntax_id(grammar()) << ':';
            print_maybe_quote(o,e.root());
            o << ':' << e.score() << ':' << e.inside_score();
            if (!opt.lisp_style_trees) o << '(';
            if (e.has_first_child()) {
                print_foreign_derivation(o,e.get_child(0));
                if (e.has_second_child()) {
                    o << sp;
                    print_foreign_derivation(o,e.get_child(1));
                }
            }
            o << ')';
        } else {
            print_maybe_quote(o,e.root());
        }

    }

    template <class Ostream>
    unsigned print_english_tree(Ostream &o,derivation_type deriv,unsigned span_left=0)
        const
    {
        bool show_f_spans_pre=false;
        bool show_fe_spans_post=opt.show_spans;
        if (!deriv) return 0;
        edge_type const&e=deriv_edge(deriv);
        assert(e.has_syntax_id(grammar()));
        precondition_children children(deriv,grammar());
        if (show_f_spans_pre) o << e.span();
        span_t const& fspan=e.span();
        unsigned yield_len=print_english_tree(o,*e.get_syntax(grammar()).lhs_root(),children,span_left);
        if (show_fe_spans_post)
            o << fspan<<span_t(span_left,span_left+yield_len);
        return yield_len;
    }

    typedef typename interp_type::precondition_children precondition_children;
    template <class Ostream>
    unsigned print_english_tree(Ostream &o,indexed_syntax_rule::tree_node const& node,precondition_children const& children,unsigned span_left) const
    {
        if (node.indexed())
            return print_english_tree(o,children[node.index()],span_left);
        if (node.lexical()) {
            print_maybe_tree_quote(o,node.get_token());
            return 1;
        }
        graehl::word_spacer sp;
        if (opt.lisp_style_trees)
            o << '(' << sp;
        print_maybe_quote(o,node.get_token());
        if (!opt.lisp_style_trees)
            o << '(';
        unsigned yield_len=0;
        for (indexed_syntax_rule::lhs_children_iterator
                 i=node.children_begin(),e=node.children_end();i!=e;++i) {
            o << sp;
            yield_len+=print_english_tree(o,*i,children,span_left+yield_len);
        }
        o << ')';
        return yield_len;
    }

    /// \todo Move this to be a method of edge_type
    /// \todo Different handling if e.root().type() == foreign_token?
    template <class Ostream>
    void print_edge_information(Ostream &o, edge_type const& e, std::string header="") const
  {
	if (e.is_null()) return;
	o << header;
	o << " / root "; print_maybe_quote(o, e.root());
	o << " / type " << e.root().type();
	o << " / span " << e.span();
	o << " / length " << e.span().size();
	o << " / score " << e.score();
	o << " / inside_score " << e.inside_score();
	o << " / children " << e.child_count();
    o << "\n";
	if (e.has_first_child())
      print_edge_information(o, e.get_child(0), header);
	if (e.has_second_child())
      print_edge_information(o, e.get_child(1), header);
  }


};

///\}

} //sbmt

//#include <sbmt/impl/derivation.ipp>

#endif

