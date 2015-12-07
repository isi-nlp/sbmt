#ifndef   SBMT_LOGMATH_LOGBASE_HPP
#define   SBMT_LOGMATH_LOGBASE_HPP

#include <cmath>
#include <boost/static_assert.hpp>

namespace sbmt { namespace logmath {
    
////////////////////////////////////////////////////////////////////////////////

template <unsigned int X, int N>
struct x_pow_n {
    enum { value = X * x_pow_n<X,N-1>::value, order_preserving=N>0 };
    static const double base() { return value; }
};

template <unsigned int X> 
struct x_pow_n<X,1> {
    enum { value = X, order_preserving=true };
    static const double base() { return double(value); }
};

template <unsigned int X> 
struct x_pow_n<X,0> {}; // not allowed

template <unsigned int N>
struct x_pow_n<1,N> {};  // not allowed

template <>
struct x_pow_n<1,0> {};  // not allowed

////////////////////////////////////////////////////////////////////////////////

template <unsigned int X, int N>
struct x_pow_n_neg
{
    enum { order_preserving=false };
    static const double base() 
    {
        return 1.0/x_pow_n<X,-N>::value;
    }
};  

////////////////////////////////////////////////////////////////////////////////

template <unsigned int N>
struct precision {
    enum { order_preserving = false };
    
    static const double base()
    {
        return 1.0 - 1.0/double(x_pow_n<10,N>::value);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <int N>
struct exp_n {
    enum { order_preserving = N > 0 };
    
    static const double base()
    {
        return std::exp(double(N));
    }   
};

template <>
struct exp_n<0> {};  // not allowed

////////////////////////////////////////////////////////////////////////////////

typedef exp_n<1> base_e;
typedef x_pow_n<10,1> base_10;
typedef x_pow_n<2,1> base_2;

} } // namespace sbmt::logmath

#endif // SBMT_LOGMATH_LOGBASE_HPP
