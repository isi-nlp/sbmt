#ifndef __BBNTrigramDLM_h__
#define __BBNTrigramDLM_h__
#include "DLM.h"

namespace sbmt {

/// The BBN trigram DLM.
template<class LMType>
class BBNTrigramDLM : public DLM<LMType>
{
public:

    BBNTrigramDLM() {}

    /// Returns the probability of a tree for a sentence.
    sbmt::score_t treeProb(string line);

    /// Returns the probability of a treebank.
    sbmt::score_t tbProb(string filename);

    /// Decompose a tree into dlm events.
    void decompose(treelib::Tree& tr, std::ostream& out);

    /// Input.
    void read(string filename) ;
protected:
private:
}

} // end namespace sbmt
#include "sbmt/dependency_lm/StupidDLMScorer.cpp"


#endif
