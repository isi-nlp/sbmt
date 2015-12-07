#ifndef   SBMT__EDGE__EDGE_HPP
#define   SBMT__EDGE__EDGE_HPP

//#define DEBUG_INFO

#include <sbmt/printer.hpp>

#include <sbmt/edge/edge_fwd.hpp>
#include <sbmt/edge/edge_equivalence.hpp>
#include <sbmt/logmath.hpp>
#include <sbmt/span.hpp>
#include <sbmt/sentence.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/token/token_io.hpp>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <stdexcept>
#include <string>

#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/reconstruct.hpp>
#include <graehl/shared/is_null.hpp>

#include <sbmt/multipass/cell_heuristic.hpp>

#include <sbmt/grammar/grammar_in_memory.hpp> //TODO: pass grammar as template arg

#include <gusc/functional.hpp>
#include <gusc/string/escape_string.hpp>
#ifdef DEBUG_INFO
# define DEBUG_INFO_OUT std::cerr
#endif

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  \defgroup edges Edges
///
///  An edge represents the root of a partially completed parse.  They contain
///  enough information to recover a forest of partial parses. Edges are
///  built up from smaller edges by applying compatible grammar rules.
///
///  Edges contain
///   - a span, to describe how much of the sentence is covered
///   - a token representing the lhs of the rule creating this edge
///   - if the rule represents a syntax rule, then the id of that rule
///     so that english parses can be recovered.
///   - inside, and heuristic scores.
///   - state information, for beyond translation-model-only scoring.
///   - references to the constituent edges that made the edge.
///
///  Edges are created in edge_factory, which is where the logic for how
///  constituent edges + rules make new edges, and how state information
///  is built up.
///
///  Edges are equivalent if
///   - they cover the same span of words.
///   - they are rooted with the same token
///   - they have equivalent state information.
///
///  Equivalent edges can be collected in an edge_equivalence.  This is
///  useful for forest representation, and in fact, when an edge is created from
///  constituents, its actually created out of edge_equivalence constituent
///  objects.
///
///  An edge dominates another edge if they are equivalent and one has a higher
///  score
///
///  <h3>State Information</h3>
///  In order to apply differing scoring techniques, and especially for
///  applying ones that require more than just the translation-model information
///  additional state information may be needed.  Examples include trying to
///  apply an n-gram score or sblm score.
///
///  State information is made by an info_factory.  An edge factory, when
///  building from constituents, will create new state info from constituent
///  infos, as well as corresponding info scores and heuristics.
///
///  People wishing to change the way edges are scored probably will do so by
///  creating a new state info type / factory.
///
////////////////////////////////////////////////////////////////////////////////
///\{

template <class InfoT>
class edge : private InfoT
{
public:
    typedef edge<InfoT> self_type;
    typedef edge_equivalence_impl<self_type> edge_equiv_impl_type;
    typedef boost::intrusive_ptr<edge_equiv_impl_type> equiv_p;

    typedef self_type edge_type;
    typedef InfoT info_type;
    typedef edge_equivalence<self_type>             edge_equiv_type;


    score_t inside_score() const { return inside; }
    score_t delta_inside_score() const
    {
        if (!has_first_child())
            return inside;
        score_t ret=inside;
        ret /= first_child().inside_score();
        if (!has_second_child())
            return ret;
        return ret/second_child().inside_score();
    }

    template <class ADerivedT>
    ADerivedT const& cast_info() const
    {
        return static_cast<ADerivedT const&>(info());
    }

    score_t score() const { return total; }
    score_t outside_score() const { return total; } // note! this is where real global outside score is stored after sbmt/forest/outside_score is computed.  in this case, heuristic_score() and score() are simply wrong.  you need outside_score() * inside_score() to get the 1best using it ...

    score_t &score() { return total; } // feel free to use as scratch after parsing

    score_t heuristic_score() const { return total/inside; }

    span_t const& span() const;
    span_index_t  split() const
    { return has_first_child() ? first_child().span().right() : 0; }


    indexed_token root() const { return rt; } // note: redundant with gram -> rule_topology -> lhs(), except foreign words?


    template <class G>
    syntax_id_type syntax_id(G const& g) const
    {
        //    return syn_rule_id;
        return has_rule_id() ? g.get_syntax_id(g.rule(id)) : NULL_SYNTAX_ID;
    }
    /*
    syntax_id_type syntax_id() const
    {
        //((detail::rule_info *)id)->syntax_id;
        //FIXME: new interface requiring grammar - does force_grammar work the same?  pointer to pair of grammar_in_mem rule, and other info ... so ought to
        return has_rule_id() ?
            grammar_in_mem::get_syntax_id(grammar_in_mem::rule(id))
            : NULL_SYNTAX_ID;
    }
    */

