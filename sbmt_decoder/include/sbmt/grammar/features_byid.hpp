#ifndef SBMT__GRAMMAR__FEATURE_VECTOR_BYID_HPP
#define SBMT__GRAMMAR__FEATURE_VECTOR_BYID_HPP

#include <graehl/shared/assoc_container.hpp>
#include <graehl/shared/accumulate.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <sbmt/token/in_memory_token_storage.hpp>
#include <sbmt/logmath.hpp>
#include <vector>
#include <utility> // std::pair
#include <graehl/shared/word_spacer.hpp>
#include <sbmt/feature/feature_vector.hpp>

namespace sbmt {

typedef std::pair<feature_id_type,score_t> score_byid_type;
typedef std::vector<score_byid_type> scores_byid_type;

struct feature_vector_byid : public scores_byid_type
{
    inline void add_byid(feature_id_type id,score_t score)
    {
        if (!score.is_one()) // FIXME: may be redundant if BRF_ARCHIVE_CLEAR_MAP defined in brf_archive_io.cpp (that is: contract is that syntax reader callbacks are given sparse maps ...
            push_back(score_byid_type(id,score));
    }
    template <class features_type>
    feature_vector_byid(features_type const& scores,feature_names_type &dict)
    {
        this->reserve(scores.size());
        for (typename features_type::const_iterator i=scores.begin(),e=scores.end();
             i!=e;++i) {
            add_byid(dict.get_index(i->first),i->second);
        }
    }
    template <class O,class Dict>
    void print(O&o,Dict const& dict) const
    {
        graehl::word_spacer sp;
        for (const_iterator i=begin(),e=end();i!=e;++i)
            o << sp << dict.get_token(i->first)<<'='<<i->second;
    }

    // return score(1) if feature id not found
    score_t at_default(feature_id_type id) const
    {
        for (scores_byid_type::const_iterator i=begin(),e=end();i!=e;++i)
            if (i->first == id)
                return i->second;
        return as_one();
    }

};


template <class O>
void print(O& o,feature_vector_byid const& fv, feature_names_type const& dict)
{
    fv.print(o,dict);
    /*
    for (feature_vector_byid::const_iterator i=fv.begin(),e=fv.end();i!=e;++i)
        out << sp << dict.get_token(i->first) << "=" << i->second;
    */
    /*
          for (int x = 0; x != fv.size(); ++x) {
        if (x == 0) out << dict.get_token(fv[x].first) << "=" << fv[x].second;
        else out << " " << dict.get_token(fv[x].first) << "=" << fv[x].second;
    }
    */
}

struct feature_accum_byid
#ifdef SPARSE_FEATURE_ACCUM
 : public std::map<feature_id_type,score_t>
#else
 : public std::vector<score_t>
#endif
{
    typedef std::vector<score_t> parent;
    feature_accum_byid(feature_names_type const& dict)
#ifndef SPARSE_FEATURE_ACCUM
     : parent(dict.iend(),score_t(as_one())) {}
#endif


    // e.g. feature_vector_byid
    template <class features_type>
    void accumulate(features_type const& feats)
    {
#ifdef SPARSE_FEATURE_ACCUM
        accumulate_at_pairs(*this,feats,graehl::accumulate_multiply());
#else
        for (typename features_type::const_iterator i=feats.begin(),e=feats.end();
             i!=e;++i)
            (*this)[i->first]*=i->second;
#endif
    }

    template <class O>
    struct print_visitor
    {
        O&o;
        graehl::word_spacer sp;
        bool print_neglog10;

        print_visitor(O &o,bool print_neglog10=false)
            : o(o),print_neglog10(print_neglog10) {}

        void operator()(std::string const& name, score_t const&val)
        {
            o << sp << name << '=';
            if (print_neglog10)
                o << val.neglog10();
            else
                o << val;
        }
    };


    // visitor(string const& name,score_t const&val)
    template <class Visitor>
    void enumerate_name_score(Visitor &v,feature_names_type const& dict,bool sparse=true) const
    {
        feature_id_type i=dict.ibegin(),e=dict.iend(); // iterate over legal indexes so we can print default (0 value), not just up to maximum nonzero index seen
#ifdef SPARSE_FEATURE_ACCUM
        feature_id_type m=e;
        if (size()<e)
            e=size(); // ensures that we don't need to boundscheck
#else
#endif
        assert(e>=size()); // otherwise you used the wrong dict
        for (;i<e;++i) { // don't check by != what if vector is empty?
            score_t s=((parent const &)*this)[i];
            if (!s.is_one() || !sparse)
                v(dict.get_token(i),s);
        }
#ifdef SPARSE_FEATURE_ACCUM
        if (!sparse)
            for (;i<m;++i)
                v(dict.get_token(i),score_t(as_one()));
#else
#endif
    }

    template <class O>
    void print(O &o,feature_names_type const& dict,bool print_neglog10=false,bool sparse=true) const
    {
        print_visitor<O> v(o,print_neglog10);
        enumerate_name_score(v,dict,sparse);
    }

};

}//sbmt


#endif
