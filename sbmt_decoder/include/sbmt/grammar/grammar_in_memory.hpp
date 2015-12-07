#ifndef   SBMT_GRAMMAR_GRAMMAR_IN_MEMORY_IMPL
#define   SBMT_GRAMMAR_GRAMMAR_IN_MEMORY_IMPL

#include <sbmt/token.hpp>
#include <sbmt/range.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/grammar/property_construct.hpp>
#include <sbmt/grammar/syntax_rule.hpp>
#include <sbmt/grammar/brf_reader.hpp>
#include <sbmt/grammar/rule_input.hpp> // also defines binary_rule
#include <sbmt/feature/feature_vector.hpp>
#include <sbmt/logmath.hpp>
#include <sbmt/grammar/tag_prior.hpp>
#include <sbmt/grammar/alignment.hpp>
#include <sbmt/grammar/text_features_byid.hpp>

#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/functional/hash.hpp>
#include <boost/scoped_ptr.hpp>

#include <graehl/shared/string_to.hpp>
#include <sbmt/printer.hpp>
#include <gusc/iterator/concat_iterator.hpp>


namespace sbmt {

struct grammar_in_memory;

struct subgrammar_dict_markers {
    indexed_token tag_end;
    indexed_token virtual_end;
    indexed_token native_end;
    indexed_token foreign_end;
};

/*
  Probably too many geographically distant function objects and callbacks are
  used, but here's what happens with feature vectors:

grammar/grammar_in_memory.hpp:

scored_syntax: syntax_rule, combined score_t, and feature_vector_byid - sparse
(id,score_t) vector (grammar/features_byid.hpp)

vector is initialized from map of name->score_t, using an
in_memory_token_storage (sbmt/token/in_memory_token_storage.hpp) to get int. ids
from name

feature_accum_byid - complete vector with max index = max id.  uses
accumulate_at_pairs (graehl/shared/assoc_container.hpp) to multiply (add logs)
the sparse vectors onto the full vector.  default value (mult. identity) is
given by the accumlate_multiply functor which sets = 1 (good).

grammar_in_memory.cpp: syntax_rule_action (constructs scored_syntax),
binary_rule_action (computes and sets heuristic and inside scores)


reading a grammar:

#include <sbmt/grammar/grammar_in_memory.hpp>
brf_stream_reader (parses rules from brf text)

#include <sbmt/grammar/brf_archive_io.hpp>
brf_archive_writer (saves archive)
brf_archive_reader (loads archive)
*/
typedef text_feature_vector_byid texts_type;
typedef feature_vector scores_type;
////////////////////////////////////////////////////////////////////////////////
///
/// an implementation detail.  this structure holds
/// - a syntax rule (xrs rule)
/// - an associated numeric feature vector, where the feature names have been indexed
///   for space efficiency
/// - a textual feature vector (empty unless keep_texts=true)
/// - alignment (null, unless keep_align=true)
/// - a combined score (the geometric inner product of the feature vector with
///   tuned feature weights
///
////////////////////////////////////////////////////////////////////////////////
struct scored_syntax {
    /// Score_Pairs_type is any container type representing a feature vector,
    /// that is, a collection of (feature-name,feature-value) pairs
    scored_syntax( indexed_syntax_rule const& rule_
                 , texts_type const& textmap
                 , size_t align_name
                 , bool keep_texts=true
                 , bool keep_align=true
                 , bool keep_headmarker=false )
        : rule(rule_)
    {
      contig_id=(unsigned)-1;
        if (keep_texts) texts = textmap;
        if (keep_align) {
            bool found = false;
            texts_type::const_iterator
                i=textmap.begin(),
                e=textmap.end();
            for (;i!=e;++i) {
                if (i->first == (unsigned)align_name) {
                    align.set(i->second);
                    found = true;
                    break;
                }
            }
            if (not found) align = rule.default_alignment();
        }
    }