    bool has_rule_id() const
    {
        //return has_first_child();
        return id != NULL_GRAMMAR_RULE_ID;
    }
  void print(std::ostream &o) const {
    o<<"rule_id=";
    if (has_rule_id())
      o<<id;
    else
      o<<"NULL";
  }
  friend inline std::ostream& operator<<(std::ostream &o,edge const& e) {
    e.print(o);
    return o;
  }

    grammar_rule_id rule_id() const
    {
        return id;
    }

/*    template <class G>
    bool has_syntax_id(G const& g) const
    {
        return has_rule_id() && syntax_id(g)!=NULL_SYNTAX_ID;
    }
*/
    template <class Grammar>
    bool has_syntax_id(Grammar const& gram) const
    {
        return /*has_rule_id() && */ syntax_id(gram)!=NULL_SYNTAX_ID;
    }

    template <class Grammar>
    typename Grammar::scored_syntax_type
    get_scored_syntax(Grammar const& gram) const
    {
        assert(has_syntax_id(gram));
        return gram.get_scored_syntax(syntax_id_unchecked(gram));
    }

    template <class Grammar>
    typename Grammar::syntax_rule_type
    get_syntax(Grammar const &gram) const
    {
        assert(has_syntax_id(gram));
        return gram.get_syntax(syntax_id_unchecked(gram));
    }

    bool equal_to(edge const& other) const;
    std::size_t hash_value() const;

    /// equivalent to: // return child[0] && child[0]->empty();
    bool is_lexical() const { return sbmt::is_lexical(rt);} // no children
    /// implies is_lexical
    bool is_foreign() const { return sbmt::is_foreign(rt); }

    bool is_unary() const; // one child equiv set
    bool is_binary() const;

    /// 0, 1, or 2
    unsigned child_count() const;

    //////////////////////////////////////////////////////////////////////////
    ///\{
    /// \return is_unary() or is_binary() - but *more efficient*
    ///
    //////////////////////////////////////////////////////////////////////////
    bool has_first_child() const;

    //////////////////////////////////////////////////////////////////////////
    ///
    /// \return is_binary()
    ///
    //////////////////////////////////////////////////////////////////////////
    bool has_second_child() const;

    //////////////////////////////////////////////////////////////////////////
    ///
    ///  \return the edge equivalence classes that were the constituents
    ///  that built this edge
    ///
    //////////////////////////////////////////////////////////////////////////
    ///\{
    edge_equiv_type first_children() const;
    edge_equiv_type second_children() const;
    //////////////////////////////////////////////////////////////////////////
    ///
    /// \return first_children().representative()
    ///
    //////////////////////////////////////////////////////////////////////////
    edge_type const& first_child() const;
    //////////////////////////////////////////////////////////////////////////
    ///
    /// \return second_children().representative()
    ///
    //////////////////////////////////////////////////////////////////////////
    edge_type const& second_child() const;



    /// child_i=0: first_child; =1: second
    edge_type const& get_child(unsigned child_i) const;
    edge_equiv_type get_children(unsigned ci) const;

    /// if first is foreign token, then var_i=0 selects second.  otherwise same as get_child(var_i)
    edge_type &get_lmchild(unsigned var_i) const;

    /// new_child must be the equivalent to get_child(i).  used for kbest
    void adjust_child(edge_equiv_type new_child,unsigned child_i);

    /// note: rule must have same root, and info (and thus heuristic) must be same
    void adjust(edge_equiv_type child1
               ,edge_equiv_type child2
               ,score_t inside_score
                , grammar_rule_id ruleid
        )
    {
        child[0]=child1.get_shared();
        child[1]=child2.get_shared();
        heuristic()=total/inside;
        inside=inside_score;
        finish();
        id=ruleid;
    }
    void adjust_score(score_t inside_score)
    {
        heuristic()=total/inside;
        inside=inside_score;
        finish();
    }

    info_type const& get_info() const { return *this; }
    info_type const& info() const { return *this; }
    info_type& info()  { return *this; }

    bool is_null() const
    {
        return rt.is_null();
    }
    void set_null()
    {
        rt.set_null();
    }
    MEMBER_IS_SET_NULL


