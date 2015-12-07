#ifndef __StupidDLMScorer3g_h__
#define __StupidDLMScorer3g_h__

#include "sbmt/dependency_lm/PushdownAutomaton.hpp"
#include "sbmt/logmath.hpp"
#include <string>
#include <stack>
#include <deque>

using namespace std;

namespace sbmt {

template<class DLM_TYPE>
class StupidDLMScorer3g : public StupidDLMScorer<DLM_TYPE>
{
    typedef StupidDLMScorer<DLM_TYPE> __base;
public:
    // StupidDLMScorer3g(vector<boost::shared_ptr<DLM_TYPE> > dlms) : 
      //   __base(dlms) {}
     StupidDLMScorer3g(MultiDLM& dlms) : 
         __base(dlms) {}

     //! \return: the weighted sum score for this events. weighted sum over
     //!          all multi dlms.
     //! \param ind_dlm_scores: the dlm scores for individual dlms, multiplied
     //!          for all the dlm events.
    score_t getScoreForEvents(const deque<string>& input, DLM::score_output_iterator ind_dlm_scores); 

protected:

    score_t getDLMProb(const vector<string>& vec, DLM::score_output_iterator ind_dlm_scores, int whichLM);
    // generate the dlm events based on the current state.
    void generate_dlm_event(vector<string>& vec);
};
}
#include "sbmt/dependency_lm/StupidDLMScorer3g.cpp"

#endif
