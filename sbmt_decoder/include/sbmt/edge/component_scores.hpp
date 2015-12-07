#ifndef SBMT_EDGE_component_scores_hpp_
#define SBMT_EDGE_component_scores_hpp_

/**
   lightweight fixed sized vector of scores initialized at 1.0 x N where N is
   the number of features some info type uses

   this is public at all only so dynamic/multi lms can dispatch on a fixed type.
   note that i'd use this vector to accumulate the exact backoff/inside
   increments anyway

   lm->component_names() should return a component_names aligned with component_scores returned from lm via ngram_info::compute_ngrams
 */

#include <sbmt/logmath.hpp>
#include <string>
#include <vector>
#include <cstddef>
#include <graehl/shared/word_spacer.hpp>

namespace sbmt {

typedef std::vector<std::string> component_names;

struct component_scores_vec 
  : protected std::allocator< std::pair<boost::uint32_t,score_t> >
{
    typedef std::pair<boost::uint32_t,score_t> value_type;
    typedef std::allocator<value_type> allocator_type;
    unsigned N;
    value_type* v;
    component_scores_vec(unsigned N) : N(N) 
    {
        v=this->allocate(N);
        reset();
    }
    component_scores_vec(component_scores_vec const& o) 
    {
        set(o);
    }
    void operator=(component_scores_vec const& o) 
    {
        dealloc();
        set(o);
    }
    ~component_scores_vec() 
    {
        dealloc();
    }

    void reset() 
    {
        for (unsigned i=0;i<N;++i)
            v[i] = value_type(i,score_t());
    }
    
    
    template <class S, class T>
    void print(std::basic_ostream<S,T>& o, feature_dictionary& dict) const 
    {
        graehl::word_spacer sp;
        for (unsigned i=0;i<N;++i) {
            if (v[i].second != as_one()) {
                o << sp << dict.get_token(v[i].first) << '=' << v[i].second;
            }
        }
    }

 private:
    void dealloc() 
    {
        if (N)
            this->deallocate(v,N);
        N=0;
    }    
    
    unsigned n_cpy() const 
    {
        return sizeof(value_type)*N;
    }
    
    void alloc() 
    {
        v=this->allocate(N);
    }
    
    void set(component_scores_vec const& o)
    {
        N=o.N;
        alloc();
        std::memcpy(v,o.v,n_cpy());
    }
    
};
    
}
    

#endif
