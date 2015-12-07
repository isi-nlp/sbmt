#ifndef __dlm_info_hpp__
#define __dlm_info_hpp__
#include <boost/shared_ptr.hpp>
#include <sbmt/edge/edge_info.hpp>
#include <sbmt/token/indexed_token.hpp>
#include <sbmt/dependency_lm/DLM.hpp>
#include <vector>
#include <string>
#include <iostream>

namespace sbmt {
/// N  how many boundary words need to be memo-ized at EACH DIRECTION.
/// Therefore, there are 2*N boundary words.
template<unsigned N, class LMID_TYPE = unsigned int>
class dlm_info {
public:
    typedef dlm_info<N, LMID_TYPE> info_type;

protected:
    /// left or right boundaries. 0 - left, 1 - right.
    LMID_TYPE boundary_words[2][N];
public:
     dlm_info() {
         int i;
         for(int dir = 0; dir < 2; ++dir){
         for(i = 0; i < (int)N; ++i) { boundary_words[dir][i] = (LMID_TYPE)-1;}
         }
     }

     dlm_info& operator=(dlm_info const& o)  {
         unsigned i;
         for(int dir = 0; dir < 2; ++dir){
         for(i = 0; i < N; ++i) { boundary_words[dir][i] = o.boundary_words[dir][i];}
         }
         return *this;
     }

     const LMID_TYPE* boundary_array(int dir) const { return boundary_words[dir];}
     LMID_TYPE* boundary_array(int dir) { return boundary_words[dir];}

    static std::string component_features() { return "deplm"; }

    bool equal_to(info_type const& other) const
    {
        for(unsigned j = 0; j < 2; ++j){
            for(unsigned i = 0; i < N; ++i) { 
                if(boundary_words[j][i] != other.boundary_words[j][i]){
                    return false;
                }
            }
        }
        return true;
    }

    LMID_TYPE get_bw(const unsigned i, const int dir) const { 
        LMID_TYPE id=boundary_words[dir][i];
        return id;
    }

    void set_boundary(const LMID_TYPE bw, const unsigned i, const int dir) { 
        boundary_words[dir][i] = bw;
    }

    //! hash the context words.
    std::size_t hash_value() const; 

    template <class C, class T, class LM>
    std::basic_ostream<C,T>& 
    print(std::basic_ostream<C,T>& o, LM const& lm) const;

    template <class C, class T>
    std::basic_ostream<C,T>& print_self(std::basic_ostream<C,T>& o) const;

    template <class C, class T, class TF>
    std::basic_ostream<C,T>& print_self(std::basic_ostream<C,T>& o, TF& tf) const {
        return print_self(o);
    }
};

template <unsigned N, class LM>
class dlm_info_factory {
public:
    typedef LM lm_type;        
    typedef typename lm_type::lm_id_type lm_id_type;
    typedef dlm_info<N, lm_id_type> info_type;
    typedef dlm_info_factory<N, LM> self_type;
};

}
#endif

#include "sbmt/edge/impl/dlm_info.ipp"

