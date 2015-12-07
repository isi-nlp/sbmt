#ifndef SBMT__GRAMMAR__TAG_PRIOR_HPP
#define SBMT__GRAMMAR__TAG_PRIOR_HPP

#include <sbmt/token.hpp>
#include <stdexcept>
#include <graehl/shared/input_error.hpp>
#include <graehl/shared/assoc_container.hpp>
#ifdef DEBUG_TAG_PRIOR
#include <graehl/shared/debugprint.hpp>
#endif
#include <sbmt/logmath.hpp>
#include <utility>
#include <vector>
#include <iostream>

namespace sbmt {

class tag_prior
{
    typedef indexed_token tag_type;
    typedef tag_type::size_type index_type;
    typedef std::pair<tag_type,score_t> prior_type;
    
    /*
    struct tag_part 
    {
        typedef tag_type result_type;
        result_type const& operator()(prior_type const& p) 
        {
            return p.first;
        }
    };
    typedef oa_hashtable<prior_type,tag_part> prior_table_type;
    */
    typedef std::vector<score_t> prior_table_type;
    prior_table_type priors;
    score_t floor_prob;
    void swap(tag_prior &o)
    {
        priors.swap(o.priors);
        floor_prob.swap(o.floor_prob);
    }
    friend inline void swap(tag_prior&a, tag_prior &b)
    {
        a.swap(b);
    }
    
    
 public:
    static inline index_type tagidx(tag_type tag) 
    {
//        assert(tag.type()==tag_token);
        return tag.index();
    }

    /// until you construct or call set, no effect (well, except that if you set
    /// anything explicitly, the gaps will be filled with this very high floor
    tag_prior() : floor_prob(as_one()) {}
        
    template <class C,class T>
    tag_prior(std::basic_istream<C,T> &i,in_memory_dictionary &dict,score_t floor_prob_=1e-9,double add_constant_count_smooth=1)
    {
        set(i,dict,floor_prob_,add_constant_count_smooth);
    }
    
    tag_prior(std::string const &i,in_memory_dictionary &dict,score_t floor_prob_=1e-9,double add_constant_count_smooth=1)
    {
        set_string(i,dict,floor_prob_,add_constant_count_smooth);
    }

    BOOST_STATIC_CONSTANT(index_type,expected_nts_padding=1000);
    template <class In>
    void set(In &i,in_memory_dictionary &dict,score_t floor_prob_=1e-9,double add_constant_count_smooth=1)
    {
        priors.clear();
        priors.reserve(dict.tag_count()+expected_nts_padding);
        floor_prob=floor_prob_;
        read(i,dict,add_constant_count_smooth);
    }

    void set(score_t floor_prob_=1) 
    {
        priors.clear();
        floor_prob=floor_prob_;
    }
    
    void set_string(std::string const&str,in_memory_dictionary &dict,score_t floor_prob_=1e-9,double add_constant_count_smooth=1)
    {
        std::istringstream i(str);
        set(i,dict,floor_prob_,add_constant_count_smooth);
    }
    
    void
    set_prior(indexed_token::size_type idx,score_t prob)
    {                
#ifdef DEBUG_TAG_PRIOR
        DBPC3("set_prior",idx,prob);
#endif 
        graehl::at_expand(priors,idx,floor_prob)=prob;
    }
    
    void
    set_prior(tag_type tag,score_t prob)
    {
        //        DBPC3("set_prior",tag,prob);
        /*
        priors.insert(prior_type(tag,prob)).first->second=prob;
        */
        set_prior(tagidx(tag),prob);
    }

    template <class I>
    void set_prior_floored(I i,score_t prob)
    {
        //        DBPC4("set_prior_floored",i,prob,floor_prob);
        if (prob > floor_prob)
            set_prior(i,prob);
    }
    
    score_t operator[](tag_type tag) const
    {
        if (tag.type()!=tag_token) 
            return score_t(as_one());
        // in case someone asks for prior for foreign word, english word, or top (shouldn't ask for virtual_token)
        //        assert(tag.type()==tag_token);
        return this->operator[](tagidx(tag));
    }
    
    score_t operator[](index_type idx) const
    {
        /*
        if ((prior_table_type::iterator p=priors.find(tag)) == priors.end())
            return floor_prob;
        else
            return p->second;
        */
        return graehl::at_default(priors,idx,floor_prob);
    }

    // WARNING: if you change this after anything has been inserted, old floor values won't be updated
    void set_prior_floor(score_t floor)
    {
        floor_prob=floor;
    }

    // you'll have to call with 1/exponent to get back to normal values if you want to add any other probs
    void raise_pow(double exponent)
    {
        for (prior_table_type::iterator i=priors.begin(),e=priors.end();i!=e;++i)
            *i ^= exponent;
        floor_prob ^= exponent;
    }

    // WARNING: may give slightly different probs due to floor_prob than normalizing on read
    void normalize()
    {
        double total=0.;
        for (prior_table_type::iterator i=priors.begin(),e=priors.end();i!=e;++i)
            total+=i->linear();
        for (prior_table_type::iterator i=priors.begin(),e=priors.end();i!=e;++i)
            *i/=total;
    }
    
    template <class In>
    void must_read(In &i,unsigned line)
    {
        if (!i)
            graehl::throw_input_error(i,
                                      "bad tag_prior format - expect alternating <tag> <count> e.g. NP 123478",
                                      "line",line);
    }

    void clear()
    {
        priors.clear();
    }
    
    
    template <class In>
    void read(In &i,in_memory_dictionary &dict,double add_constant_count_smooth=1)
    {
        clear();
        
        std::string nt;
        double count,total=0;
        unsigned N=0;
        typedef std::pair<index_type,double> raw_count;
        typedef std::vector<raw_count> C; // I prefer to normalize using doubles, not score_t
        C tag_counts;
        while (i>>nt) {
            ++N;
            must_read(i>>count,N);
            count+=add_constant_count_smooth;
            index_type idx=dict.tag(nt).index();
            tag_counts.push_back(raw_count(idx,count));
            total+=count;
#ifdef DEBUG_TAG_PRIOR            
            DBP4(nt,idx,count,total);
#endif 
        }
        for (C::const_iterator i=tag_counts.begin(),e=tag_counts.end();i!=e;++i)
            set_prior_floored(i->first,i->second/total);
    }

    template <class Out>
    void print(Out &o,in_memory_dictionary const& dict) const
    {
        /*        for (prior_table_type::const_iterator i=priors.begin(),e=priors.end();i!=e;++i)
            o << dict.label(i->first) << " " << i->second.linear()<<"\n";
        */
        for (unsigned i=0,e=priors.size();i<e;++i)
            if (priors[i] > floor_prob)
                o << dict.label(indexed_token(i,tag_token)) << " "<<priors[i].linear()<<"\n";                    
    }
    
};

    

}//sbmt

#endif
