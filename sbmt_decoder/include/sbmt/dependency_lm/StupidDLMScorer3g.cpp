#ifndef __StupidDLMScorer3g_cc__
#define __StupidDLMScorer3g_cc__

//#include "sbmt/dependency_lm/StupidDLMScorer.h"
#include <iostream>
#include <assert.h>
#include <deque>
#include <vector>
#include <string>
#include "sbmt/logmath.hpp"


using namespace std;

namespace sbmt {


template<class DLM_TYPE>
score_t StupidDLMScorer3g<DLM_TYPE>::
getScoreForEvents(const deque<string>& input, DLM::score_output_iterator scores_out)
{
    unsigned i = 0; 

    __base::m_boundaryWords.clear();

    __base::m_dontScore = false;

    score_t s = as_one();

    bool right = true;
    vector<string> vec;
    while(i < input.size()){
        // skip <PH>.
        if(input[i] == "<PH>"){
            ++i;
        } else if(input[i] == "<E>"){
            vec.clear();
            ++i;
        } else if (input[i] == "</E>"){
            s  *= getDLMProb(vec, scores_out, right);
            ++i;
        } else if(input[i] == "<L>"){
            right = false;
            ++i;
        } else if(input[i] == "<R>") {
            right = true ;
            ++i;
        } else  if(input[i] == "<H>") {
            __base::m_currState.headword = input[i+1];
            __base::m_unscored.push_back(input[i+1]);
            ++i;
        } else if(input[i] == "</H>"){
            ++i;
        } else if(input[i] == "<LB>"){
            if(input[i+1] != "</LB>"){
                __base::m_boundaryWords.push_back(input[i+1]);
            } else {
                __base::m_boundaryWords.push_back("");
            }
            ++i;
        } else if (input[i] == "</LB>"){
            ++i;
        } else if(input[i] == "<RB>"){
            if(input[i+1] != "</RB>"){
                __base::m_boundaryWords.push_back(input[i+1]);
            } else {
                __base::m_boundaryWords.push_back("");
            }
            ++i;
        } else if (input[i] == "</RB>"){
            ++i;
        } else {
            vec.push_back(input[i]);
            ++i;
        }
    }
    return  s;
}

template<class DLM_TYPE>
void StupidDLMScorer3g<DLM_TYPE>::generate_dlm_event(vector<string>& vec)
{
    // directory
    if(__base::m_currState.headposit < __base::m_inputPosit){
        vec.push_back("<R>");
        if(__base::stackSize()){
            __base::m_currState.rightmostNonheadWord = __base::stackTop();
            //cout<<"right "<<__base::m_currState.rightmostNonheadWord<<endl;
        }
    } else {
        vec.push_back("<L>");
        if(__base::stackSize()){
            __base::m_currState.leftmostNonheadWord = __base::stackTop();
            // cout<<"left "<<__base::m_currState.leftmostNonheadWord<<endl;
        }
    }
    if(__base::m_currState.lastWord != "") {
        if(__base::m_currState.headposit < __base::m_inputPosit &&
                         __base::m_currState.headposit > __base::m_currState.lastWordPosit){
                             // do nothing.
         } else {
            vec.push_back(__base::m_currState.lastWord );
         }
    }
    vec.push_back(__base::m_currState.headword);
    __base::m_currState.lastWord = __base::stackTop();
    __base::m_currState.lastWordPosit = __base::m_positStack[__base::stackSize()-1];
    vec.push_back(__base::stackTop());
}

template<class DLM_TYPE>
score_t StupidDLMScorer3g<DLM_TYPE>::getDLMProb(const vector<string>& vec,
                                               DLM::score_output_iterator scores_out,
                                               int whichLM
                                               )
{
    if(__base::m_dontScore){
        __base::m_dlmEvents.push_back("<E>");
        for(size_t i = 0; i < vec.size(); ++i){
            if(vec.size() == 3) {
                if(i == 1){ // i=0 points to the head.
                    // <PH> will be a possible space holder 
                    // for the boundary word.
                    __base::m_dlmEvents.push_back("<PH>");
                }
            }
            __base::m_dlmEvents.push_back(vec[i]);
        }
        __base::m_dlmEvents.push_back("</E>");
        return as_one();
    } else {
        //cout<<"[dlm.ngram] ";
        //for(size_t l =0; l < vec.size(); ++l){
            //cout<<vec[l]<<" ";
        //}

        // we -1 because the vec[0] is the direction.
        ostringstream ost;
        ost<<whichLM;
        score_t ss = __base::m_dlms.prob(vec, whichLM, ost.str(), scores_out);
        //typename DLM_TYPE::lm_id_type ar[4];
        //for(unsigned n = 0; n < vec.size(); ++n){
         //   ar[n] = __base::m_dlms[whichLM]->id(vec[n]);
        //}
        //score_t ss = __base::m_dlms[whichLM]->sub().prob(ar, vec.size());
        //cout<<ss<<endl;

        return ss;

    }
}

} // namespace sbmt
#endif