    indexed_syntax_rule rule;
    score_t             score_; // not meant to be used outside grammar...
                                // only for book-keeping the rule_info heuristic
    texts_type texts;
    alignment align;
  unsigned contig_id; // for infos associating data without having to hash.
  bool no_contig_id() const {
    return contig_id==(unsigned)-1;
  }
  void set_contig_id(unsigned i) {
    //assert(no_contig_id()); // no: because the same global rules will get reinit multiple times (each time a sblm_info_factory is built)
    contig_id=i;
  }

  syntax_id_type syntax_id() const { return rule.id(); }
    //boost::uint8_t  subgrammar_id;
    //std::string headmarker;

    // for debugging
    template <class O> void print(O&o) const
    {
        o << "syntax ###";
        o << " align="<<align;
        //o<<" headmarker="<<headmarker;
    }


    // for really printing.
    // this signature is similar to other indexed objects, like
    // grammar(). dict() -> tf,grammar().feature_names() -> dict
    template <class O>
    void print( O&o
                , indexed_token_factory const& tf
                , feature_names_type const& dict
                , bool sbtm_score=true ) const
    {
        o << sbmt::print(rule,tf); // prints id=XYZ
        //print_scores(o,dict,sbtm_score);
    }
};

typedef boost::shared_ptr<scored_syntax> scored_syntax_ptr;

inline
std::ostream & operator << (std::ostream & o, scored_syntax const& me)
{
    me.print(o);
    return o;
}


namespace detail {

////////////////////////////////////////////////////////////////////////////////
///
///  warning: implementation detail only.
///  for those unfamiliar with multi_index_container, in order to store objects
///  based on some key internal to the object, you have to provide the
///  container with some way of extracting the key from the object.  that is
///  the purpose of this functor.
///
////////////////////////////////////////////////////////////////////////////////
struct scored_syntax_key_extractor {
    typedef syntax_id_type result_type;
    result_type operator() (scored_syntax_ptr const& ss) const
    {
        return ss->rule.id();
    }
};

/// chosen values make for quick hashing (better than 0, 1, 2)
enum rule_type_code {
     BINARY=0
   , HIDDEN=0x9e377001
   , UNARY=0x9e3779b9
   , TOPLEVEL_UNARY=0xbc9fe324
   , TOPLEVEL_BINARY=0xbc9fe001
};

////////////////////////////////////////////////////////////////////////////////
///
///  warning: implementation detail only.
///  binarized rules in the grammar can be accessed based on
///   # whether it is unary or not
///   # whether it is toplevel or not
///   # what the rhs[0] token is.
///
///  this rule_key class encompasses that information.  rules are stored in our
///  grammar_in_mem based on this key, which is extracted from a binarized_rule
///  using rule_info_key_extractor below.
///
////////////////////////////////////////////////////////////////////////////////
struct rule_key {
    rule_key() : type_code(HIDDEN) {}
    rule_key(indexed_token tok, bool toplevel, bool unary)
        : tok(tok)
    {
        if (toplevel) type_code = unary ? TOPLEVEL_UNARY : TOPLEVEL_BINARY;
        else type_code = unary ? UNARY : BINARY;
    }
    indexed_token tok;
    rule_type_code type_code;
};

struct rule_key2 {
    rule_key2()
        : type_code(HIDDEN) {}
    rule_key2(indexed_token tok1, indexed_token tok2, bool toplevel, bool unary)
        : tok1(tok1)
        , tok2(tok2)
    {
        if (toplevel) type_code = unary ? TOPLEVEL_UNARY : TOPLEVEL_BINARY;
        else type_code = unary ? UNARY : BINARY;
    }
    indexed_token tok1;
    indexed_token tok2;
    rule_type_code type_code;
};

////////////////////////////////////////////////////////////////////////////////

inline bool operator == (rule_key const& k1, rule_key const& k2)
{
    return k1.tok == k2.tok and k1.type_code == k2.type_code;

}

inline bool operator != (rule_key const& k1, rule_key const& k2)
{
    return !(k1 == k2);
}
inline std::size_t hash_value(rule_key const& k)
{
    std::size_t retval = 0;
    boost::hash_combine(retval,k.tok);
    boost::hash_combine(retval,k.type_code);
    return retval;
}

inline bool operator == (rule_key2 const& k1, rule_key2 const& k2)
{
    return k1.tok1 == k2.tok1
       and k1.tok2 == k2.tok2
       and k1.type_code == k2.type_code;
}

inline bool operator != (rule_key2 const& k1, rule_key2 const& k2)
{
    return !(k1 == k2);
}

inline std::size_t hash_value(rule_key2 const& k)
{
    std::size_t retval = 0;
    boost::hash_combine(retval,k.tok1);
    boost::hash_combine(retval,k.tok2);
    boost::hash_combine(retval,k.type_code);
    return retval;
}

////////////////////////////////////////////////////////////////////////////////
///
///  warning: implementation detail only.
///  rule_info should be named scored_binarized_rule, since it is just a
///  binary_rule<> with a feature-combined score attached.
///  it is the data node in grammar_in_mem 's big-hashmap-of-rules
///
////////////////////////////////////////////////////////////////////////////////
struct rule_info {
    rule_info( indexed_binary_rule const& rule
             , score_map_type const& scores_
             , texts_type const& texts_
             , weight_vector const& weights
             , score_t heuristic ); //for virtual rules