    // make private ? edge factory uses.  test cases use.
    void set_lexical(indexed_token const& t, span_t s,score_t score=as_one(), info_type const& n=info_type())
    {
        rt=t;
        spn = s;
        child[0]= NULL;
        child[1] = NULL;
        total.set(score);
        inside.set(score);
//        syn_rule_id=NULL_SYNTAX_ID;
        id=NULL_GRAMMAR_RULE_ID;
        info() = n;
#ifdef DEBUG_INFO_PORTION
//    info_portion.set(as_one());
#endif
    }

    edge() {}
private:
    template <class G>
    syntax_id_type syntax_id_unchecked(G const& g) const
    {
        //    return syn_rule_id;
        return g.get_syntax_id(g.rule(id));
    }

    score_t & heuristic() // only use this before finish()
    { return total; }
    void finish()
    {
#ifdef DEBUG_INFO
        DEBUG_INFO_OUT << "inside="<<inside<<" h="<<total<<" ";
#endif
        total *= inside;
    }

    template <class GT>
    void set_unary_part(GT &gram,
                        typename GT::rule_type r, edge_equiv_type const& c)
    {
        rt=gram.rule_lhs(r);
        id=gram.id(r);
//        syn_rule_id=gram.get_syntax_id(r);

        // the below is now handled in edge_factory instead.
//        heuristic() = pow(gram.rule_score_estimate(r),h_wt);
//        inside = pow(gram.rule_score(r),i_wt) * c.representative().inside;
        child[0]=c.get_shared();
    }
    void finish_binary(edge_equiv_type const&c2)
    {
        inside *= c2.representative().inside;
        child[1]=c2.get_shared();
        spn = combine(child[0]->span(),c2.span());
        finish();
    }
    void finish_unary()
    {
        child[1] = NULL;
        spn = child[0]->span();
        finish();
    }

    template <class InfoFactory> friend class edge_factory;
    template <class InfoFactory> friend class edge_generator;

    /// only edge_factory can make new edges (but you may copy and adjust_child)
    edge( indexed_token rt
        , grammar_rule_id id
        , info_type info
        , score_t inside
        , score_t heuristic
        , equiv_p child0
        , equiv_p child1
        )
    : info_type(info)
    , rt(rt)
    , id(id)
    , total(heuristic)
    , inside(inside)
    , spn(child1 ? combine(child0->span(), child1->span()) : child0->span())
    {
        child[0] = child0;
        child[1] = child1;
        assert(child0);
        this->inside *= child0->representative().inside_score();
        if (child1) this->inside *= child1->representative().inside_score();

        this->total *= this->inside;
    }

    /// \todo: rt and syn_rule_id are redundant (given grammar) with ruleid.
    /// but would need pseudo-rule added to grammar for each initial chart
    /// item (foreign word)
    indexed_token         rt;

    grammar_rule_id id;

    equiv_p               child[2];
    score_t               total; /// = heuristic * inside
    score_t               inside;
    span_t                spn; //FIXME: only need to record split point?  but span indices are 2 byte so not too bad

};

///\}

struct stop_on_xrs {
    typedef bool result_type;
    template <class Info>
    bool operator()(edge<Info> const& e) const
    {
        return e.root().type() == tag_token;
    }
};

////////////////////////////////////////////////////////////////////////////////
///
///  \defgroup edge_functors edge functors
///  \ingroup edges
///  useful functors to get representative data from edge equivalence. suitable
///  for containers, in particular: boost::multi_index_container and
///  oa_hashtable
///
////////////////////////////////////////////////////////////////////////////////
///\{

////////////////////////////////////////////////////////////////////////////////
///
///  functor to extract representative edge from an edge_equivalence.
///  the representative edge is the edge with the best score in an
///  edge_equivalence.
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT>
struct representative_edge
{
    typedef edge_equivalence<EdgeT> argument_type;
    typedef
        typename edge_equivalence<EdgeT>::edge_type const&
        result_type;
    result_type operator()(argument_type const& arg) const
    {
        return arg.representative();
    }
};

////////////////////////////////////////////////////////////////////////////////
///
///  functor to extract score from the representative edge of an
///  edge_equivalence.  the representative edge is the edge with the best score
///  in an edge_equivalence.
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT>
struct representative_score
{
    typedef edge_equivalence<EdgeT> argument_type;
    typedef score_t                 result_type;
    result_type operator()(argument_type const& arg) const
    {
        return arg.representative().score();
    }
};

