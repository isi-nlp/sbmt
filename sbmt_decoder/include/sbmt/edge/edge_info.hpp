#ifndef   SBMT__EDGE__EDGE_INFO_HPP
#define   SBMT__EDGE__EDGE_INFO_HPP

# include <cstddef>
# include <bitset>
# include <boost/functional/hash.hpp>
# include <boost/utility/in_place_factory.hpp>
# include <sbmt/edge/constituent.hpp>

namespace sbmt {
    
////////////////////////////////////////////////////////////////////////////////
///
///  \defgroup edge_info state information (edge_info and info factories)
///  \ingroup edges
///
///  state information is how additional scoring information and edge equivalence
///  information is passed to an edge.
///
///  <h3>the example: integrated ngram scoring</h3>
///  integrated ngram scoring of the native sentence being translated  
///  requires that more information be stored in the edge.  what we want to do
///  is gleen from the edge the partial sentence created, and apply as much of
///  an ngram score as possible.  we'd like to do it incrementally and avoid 
///  alot of duplication.  each rule contains the amount of english it 
///  contributes to the parse, like so (sort of)
///
///  \code
///  A (X:0 "of" Y:1 ) -> Y:1 X:0
///  \endcode
///
///  I dont even pretend to have an understanding of linguistics, so I just 
///  make up rules.
///
///  If I am using a bigram lm, and I want to apply this rule to a pair of 
///  edges, and they are scored with respect to the bigram as 
///  much as possible, then to bigram score the resulting edge, I just need to know the
///  left and right most words in the english sentences of the constituent 
///  edges.
///
///  so ngram_info<2> (bigram info) is a structure with room for two english
///  tokens, left and right.
///
///  so an edge< ngram_info<2> > e1 is equivalent to another one e2 if
///  - the root nonterminal is the same
///  - the span is the same
///  - the context words are the same.
///  edge<> handles this calculation by deferring to ngram_info<2>::equal_to()
///
///  the ngram score is not stored in the ngram_info.  this is primarily to save
///  space.  edges are plentiful in parsing, and if you start having a score in
///  every info (multiple infos can be used by creating a join_info ), it adds 
///  up.
///  
///  instead, the info_factory< ngram_info<2> > passes the score and possibly a
///  heuristic score to the edge_factory when new infos are made.
///
///  an edge_info_factory has similar methods to an edge_factory, and the 
///  edge_factory uses them when it makes new edges.
///
///  there is a create_info() function for combining two edges into a new edge 
///  via a binary rule, and one for combining one edge into a new edge via a
///  unary rule.
///  there is no create_info() edges that are just foreign words.  not sure if
///  that is an oversight, as i havent anticipated all possible types of 
///  state info.  right now the code just assumes that a default constructed 
///  info is servicable for a foreign-word edge.
///
///  as an example, here is roughly what info_factory<ngram_info<2>>::create_info()
///  does to the rule above, with edge1 having context "elbonia" and edge2 having
///  context "president" (left and right identical for each).
///  - the new edge will have left context "president" and right context 
///    "elbonia"
///  - the incremental score provided to the edge will be 
///    p(of|president)*p(elbonia|of)
///
///  - edge_factory::create_edge() will then multiply the two edge scores,
///    the new info score, and the rule score (with some user defined weights
///    to control the mix between the rule score and the info score)
///
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
///
///  represents the variable state information/score in an edge.
///  examples of edge_info: 
///  - ngram_info
///  - sblm_info
///  - join_info
///
///  any new info you create should be derived from edge_info like so:
///  \code
///  struct sblm_info : public edge_info<sblm_info>
///  {
///     size_t hash_value() const
///     bool   equal_to(sblm_info const& other)
///  }
///  \endcode
///
///  they should also be safely assignable and copy-constructable, and 
///  as light-weight as possible.
///
///  i realize "info" is a horrible name.  its a name like "blob", "data", etc.
///  it tells you nothing.  if anyone has a better name, please suggest it.
///  i will gladly find/replace all instances of "_info"
///
////////////////////////////////////////////////////////////////////////////////

struct unscoreable_info_tag {};
struct greedy_unscoreable_info_tag {};

namespace {
    static unscoreable_info_tag unscoreable_info_;
    static greedy_unscoreable_info_tag greedy_unscoreable_info_;
    void edge_info_hpp_anonymous_warning(unscoreable_info_tag&,greedy_unscoreable_info_tag&) {}
    void edge_info_hpp_anonymous_warning()
    {
        edge_info_hpp_anonymous_warning(unscoreable_info_,greedy_unscoreable_info_);
    }
}

template <class Info>
struct edge_info 
{
public:
    edge_info() : i() 
    {
        status[comparable_] = false;
        status[stateable_] = false;
    }
    explicit edge_info(Info const& other) : i(other) 
    {
        status[comparable_] = true;
        status[stateable_] = true;
    }
    edge_info(unscoreable_info_tag const&) : i() 
    {
        status[comparable_] = false;
        status[stateable_] = false;
    }
    edge_info(greedy_unscoreable_info_tag const&) : i()
    {
        status[comparable_] = true;
        status[stateable_] = false;
    }
    bool stateable() const { return status[stateable_]; }
    bool comparable() const { return status[comparable_]; }
    bool equal_to(edge_info const& other) const 
    { 
        assert(comparable());
        assert(other.comparable() == true); 
        return i == other.i; 
    }
    size_t hash_value() const 
    { 
        assert(comparable() == true); 
        return boost::hash<Info>()(i); 
    }
    Info const& info() const 
    {
        assert(stateable()); 
        return i; 
    }
private:
    Info i;
    std::bitset<2> status;
    static const unsigned int stateable_ = 0;
    static const unsigned int comparable_ = 1;
};

template <class I>
struct edge_info_constituent_base {
    typedef constituent<I> result_type;