    rule_info( indexed_binary_rule const& rule
             , syntax_id_type sid
             , score_map_type const& scores_
             , texts_type const& texts_
             , weight_vector const& weights
             , score_t heuristic ); // for real rules


    void reweight_scores(weight_vector const& w);

    void construct_properties( property_constructors<> const& pc
                             , indexed_token_factory& dict
                             , feature_names_type const& fdict );

    indexed_binary_rule rule;
    syntax_id_type      syntax_id; // NULL_SYNTAX_ID if rule has no scored_syntax
    feature_vector      scores;
    texts_type          texts;
    score_t             score;
    score_t             heuristic;
    bool hidden;
};

////////////////////////////////////////////////////////////////////////////////
///
///  warning: implementation detail only
///  this is a functor whose sole job is to specify to
///  boost::multi_index_container how to index the rule_info objects, by
///  specifying what the rule's index key is.
///
///  boost::multi_index_container is a powerful abstraction of the standard
///  indexing containers (set, multiset, map, multimap).  you could accuse me
///  (michael) of over-using it.  consequently, our code has a lot of
///  key_extractor types in it, and even our own oa_hashtable uses the
///  key_extractor idea.
///
////////////////////////////////////////////////////////////////////////////////
struct rule_info_key_extractor {
    typedef rule_key result_type;
    rule_key operator()(rule_info const* info) const;
};

struct rule_info_double_key_extractor {
    typedef rule_key2 result_type;
    rule_key2 operator()(rule_info const* info) const;
};

struct rule_info_lhs_key_extractor {
    typedef indexed_token result_type;
    result_type operator()(rule_info const* info) const;
};

////////////////////////////////////////////////////////////////////////////////

class syntax_rule_action;
class binary_rule_action;

} // namespace detail


////////////////////////////////////////////////////////////////////////////////
///
///  really simple grammar implementation.  everything is just stored in memory
///  in a big hash table.
///
///  this grammar implements a weak CNF grammar (unary non-lexical productions
///  are allowed, as are lexical items in the binary rules.
///  this grammar also indexes its rules based on the left most token on the
///  right hand side, making it suitable only for bottom-up parsing
///  strategies.
///
///  mirroring syntax/brf rules, the grammar recognizes that some binarized
///  rules represent the roots of syntax-tree rules.  these rules can be
///  queried for their syntax via the grammar.
///
///  <h3> loading a grammar </h3>
///  grammar_in_mem does not worry about how a grammar is persistently stored.
///  it may be in some text file, some archived binary file, or could be in a
///  database waiting to be queried (although no such database exists yet).
///  grammar_in_mem uses the interface brf_reader to initialize itself.
///  Different brf_reader implementations correspond to differing ways of
///  loading a weak CNF grammar.  see brf_reader.hpp .
///
///  <h3> dictionaries </h3>
///  throughout the library, dictionaries are used to keep memory usage down.
///  any object that would have a string label (lexical items, non-terminals,
///  feature-values) stores its label in a dictionary, and keeps an integer
///  representation.  to recover the label the dictionary is queried.  see
///  token/token.hpp and token/indexed_token.hpp
///  grammar_in_mem has two dictionaries:  one provides label access to all
///  tokens in the grammar (tokens are the the symbols that make up rules),
///  and the other provides label access to the feature names in a feature
///  vector.
///
///  \todo iterator access to the syntax rules should be exposed.
///
////////////////////////////////////////////////////////////////////////////////
class grammar_in_mem
{
public:
  //tag_prior prior; // must load this before loading grammar
  double weight_tag_prior; // not used after loading grammar
    struct single_key{};
    struct double_key{};
    struct lhs_key{};
    struct subgrammar_key{};
    typedef boost::multi_index_container <
                detail::rule_info const*
              , boost::multi_index::indexed_by<
                    boost::multi_index::hashed_non_unique<
                        boost::multi_index::tag<single_key>
                      , detail::rule_info_key_extractor
                    >
                  , boost::multi_index::hashed_non_unique<
                        boost::multi_index::tag<double_key>
                      , detail::rule_info_double_key_extractor
                    >
                >
            > rule_info_multiset;
    typedef boost::multi_index_container <
                scored_syntax_ptr
              , boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<
                        detail::scored_syntax_key_extractor
                    >
                >
            > scored_syntax_set;

