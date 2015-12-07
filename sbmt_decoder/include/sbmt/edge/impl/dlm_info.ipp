#ifndef SBMT_EDGE_dlm_info_ipp_
#define SBMT_EDGE_dlm_info_ipp_

#include <sbmt/grammar/rule_input.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <boost/scoped_array.hpp>
#include <algorithm>
#include <deque>

namespace sbmt {

// hash the boundary words.
// Do we have other good hash function for the boundary words?
template<unsigned N, class LMID_TYPE>
std::size_t dlm_info<N, LMID_TYPE>::hash_value() const {
    std::size_t ret = 0;
    for(unsigned j = 0; j < 2; ++j) {
        for(unsigned i = 0; i < N; ++i){ 
            boost::hash_combine(ret, (unsigned)boundary_words[j][i]); 
        }
     }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned N, class LMID_TYPE>
template <class C, class T, class LM>
std::basic_ostream<C,T>& 
dlm_info<N, LMID_TYPE>::print( std::basic_ostream<C,T> &o , LM const& lm ) const 
{
    LMID_TYPE const* begin = &(boundary_words[0][0]);
    LMID_TYPE const* end = begin + N;
    lm.print(o,begin,end);         
    o << "//";
    begin = &(boundary_words[1][0]);
    end = begin + N;
    lm.print(o,begin,end);
    return o;
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned N, class LMID_TYPE>
template <class C, class T>
std::basic_ostream<C,T>& 
dlm_info<N, LMID_TYPE>::print_self(std::basic_ostream<C,T>& o) const
{
    o<<"[";
    LMID_TYPE const* begin = &(boundary_words[0][0]);
    LMID_TYPE const* end = begin + N;
    sbmt::print(o,begin,end);         
    o << "//";
    begin = &(boundary_words[1][0]);
    end = begin + N;
    sbmt::print(o,begin,end);
    o<<"]";
    return o;
}


}



#endif

