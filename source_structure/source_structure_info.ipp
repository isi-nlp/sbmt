#ifndef __source_structure_ipp__
#define __source_structure_ipp__

#include <sstream>
#include "constituent_list.hpp"
#include "source_structure_info.hpp"
#include <boost/function_output_iterator.hpp>
#include "ww/ww_utils.hpp"

namespace source_structure {

template <class ConstituentIterator>
source_structure_info_factory::result_generator
source_structure_info_factory::
create_info( const grammar_in_mem& grammar
           , grammar_in_mem::rule_type rule
           , span_t
           , iterator_range<ConstituentIterator> const& range )
{
    score_t inside;
    span_t fspan;
    boost::function_output_iterator<ww_util::noout> nooutr;
    tie(inside,fspan) = create_info_data(grammar,rule,range, nooutr);
    info_type info(fspan);
    return make_tuple(info,inside,1.0);
}

////////////////////////////////////////////////////////////////////////////

template <class ConstituentIterator, class ScoreOutputIterator>
ScoreOutputIterator
source_structure_info_factory::
component_scores_old( const grammar_in_mem& grammar
                    , grammar_in_mem::rule_type rule
                    , span_t
                    , iterator_range<ConstituentIterator> const& range
                    , info_type const& result
                    , ScoreOutputIterator scores_out )
{
    create_info_data(grammar,rule,range, scores_out);
    return scores_out;
}






/// compute the resulting spans.
/// check the constituent_list to see if there is any conflicts.
/// compute the inside.
template <class ConstituentIterator, class Accumulator>
tuple<score_t,span_t>
source_structure_info_factory::
create_info_data( grammar_in_mem const& grammar
                , grammar_in_mem::rule_type rule
                , iterator_range<ConstituentIterator> const& range
                , Accumulator & accum)
{
    score_t inside = 1.0;
    score_t scr = score_t(1.0, as_neglog10());
    span_t result_span = compute_resulting_span(grammar, rule, range);
    if (!is_foreign(grammar.rule_lhs(rule))) {
            string constituent_e_label = grammar.dict().label(grammar.get_syntax(rule).lhs_root_token());

            // remove the splits.
            boost::regex e("(-\\d+\\s*$)|(-\\d+-BAR\\s*$)");
            const char* fmt="(?1)(?2-BAR)";
            ostringstream olabel;
            boost::regex_replace(ostream_iterator<char,char>(olabel), constituent_e_label.begin(), constituent_e_label.end(), e, 
                            fmt, boost::match_default | boost::format_all);
            constituent_e_label=olabel.str();

            if(!compute_matched_features(grammar, result_span, accum, inside, constituent_e_label) && 
               !compute_cross_features(grammar, result_span, accum, inside, constituent_e_label)) {
                    grammar_in_mem& g = const_cast<grammar_in_mem&>(grammar);
                    ostringstream ost;
                    ost<<"neither_match_nor_cross"; 
                    boost::uint32_t fid=g.feature_names().get_index(ost.str());
                    *accum = make_pair(fid, scr);
                    ++accum;
                    inside*= scr ^ g.get_weights()[fid];
            }
    }

    return make_tuple(inside,result_span);
}

/// generate constituent matching features.  three types of matching features
/// (i) source tree constituent; (ii) target tree constituent; 
/// (iii) combine of source and target.
/// we consider only the the list of nonterminals under our interest (for example
///    most frequent ones.
/// 
/// if there is any matching feature generated, return true.
template <class Accumulator>
bool 
source_structure_info_factory::
compute_matched_features(const grammar_in_mem& g,
                         sbmt::span_t& result_span, 
                         Accumulator & accum, 
                         sbmt::score_t& inside, 
                         std::string& constituent_e_label)
{
    grammar_in_mem& grammar = const_cast<grammar_in_mem&>(g);
    //return false;
    bool ret = false;
    constituent_list::const_iterator it_const_list = m_fconstituent_list.find(result_span); 
    // span matching, we check the nontemrinals.
    if(it_const_list != m_fconstituent_list.end()){
        score_t scr = score_t(1.0, as_neglog10());

        // target-only match
        if(is_concerned_trg_nt(constituent_e_label)) { 
            ostringstream ost;
            ost<<constituent_e_label<<"_matched_t"; // t means target.
            boost::uint32_t fid=grammar.feature_names().get_index(ost.str());
            *accum = make_pair(fid, scr);
            ++accum;
            inside*= scr ^ grammar.get_weights()[fid];
            ret = true;
        }


        // foreign nonterminals and joint nonterminals.
        // there might be more than one nonterminal covering the matched span.
        BOOST_FOREACH(const string ss, it_const_list->second){
            // source-only match
            if(is_concerned_src_nt(ss)){ ////
                ostringstream ost;
                ost<<ss<<"_matched_s"; // s means source.
                boost::uint32_t fid=grammar.feature_names().get_index(ost.str());
                *accum = make_pair(fid, scr);
                ++accum;
                inside*= scr ^ grammar.get_weights()[fid];
                ret = true;

                // both target-source match .
                if(is_concerned_trg_nt(constituent_e_label)){
                    ostringstream ost;
                    ost<<constituent_e_label<<"_"<<ss<<"_matched_ts";
                    boost::uint32_t fid=grammar.feature_names().get_index(ost.str());
                    *accum = make_pair(fid, scr);
                    ++accum;
                    inside*= scr ^ grammar.get_weights()[fid];
                    ret = true;
                }
            }
        }
    }
        
    return ret;
}

template <class Accumulator>
bool
source_structure_info_factory::
compute_cross_features(const grammar_in_mem& g, 
                       sbmt::span_t& result_span, 
                       Accumulator & accum, 
                       sbmt::score_t& inside, 
                       std::string& constituent_e_label)
{
    grammar_in_mem& grammar = const_cast<grammar_in_mem&>(g);
    bool ret = false;
    constituent_list::const_iterator it_const_list = m_fconstituent_list.find(result_span); 
    // span not matching, then we look for crossing.
    if(it_const_list == m_fconstituent_list.end()){

        // for each span in the constituent list.
        BOOST_FOREACH(constituent_list::value_type v, m_fconstituent_list){
            // check if the constituent span overlapps with the result span.
            if((result_span.left() < v.first.left() && result_span.right() > v.first.left()
                    && result_span.right() < v.first.right()) ||
                (result_span.left() > v.first.left() && result_span.left() < v.first.right()
                    && result_span.right() > v.first.right()))
            
            {
                // this is a crossing, and compute the features here
                score_t scr = score_t(1.0, as_neglog10());

                // english nonterminals
                if(is_concerned_trg_nt(constituent_e_label)) { 
                    ostringstream ost;
                    ost<<constituent_e_label<<"_cross_t"; // t means target.
                    boost::uint32_t fid=grammar.feature_names().get_index(ost.str());
                    *accum = make_pair(fid, scr);
                    ++accum;
                    inside*= scr ^ grammar.get_weights()[fid];
                    ret = true;
                }


                // foreign nonterminals and joint nonterminals.
                // there might be more than one nonterminal covering the matched span.
                BOOST_FOREACH(string fnt, v.second){
                    if(is_concerned_src_nt(fnt)){ ////
                        ostringstream ost;
                        ost<<fnt<<"_cross_s"; // s means source.
                        boost::uint32_t fid=grammar.feature_names().get_index(ost.str());
                        *accum = make_pair(fid, scr);
                        ++accum;
                        inside*= scr ^ grammar.get_weights()[fid];
                        ret = true;

                        // the target-source labels.
                        if(is_concerned_trg_nt(constituent_e_label)){
                            ostringstream ost;
                            ost<<constituent_e_label<<"_"<<fnt<<"_cross_ts";
                            boost::uint32_t fid=grammar.feature_names().get_index(ost.str());
                            *accum = make_pair(fid, scr);
                            ++accum;
                            inside*= scr ^ grammar.get_weights()[fid];
                            ret = true;
                        }
                    }
                }
            }

        }

    }
        
    return ret;
}


template <class ConstituentIterator>
sbmt::span_t 
source_structure_info_factory::
compute_resulting_span(grammar_in_mem const& grammar, 
                       grammar_in_mem::rule_type rule, 
                       iterator_range<ConstituentIterator> const& range)
{
    // note: in this case, the constituent list is empty.
    if (is_foreign(grammar.rule_lhs(rule))) {
       return grammar.template rule_property<span_t>(rule,span_id);
    } else {
        ConstituentIterator it, it_first, it_last;
        it_first=boost::begin(range);
        for(it=begin(range);it!=end(range);++it){ it_last=it; }
        return sbmt::span_t(it_first->info()->span().left(), it_last->info()->span().right());
    }
}


} // namespace source_structure

#endif
