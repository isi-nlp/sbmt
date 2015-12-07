#ifndef __word_context_info_hpp__
#define __word_context_info_hpp__

#include <sbmt/edge/any_info.hpp>
#include <sbmt/edge/info_base.hpp>
#include "sbmt/span.hpp"
#include "boost/function_output_iterator.hpp"
#include "lattice_graph.hpp"
#include "wsd_features.hpp"
#include "sbmt/search/block_lattice_tree.hpp"
# include <gusc/generator/single_value_generator.hpp>
# include <boost/function_output_iterator.hpp>
#include <source_structure/constituent_list.hpp>
#include <ww/ww_utils.hpp>

namespace word_context {
using namespace sbmt;
using namespace std;
using namespace boost;
using namespace source_structure;




class word_context_info: public info_base<word_context_info> {
public:
    word_context_info(const span_t fspan) : m_fspan(fspan) {}
    word_context_info() {} // by default, it is an empty span (0,0).

    bool equal_to(word_context_info const& other) const 
    { 
        return m_fspan == other.span() ;
    }

    // bool lexical() const { return m_fspan.size(); }

    size_t hash_value() const { return sbmt::hash_value(m_fspan); }

    /// returnst he foreign span covered by this info.
    span_t span() const { return m_fspan; }
private:
    /// the fspan is meaningful only if the info convers a foreign
    /// word. because we want to distinguish two foreign identical 
    /// words that cover overlapping spans.  In that case, if we 
    /// dont mark them as different spans, they will be collaps into
    /// the same signature.
    span_t m_fspan;
};


template <class C, class T, class TF>
void print(std::basic_ostream<C,T>& os, word_context_info const& d, TF const&)
{
    os<<d.span();
}


class word_context_info_factory 
: public sbmt::info_factory_new_component_scores<word_context_info_factory>
{
public:
    // required by interface
    typedef word_context_info info_type;

    // info, inside-score, heuristic
    typedef tuple<info_type,score_t,score_t> result_type;

    template<class Grammar>
    word_context_info_factory(Grammar& grammar,
                             const lattice_tree & lattice,
                              const property_map_type & pmap, 
                              const wsd_features& wsd, 
                              const string source_constituents)
             : align_attr_id(pmap.find("align")->second) ,
             span_id(pmap.find("span")->second) ,
       m_lattice(lattice, grammar.dict()),
       m_wsd(wsd),
       m_fconstituent_list(source_constituents)
    { }



    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  result_generator: required by interface
    ///  info factories are allowed to return multiple results for a given
    ///  set of constituents.  it returns them as a generator.
    ///  a generator is a result_type functor(void) object that is convertible to
    ///  bool.  the generator converts to false when there are no more results
    ///  to retrieve.  this is analogous to an input iterator.
    ///
    ///  your generator will be called like so:
    ///  while (generator) { result_type res = generator(); }
    ///
    ///  for those that are familiar with python/ruby, yes there are libraries that
    ///  embue c++ with equivalent generator/coroutine behavior
    ///
    ///  if your create_info method only ever returns one result, you
    ///  can use single_value_generator as your result_generator
    ///
    ////////////////////////////////////////////////////////////////////////////
    typedef gusc::single_value_generator<result_type> result_generator;

    template <class ConstituentIterator, class Grammar>
    result_generator
    create_info( Grammar& grammar
               , typename Grammar::rule_type rule
               , span_t const& span
               , iterator_range<ConstituentIterator> const& range );

    ////////////////////////////////////////////////////////////////////////////

    template <class ConstituentIterator, class ScoreOutputIterator, class Grammar>
    ScoreOutputIterator
    component_scores_old( Grammar& grammar
                        , typename Grammar::rule_type rule
                        , span_t const& span
                        , iterator_range<ConstituentIterator> const& range
                        , info_type const& result
                        , ScoreOutputIterator scores_out );

    template <class Grammar>
    bool scoreable_rule( Grammar const& grammar
                       , typename Grammar::rule_type r ) const
    {   
        return !is_virtual_tag(grammar.rule_lhs(r));
    }

    template <class Grammar>
    score_t
    rule_heuristic(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        return 1.0;
    }

     template <class Grammar>
    std::string hash_string(Grammar const& grammar, info_type const& info) const
    {
        
        std::stringstream ssr;
        print(ssr,info);
        //std::cerr<<"Hash STRING: "<<ssr.str()<<std::endl;
        return ssr.str();
    }

    vector<indexed_token> 
    get_lhs_yield(const indexed_syntax_rule& rule)  const;

protected:


    template <class ConstituentIterator, class Grammar>
    sbmt::span_t 
    compute_resulting_span(Grammar& grammar, 
                           typename Grammar::rule_type rule, 
                       iterator_range<ConstituentIterator> const& range);


    template <class ConstituentIterator, class Accumulator, class Grammar>
    tuple<score_t,span_t>
    create_info_data( Grammar& grammar
                    , typename Grammar::rule_type rule
                    , iterator_range<ConstituentIterator> const& range
                    , Accumulator& accum);
private:
    size_t align_attr_id;
    size_t span_id;
    lattice_graph<indexed_token_factory> m_lattice;
    const wsd_features& m_wsd;
    constituent_list m_fconstituent_list;
};


} // namespace word_context

#include "word_context_info.ipp"

#endif
