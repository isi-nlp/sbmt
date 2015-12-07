#ifndef __word_context_ipp__
#define __word_context_ipp__

#include "wsd_features.hpp"
#include "ww/ww_utils.hpp"
#include "sbmt/span.hpp"

using namespace sbmt;

namespace word_context {

template <class ConstituentIterator, class Grammar>
word_context_info_factory::result_generator
word_context_info_factory::
create_info( Grammar& grammar
           , typename Grammar::rule_type rule
           , span_t const& span
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

template <class ConstituentIterator, class ScoreOutputIterator, class Grammar>
ScoreOutputIterator
word_context_info_factory::
component_scores_old( Grammar & grammar
                    , typename Grammar::rule_type rule
                    , span_t const& span
                    , iterator_range<ConstituentIterator> const& range
                    , info_type const& result
                    , ScoreOutputIterator scores_out )
{
    create_info_data(grammar,rule,range, scores_out);
    return scores_out;
}

/// iterate the rule RHS, yielding the foreign words and their f spans.
/// for each foreign word, and for each of its adjacent words, generate the
/// word context features.
/// transform the word context features into the right form.
/// and generate the results.
template <class ConstituentIterator, class Accumulator, class Grammar>
tuple<score_t,span_t>
word_context_info_factory::
create_info_data( Grammar& grammar
                , typename Grammar::rule_type rule
                , iterator_range<ConstituentIterator> const& range
                , Accumulator& accum)
{
    score_t inside = as_one();
    span_t sp;

    // this is an foreign->foreign introduction rule.
    if (is_foreign(grammar.rule_lhs(rule))) {
       sp = grammar.template rule_property<span_t>(rule,span_id);
       return make_tuple(inside, sp);
    } else {
        indexed_syntax_rule const& xrs_rule = grammar.get_syntax(rule);
        vector<indexed_token> lhs_yield = get_lhs_yield(xrs_rule);
        indexed_syntax_rule::rhs_iterator ri = xrs_rule.rhs_begin(), re = xrs_rule.rhs_end();
        ConstituentIterator ci = begin(range), ce = end(range);
        ConstituentIterator clast_it;
        int rhs_index = 0;
        int naligns = 0;
        for (; ri != re; ++ri, ++ci, rhs_index++) {

            // record the last constituent.
            clast_it=ci;

            assert(ci != ce);
            // this is a lexical foreign word.
            if (is_lexical(ri->get_token())) { 
                string f = grammar.dict().label(ri->get_token());
                if(grammar.template rule_has_property(rule, align_attr_id)) {
                    const alignment& align= grammar.template rule_property<alignment>(rule, align_attr_id);
                    if(align.sa.size()){
                        const vector<unsigned>& aligns = align.sa[rhs_index];
                        map<string, int>  wcs;
                        bool bora;
                        // foreach english word aligned to this foreign, 
                        //
                        BOOST_FOREACH(unsigned a, aligns){
                            naligns++;
                            string e = grammar.dict().label(lhs_yield[a]);
                            // generate the prev word context feature
                            BOOST_FOREACH(string fprev,m_lattice.in_edges(ci->info()->span().left())){
                                // lexical
                                string f1, fprev1, e1;
                                tie(bora,fprev1, f1,e1) = m_wsd.transform(make_tuple(true, fprev, f,e)); 
                                if(fprev1!="unk" && (f1!="unk"||e1!="unk")){ fprev1+="_L"; }
                                ostringstream ost;
                                ost<<fprev1<<"_"<<f1<<"_"<<e1;
                                wcs[ost.str()]++;

                                // syntactic
                                if(f1 != "unk" && e1 != "unk"){
                                    typename constituent_list :: data_type cons ;
                                    cons = m_fconstituent_list.leftadj(ci->info()->span().left());
                                    BOOST_FOREACH(const string lab, cons){
                                        ostringstream ost1;
                                        ost1<<lab<<"_L"<<"_"<<f1<<"_"<<e1;
                                        wcs[ost1.str()]++;
                                    }
                                }
                            }
                            // generate the after word context feature
                            BOOST_FOREACH(string fafter,m_lattice.out_edges(ci->info()->span().right())){
                                string f1, fafter1, e1;
                                tie(bora,fafter1, f1,e1) = m_wsd.transform(make_tuple(false, fafter, f,e)); 
                                if(fafter1!="unk"&&(f1!="unk"||e1!="unk")){ fafter1+="_R"; }
                                // lexical
                                ostringstream ost;
                                ost<<fafter1<<"_"<<f1<<"_"<<e1;
                                wcs[ost.str()]++;

                                // syntactic
                                if(f1 != "unk" && e1 != "unk"){
                                    if(ci->info()->span().right() > 0){
                                        typename constituent_list :: data_type cons ;
                                        cons = m_fconstituent_list.rightadj(ci->info()->span().right()-1);
                                        BOOST_FOREACH(const string lab, cons){
                                            ostringstream ost1;
                                            ost1<<lab<<"_R"<<"_"<<f1<<"_"<<e;
                                            wcs[ost1.str()]++;
                                        }
                                    }
                                }
                            }
                        }
                        std::pair<string, int> v;
                        grammar_in_mem& g = const_cast<grammar_in_mem&>(grammar);
                        BOOST_FOREACH(v, wcs){
                            score_t sc = score_t(v.second, as_neglog10());
                            boost::uint32_t fid=g.feature_names().get_index(v.first);
                            *accum = make_pair(fid, sc);
                            ++accum;
                            inside*= sc ^ grammar.get_weights()[fid];
                        }

                        // how many links there are.
                        score_t sc = score_t(naligns, as_neglog10());
                        boost::uint32_t fid=g.feature_names().get_index("num_alignments");
                        *accum = make_pair(fid, sc);
                        ++accum;
                        inside*= sc ^ grammar.get_weights()[fid];

                    }
                } else {
                    //std::cout<<"RULE ID: "<<xrs_rule.id()<<" no align"<<endl;
                }
            }
        }

        sp = sbmt::span_t(begin(range)->info()->span().left(), clast_it->info()->span().right());
    }
    
    return make_tuple(inside,sp);
}

template <class ConstituentIterator, class Grammar>
sbmt::span_t 
word_context_info_factory::
compute_resulting_span(Grammar& grammar, 
                       typename Grammar::rule_type rule, 
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

} // namespace word_context

#endif