////////////////////////////////////////////////////////////////////////////////
///
///  functor to extract the hash value from the representative edge of an
///  edge_equivalence.  the representative edge is the edge with the best score
///  in an edge_equivalence.
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT>
struct representative_hash
{
    typedef edge_equivalence<EdgeT> argument_type;
    typedef std::size_t             result_type;
    result_type operator()(argument_type const& arg) const
    {
        return arg.representative().hash_value();
    }
};

template <class EdgeT>
struct representative_equal
{
    typedef edge_equivalence<EdgeT> argument_type;
    typedef bool                    result_type;
    result_type operator()(argument_type const& a1, argument_type const& a2) const
    {
        return a1.representative() == a2.representative();
    }
};

////////////////////////////////////////////////////////////////////////////////
///
///  functor to insert an edge into an edge_equivalence
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT>
struct edge_inserter
{
    typedef edge_equivalence<EdgeT> argument_type;
    typedef void                    result_type;

    void operator()(argument_type& arg)
    {
        arg.insert_edge(*e);
    }

    edge_inserter(EdgeT const& e)
    : e(&e) {}
private:
    EdgeT const* e;
};

////////////////////////////////////////////////////////////////////////////////
///
///  functor to merge two edge_equivalence objects
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT>
struct edge_equivalence_merge
{
    typedef edge_equivalence<EdgeT> argument_type;
    typedef void                    result_type;
    void operator()(argument_type& arg)
    {
        arg.merge(*eq);
    }
    edge_equivalence_merge(argument_type const& eq)
    : eq(&eq) {}
private:
    argument_type const* eq;
};

////////////////////////////////////////////////////////////////////////////////
///
///  functor to decide if edge1.score() > edge2.score() or
///  eq.representative().score() > eq.representative().score()
///
////////////////////////////////////////////////////////////////////////////////
template <class ET>
struct greater_edge_score
{
    typedef edge_equivalence<ET> edge_equiv_type;
    typedef bool                 result_type;
    bool operator()( edge_equiv_type const& e1, edge_equiv_type const& e2) const
    {
        return e1.representative().score() > e2.representative().score();
    }
    bool operator()( ET const& e1, ET const& e2) const
    {
        return e1.score() > e2.score();
    }
};

////////////////////////////////////////////////////////////////////////////////
///
///  functor to decide if edge1.score() < edge2.score() or
///  eq.representative().score() < eq.representative().score()
///
////////////////////////////////////////////////////////////////////////////////
template <class ET>
struct lesser_edge_score
{
    typedef edge_equivalence<ET> edge_equiv_type;
    typedef bool                 result_type;
    bool operator()( edge_equiv_type const& e1, edge_equiv_type const& e2) const
    {
        return e1.representative().score() < e2.representative().score();
    }
    bool operator()( ET const& e1, ET const& e2) const
    {
        return e1.score() < e2.score();
    }
};

///\}

/// \addtogroup edges
///\{

////////////////////////////////////////////////////////////////////////////////

template <class InfoT>
std::size_t hash_value(edge<InfoT> const& e) { return e.hash_value(); }

template <class InfoT>
bool operator==(edge<InfoT> const& e1, edge<InfoT> const& e2)
{ return e1.equal_to(e2); }

template <class InfoT>
bool operator!=(edge<InfoT> const& e1, edge<InfoT> const& e2)
{ return !(e1 == e2); }

////////////////////////////////////////////////////////////////////////////////


/*
  //TODO: stuff in factories -> constructors, taking info (lm) context as argument?  but keep separate allocator object so diff. threads can have diff allocs.  or single alloc when running mthead that uses threadlocal (global) or indexes into array of N pools via thread id.
template <class Edge>
class edge_equiv_alloc_base
{
 public:
    typedef Edge                  edge_type;
    typedef edge_equivalence<edge_type>      edge_equiv_type;
    typedef edge_equivalence_impl<edge_type>* equiv_p;
    virtual edge_equiv_type create_edge_equivalence(edge_type const& e)=0;
};
*/

////////////////////////////////////////////////////////////////////////////////
///
///  edge_factory is responsible for creating new edges from constituent edges
///  by applying grammar rules.  scores for the new edge are also calculated.
///
///  If you are planning to create a new state (edge_info type), you should be
///  aware of some of the details of edge creation.
///
///  <h3>creating an edge from a foreign word</h3>
///
///
////////////////////////////////////////////////////////////////////////////////
template <class InfoFactory>
class edge_factory : boost::noncopyable
{
public:
    typedef typename InfoFactory::info_type info_type;
    typedef edge<info_type> edge_type;
    typedef InfoFactory info_factory_type;

