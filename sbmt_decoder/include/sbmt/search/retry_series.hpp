#ifndef SBMT_SEARCH_RETRY_SERIES_HPP
#define SBMT_SEARCH_RETRY_SERIES_HPP

//#define SBMT_RETRY(i,name) retry_ ## name (i,name)


#include <sbmt/logmath.hpp>
#include <boost/function.hpp>

namespace sbmt {

typedef unsigned retry_time_type;

template <class V>
struct retry_series 
{
    typedef V value_type;
    typedef retry_series<V> self_type;
    typedef boost::function<value_type(retry_time_type)> function_type;
    function_type func;
    retry_series() {}
//    template <class F> retry_series(F const& func) : func(func) {}
    retry_series(function_type const& func_) : func(func_) {}
    retry_series(self_type const& o) : func(o.func) {}
    bool operator()(retry_time_type retry_i,value_type &val) const
    {
        if (func.empty())
            return false;
        if (retry_i==0) {
            val=func(retry_i);
            return true;
        }
        value_type prev=val;
        val=func(retry_i);
        return val != prev;
    }
    value_type operator()(retry_time_type i = 0) const
    { return func(i); }
};

// purpose: avoid short-circuit
inline bool any_change(bool a,bool b) 
{
    return a || b;
}

// purpose: avoid short-circuit
inline bool any_change(bool a,bool b,bool c) 
{
    return a || b || c;
}


typedef score_t beam_t;
typedef unsigned hist_t;
typedef retry_series<unsigned> hist_retry;
typedef retry_series<score_t> beam_retry;
typedef hist_retry::function_type hist_retry_f;
typedef beam_retry::function_type beam_retry_f;

}


#endif
