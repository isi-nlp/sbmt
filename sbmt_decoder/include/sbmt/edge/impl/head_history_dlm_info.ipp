#ifndef SBMT_EDGE_head_history_dlm_info_ipp_
#define SBMT_EDGE_head_history_dlm_info_ipp_

#include <sbmt/grammar/rule_input.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <boost/scoped_array.hpp>
#include <algorithm>
#include <deque>
#include <stack>
#include "sbmt/logmath.hpp"

# include <sbmt/logging.hpp>

namespace sbmt {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(dlm_info_log,"dlm-info",root_domain);

// hash the boundary words.
// Do we have other good hash function for the boundary words?
template<unsigned N, class LMID_TYPE>
std::size_t head_history_dlm_info<N, LMID_TYPE>::hash_value() const {
    std::size_t ret = __base::hash_value();
    boost::hash_combine(ret, head); 
    return ret;
}


////////////////////////////////////////////////////////////////////////////////
//
//  head_history_dlm_info_factory methods
//
////////////////////////////////////////////////////////////////////////////////
template <unsigned N, class LM, bool Greedy> 
void head_history_dlm_info_factory<N, LM, Greedy>::
init(indexed_token_factory& tf)
{
    tf.native_word("@UNKNOWN@");
    PH= tf.tag("<PH>");
    L=  tf.tag("<L>");
    R=  tf.tag("<R>");
    E=  tf.tag("<E>");
    E_close = tf.tag("</E>");
    LB= tf.tag("<LB>");
    RB= tf.tag("<RB>");
    H= tf.tag("<H>");
}

/// Converts indexed dlm vector into strings. 
/// (i)  Fills the "PlaceHolder" with the leftmost/rightmost child of the head.
/// (ii) Decides new leftmost/rightmost words according to the content within
///       <LB> * </LB> and <RB> * <RB>; fills in the new words --- if the content
///       is an index, we need to fill the leftmost/rightmost dependent of the
///       head pointed to by the index if the index is the head; otherwise, we
///       merely fill the head of the index.
template <unsigned N, class LM, bool Greedy> 
template <class Gram>
void head_history_dlm_info_factory<N, LM, Greedy>::
deintegerize_dlm_string(const indexed_lm_string& lmstr, 
                                   Gram const& gram,
                                   std::vector<const info_type*> antecedent_ITs,
                                   std::deque<std::string>& dlmEvents) const 
{ 
    SBMT_DEBUG_STREAM(dlm_info_log, print(lmstr, gram.dict()));
 

    string eventType = "";  // <E>/<H>/<LB>/<RB>
    indexed_token ieventType=PH;
    string tok;  
    indexed_token itok;  
    unsigned int headIndex = (unsigned int) -1; //default -1 is for non-variable head.
    int start = -1;
    int end = -1;
    bool dlm_event_dir = false; // false -> left, true -> right.

    // generate a vector from the dep_lm_string and use the PDA to score it.
    for(size_t inx = 0; inx < lmstr.size(); ++inx){
        if(lmstr[inx].is_token()){ // token
            tok = gram.dict().label(lmstr[inx].get_token());
            itok = lmstr[inx].get_token();

            if(itok == PH){  continue; } // only for backward compatibility.

            if(itok == L) { 
                assert(ieventType == E);
                dlm_event_dir = false;
                start = (int)inx + 1;
            }
            else if(itok == R) { 
                assert(ieventType == E);
                dlm_event_dir = true; 
                start = (int)inx + 1;
            } else if(itok == E || itok == H || itok == LB || itok == RB){
                eventType = tok;
                ieventType = itok;
            } else {
                if(itok == E_close) {  // put the events into dlmEvents.
                    end = (int)inx ;
                    process_an_event(dlmEvents, lmstr, start, end, dlm_event_dir, gram, antecedent_ITs); ///
                    start = -1;
                }
            }

            // if start is not defined, then we are not in the processing event mode.
            // if start is defined, but we have not yet reached that point, then we are not
            // in processing event mode, either.
            if( start == -1 || start > (int)inx)  { dlmEvents.push_back(tok);}

        } else if(lmstr[inx].is_index()){ // var
            size_t varIndex = lmstr[inx].get_index();

            // if we are in an <E> event.
            if(ieventType == E){
                // we don nothing because we are recording the start and end of an event.
            } else if(ieventType == H){
                headIndex = varIndex;
                dlmEvents.push_back(
                    gram.dict().label(
                        indexed_token(antecedent_ITs[varIndex]->get_head(), 
                                      native_token)));
            } else if(ieventType == LB){
                if(headIndex != varIndex) { // this is a non-head var.
                        dlmEvents.push_back(
                            gram.dict().label(
                                indexed_token(antecedent_ITs[varIndex]->get_head(), 
                                              native_token)));
                } else {
                    for(int d = 0; d < (int)N; ++d) {
                        if( antecedent_ITs[varIndex]->get_bw(d,0) != (lm_id_type)-1){ 
                            dlmEvents.push_back(
                                gram.dict().label(
                                    indexed_token(antecedent_ITs[varIndex]->get_bw(d,0), 
                                                  native_token)));
                        }
                    }
                }
            } else if(ieventType == RB){
                if(headIndex != varIndex) { // this is a non-head var.
                        dlmEvents.push_back(
                            gram.dict().label(
                                indexed_token(antecedent_ITs[varIndex]->get_head(), 
                                              native_token)));
                } else {
                    for(int d = 0; d < (int)N; ++d) {
                        if( antecedent_ITs[varIndex]->get_bw(d,1) != (lm_id_type)-1){ 
                            dlmEvents.push_back(
                                gram.dict().label(
                                    indexed_token(antecedent_ITs[varIndex]->get_bw(d,1), 
                                                  native_token)));
                        }
                    }
                }
            } else {
                dlmEvents.push_back(
                    gram.dict().label(
                        indexed_token(antecedent_ITs[varIndex]->get_head(), 
                                      native_token)));
            }
        } else {
            throw std::runtime_error("dlm-info-factory: illegal contents in dlm_string");
		}

    }


    // if the PDA input has nonthing, we put an '<unk>' there.
    if(!dlmEvents.size()){
      // <H> means the head word that is decomposed from the old dep_lm_string.
      dlmEvents.push_back("<H>");
      dlmEvents.push_back("@UNKNOWN@");
      dlmEvents.push_back("</H>");
    }

//     std::cout<<"DLM EVENTS: ";
//     for(size_t ii = 0; ii < dlmEvents.size(); ++ii){
//          std::cout<<dlmEvents[ii]<<" ";
//     }
//    std::cout<<endl;
}

// converts the event in the dlm string (containing vars) into a vector of words.
// [start, end)
template <unsigned N, class LM, bool Greedy> 
template <class Gram>
void head_history_dlm_info_factory<N, LM, Greedy>::
process_an_event(std::deque<std::string>& eventsV, 
                 const indexed_lm_string& lmstr, 
                 int start, int end, 
                 bool event_dir, 
                 Gram const& gram,
                 std::vector<const info_type*> antecedent_ITs) const
{
    int i ;

    if(end > start) {

        // process boundary words. if we have a head in this event and the head is a variable,
        // we put the into the eventsV.
        if(end - 2 >= start && lmstr[end-2].is_index()){
            int head_i = end-2;
            if(!event_dir) { // a left event
                for(int d = (int)N-1; d >= 0; --d){ 
                    lm_id_type left_id=antecedent_ITs[lmstr[head_i].get_index()]->get_bw(d, 0);
                    if(left_id != (lm_id_type)-1){
                        eventsV.push_back( gram.dict().label(indexed_token(left_id, native_token)));
                    }
                }
            } else { // a right event.
                for(int d = 0; d != N; ++d){ 
                    lm_id_type right_id=antecedent_ITs[lmstr[head_i].get_index()]->get_bw(d,1);
                    if(right_id != (lm_id_type)-1){
                        eventsV.push_back( gram.dict().label(indexed_token(right_id, native_token)));
                    }
                }
            }
        }
        for( i = start; i < end;   ++i){

            // then we process the current token. but we put
            if(lmstr[i].is_index()) {
                if(antecedent_ITs[lmstr[i].get_index()]->get_head() != (lm_id_type)-1){
                    eventsV.push_back( gram.dict().label(
                                      indexed_token(antecedent_ITs[lmstr[i].get_index()]->get_head(), native_token)));
                }
            } else {
                string tok = gram.dict().label(lmstr[i].get_token());
                if (!(lmstr[i].get_token() == PH)) { eventsV.push_back(tok); }
            }
        }

    } // end > start

}

// assumes only scoreable infos.
template <unsigned int N, class LM, bool Greedy>
template <class G, class CR, class Accumulator>
void
head_history_dlm_info_factory<N, LM, Greedy>::
create_info( G const& gram
           , typename G::rule_type r
           , CR const& constituents
           , info_type& n
           , score_t& inside
           , score_t& heuristic
           , Accumulator const& accum_scores
           )
{
    // cout<<"IN compute_ngram 1\n";

    lm_id_type headid;
    //std::vector<score_t> ind_scores(m_depLMs->size());
    //for(size_t i = 0; i < ind_scores.size(); ++i){ ind_scores[i].set(as_one());}

    inside.set(as_one());
    heuristic.set(as_one());
    
    
    
    std::vector<const info_type*> decedent_infos;
    std::deque<string> dlmEvents;
    typedef typename boost::range_iterator<CR>::type CI;
    if (gram.rule_has_property(r,lmstrid)) {
        for (CI ci = boost::begin(constituents); ci != boost::end(constituents); ++ci) {
            if (not is_lexical(ci->root())) decedent_infos.push_back(ci->info());
        }
        deintegerize_dlm_string( gram.template rule_property<indexed_lm_string>(r,lmstrid)
                               , gram
                               , decedent_infos
                               , dlmEvents );
    } else {
        dlmEvents.push_back("<H>"); 
        dlmEvents.push_back("@UNKNOWN@");
        dlmEvents.push_back("</H>"); 
    }
    inside *= do_dlm_string( gram
                           , dlmEvents
                           , n.boundary_array(0)
                           , n.boundary_array(1)
                           , headid
                           , accum_scores );
    // n.print_self(cout,  gram.dict());
    n.set_head(headid);
    //scores_out.swap(ind_scores);
    //cout<<"OUT compute_ngram 1\n";
}

template <unsigned N, class LM, bool Greedy> 
template <class Grammar, class Accumulator>
score_t head_history_dlm_info_factory<N, LM, Greedy>::
compute_dlm_delta_score(const Grammar&gram, 
                        const indexed_lm_string& lmstr, 
                        Accumulator const& accum_scores) const
{
	size_t m;
    score_t ret ;
    ret.set(as_one());
    
    // generate a vector from the dep_lm_string and use the PDA to score it.
    std::deque<string> dlmEvents;

    for(indexed_lm_string::const_iterator i = lmstr.begin(),end= lmstr.end();
        i != end; ++i) {
        if(i->is_token()){
            // TODO: double check this function call.
            dlmEvents.push_back(gram.dict().label(i->get_token()));
        } else if(i->is_index()){ // var
            assert(0);
        } else {
			assert(0);
		}
    }

    if(!dlmEvents.size()){
      // cerr<<"unkunk\n";
      // <H> means the head word that is decomposed from the old dep_lm_string.
      dlmEvents.push_back("<H>");
      dlmEvents.push_back("@UNKNOWN@");
      dlmEvents.push_back("</H>");
    }

    // no need for tmp vector.  instead, an Accumulator is passed in...
    // and for create_info, the accumulator is a do-nothing operation, for faster
    // code.
    //vector<score_t> tmp_scores(m_depLMs->size());
    //for(int q = 0; q < (int)m_depLMs->size(); ++q){
    //    tmp_scores[q] = as_one();
    //}

    lm_id_type headid;
    ret*=do_dlm_string(gram, dlmEvents, NULL, NULL, headid, accum_scores);

    //for(m = 0; m < m_depLMs->size(); ++m){
    //    ind_dlm_scores[m] *= tmp_scores[m];
    //}

    return ret;
}

//// returns the probs of all the dlm events in the lmstr.
//// passes out the left boundary, right boundary, and the
//// head.
template <unsigned N, class LM, bool Greedy> 
template <class Grammar, class Accumulator>
score_t
head_history_dlm_info_factory<N, LM, Greedy>::
do_dlm_string(Grammar const& gram
              , std::deque<std::string>& dlmEvents
//              , std::vector<const edge<IT>*> edgevec
              , lm_id_type* lB
              , lm_id_type* rB
              , lm_id_type& headid
              , Accumulator const& accum_scores) const
{
    // cout<<"IN do dlm string\n";
    int i = 0;
    score_t ret;
    ret.set(as_one());

    headid = (lm_id_type) -1;

    while(i < (int)dlmEvents.size()){
        // cout<<dlmEvents[i]<<endl;
        int j;
        // event
        if(dlmEvents[i] == "<E>"){
            j=i+1;
            string index;
            int inx=-1;
            if(dlmEvents[j] == "<L>"){
                index = "0";
                inx = 0;
            } else if(dlmEvents[j] == "<R>"){
                index = "1";
                inx = 1;
            } else { throw std::logic_error("nonsensical dlm event " + dlmEvents[j]); }

            j++;

            vector<string> dependency;
            while(dlmEvents[j] != "</E>"){
                dependency.push_back(dlmEvents[j]); 
                ++j;
            }

            if(dependency.size() > N + 1){
                int sz =  dependency.size();
                for(int k = 0; k < sz - ((int)N + 1) ; ++k){
                    dependency.erase(dependency.begin());
                }
            }

            //vector<score_t> ind_scores_tmp(m_depLMs->size());
            score_t tt = m_depLMs->prob(dependency, inx, index, accum_scores);
            ret *= tt;

            //for(size_t p = 0; p < m_depLMs->size(); ++p){
            //    ind_scores[p] *= ind_scores_tmp[p];
            //}

            ++j;
            i = j;

        } else if(dlmEvents[i] == "<H>") {
            i++;
            headid= gram.dict().find_native_word(dlmEvents[i]).index();
            ++i;
            assert(dlmEvents[i] == "</H>");
            ++i;
        } else if(dlmEvents[i] == "<LB>"){
            i++;
            vector<string> bnds;
            while(dlmEvents[i] != "</LB>"){
                bnds.push_back(dlmEvents[i]);
                ++i;
            }

            //cout<<"LB: ";
            //for(int q = 0; q < bnds.size(); ++q){
              //  cout<<bnds[q]<<" ";
            //}
            //cout<<"/LB"<<endl;

            if(bnds.size() > N){
                bnds.resize(N);
            }
            //cout<<"LB-resize: ";
            //for(int q = 0; q < bnds.size(); ++q){
             //   cout<<bnds[q]<<" ";
            //}
           // cout<<"/LB-resize"<<endl;

            if(lB){
                int sz = N - bnds.size();
                for(int l = 0; l < sz; ++l){
                    lB[l] = (lm_id_type)-1;
                }
                for(int l = sz; l < (int)N; ++l){
                    lB[l] = gram.dict().find_native_word(bnds[l-sz]).index();
                }
            }

            i++;

        } else if(dlmEvents[i] == "<RB>"){
            i++;
            vector<string> bnds;
            while(dlmEvents[i] != "</RB>"){
                bnds.push_back(dlmEvents[i]);
                ++i;
            }

            //cout<<"LB: ";
            //for(int q = 0; q < (int)bnds.size(); ++q){
                //cout<<bnds[q]<<" ";
            //}
            //cout<<"/LB"<<endl;
            int szz = bnds.size() - N;
            for(int p = 0; p < szz; ++p){
                bnds.erase(bnds.begin());
            }
            //cout<<"LB-resize: ";
            //for(int q = 0; q < bnds.size(); ++q){
                //cout<<bnds[q]<<" ";
            //}
            //cout<<"/LB-resize"<<endl;

            int l, k;
            //int sz = (int)N-bnds.size();
            if(rB){
                for(l = 0; l< (int)bnds.size(); ++l){
                    rB[l] = gram.dict().find_native_word(bnds[l]).index();
                }
                //cout<<"SETTING RB: ";
                for(k = bnds.size();  k < (int)N; ++k){
                    rB[k] = (lm_id_type) -1;
                     //cout<<" "<<bnds[k];
                }
                 //cout<<endl;
            }

            i++;
        } else {
            assert(0);
        }
    }
    // cout<<"OUT do dlm string\n";
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

 //TODO: cache not through hashtable, but as member of rule
template<unsigned N, class LM, bool Greedy> 
template <class G>
score_t 
head_history_dlm_info_factory<N, LM, Greedy>::
rule_heuristic(G const& gram, typename G::rule_type r) const
{   
    // wei, ngram-info calculates the product of the ngrams of lm strings as
    // a rule heuristic.  can deplm do anything similar? --michael
    return as_one();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt


#endif
