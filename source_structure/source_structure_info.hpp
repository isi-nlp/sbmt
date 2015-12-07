#ifndef __source_structure_info_hpp__
#define __source_structure_info_hpp__

#include <iterator>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>

#include <sbmt/edge/any_info.hpp>
#include <sbmt/edge/info_base.hpp>
#include <boost/tuple/tuple.hpp>
#include "sbmt/span.hpp"
#include "ww/ww_utils.hpp"
#include "boost/function_output_iterator.hpp"
#include "sbmt/search/block_lattice_tree.hpp"
# include <gusc/generator/single_value_generator.hpp>
# include <boost/function_output_iterator.hpp>
#include "constituent_list.hpp"

namespace source_structure {

using namespace sbmt;
using namespace std;
using namespace boost;

typedef stlext::hash_set<std::string>  ntvcb_t;


class source_structure_info: public sbmt::info_base<source_structure_info> {
public:
    source_structure_info(const span_t fspan) : m_fspan(fspan) {}
    source_structure_info() {} // by default, it is an empty span (0,0).

    bool equal_to(source_structure_info const& other) const { 
        return (m_fspan == other.span()) ;
    }

    size_t hash_value() const { return sbmt::hash_value(m_fspan); }

    /// returnst he foreign span covered by this info.
    const span_t& span() const { return m_fspan; }
    span_t& span() { return m_fspan; }

private:
    /// the fspan is meaningful only if the info convers a foreign
    /// word. because we want to distinguish two foreign identical 
    /// words that cover overlapping spans.  In that case, if we 
    /// dont mark them as different spans, they will be collaps into
    /// the same signature.
    span_t m_fspan;
};


template <class C, class T, class TF>
void print(std::basic_ostream<C,T>& os, source_structure_info const& d, TF const&)
{
    os<<d.span();
}


class source_structure_info_factory
: public sbmt::info_factory_new_component_scores<source_structure_info_factory>
{
public:
    // required by interface
    typedef source_structure_info info_type;

    // info, inside-score, heuristic
    typedef boost::tuple<info_type,score_t,score_t> result_type;


    source_structure_info_factory(const grammar_in_mem& grammar, 
                                  const property_map_type & pmap, 
                                  const string& src_nts_file, 
                                  const string& trg_nts_file,
                                  const string constituent_list_string) 
    : span_id(pmap.find("span")->second)
    , m_fconstituent_list(constituent_list_string) 
    {

        ifstream in1(src_nts_file.c_str());
        if(!in1){ cerr<<"Cannot open "<<src_nts_file<<endl; exit(1);}
        copy(istream_iterator<string>(in1), istream_iterator<string>(), inserter(m_src_nts,m_src_nts.begin()));
        in1.close();

        ifstream in2(trg_nts_file.c_str());
        if(!in2){ cerr<<"Cannot open "<<trg_nts_file<<endl; exit(1);}
        copy(istream_iterator<string>(in2), istream_iterator<string>(), inserter(m_trg_nts, m_trg_nts.begin()));
        in2.close();

    }

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

    template <class ConstituentIterator>
    result_generator
    create_info( grammar_in_mem const& grammar
               , typename grammar_in_mem::rule_type rule
               , span_t
               , iterator_range<ConstituentIterator> const& range );

    ////////////////////////////////////////////////////////////////////////////

    template <class ConstituentIterator, class ScoreOutputIterator>
    ScoreOutputIterator
    component_scores_old( grammar_in_mem const& grammar
                        , grammar_in_mem::rule_type rule
                        , span_t
                        , iterator_range<ConstituentIterator> const& range
                        , info_type const& result
                        , ScoreOutputIterator scores_out );

    template <class Grammar>
    bool scoreable_rule( Grammar const& grammar
                       , typename Grammar::rule_type r ) const
    {   
        return ! is_virtual_tag(grammar.rule_lhs(r));
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
        return ssr.str();
    }


protected:
    bool is_concerned_src_nt(const string nt) const;
    bool is_concerned_trg_nt(const string nt) const;

    template <class ConstituentIterator, class Accumulator>
    boost::tuple<score_t,span_t>
    create_info_data( const grammar_in_mem& grammar
                    , grammar_in_mem::rule_type rule
                    , iterator_range<ConstituentIterator> const& range
                    , Accumulator & accum);

    template <class ConstituentIterator>
    sbmt::span_t 
    compute_resulting_span(grammar_in_mem const& grammar, 
                           grammar_in_mem::rule_type rule, 
                           iterator_range<ConstituentIterator> const& range);



    template <class Accumulator> 
        bool compute_matched_features(const grammar_in_mem& grammar, 
                                      sbmt::span_t& result_span, 
                                      Accumulator & accum, 
                                      sbmt::score_t& inside, 
                                      std::string& constituent_e_label);

    template <class Accumulator> 
    bool compute_cross_features(const grammar_in_mem& grammar,
                                sbmt::span_t& result_span, 
                                Accumulator & accum, 
                                sbmt::score_t& inside, 
                                std::string& constituent_e_label);

private:
    boost::uint32_t span_id;
    // N.B.: this constituent list contains spans only for concerned set of nonterminals.
    // not necessarily all the nonterminals. irelavent spans will be filtered out 
    // during the factory constructor.  by iterating this list, you can get the set
    // of concerned nonterminal set.
    constituent_list m_fconstituent_list;
    ntvcb_t m_src_nts;
    ntvcb_t m_trg_nts;
};


} // namespace source_structure

#include "source_structure_info.ipp"

#endif