    feature_names_type & feature_names()
    { return feature_dict; }
    feature_names_type const& feature_names() const
    { return feature_dict; }
    std::string const& feature_name(feature_id_type feature_id) const
    { return feature_names().get_token(feature_id); }
    feature_id_type feature_id(std::string const& feature_name)
    { return feature_names().get_index(feature_name); }

//    typedef dictionary<in_memory_token_storage> dict_t;
    typedef indexed_token_factory dict_t;
    dict_t & dict()
    { return tokens; }
    dict_t const& dict() const
    { return tokens; }

    typedef scored_syntax_set::iterator syntax_iterator_;
    typedef gusc::sequence_of_iterators<
              std::list<scored_syntax_set> const
            , syntax_iterator_
            , gusc::begin_end_extractor
            > syntax_iterator;
    typedef boost::iterator_range<syntax_iterator> syntax_range;

    syntax_range all_syntax_rules() const
    {
        return syntax_range( syntax_iterator(syntax_rules, gusc::begin_end_extractor())
                           , syntax_iterator(syntax_rules, gusc::begin_end_extractor(), false)
                           )
                           ;
    }


    typedef indexed_syntax_rule syntax_rule_type;
    typedef scored_syntax scored_syntax_type;

    typedef indexed_token                 token_type;
    typedef indexed_binary_rule binary_rule_type;
    typedef detail::rule_info const*      rule_type;

    struct not_hidden {
        typedef bool result_type;
        bool operator()(detail::rule_info const* ri) const;
    };


    typedef boost::function<
              std::pair<
                rule_info_multiset::index<double_key>::type::iterator
              , rule_info_multiset::index<double_key>::type::iterator
              > (rule_info_multiset const&)
            > iterator_accessor2_;
    typedef gusc::sequence_of_iterators<
              std::list<rule_info_multiset> const
            , rule_info_multiset::index<double_key>::type::iterator
            , iterator_accessor2_
            > rule_iterator2_;