    constituent<I> operator()(constituent< edge_info<I> > const& c) const
    {
        return constituent<I>(&c.info()->info(),c.root());
    }
};

template <class I, class Range>
boost::iterator_range<
  boost::transform_iterator<
    edge_info_constituent_base<I>
  , typename boost::range_iterator<Range>::type
  >
>
edge_info_base_range(Range const& range)
{
    edge_info_constituent_base<I> f;
    return boost::make_iterator_range(
               boost::make_transform_iterator(boost::begin(range),f)
             , boost::make_transform_iterator(boost::end(range),f)  
           );
}

////////////////////////////////////////////////////////////////////////////////

template <class Info>
bool operator == (edge_info<Info> const& i1, edge_info<Info> const& i2)
{
    return i1.equal_to(i2);
}

template <class Info>
bool operator != (edge_info<Info> const& i1, edge_info<Info> const& i2)
{
    return not (i1 == i2);
}

template <class Info>
std::size_t hash_value(edge_info<Info> const& i)
{
    return i.hash_value();
}

template <class C, class T, class Info, class TF>
void print(std::basic_ostream<C,T>& os, edge_info<Info> const& ei, TF const& tf)
{
    if (ei.scoreable()) os << print(ei.info(),tf);
}

template <class Generator, class Info>
class edge_info_result_generator
{
public:
    typedef boost::tuple<edge_info<Info>,score_t,score_t> result_type;
    operator bool() const { return generator; }
    result_type operator()() { return generator(); }
    
    explicit edge_info_result_generator(Generator const& g) : generator(g) {}
private:
    Generator generator;
};

template <class Info, class Generator>
edge_info_result_generator<Generator,Info>
generate_edge_info_results(Generator const& generator)
{
    return edge_info_result_generator<Generator,Info>(generator);
}

////////////////////////////////////////////////////////////////////////////////

template <class InfoFactory>
struct edge_info_factory {
    
    typedef typename InfoFactory::info_type base_info;
    typedef edge_info<base_info> info_type;
    
    explicit edge_info_factory(bool greedy = false) 
      : factory(new InfoFactory()), greedy(greedy) {}
    
    explicit edge_info_factory(InfoFactory const& factory, bool greedy = false) 
      : factory(new InfoFactory(factory)), greedy(greedy) {}
    
    template <class Expression>
    explicit edge_info_factory(Expression expr)
     : factory(storage())
    {
        expr.template apply<InfoFactory>(factory.get());
    }
    
    info_type empty_info() const
    {
        if (greedy) return greedy_unscoreable_info_;
        else return unscoreable_info_;
    }
    
    template <class Grammar>
    std::string hash_string(Grammar const& grammar, info_type const& info) const
    {
        if (info.comparable()) {
            if (not info.stateable()) return "*";
            else return factory->hash_string(grammar,info.info());
        } else {
            return "#";
        }
    }
    
    template <class Grammar>
    bool scoreable_rule(Grammar& grammar, typename Grammar::rule_type rule)
    {
        return factory->scoreable_rule(grammar,rule);
    }
    
    template <class Grammar>
    score_t rule_heuristic(Grammar& grammar, typename Grammar::rule_type rule)
    {
        return factory->rule_heuristic(grammar,rule);
    }
    
    typedef boost::tuple<info_type,score_t,score_t> result_type;
    
    typedef edge_info_result_generator<
                typename InfoFactory::result_generator
            , base_info > result_generator;
    
    template <class Grammar, class ConstituentRange>
    result_generator
    create_info( Grammar& grammar
               , typename Grammar::rule_type rule
               , span_t const& span
               , ConstituentRange const& constituents
               )
    {
        return generate_edge_info_results<base_info>(
                   factory->create_info( grammar
                                       , rule
                                       , span
                                       , edge_info_base_range<base_info>(constituents)
                                       )
               );
    }
    
