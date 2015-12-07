#ifndef SBMT_GRAMMAR__UNIQUE_LMSTRING_COUNTS_HPP
#define SBMT_GRAMMAR__UNIQUE_LMSTRING_COUNTS_HPP

#include <boost/ref.hpp>

#include <sbmt/hash/oa_hashtable.hpp>

#include <graehl/shared/byref.hpp>
#include <graehl/shared/identity.hpp>
#include <graehl/shared/percent.hpp>

namespace sbmt {

struct unique_lmstring_counts 
{
    typedef unsigned long count_t;
    count_t n_rules,n_unique_lmstring;
    count_t cum_rules,cum_unique,n_sets;
    double avg_frac_uniq;
    
    double fraction_unique() const 
    {
        return (double)n_unique_lmstring/n_rules;
    }

    double cum_fraction_unique() const 
    {
        return (double)cum_unique/cum_rules;
    }
    
    template <class O>
    void print(O& o) const 
    {
        o << graehl::portion<5>(cum_unique,cum_rules)<< " total unique/total lmstrings, over "<<n_sets<<" sets; unweighted average over sets of "<<graehl::percent<5>(avg_frac_uniq)<<" unique";
    }
    
    unique_lmstring_counts() 
    {
        clear();
    }
    void clear_set() 
    {
        n_rules=n_unique_lmstring=0;
    }

    void finish_set()
    {
        cum_rules += n_rules;
        cum_unique += n_unique_lmstring;

        double u=fraction_unique();
        avg_frac_uniq *= n_sets;
        avg_frac_uniq += u;
        ++n_sets;
        avg_frac_uniq /= n_sets;
    }
    
    void clear_cumulative()
    {
        cum_rules=cum_unique=n_sets=0;
        avg_frac_uniq=0;
    }

    void clear()
    {
        clear_set();
        clear_cumulative();
    }
    
    
    template <class GT,class RR>
    void count_rule_range(GT const& gram,RR const& rr) 
    {
        clear_set();
        
        typedef typename RR::iterator Iter;
        typedef typename RR::value_type Rule;
        typedef typename GT::lm_string_type Lmstr;
        
        typedef boost::reference_wrapper<Lmstr const> Key;
        
        oa_hashtable<Key,graehl::identity_ref<Key> > unique_lmstrings;
        for (Iter i=rr.begin(),e=rr.end();i!=e;++i) {
            ++n_rules;
            if (unique_lmstrings.insert(Key(gram.rule_lm_string(*i))).second) {
                ++n_unique_lmstring;
            }
        }

        finish_set();
    }

    template <class GT,class RR>
    void visit_binary_by_rhs(GT const& gram,RR const& rr) 
    {
        count_rule_range(gram,rr);        
    }
    
    template <class GT>
    void count_binary_rules_by_rhs(GT const& gram)
    {
        gram.visit_binary_by_rhs(*this);
    }

    template <class O,class GT>
    void print_binary_rules_summary(O &o,GT const& gram)
    {
        clear_cumulative();
        count_binary_rules_by_rhs(gram);
        o << "For binary rules grouped by same rhs, ";
        print(o);
        o << std::endl;
    }
    
};

template <class C, class T>
std::basic_ostream<C,T>& 
operator << (std::basic_ostream<C,T>& os, unique_lmstring_counts const& p)
{
    p.print(os);
    return os;
}

}


#endif