    typedef boost::function<
              std::pair<
                rule_info_multiset::iterator
              , rule_info_multiset::iterator
              > (rule_info_multiset const&)
            > iterator_accessor_;
    typedef gusc::sequence_of_iterators<
              std::list<rule_info_multiset> const
            , rule_info_multiset::iterator
            , iterator_accessor_
            > rule_iterator_;


    typedef boost::filter_iterator<
                not_hidden
              , rule_iterator_
            > rule_iterator;
    typedef boost::iterator_range<rule_iterator> rule_range;

    typedef boost::filter_iterator<
                not_hidden
              , rule_iterator2_
              > rule_iterator2;
    typedef boost::iterator_range<rule_iterator2> rule_range2;
//    typedef indexed_token_factory         token_factory_type;
    typedef dict_t &token_factory_type;


    /// number of binarized rules in the grammar
    std::size_t size() const;

    /// \name rule range functions
    /// These functions return a range of rules in the grammar based on the
    /// query type.
    //\{
    rule_range all_rules() const;
    /// \return all unary rules that have rhs as the constituent, excluding
    /// rules that have a toplevel token on the left hand side.
    rule_range unary_rules(token_type rhs) const;
    /// \return all binary rules that have first_rhs as the left most
    ///         constituent.  does not include toplevel rules.
    rule_range binary_rules(token_type first_rhs) const;

    rule_range2 binary_rules(token_type first_rhs, token_type second_rhs) const;
    /// \return all unary rules that have rhs as the constituent, and are
    ///         toplevel rules.
    rule_range toplevel_unary_rules(token_type rhs) const;
    /// \return all binary rules that have first_rhs as the left most
    ///         constituent, and are toplevel rules.
    rule_range toplevel_binary_rules(token_type first_rhs) const;

    rule_range2 toplevel_binary_rules( token_type first_rhs
                                     , token_type second_rhs ) const;

    /// \return all rules grouped by key2 (binary rule rhs).  this means same rhs binary rules are adjacent
    rule_range2 all_rules2() const;

    //\}

private:
    bool equal_rhs2(rule_type r1,rule_type r2) const;

 public:

    // V::visit_binary_by_rhs(grammar_in_mem const& g,rule_range2 rr)
    template <class V>
    void visit_binary_by_rhs(V & v) const
    {
        rule_range2 all=all_rules2();
        rule_iterator2 ritr = boost::begin(all), rend = boost::end(all);
        rule_iterator2 last_it;
        rule_type last=NULL;

        for (;;++ritr) {
            rule_type r=*ritr;
            if (ritr == rend || !last || rule_rhs_size(r)!=2 || !equal_rhs2(last,r)) {
                if (last)
                    // end of a range
                    v.visit_binary_by_rhs(*this,rule_range2(last_it,all.begin()));

                // advance to first binary rule to start next range
                for (; ritr != rend; ++ritr) {
                    rule_type r=*ritr;
                    if (rule_rhs_size(r)==2) {
                        last_it=ritr;
                        last=r;
                        goto next;
                    }
                }
                return;
            next: ;
            }
        }
    }



    std::string const& token_label(token_type t) const
    { return tokens.label(t); }

    grammar_in_mem();
    ~grammar_in_mem();

    /// load a grammar from some persistent memory source, using reader as
    /// the interface to the source.  combine_scores is used to turn the
    /// feature vectors attached to syntax rules into a single score.
    /// unused nonnumeric syntax rule features will be remembered if keep_texts; align={{{...}}} will be parsed if keep_align (note: keep_text
    void load( brf_reader& reader
             , fat_weight_vector const& w
             , property_constructors<dict_t> const& prop_constructors
             , bool keep_texts=true
             , bool keep_align=true
             , bool keep_headmarker=true);
    void push(brf_reader& reader, bool keep_texts=true,bool keep_align=true,
              bool keep_headmarker = true);
    void pop();