    template < class Grammar
             , class ConstituentRange
             , class ScoreOutputIterator
             , class HeurOutputIterator >
    boost::tuple<ScoreOutputIterator,HeurOutputIterator>
    component_scores( Grammar& grammar
                    , typename Grammar::rule_type rule
                    , span_t const& span
                    , ConstituentRange const& constituents
                    , info_type const& result
                    , ScoreOutputIterator scores_out
                    , HeurOutputIterator heuristics_out )
    {
        return factory->component_scores( grammar
                                        , rule
                                        , span
                                        , edge_info_base_range<base_info>(constituents)
                                        , result.info()
                                        , scores_out
                                        , heuristics_out );
    }
    
private:
    boost::shared_ptr<InfoFactory> factory;
    bool greedy;
    InfoFactory* storage() 
    { 
        return reinterpret_cast<InfoFactory*>(new char[sizeof(InfoFactory)]); 
    }
};



////////////////////////////////////////////////////////////////////////////////

/* \page example2 a new state-info type
 model-s is my name for this span-level model1 (or model1inv? i dont know 
 the names)


 to calculate model-s, we need no additional state, so our edge-info 
 object is empty.

 all info objects need to be able to hash, and check for equality with another
 state.

/code
struct model_s_info : public edge_info<model_s_info> {
    std::size_t hash_value() const { return 0; }
    bool equal_to(model_s_info const& other) const { return true; }
};
/endcode

 the info_factory will need a translation table to apply scores

/code
template <>
class info_factory<model_s_info>
{
   shared_ptr<translation_table> ttable; 
public:
	typedef model_s_info info_type;
	
	info_factory(shared_ptr<translation_table> ttable)
	: ttable(ttable) {}
	
	/// in this version we apply at every rule, because we have the
	/// english string available from binarization; 
	/// we rely on good or at least delayed english-side binarization
	template <class IT, class GT>
	score_t create_info( info_type& out
	                   , GT const& gram
	                   , typename GT::rule_type r
	                   , edge<IT> const& e )
	{
	   // the english words of a rule are in an object called lm_string
	   indexed_lm_string const& lms = gram.rule_lm_string(r);
	   
	   // the resulting span will be the span of e
	   span_t spn = e.span();
	   
	   // okay, this doesnt actually exist, but seems useful
	   std::pair<indexed_sentence::iterator, indexed_sentence::iterator>
	         spn_range = subsentence(sentence,spn);
	          
	   score_t retval = 1.0;
	   
	                              
	   for (; lm_itr != lm_end; ++lm_itr) {
	       if (lm_itr->is_token()){
	           indexed_sentence::iterator sitr = spn_range.first;
	                                      send = spn_range.second;
	           score_t mult = 0;
	           for (;sitr != send; ++sitr) {
	               mult = std::max( mult
	                              , ttable.prob(lm_itr->get_token(),*sitr)
	                              );
	           }
	           retval *= mult;
	       }
	   }
	   return retval;
	}
	
	/// in this version, we only apply scores when we are at a syntax rule
	template <class IT, class GT>
	score_t create_info( info_type& out
	                   , GT const& gram
	                   , typename GT::rule_type r
	                   , edge<IT> const& e )
	{
	   if (!gram.is_complete_rule()) return score_t(1.0);
	   
	   indexed_syntax_rule syn = gram.get_syntax(r);
	   
	   indexed_syntax_rule::lhs_preorder_iterator lhs_itr = syn.lhs_begin(),
	                                              lhs_end = syn.lhs_end();
	                                              
	   
	   
	   // the resulting span will be the span of e
	   span_t spn = e.span();
	   
	   // okay, this doesnt actually exist, but seems useful
	   std::pair<indexed_sentence::iterator, indexed_sentence::iterator>
	             spn_range = subsentence(sentence,spn);
	          
	   score_t retval = 1.0;
	   
	                              
	   for (; lhs_itr != lhs_end; ++lhs_itr) {
	       if (lhs_itr->lexical()){
	           indexed_sentence::iterator sitr = spn_range.first;
	                                      send = spn_range.second;
	           score_t mult = 0;
	           for (;sitr != send; ++sitr) {
	               mult = std::max( mult
	                              , ttable.prob(lhs_itr->get_token(),*sitr)
	                              );
	           }
	           retval *= mult;
	       }
	   }
	   return retval;
	}
};
\endcode

 to use model-s info with ngram-info, we combine like so:
 
 \code
 shared_ptr<translation_table> ttable(new translation_table("ttable.file"));
 shared_ptr<LWNgram> ngram(new LWNgram("ngram.lw"));
 
 info_factory<ngram_info> ngram_fact(ngram);
 info_factory<model_s_info> model_s_fact(ttable);
 
 info_factory<join_info<ngram_info,model_s_info> > join_fact( ngram_fact
                                                            , model_s_fact );
                                                            
 edge_factory<join_info<ngram_info,model_s_info> > ef(join_fact);
 \endcode
 
 
 
**/
} // namespace sbmt

#endif // SBMT__EDGE__EDGE_INFO_HPP
