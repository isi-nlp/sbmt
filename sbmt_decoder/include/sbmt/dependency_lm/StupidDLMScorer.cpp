#ifndef __StupidDLMScorer_cc__
#define __StupidDLMScorer_cc__

//#include "sbmt/dependency_lm/StupidDLMScorer.h"
#include <iostream>
#include <assert.h>
#include <deque>
#include <vector>
#include <string>
#include "sbmt/logmath.hpp"
#include <boost/function_output_iterator.hpp>

struct nullout {
    template <class T>
    void operator()(T const& t) const { return; }
};

namespace {
boost::function_output_iterator<nullout> nulloutitr;
}

using namespace std;

namespace sbmt {

template<class DLM_TYPE>
StupidDLMScorer<DLM_TYPE>::
StupidDLMScorer(MultiDLM& dlms)  : m_dlms(dlms)
{ 
    m_score.set(as_one()); 
    m_dlmEvents.clear();
    m_dontScore = false;
}

template<class DLM_TYPE>
void StupidDLMScorer<DLM_TYPE>::computeForTree(const deque<string>& input) 
{ 
    deque<string> in ;
    for(size_t i = 0; i < input.size(); ++i) {
        if(input[i] != "<PH>"){ in.push_back(input[i]);}
    }

    __base::setInput(in);
    if(in.size()){
        __base::m_input.push_front(string("<D>"));
        __base::m_input.push_back(string("</D>"));
    }
    __base::m_stack.resize(__base::m_input.size()); 
    __base::m_positStack.resize(__base::m_input.size()); 

    m_score.set(as_one()); 

    transit();

}

template<class DLM_TYPE>
score_t StupidDLMScorer<DLM_TYPE>::getScoreForTree(const deque<string>& input) 
{
    m_dontScore = false;
    computeForTree(input);
    return m_score;
}

template<class DLM_TYPE>
vector<string> StupidDLMScorer<DLM_TYPE>::getDLMEventsForTree(const deque<string>& input) 
{
    m_dontScore = true;
    computeForTree(input);
    return m_dlmEvents;
}

template<class DLM_TYPE>
score_t StupidDLMScorer<DLM_TYPE>::
getScoreForEvents(const deque<string>& input, DLM::score_output_iterator scores_out) 
{
    unsigned i = 0; 

    m_dontScore = false;

    score_t s = as_one();

    while(i < input.size()){
        if(input[i] == "<E>"){
            vector<string> vec(2);
            vec[0] = input[i+1];
            vec[1] = input[i+2];
            s  *= getDLMProb(vec,scores_out, 0);
            i+=4;
        } else  if(input[i] == "<H>") {
            m_currState.headword = input[i+1];
            m_unscored.push_back(input[i+1]);
            break;
        }
    }
    //std::cout.flush();
    return  s;
}

template<class DLM_TYPE>
void StupidDLMScorer<DLM_TYPE>::reset()
{
    m_dlmEvents.clear();
    m_stack.clear();
    m_top = 0;
    m_currState.clear();
    m_inputPosit = 0;
    while(m_stateStack.size()) {
        m_stateStack.pop();
    }

    m_score.set(as_one());

    m_boundaryWords.clear();
}

template<class DLM_TYPE>
typename StupidDLMScorer<DLM_TYPE>::InputAlphabet 
StupidDLMScorer<DLM_TYPE>::getCurrentInput() const 
{
    if(m_inputPosit >= (int)m_input.size()){
        cerr<<"No input any more, but you still read in StupidDLMScorer!\n";
        exit(1);
    }
    return m_input[m_inputPosit];
}


template<class DLM_TYPE>
void StupidDLMScorer<DLM_TYPE>::reduce()
{
    if(stackTop() == "</D>") {
        assert(stackSize()>2);
        assert(m_stack[stackSize() - 1 - 2] == "<D>");
        m_stack[stackSize() - 3] = m_stack[stackSize() - 2];
        m_top -= 2;
    } else if(stackTop() == "</H>"){
        assert(stackSize()>2);
        assert(m_stack[stackSize() - 1 - 2] == "<H>");
        // put the word still on the top.
        m_stack[stackSize() - 3] = m_stack[stackSize() -2];
        m_top -= 2;
    }
}

template<class DLM_TYPE>
void StupidDLMScorer<DLM_TYPE>::transit()
{
    reset();
    while(m_inputPosit < (int) m_input.size() || stackSize() > 1){
        oneStepTransit();
    }
    if(m_currState.headword != ""){
        m_unscored.push_back(m_currState.headword);
    } else {
        m_unscored.push_back(stackTop());
    }

    m_boundaryWords.push_back(m_prevState.leftmostNonheadWord);
    m_boundaryWords.push_back(m_prevState.rightmostNonheadWord);
}

template<class DLM_TYPE>
void StupidDLMScorer<DLM_TYPE>::post_transit()
{}

template<class DLM_TYPE>
void StupidDLMScorer<DLM_TYPE>::oneStepTransit()
{
    // printStack();
    // input exhausted.
    if(m_inputPosit >= (int) m_input.size()){ return;}

    /// cout<<"CUR INPUT"<<getCurrentInput()<<endl;
    ///if(stackSize()) {
       // cout<<"STACK TOP"<<stackTop()<<endl;
    /// }
    /// cout<<"Dir state: "<<m_currState.dirState<<endl;
    /// cout<<"head state: "<<m_currState.headState<<endl;
    /// cout<<endl;

    if(m_currState.dirState == "Qshift") {
        if(m_currState.headState == "Qn"){
            if(getCurrentInput() == "<D>") {
                __base::m_positStack[stackSize() - 1 ] = m_inputPosit;
                enqueue(getCurrentInput());
                m_stateStack.push(m_currState);
                m_currState.clear();
                m_currState.dirState = "Qshift";
                m_inputPosit++;
            } else if(getCurrentInput() == "</D>"){
                __base::m_positStack[stackSize() - 1 ] = m_inputPosit;
                enqueue(getCurrentInput());
                m_currState.dirState = "Qreduce";
            } else if(getCurrentInput() == "<H>") {
                __base::m_positStack[stackSize() - 1 ] = m_inputPosit;
                enqueue(getCurrentInput());
                m_currState.headState = "Qe";
                m_currState.dirState = "Qshift";
                m_inputPosit++;
            } else if(getCurrentInput() == "</H>"){
                std::cerr<<"Invalid state ("<<m_currState.dirState<<" " <<getCurrentInput()<<")"<<endl;
                exit(1);
            } else { // word
                __base::m_positStack[stackSize() - 1 ] = m_inputPosit;
                enqueue(getCurrentInput());
                m_currState.dirState = "Qshift";
                ++m_inputPosit;
            }
        } else if(m_currState.headState == "Qe") {
            if(getCurrentInput() == "</H>"){
                //enqueue(getCurrentInput());
                //m_currState.dirState = "Qshfit";
                //m_inputPosit ++;
                __base::m_positStack[stackSize() - 1 ] = m_inputPosit;
                enqueue(getCurrentInput());
                m_currState.dirState = "Qreduce";
            } else if(getCurrentInput() ==  "<D>" ||
                      getCurrentInput() ==  "</D>" ||
                      getCurrentInput() == "<H>"){
                std::cerr<<"Invalid state ("<<m_currState.dirState<<" " <<getCurrentInput()<<")"<<endl;
                exit(1);
            } else { // word
                __base::m_positStack[stackSize() - 1 ] = m_inputPosit;
                enqueue(getCurrentInput());
                m_currState.dirState = "Qshift";
                m_inputPosit++;
            }
        } else if(m_currState.headState == "Qh"){
            if(getCurrentInput() == "<D>"){
                __base::m_positStack[stackSize() - 1 ] = m_inputPosit;
                enqueue(getCurrentInput());
                m_stateStack.push(m_currState);
                m_currState.clear();
                m_currState.dirState = "Qshift";
                m_inputPosit ++;
            } else if(getCurrentInput() == "</D>"){
                __base::m_positStack[stackSize() - 1 ] = m_inputPosit;
                enqueue(m_currState.headword);
                __base::m_positStack[stackSize() - 1 ] = m_inputPosit;
                enqueue(getCurrentInput());
                m_currState.dirState = "Qreduce";
            } else {
                std::cerr<<"Invalid state ("<<m_currState.dirState<<" " <<getCurrentInput()<<")"<<endl;
                exit(1);
            }

        } else {
            std::cerr<<__LINE__<<"Wrong head state in StupidDLMScorer!\n exiting \n";
            exit(1);
        }
    }  else if(m_currState.dirState == "Qreduce"){
        if(m_currState.headState == "Qn"){
            if(stackTop() == "</D>"){
                // reduce <D> word </D> to just word.
                reduce();
                assert(m_stateStack.size());
                /// hmmmm
                if(m_stateStack.size() == 1){
                    m_prevState = m_currState;
                }
                m_currState = m_stateStack.top();
                //m_currState.print();
                // resume the state of the upper level.
                /// cout<<"Popping\n";
                if(m_stateStack.size()==2){
                    m_prevState = m_stateStack.top();
                }
                m_stateStack.pop();
                m_currState.dirState = "Qreduce";
            } else if(stackTop() == "<D>" ||
                      stackTop() == "<H>" ||
                      stackTop() == "</H>"){
                std::cerr<<"Invalid state ("<<m_currState.dirState<<" " <<getCurrentInput()<<")"<<endl;
                exit(1);
            } else { // word
                m_currState.dirState = "Qshift";
                ++m_inputPosit;
            }

        } else if(m_currState.headState == "Qe") {
            if(stackTop() == "</H>"){
                reduce(); // head word is on the top.
                m_currState.headword = stackTop();
                m_currState.headposit= m_inputPosit;

                m_top --; // we dont put the word on the top.
                m_currState.dirState = "Qreduce";
                m_currState.headState = "Qh";
            } else {
                std::cerr<<"Invalid state ("<<m_currState.dirState<<" " <<getCurrentInput()<<")"<<endl;
                exit(1);
            }

        } else if(m_currState.headState == "Qh"){
            if(stackTop() == "<D>"){
                //enqueue(m_currState.headword);
                m_currState.dirState = "Qshift";
                ++m_inputPosit;
            } else if(stackTop() == "</D>"){
                reduce();
                assert(m_stateStack.size());
                // hmmm: 1 means that we are at the top level.
                // we are now at the end of the top level, 
                // thus we memo-size the top level state using 
                // the m_prevState;
                if(m_stateStack.size() == 1){
                    m_prevState = m_currState;
                }
                m_currState = m_stateStack.top();
                //m_currState.print();
                /// cout<<"Popping\n";
                if(m_stateStack.size()==2){
                    m_prevState = m_stateStack.top();
                } 
                m_stateStack.pop();
                if(m_currState.headState == "Qh"){
                    m_currState.dirState = "Qreduce";
                } else {
                    m_currState.dirState = "Qshift";
                    m_inputPosit++;
                }
            } else if(stackTop() == "<H>" ||
                      stackTop() == "</H>" ){
                std::cerr<<"Invalid state ("<<m_currState.dirState<<" " <<getCurrentInput()<<")"<<endl;
                exit(1);
            } else { // word
                //cout<<"DPAIR: "<<m_currState.headword<<" "<<stackTop()<<endl;
                // we compute the score here.
                vector<string> vec;
                assert(m_currState.headword != "");
                ///vec.push_back(m_currState.headword);
                ///vec.push_back(stackTop());
                generate_dlm_event(vec);
                /// cout<<"score: "<<getDLMProb(vec);
                
                m_score *= getDLMProb(vec, DLM::score_output_iterator(nulloutitr)); // compute the prob.
                // remove word from top;
                --m_top;
            }
        } else {
            std::cerr<<__LINE__<<"Wrong head state in StupidDLMScorer!\n exiting \n";
            exit(1);
        }
    } else {
        std::cerr<<__LINE__<<"Wrong direction state in StupidDLMScorer!\n exiting \n";
        exit(1);
    }

}

template<class DLM_TYPE>
void StupidDLMScorer<DLM_TYPE>::generate_dlm_event(vector<string>& vec)
{
    vec.push_back(m_currState.headword);
    vec.push_back(stackTop());
}

template<class DLM_TYPE>
score_t StupidDLMScorer<DLM_TYPE>::getDLMProb(const vector<string>& vec,
                                              DLM::score_output_iterator scores_out,
                                             int whichLM)
{
    if(m_dontScore){
        m_dlmEvents.push_back("<E>");
        for(size_t i = 0; i < vec.size(); ++i){
            m_dlmEvents.push_back(vec[i]);
        }
        m_dlmEvents.push_back("</E>");
        return as_one();
    } else {
        assert(vec.size() == 2);
        score_t ss = m_dlms.prob(vec, 0, string(""), scores_out);
        //typename DLM_TYPE::lm_id_type ar[3];
        //ar[0] = m_dlms[whichLM]->id(vec[0]);
        //ar[1] = m_dlms[whichLM]->id(vec[1]);
        cout<<"ngram: "<<vec[0]<<" " <<vec[1];
        //score_t ss = m_dlms[whichLM]->sub().prob(ar, 2);
        // cout<<" "<<ss<<endl;
        return ss;
    }
}

} // namespace sbmt
#endif