    bool        equal_foreign_sides(rule_type r1, rule_type r2) const;

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  \name binarized rule access methods
    ///
    ///  why are lhs, rhs, rhs_size, score, score_estimate, scoreable, etc
    ///  implemented as member functions of grammar !?! because it keeps the
    ///  interface generic.  its not that unnatural, because rules are owned
    ///  by grammars anyway, and their relationship can be considered tighter
    ///  than that of a simple collection.  and this way a rule could be
    ///  represented as a thin handle into a database, or a fatter handle with
    ///  some data internal to the rule, and some data stored externally.  the
    ///  user doesnt have to care which is which, and the implementer is free
    ///  to represent the internals as flexibly as wished.
    ///
    ////////////////////////////////////////////////////////////////////////////
    ///
    //\{
    token_type  rule_lhs(rule_type r) const;

  bool is_complete_rule(rule_type r) const; // does this imply having syntax id?

    binary_rule_type const& binary_rule(rule_type r) const;

    std::size_t rule_rhs_size(rule_type r) const;

    token_type  rule_rhs(rule_type r, std::size_t index) const;

    template <class Type>
    Type const& rule_property(rule_type r, size_t idx) const
    { return binary_rule(r).template get_property<Type>(idx); }

    bool rule_has_property(rule_type r, size_t idx) const;

    score_t rule_score_estimate(rule_type r) const;

    score_t rule_score(rule_type r) const;

    feature_vector const& rule_scores(rule_type r) const;

    texts_type rule_text(rule_type r) const;

    //\}

    /// primary syntax rule access:
    /// no non-debug checking
    scored_syntax_type const& get_scored_syntax(syntax_id_type id) const
    {
        assert(id!=NULL_SYNTAX_ID);
        BOOST_FOREACH(scored_syntax_set const& sset, syntax_rules) {
            scored_syntax_set::const_iterator i=sset.find(id);
            if (i != sset.end()) return **i;
        }
        throw std::runtime_error("syntax rule not found");
    }


    rule_type insert_scored_syntax( indexed_syntax_rule
                                  , score_map_type const& svec
                                  , texts_map_type const& text_feats=texts_map_type()
                                  );

    rule_type insert_terminal_rule( indexed_token
                                  , score_map_type const& svec
                                  , texts_map_type const& text_feats=texts_map_type()
                                  );
    void erase_terminal_rules();

    /// \name printer-helper functions
    /// use
    /// the free function print instead, with grammar_in_mem as the dictionary
    /// (unless you want to suppress sbtm_score printing via optional arg) \see
    /// printer.hpp //\{
    template <class O>
    void print_syntax_rule(O&o,scored_syntax_type const& ss,bool sbtm_score=true) const
    {
        ss.print(o,dict(),feature_names(),sbtm_score);
    }
/*
    template <class O>
    void print_syntax_rule_scores(O&o,scored_syntax_type ss,bool sbtm_score=true) const
    {
        ss.print_scores(o,feature_names(),sbtm_score);
    }
*/

    template <class O>
    void print_syntax_rule_by_id(O&o,syntax_id_type id,bool sbtm_score=true) const
    {
        print_syntax_rule(o,get_scored_syntax(id),sbtm_score);
    }
    //\}

    /// \deprecated these functions should be made free functions, as they
    /// could be completely described using only the class interface,
    /// simply by exposing iterator access to syntax rules, which would be
    /// more useful.
    ///\{
    template <class V>
    void visit_all_scored_syntax(V v) const
    {
        syntax_iterator i, e;
        boost::tie(i,e) = all_syntax_rules();
        for (; i != e; ++i) v(**i);
    }

    // output iterator, e.g. ostream_output_iterator, gets the string for each native token
    template <class OI>
    void output_native_vocab(OI &o) const
    {
        typename dict_t::range r=tokens.native_words();
        for (typename dict_t::iterator i=r.begin(),e=r.end();i!=e;++i)
            *o++=tokens.label(*i);
    }


    /// NULL_SYNTAX_ID if not a syntax rule
    static syntax_id_type get_syntax_id(rule_type r) //const
    { return r->syntax_id; }