    typedef edge_equivalence<edge_type>      edge_equiv_type;
    typedef boost::shared_ptr<info_factory_type> info_factory_p;
    typedef edge_equivalence_impl<edge_type>* equiv_p;

    void set_cells()
    {
        cells=0;
    }

    void set_cells(cell_heuristic &c,double weight,score_t unseen_cell_outside=as_zero())
    {
        cells=&c;
        weight_cell_outside=weight;
        h_unseen_cell=pow(unseen_cell_outside,weight_cell_outside);
    }

    edge_factory(InfoFactory const& ifact);


    edge_factory( info_factory_p info_factory
                , double tm_heuristic_scale=1 // tm_wt * (inside + tm_heuristic_scale*heuristic)
                , double info_heuristic_scale=1 // likewise
                );

    edge_factory( double tm_heuristic_scale=1 // tm_wt * (inside + tm_heuristic_scale*heuristic)
                , double info_heuristic_scale=1 // likewise
                );

    /// create an edge from a binary rule
    template <class GT>
    edge_generator<InfoFactory>
    create_edge( GT const& grammar
               , typename GT::rule_type r
               , edge_equiv_type const& e1
               , edge_equiv_type const& e2 );

    /// create an edge from a unary rule
    template <class GT>
    edge_generator<InfoFactory>
    create_edge( GT const& grammar
               , typename GT::rule_type r
               , edge_equiv_type const& e );


    /// create an edge from a (foreign) token
    void create_edge( edge_type &ret
                    , indexed_token const& word
                    , span_t s
                    , score_t scr = as_one() )
    {
        log_create_edge();
        info_type n;
        ret.set_lexical(word,s,scr,n);
        ret.total *= get_cell_heuristic(ret.rt, ret.spn);
    }

    // derived (convenience?) methods
    /// create an edge from a foreign word
    template <class GT>
    void create_edge( edge_type &ret
                    , GT& grammar
                    , std::string const& word
                    , span_t s
                    , score_t scr = as_one() )
    {
        create_edge(ret,grammar.dict().foreign_word(word),s,scr);
    }

    template <class GT>
    void create_edge( edge_type &ret
                    , GT& grammar
                    , fat_token const& word
                    , span_t s
                    , score_t scr = as_one())
    {
        assert(word.type() == foreign_token);
        create_edge(ret,grammar,word.label(),s,scr);
    }

    template <class GT>
    edge_type create_edge( GT& grammar
                         , indexed_syntax_rule const& rule
                         , grammar_rule_id rid
                         , span_t spn
                         , score_t scr = as_one() )
    {
        edge_type ret;
        create_edge(ret,grammar,rule,rid,spn,scr);
        return ret;
    }

    template <class GT>
    edge_type create_edge( GT& grammar
                         , std::string const& word
                         , span_t s
                         , score_t scr = as_one() )
    {
        edge_type ret;
        create_edge(ret,grammar,word,s,scr);
        return ret;
    }

    edge_type create_edge( indexed_token const& word
                         , span_t s
                         , score_t scr = as_one() )
    {
        edge_type ret;
        create_edge(ret,word,s,scr);
        return ret;
    }

