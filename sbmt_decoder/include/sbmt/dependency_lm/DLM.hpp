#ifndef __DLM_H__
#define __DLM_H__

#include "sbmt/logmath.hpp"
#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <sbmt/feature/feature_vector.hpp>
#include <vector>
#include <iostream>
#include <string>
#include "RuleReader/Rule.h"
#include <gusc/iterator/any_output_iterator.hpp>
#include <gusc/string/escape_string.hpp>

using namespace std;

namespace sbmt {

/////////////////////////////////////////////////////////////////////////////////////////////////

//! the abstract class for dependency LMs.
//! This is the wrapper of different DLMS for decoder to use.
class DLM {
    int m_order ;
public:
    typedef gusc::any_output_iterator< 
                std::pair<boost::uint32_t,score_t>
            > score_output_iterator;
    DLM() { m_order = 3;}
    virtual ~DLM() {}

    typedef dynamic_ngram_lm::lm_id_type lm_id_type;
    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;

    //! if there are more than one submodel in this DLM,
    //! the memo will contain the index of the submodel.
    //! the returned value is the weighted sum if this DLM is multi-dlm
    virtual score_t prob(const vector<string>&, int inx, string memo, score_output_iterator out) const = 0;
    
    template <class ScoreOut>
    score_t prob(const vector<string>& v, int inx, string memo, ScoreOut out) const
    {
        return prob(v,inx,memo,score_output_iterator(out));
    }

    virtual std::string const& word(lm_id_type id) const = 0;
    // virtual lm_id_type const& id(std::string wd) const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;
    virtual void create(string filename) = 0;
    virtual void set_weights(weight_vector const& combine,feature_dictionary& dict) = 0;

    virtual void setOrder(int order)  { m_order = order;}
    virtual int order() const { return m_order;}
};

/////////////////////////////////////////////////////////////////////////////////////////////////



//! switch dlm (i.e., to the left or to the right).
class SwitchDLM : public DLM {
    typedef DLM _base;
public:
    typedef _base::lm_id_type lm_id_type;
    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;

    SwitchDLM(const string name);
    SwitchDLM() {}
    virtual ~SwitchDLM();

    //! if there are more than one submodel in this DLM,
    //! the memo will contain the index of the submodel.
    //! the ind_dlm_scores is never useful.
    virtual score_t prob(const vector<string>&, int inx, string memo, score_output_iterator out ) const;

    std::string const& word(lm_id_type id) const;
    //lm_id_type const& id(std::string wd) const;
    size_t size() const { return m_dlms.size();}
    virtual void clear() ;
    virtual void create(string filename) {}
    virtual void set_weights(weight_vector const& weights, feature_dictionary& dict)  {}

protected:
    vector<dynamic_ngram_lm*> m_dlms;
};


/////////////////////////////////////////////////////////////////////////////////////////////////

//! multi dlm (i.e., dlm1, dlm2)
//! Assumes that all the dlms have the same types of submodels.
//! I.e., so far, you cannot use multi-lm to store a 3-gram dlm
//! and a bi-gram dlm. 
class MultiDLM : public DLM {
    typedef DLM _base;
public:
    typedef _base::lm_id_type lm_id_type;
    typedef lm_id_type * iterator;
    typedef lm_id_type const* const_iterator;

    MultiDLM(const string filename, weight_vector const& weights, feature_dictionary& dict);
    MultiDLM() {}
    virtual ~MultiDLM();
    
    template <class ScoreOut>
    score_t prob(const vector<string>& v, int inx, string memo, ScoreOut out) const
    {
        return prob(v,inx,memo,score_output_iterator(out));
    }

    //! if there are more than one submodel in each dlm of this multi-dlm,
    //! the memo will contain the index of the submodel.
    //! the returned value is the weighted sum of multi-dlms. the weights are
    //! according to the combine passed via the constructor.
    //! the ind_dlm_scores stores the dlm scores for individual dlms.
    //!
    //! the dlm score for any individual dlm will be prob 1.0 (or logprob 0.0)
    //! if the corresponding dlm is not found in the score_combiner.
    //!
    //! the returned score will be prob 1.0 (or logprob 0) if none of the
    //! individual dlm is used according to the score_combiner.
    virtual score_t prob( const vector<string>&
                        , int inx
                        , string memo
                        , score_output_iterator ) const;

    std::string const& word(lm_id_type id) const;
    
    template <class WordIterator>
    void print(std::ostream& out, WordIterator itr, WordIterator end) const
    {
        if (itr != end) {
            out << '"' << gusc::escape_c(word(*itr)) << '"';
            ++itr;
        }
        for (; itr != end; ++itr) {
            out << ',' << '"' << gusc::escape_c(word(*itr)) << '"';
        }
    }
    size_t size() const { return m_dlms.size();}
    virtual void clear() ;
    virtual void create(string filename);
    virtual void set_weights(weight_vector const& weights, feature_dictionary& dict);

    /// Decompose a rule into DLM events. Output via the ostream.
    /// Rule r must have head markers.
    virtual void decompose(ns_RuleReader::Rule& r, ostream& out);

    /// Returns the pointer to the head word (leaf) of the node (input).
    /// Passes out the map from the head word pointer (to leaves) to the left & right dependents.
    /// (in the dependents list, the head word is not pushed in yet.)
    ns_RuleReader::RuleNode*
    compileHead2DependencyMap(ns_RuleReader::Rule& r, 
                              ns_RuleReader::RuleNode* node, 
                              map<ns_RuleReader::RuleNode*, pair<deque<string>, deque<string> > >& m);

protected:
    vector<DLM*> m_dlms;
    vector< std::pair<boost::uint32_t,double> > m_weights;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

/// Ordinary N-gram DLM is p_L(w | w_prev1, w_prev2, w_prev3) or p_R(w | w_prev1, w_prev2, w_prev3).
/// These DLMs can be smoothed using the stardard backing-off smoothing method.
class OrdinaryNgramDLM : public MultiDLM {
public:
    OrdinaryNgramDLM() {}
    virtual ~OrdinaryNgramDLM() {}

    /// Decompose a rule into DLM events. Output via the ostream.
    /// Rule r must have head markers.
    void decompose(ns_RuleReader::Rule& r, ostream& out);


};


} // sbmt

#endif