    //    { return r==NULL_GRAMMAR_RULE_ID ? NULL_SYNTAX_ID : r->syntax_id; }
    //FIXME: enforce check for Edge::has_rule_id() in all test cases, don't check for null here?

    static binary_rule_type const& binary_rule(grammar_rule_id id) //const
    { return rule(id)->rule; }

    static rule_type rule(grammar_rule_id id) //const
    { return (rule_type)id; }

    static grammar_rule_id id(rule_type r) //const
    { return (grammar_rule_id)r; }

    /// redundant access:
    indexed_syntax_rule const& get_syntax(syntax_id_type id) const
    { return get_scored_syntax(id).rule; }

    indexed_syntax_rule const& get_syntax(rule_type r) const
    { return get_syntax(get_syntax_id(r)); }

    scored_syntax_type const& get_scored_syntax(rule_type r) const
    { return get_scored_syntax(get_syntax_id(r)); }

    //score_t get_syntax_score(syntax_id_type id) const
    //{ return get_scored_syntax(id).score; }

    /*
    scores_type const&
    get_syntax_scores(syntax_id_type id) const
    { return get_scored_syntax(id).scores; }
    */


    std::string const& label(token_type t) const { return tokens.label(t); }

    double weight_virtual_completion; // not used after loading grammar.  max prob of real rule using virtual is raised to this power, and multiplied into virtual rule's heuristic.  //FIXME: UNUSED.

    score_t tag_prior_bonus; // multiplied into (nonvirtual) tag heuristic.  greater than 1 <=>  favor tags more than virtuals

    // uses prior, weight_tag_prior, weight_virtual_completion,
    // and score_combiner to arrive at new inside and heuristic costs for
    // all binary rules.


    void swap(grammar_in_mem &o)
    {
        swap_impl(o);
    }

    void update_weights(fat_weight_vector const& w, property_constructors<dict_t> const& pc);

    double feature_weight(std::string const& name) const
    {
        return get(weights,feature_dict,name);
    }

    void set_constant_heuristic(score_t h=as_one());

    score_t weighted_prior(indexed_token const& root) const
    {
        return root.type() != virtual_tag_token ?
               score_t(prior[root],weight_tag_prior) :
               as_zero();
    }

    score_t weighted_prior(scored_syntax_type const& ssyn) const
    {
        return weighted_prior(ssyn.rule.lhs_root_token());
    }

    void load_prior(std::istream& in, score_t floor = 1e-9, double smooth = 1.0, double power = 1.0, double wt = 1.0);
    void load_prior(score_t floor = 1.0, double wt = 1.0);

    // used anyway when loading rules.
    // keep around for validation of feature weights later
    weight_vector weights; // use update_weights any time this changes
    weight_vector const& get_weights() const {return weights;}
    property_constructors<dict_t> prop_constructors;
private:
    void recompute_scores();
    void recompute_heuristics();
    void swap_impl(grammar_in_mem &o);
    score_t                             prior_floor;
    double                              prior_smooth;
    double                              prior_pow;
    std::string                         prior_string;
    tag_prior                           prior;
    std::list<rule_info_multiset>       rules;
    dict_t                              tokens;
    feature_names_type feature_dict;
    std::list<scored_syntax_set>        syntax_rules;
    syntax_id_type                      min_syntax_id;
    std::vector<subgrammar_dict_markers> subgrammar_dict_max;

    friend class detail::syntax_rule_action;
    friend class detail::binary_rule_action;
};

// should be found by sbmt/print.hpp: print(ss,gram)
template <class O> inline
void print(O &o,scored_syntax const& ss,grammar_in_mem const& gram)
{
    gram.print_syntax_rule(o,ss);
}

template <class O> inline
void print(O &o,syntax_id_type id,grammar_in_mem const& gram)
{
    gram.print_syntax_rule_by_id(o,id);
}

} // namespace sbmt

#endif // SBMT_GRAMMAR_GRAMMAR_IN_MEMORY_IMPL
