#ifndef __StupidDLMScorer_h__
#define __StupidDLMScorer_h__

#include "sbmt/dependency_lm/PushdownAutomaton.hpp"
#include "sbmt/dependency_lm/DLM.hpp"
#include "sbmt/logmath.hpp"
#include <string>
#include <stack>
#include <deque>

using namespace std;

namespace sbmt {

template<class DLM_TYPE>
class StupidDLMScorer : public PushdownAutomaton<string, string>
{
    typedef PushdownAutomaton<string, string> __base;
    typedef string InputAlphabet;
    typedef string StackAlphabet;
public:
    //StupidDLMScorer(vector<boost::shared_ptr<DLM_TYPE> > dlms);
    StupidDLMScorer( MultiDLM& dlms);
    void reset(); 
    virtual void transit();

    virtual score_t getScoreForTree(const deque<string>& input); 
    virtual score_t getScoreForEvents(const deque<string>& input, DLM::score_output_iterator ind_dlm_scores); 

   virtual const vector<string>& getUnscoredWords() const {
        return m_unscored;
    }
    virtual vector<string>& getUnscoredWords() {
        return m_unscored;
    }

    virtual vector<string> getBoundaryWords() const { return m_boundaryWords; }

    //! Get the decomposed dlm events.
    virtual vector<string> getDLMEventsForTree(const deque<string>& input) ;

protected:
    virtual void post_transit();

    //! ind_dlm_scores is resized (to the number of dlms in the multi-dlm), and its contents are changed.
    virtual score_t getDLMProb(const vector<string>& vec, DLM::score_output_iterator ind_dlm_scores, int whichLM=0);

    class StateType {
    public:
        StateType() {clear();}
        void clear() {
            headposit = -1;
            headword = ""; 
            headState= "Qn"; 
            dirState= "Qshift";
            lastWord = "";
            lastWordPosit = -1;
            leftmostNonheadWord = "";
            rightmostNonheadWord = "";
        }
        void print() {
            cout<<" >>>>>>>>>>>>>>>\n";
            cout<<"head posit: "<<headposit<<endl;
            cout<<"head word: "<<headword<<endl;
            cout<<"headState: "<<headState<<endl;
            cout<<"dirState: "<<dirState<<endl;
            cout<<"lastWord: "<<lastWord<<endl;
            cout<<"left: "<<leftmostNonheadWord<<endl;
            cout<<"right: "<<rightmostNonheadWord<<endl;
            cout<<" <<<<<<<<<<<<<<<<<<<<<<\n";
        }
        string headword;
        string headState;
        int headposit; // head poistion in the m_input
        string dirState;
        string lastWord; // the last word processed. NB. we are processing
                         // in a head-out manner.
        int lastWordPosit;
    
        string leftmostNonheadWord; // the leftmost non-head word in the same level. can be "".
        string rightmostNonheadWord; // the rightmost non-head word in the same level. can be "".

    };
    //! The current state.
    StateType m_currState;
    StateType m_prevState;
    int m_inputPosit;

    //! Transit one step.
    void oneStepTransit();
       
    //! Reduce the stack.
    void reduce();

    // generate the dlm events based on the current state.
    virtual void generate_dlm_event(vector<string>& vec);

    //! Returns the first symbol in the current input.
    InputAlphabet getCurrentInput() const ;

    //vector<boost::shared_ptr<DLM_TYPE> > m_dlms;
    MultiDLM& m_dlms;
    score_t m_score; 

    vector<string> m_unscored;
    vector<string> m_boundaryWords;

    //! Used to memo-size the states when going deeper into children nodes.
    //! The state information can be memo-ized in the stack of pushdown
    //! machine but I used this stateStack just for the convenience of 
    //! implementation.
    stack<StateType> m_stateStack;

    //! Get DLMEvents only.
    vector<string> m_dlmEvents;
    bool m_dontScore;

    void computeForTree(const deque<string>& input) ;

};



}
#include "sbmt/dependency_lm/StupidDLMScorer.cpp"

#endif