    template <class Grammar>
    std::string hash_string(Grammar const& grammar, edge<info_type> const& e) const
    {
        std::stringstream sstr;
        if (is_lexical(e.root())) {
            sstr << '"';
            sstr << gusc::escape_c(grammar.dict().label(e.root()));
            sstr << '"';
        } else {
            sstr << grammar.dict().label(e.root());
        }
        sstr << e.span();
        sstr << info_factory->hash_string(grammar,e.info());
        return sstr.str();
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    // the StoppingCondition is a predicate function that takes an edge as
    // an argument and determines how deep to calculate the component scores.
    // for instance, if stop always returns false,
    // as in StoppingCondition = gusc::always_false, then component_info_scores
    // will compute the component scores of the entire derivation rooted at e.
    // if StoppingCondition = gusc::always_true, then component_info_scores will
    // just provide the scores contributed by transitioning from e's children to
    // e.
    // if StoppingCondition = sbmt::is_xrs_edge, then component_info_scores
    // computes the total contribution of component info scores from all edges
    // from e's xrs children to e.
    //
    ////////////////////////////////////////////////////////////////////////////
    template <class Grammar, class StoppingCondition>
    void component_info_scores( edge<info_type> const& e
                              , Grammar& grammar
                              , feature_vector& scorevec
                              , feature_vector& heurvec
                              , StoppingCondition const& stop
                              ) const;

    template <class Grammar, class StoppingCondition>
    void component_info_scores( edge<info_type> const& e
                              , Grammar& grammar
                              , feature_vector& scorevec
                              , StoppingCondition const& stop
                              ) const
    {
        feature_vector heurvec;
        component_info_scores(e,grammar,scorevec,heurvec,stop);
    }

    template <class Grammar>
    void component_info_scores( edge<info_type> const& e
                              , Grammar& grammar
                              , feature_vector& scorevec
                              , feature_vector& heurvec
                              ) const
    {
        return component_info_scores(e,grammar,scorevec,heurvec,gusc::always_false());
    }

    template <class Grammar>
    void component_info_scores( edge<info_type> const& e
                              , Grammar& grammar
                              , feature_vector& scorevec
                              ) const
    {
        feature_vector heurvec;
        component_info_scores(e,grammar,scorevec,heurvec,gusc::always_false());
    }

    template <class GT>
    edge_type create_edge( GT& grammar
                         , fat_token const& word
                         , span_t s
                         , score_t scr = as_one() )
    {
        edge_type ret;
        create_edge(ret,grammar,word,s,scr);
        return ret;
    }


    // note: returns actual (inside) rule cost combined with heuristic (for virtual)
    template <class GT>
    score_t rule_heuristic(GT& g, typename GT::rule_type r) const
    {
        return g.rule_score(r) *
               pow(g.rule_score_estimate(r),tm_h_wt) *
               pow(info_factory->rule_heuristic(g,r),info_h_wt);
    }

    void finish(span_t spn) {  }

    //NOTE: doesn't include initial (foreign word on span) edges
    edge_stats stats() const
    {
        return edge_stats(long(num_edges),edge_equiv_type::equivalence_count());
    }

    // ret must have its root and span set; i.e. finish() should have been called already
    score_t get_cell_heuristic(indexed_token const& root, span_t const& span)
    {
        score_t retval(1.0);
        if (cells) {
			cell_heuristic::seen_nts::iterator pos = (*cells)[span].find(root);

            if (pos != (*cells)[span].end()) {
                retval = pow(pos->second,weight_cell_outside);
            } else {
                // filtering will destroy anything outside a seen cell, so we don't care what h
                retval = h_unseen_cell;
            }
        }
        return retval;
#ifdef DEBUG_INFO
        ret.print_state(DEBUG_INFO_OUT,grammar.dict(),1);
#endif
    }

 private:
    template <class GT>
    void create_unary_part( edge_type &ret
                          , GT &gram
                          , typename GT::rule_type r
                          , edge_equivalence<edge_type> const& c
                          , score_t info_inside
                          , score_t info_heuristic )
    {
        log_create_edge();
        ret.set_unary_part(gram,r,c);
        assert(tm_wt==1.); // otherwise would scale rule score by tm_wt

        // infos need to be responsible for their own weights.
        assert(info_wt==1.0);
        ret.inside = gram.rule_score(r)*c.representative().inside*info_inside;
        ret.heuristic() = pow(gram.rule_score_estimate(r),tm_h_wt)*pow(info_heuristic, info_h_wt); //FIXME: pass in heuristic (compute at same time as info_inside)
#ifdef DEBUG_INFO
        //"|h="<<info_factory->heuristic_score(gram,ret.info())<<
        DEBUG_INFO_OUT <<"info="<<info_inside<<"("<<print(ret.first_child(),gram.dict())<<")";
#endif
#ifdef DEBUG_INFO_PORTION
//    ret.info_portion = e1.representative().info_portion *
//                      e2.representative().info_portion *
//                      pow(info_inside,info_wt);
#endif

    }

    void log_create_edge()
    { ++num_edges; }

    double tm_wt;
    double info_wt;
    info_factory_p info_factory;
    cell_heuristic *cells;
    double weight_cell_outside;
    score_t h_unseen_cell;
    double tm_h_wt;
    double info_h_wt;
    boost::detail::atomic_count num_edges;
    friend class edge_generator<InfoFactory>;
    void assert_1_wt() const
    {
        if (tm_wt!=1.0 || info_wt!=1.0)
            throw std::runtime_error("edge_factory with tm_wt or info_wt!=1.0.  set component feature weights instead");
        assert(tm_wt==1.0&&info_wt==1.0);
    }

};



////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/edge/impl/edge.ipp>

#endif // SBMT__EDGE__EDGE_HPP

