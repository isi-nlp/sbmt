#ifndef   SBMT_GRAMMAR_SCORE_COMBINER_HPP
#define   SBMT_GRAMMAR_SCORE_COMBINER_HPP

#include <exception>
#include <stdexcept>
#include <string>
#include <map>
#include <functional>
#include <sbmt/logmath.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/assoc_container.hpp>
#include <sbmt/grammar/features_byid.hpp>

#if 0
namespace sbmt {


class score_combiner_byid;

////////////////////////////////////////////////////////////////////////////////

class score_combiner : public boost::additive<score_combiner>
{
public:
    typedef std::string feature_key_type;
    typedef std::map<feature_key_type, score_t> scores_type;
    typedef std::map<feature_key_type, double>::iterator iterator;
    typedef std::map<feature_key_type, double>::const_iterator const_iterator;
    score_combiner() {}
    
    explicit score_combiner(std::string const& weight_string);

    void add_feature_weights(std::string const& weight_string);
    
    template<class FeatureWeightPairItr>
        score_combiner(FeatureWeightPairItr begin, FeatureWeightPairItr end)
        : weight_map(begin,end) {}

    iterator begin() { return weight_map.begin(); }
    const_iterator begin() const { return weight_map.begin(); }
    iterator end() { return weight_map.end(); }
    const_iterator end() const { return weight_map.end(); }
        
    void set_feature_weight(feature_key_type const& feature, double weight);
    
    bool has_feature_named(std::string const& name) const;
    
    double get_feature_weight(feature_key_type const& feature) const;
    
    score_combiner& operator += (score_combiner const& other);
    
    score_combiner& operator -= (score_combiner const& other);
                     
    void rename_feature( feature_key_type const& old_name
                       , feature_key_type const& new_name );

    void clear() 
    {
        reset();
    }
    
    void reset();
    void reset(std::string const& weight_string);

    template <class C, class T>
    void print( std::basic_ostream<C,T>& o
              , char pair_sep_char=','
              , char key_val_sep_char=':' ) const
    {
        graehl::word_spacer sep(pair_sep_char);
        for (weights_type::const_iterator i=weight_map.begin(),e=weight_map.end();
             i!=e;++i)
            o << sep << i->first << key_val_sep_char << i->second;
    }

    template <class C, class T>
    friend std::basic_ostream<C,T>& 
    operator<<(std::basic_ostream<C,T>& o,score_combiner const& me)
    {
        me.print(o);
        return o;
    }

    template <class C,class T>
    friend std::basic_istream<C,T>& 
    operator >>(std::basic_istream<C,T>& i,score_combiner & me)
    {
        std::string s;
        if (i>>s)
            me.reset(s);
        return i;
    }
    
    
    score_t operator()(scores_type const& components) const;

    void copy_to_byid(score_combiner_byid &to,feature_names_type &feature_names_dict) const;

    friend inline void swap(score_combiner &a, score_combiner &b) 
    {
        swap(a.weight_map,b.weight_map);
    }
    
    double operator[](feature_key_type const& key) const
    {
        return get_feature_weight(key);
    }
    
    double& operator[](feature_key_type const& key)
    {
        if (not has_feature_named(key)) {
            set_feature_weight(key,0);
        }
        return weight_map[key];
    }
    
private:
    typedef std::map<feature_key_type,double> weights_type;
    std::map<feature_key_type,double> weight_map;
};

////////////////////////////////////////////////////////////////////////////////

class score_combiner_byid : public boost::additive<score_combiner_byid>
{
    typedef std::vector<double> feature_exponents_type;
    feature_exponents_type exps;
 public:
    friend inline void swap(score_combiner_byid &a,score_combiner_byid &b) 
    {
        swap(a.exps,b.exps);
    }
    
    score_combiner_byid() {}
    score_combiner_byid(score_combiner const& sc,feature_names_type & dict)
    {
        set(sc,dict);
    }

    /// may add new tokens to dict
    void set(score_combiner const& sc,feature_names_type & dict)
    {
        exps.clear();
        sc.copy_to_byid(*this,dict);
    }
    
    double feature_exponent(feature_id_type feature_id) const
    {
        return feature_id >= exps.size() ? 0 : exps[feature_id];
    }
    void set_feature_exponent(feature_id_type feature_id,double feature_exponent)
    {
        //graehl::at_expand(exps,feature_id,feature_exponent) = feature_exponent; 
        // the above at_expand doesnt work.  it sets missing values not to zero.      
        if (feature_id >= exps.size())
             exps.resize(feature_id + 1, 0);
        exps[feature_id] = feature_exponent;
    }
    
    score_combiner_byid& operator += (score_combiner_byid const& other)
    {
        for (std::size_t idx = 0; idx != other.exps.size(); ++idx) {
            if (idx < exps.size()) exps[idx] += other.exps[idx];
            else exps.push_back(other.exps[idx]);
        }
        return *this;
    }
    
    score_combiner_byid& operator -= (score_combiner_byid const& other)
    {
        for (std::size_t idx = 0; idx != other.exps.size(); ++idx) {
            if (idx < exps.size()) exps[idx] -= other.exps[idx];
            else exps.push_back(other.exps[idx]);
        }
        return *this;
    }
    
    // usual: feature_vector_byid - pairs of <id,score>
    template <class scores_byid_iter>
    score_t operator()(scores_byid_iter i,scores_byid_iter end) const
    {
        score_t ret=as_one();
        for(;i!=end;++i) {
            double p = feature_exponent(i->first);
            if (p != 0 and i->second != 0)
                ret *= pow(i->second,feature_exponent(i->first));
        }
        return ret;
    }
    template <class scores_byid_t>
    score_t operator()(scores_byid_t &rule_scores) const
    {
        return this->operator()(rule_scores.begin(),rule_scores.end());
    }

    // e.g. feature_accum_byid is a vector where a[id]=score
    template <class FA>
    score_t score_accum_vector(FA const&a) const
    {
        feature_exponents_type::const_iterator ei=exps.begin(),ee=exps.end();
        typename FA::const_iterator i=a.begin(),e=a.end();
        score_t ret=as_one();
        while(i!=e&&ei!=ee)
            ret *= score_t(*i++,*ei++);
        return ret;
    }
    
    size_t size() const { return exps.size(); }
};

template <class C, class T, class Dict>
void print( std::basic_ostream<C,T>& o
          , score_combiner_byid const& sc
          , Dict const& dict )
{
    graehl::word_spacer sep(',');
    for (size_t x=0; x != sc.size();++x)
        o << sep << dict.get_token(x) << ':' << sc.feature_exponent(x);
}


} // namespace sbmt
# endif

#endif // SBMT_GRAMMAR_SCORE_COMBINER_HPP
