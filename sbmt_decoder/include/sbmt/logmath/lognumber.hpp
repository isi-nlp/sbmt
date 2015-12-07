#ifndef   SBMT_LOGMATH_LOGNUMBER_HPP
#define   SBMT_LOGMATH_LOGNUMBER_HPP

#ifndef _WIN32
#define HAVE_LOG1P
#endif

#include <cmath>
#include <limits>
#include <algorithm>

#include <boost/operators.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/serialization/access.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include <sbmt/logmath/logbase.hpp>

namespace sbmt {

struct as_log {};
struct as_ln {};
struct as_log10 {};
struct as_neglog10 {};
struct as_log_base_ln {};
struct as_zero {};
struct as_one {};
struct as_infinity {};

namespace logmath {

namespace detail {

#include <math.h>

inline float log_1p(float x)
{
    return
#ifdef HAVE_LOG1P
        log1pf(x);
#else
    logf(1+x);
#endif
}

inline double log_1p(double x)
{
    return
#ifdef HAVE_LOG1P
        log1p(x);
#else
    log(1+x);
#endif
}

inline float log_e(float x)
{
    return logf(x);
}

inline double log_e(double x)
{
    return log(x);
}


////////////////////////////////////////////////////////////////////////////////
///
///  controls lognumber functionality that changes
///  depending on whether the lognumber representation is order-preserving or
///  not.  that is, whether the log base is less or greater than 1.0.
///
////////////////////////////////////////////////////////////////////////////////
template <bool C,class F>
struct order_preserving_ops {};

template <class F> struct order_preserving_ops<true,F>
{
    typedef F float_type;
    // exp(t1)<exp(t2)
    template <class T>
    bool less(T t1, T t2) const
    { return t1 < t2; }

    float_type log_zero() const {
        return -std::numeric_limits<float_type>::infinity();
    }
};

template <class F> struct order_preserving_ops<false,F>
{
    typedef F float_type;
    template <class T>
    bool less(T t1, T t2) const
    { return t1 > t2; }
    float_type log_zero() const {
        return std::numeric_limits<float_type>::infinity();
    }
};

} // namespace detail

// computes (for caching) base->e (ln) and e->base (oo_ln) conversion multipliers
template <class BaseT,class F>
struct compute_ln_base
{
    typedef F float_type;
    compute_ln_base() : ln(detail::log_e(BaseT::base())),oo_ln(1./ln) {}
    const float_type ln;
    const float_type oo_ln;
};

template <class BaseT,class F>
struct ln_base
{
    typedef F float_type;
    typedef BaseT base;
    // ln(base)
    static inline float_type ln()
    {
        return lnbase.ln;
    }
    // 1/ln(base) (one over)
    static inline float_type oo_ln()
    {
        return lnbase.oo_ln;
    }
 private:
    typedef compute_ln_base<base,F> lnbase_type;
    static lnbase_type lnbase;
};

template <class Base_From,class Base_To,class F>
struct compute_log_base_conversion_ratio
{
    typedef F float_type;
/// schematically:
/// start from base_from
/// => [by * ln(from)]
/// base_e
/// => [ by / ln(to) - since log_from(e) = 1/log_e(from)
/// base_to.
    compute_log_base_conversion_ratio() :
        multiply_by(std::log(Base_From::base())/std::log(Base_To::base())) {}
    /// was: <code>ln_base< Base_From>::ln()*ln_base< Base_To>::oo_ln()</code>
                                     /// but order of initialization was compute_log_base_conversion_ratio first, then compute_ln_base (i.e. ln_base was still 0)
    const float_type multiply_by;
};

template <class Base_From,class Base_To,class F>
struct log_base_convert
{
    typedef F float_type;
    typedef Base_From from_base;
    typedef Base_To to_base;
    static inline float_type convert(float_type exponent_from_base)
    { return exponent_from_base * converter.multiply_by; }
 private:
    typedef compute_log_base_conversion_ratio<from_base,to_base,F> converter_type;
    static converter_type converter;
};

/// don't need to special case b->e or e->b because computation is always a
/// single multiplication; don't need to optimize static
/// log_base_conversion_ratio.  but same-base is no multiplication and we don't
/// want rounding
template <class Same_Base_From_To,class F>
struct log_base_convert<Same_Base_From_To,Same_Base_From_To,F>
{
    typedef F float_type;
    typedef Same_Base_From_To from_base;
    typedef from_base to_base;
    static inline float_type convert(float_type exponent_from_base)
    {
        return exponent_from_base;
    }
};

template <class BaseT,class F>
struct logmath_compute
{
    typedef F float_type;
    typedef BaseT my_base;
    typedef ln_base<my_base,F> ln_my_base;
//    typedef ln_base<base_e> ln_base_e; // attempt to force initialization of
//    statics in right order - superseded by avoiding dependency
    typedef log_base_convert<my_base,base_e,F> to_base_e;
    typedef log_base_convert<base_e,my_base,F> from_base_e;

    static inline float_type log(float_type number) { /// log_b(number)
        if (number > 0.0)
            return from_base_e::convert(detail::log_e(number));
        else
             return order.log_zero();
    }

    static inline float_type log1p(float_type e)
    {
        return
            from_base_e::convert(detail::log_1p(e));
    }


    static inline float_type exp(float_type exponent)
    { return std::exp(to_base_e::convert(exponent)); }

    /// requires e2>e1! (or you lose precision)
    static inline float_type add(float_type e1, float_type e2)
    {
        // without this, logprob(0) + logprob(0) is undefined
        if (e1 == log_zero()) {
            return e2;
        } else if (e2 == log_zero()) {
            return e1;
        } else return e1 +log1p(exp(e2 - e1));
    }

    ///returns absolute difference
    static inline float_type difference(float_type e1, float_type e2)
    {
        if (order.less(e1,e2))
            return subtract_unguarded(e2,e1);
        else
            return subtract_unguarded(e1,e2);
    }

    ///returns absolute difference / smallest - gives strictest possible < epsilon criteria for near-equality
    static inline float_type relative_difference(float_type e1, float_type e2)
    {
        if (order.less(e1,e2))
            return subtract_unguarded(e2,e1)-e1;
        else
            return subtract_unguarded(e1,e2)-e2;
    }

    // returns log(max{exp(e1)-exp(e2),0})
    static inline float_type subtract_clamp(float_type e1, float_type e2)
    {
        //    if (e1 == e2) return order.log_zero();
        if (order.less(e1,e2))
            return order.log_zero();
        else
            return subtract_unguarded(e1,e2);
    }

    /// returns log(exp(e1)-exp(e2))
    /// requires e1>e2
    static inline float_type subtract_unguarded(float_type e1, float_type e2)
    {
        //    if (e1 == e2) return order.log_zero();
        return e1 + log1p(-exp(e2 - e1));
    }

    static inline bool less(float_type e1, float_type e2)
    { return order.less(e1,e2); }


    static inline float_type ratio_to_other_base_ln(float_type ln_of_other_base)
    { return ln_my_base::ln()/ln_of_other_base; }
    static inline float_type ratio_from_other_base_ln(float_type ln_of_other_base)
    { return ln_of_other_base*ln_my_base::oo_ln(); }

    static inline float_type ratio_to_other_base(float_type other_base)
    { return ratio_to_other_base_ln(detail::log_e(other_base)); }

    static inline float_type ratio_from_other_base(float_type other_base)
    { return ratio_from_other_base_ln(detail::log_e(other_base)); }


    static inline bool exp_is_zero(float_type exponent)
    { return exponent==order.log_zero(); }
    static inline float_type log_zero()
    { return order.log_zero(); }
    static inline float_type log_infinity()
    { return -order.log_zero(); }
 private:
    typedef detail::order_preserving_ops<BaseT::order_preserving,F> order_type;
    static order_type order;
};

////////////////////////////////////////////////////////////////////////////////
///
///  \brief implements positive numbers using logarithms
///
///  \param BaseT forces a different type for different log bases.  you cant
///         just try to define basic_lognumber<double base> because floating
///         point values cannot be template parameters.
///         to be a valid BaseT, BaseT must implement static double base()
///
///  logarithms are frequently used in weighted search computations to avoid
///  underflow problems near 0.0, while sacrificing some accuracy near 1.0.
///  bases closer to 1.0 increase accuracy near 1.0 but lose accuracy near
///  0.0.
///
///  class basic_lognumber lets you take advantage of logarithms while keeping
///  normal linear notation.  also, conversions from one base to another are
///  handled automatically, and logarithms of different bases are safely
///  treated as different types.
///
///  basic_lognumber uses boost::operators to fill out all the operators that
///  make using numeric types convenient:
///  ==, !=, <, >, <=, >=, +, +=, -, -=, *, *=, /, /=
///
///  in addition, lognumber has overloaded versions of min(), max(), and pow(),
///  and can be converted from strings using lexical_cast<my_lognumber>()
///
///  \note:  because of its logarithm implementation, calls to +, +=, -, -=
///  are much slower than their floating point counterparts, as they require a
///  call to std::log() to implement.
///
////////////////////////////////////////////////////////////////////////////////
template <class BaseT,class F>
class basic_lognumber :
        public boost::ordered_field_operators< basic_lognumber<BaseT,F> >
{
public:
    typedef F float_type;
    typedef BaseT my_base;
    typedef my_base base_type;
    typedef logmath_compute<my_base,F> log_comp;
    typedef basic_lognumber<BaseT,F> lognumber_type;

    typedef log_base_convert<my_base,base_e,F> to_base_e;
    typedef log_base_convert<base_e,my_base,F> from_base_e;
    typedef log_base_convert<my_base,base_10,F> to_base_10;
    typedef log_base_convert<base_10,my_base,F> from_base_10;

    /// exception to using float_type: because double already can't hold very large exponents, float would be too small quite often
    basic_lognumber(double number = 1.0) :
        m_exponent(log_comp::log(number)) {}

    basic_lognumber(as_zero tag) :
        m_exponent(log_comp::log_zero()) {}

    basic_lognumber(as_one tag) :
        m_exponent(0) {}

    basic_lognumber(as_infinity tag) :
        m_exponent(log_comp::log_infinity()) {}

    template <class B,class F2>
    basic_lognumber( basic_lognumber<B,F2> const& other) :
        m_exponent(log_base_convert<B,BaseT,F>::convert(other.m_exponent)) {}

    /// same as basic_lognumber((float_type)base^exponent)
    template <class B,class F2,class F3>
    basic_lognumber(basic_lognumber<B,F2> const& base,F3 exponent)
        : m_exponent(log_base_convert<B,BaseT,F>::convert(base.m_exponent)*exponent) {}

    /// same as basic_lognumber((float_type)base^exponent)
    basic_lognumber(float_type base,float_type exponent) :
        m_exponent(exponent*log_comp::ratio_from_other_base(base)) {}

    /// same as basic_lognumber((float_type)(e^ln_of_base)^exponent)
    basic_lognumber(float_type ln_of_base,float_type exponent,as_log_base_ln tag)
        : m_exponent(exponent*log_comp::ratio_from_other_base_ln(ln_of_base)) {}

    /// same as basic_lognumber(10,exponent_base_10)
    basic_lognumber(float_type exponent_base_10,as_log10 tag)
        : m_exponent(from_base_10::convert(exponent_base_10)) {}

    basic_lognumber(float_type exponent_base_10,as_neglog10 tag)
        : m_exponent(-from_base_10::convert(exponent_base_10)) {}

    /// same as basic_lognumber(e,exponent_base_e)
    basic_lognumber(float_type exponent_base_e,as_ln tag)
        : m_exponent(from_base_e::convert(exponent_base_e)) {}

    /// basic_lognumber(b.log(),as_log()) same as basic_lognumber(b) if b is also of type basic_lognumber
    basic_lognumber(float_type exponent_my_base,as_log tag)
        : m_exponent(exponent_my_base) {}


    float_type log_base_me(lognumber_type const& x) const
    {
        return x.m_exponent/m_exponent;
    }

    void swap(lognumber_type &o)
    {
        std::swap(m_exponent,o.m_exponent);
    }

/// set(arg): versions of above constructors, equivalent to = constructor(arg)
    void set(double number = 0.0)
    { m_exponent=log_comp::log(number); }

    void set(as_zero tag)
    { m_exponent=log_comp::log_zero(); }

    void set(as_one tag)
    { m_exponent=0; }

    void set(as_infinity tag)
    { m_exponent=log_comp::log_infinity(); }

    template <class B,class F2>
    void set( basic_lognumber<B,F2> const& other)
        { m_exponent=log_base_convert<B,BaseT,F>::convert(other.m_exponent); }

    /// same as set((float_type)base^exponent)
    template <class B,class F2,class F3>
    void set(basic_lognumber<B,F2> const& base,F3 exponent)
        { m_exponent=log_base_convert<B,BaseT,F>::convert(base.m_exponent)*exponent; }

    /// same as set((float_type)base^exponent)
    void set(float_type base,float_type exponent)
        { m_exponent=exponent*log_comp::ratio_from_other_base(base); }

    /// same as set((float_type)(e^ln_of_base)^exponent)
    void set(float_type ln_of_base,float_type exponent,as_log_base_ln tag)
        { m_exponent=exponent*log_comp::ratio_from_other_base_ln(ln_of_base); }

    /// same as set(10,exponent_base_10)
    void set(float_type exponent_base_10,as_log10 tag)
        { m_exponent=from_base_10::convert(exponent_base_10); }

    void set(float_type exponent_base_10,as_neglog10 tag)
        { m_exponent=-from_base_10::convert(exponent_base_10); }

    /// same as set(e,exponent_base_e)
    void set(float_type exponent_base_e,as_ln tag)
        { m_exponent=from_base_e::convert(exponent_base_e); }

    /// void set(b.log(),as_log()) same as basic_lognumber(b) if b is also of type basic_lognumber
    void set(float_type exponent_my_base,as_log tag)
        { m_exponent=exponent_my_base; }


    template <class B,class F2>
    void operator=(basic_lognumber<B,F2> const& other)
    { set(other); }

    void operator=(double number)
    { set(number); }

    /// returns log (our base)
    float_type log() const { return m_exponent; }

    float_type ln() const { return to_base_e::convert(m_exponent); }

    float_type log10() const { return to_base_10::convert(m_exponent); }

    float_type neglog10() const // necessary for pretty printouts; negating 0 gives -0.
    {
        if (is_one()) return 0;
        return -log10();
    }

    float_type log_base(float_type base) const {
        return m_exponent*log_comp::ratio_to_other_base(base); }

    float_type log_base_ln(float_type ln_of_base) const {
        return m_exponent*log_comp::ratio_to_other_base_ln(ln_of_base); }

  bool is_nan() const { return boost::math::isnan(m_exponent); }
  bool is_positive_finite() const { return boost::math::isfinite(m_exponent); }
  bool is_positive() const { return !is_zero() && !is_nan(); }
  bool is_zero() const { return log_comp::exp_is_zero(m_exponent); }
  bool is_one() const { return m_exponent==0; }

    lognumber_type inverse() const
    {
        return lognumber_type(-m_exponent,as_log());
    }

    /// gives |*this-other| (nonnegative difference)
    lognumber_type difference(basic_lognumber const &other)
    {
        lognumber_type ret;
        ret.m_exponent=log_comp::difference(m_exponent,other.m_exponent);
        return ret;
    }

    /// gives pessimistic relative difference (divide difference by smaller)
    lognumber_type relative_difference(basic_lognumber const &other)
    {
        lognumber_type ret;
        ret.m_exponent=log_comp::relative_difference(m_exponent,other.m_exponent);
        return ret;
    }

    /// returns true if both this-other/other and this-other/this are less than
    /// epsilon.  note that (linear) 0 is not nearly_equal to anything except 0.
    bool nearly_equal(lognumber_type const&  other,lognumber_type const&  epsilon=1e-4)
    {
        return relative_difference(other) < epsilon;
    }

    /// exception to using float_type: because double already can't hold very large exponents, float would be too small quite often
    double linear() const
    { return log_comp::exp(m_exponent); }
#ifdef LOGMATH_DISABLE_ADDITION
 private:
#endif
    basic_lognumber& operator += (lognumber_type const&  other)
    {
        // if you don't do this, you lose the ability to add out of non-log-range numbers to normal-range half the time
        m_exponent=
            m_exponent > other.m_exponent ?
            log_comp::add(m_exponent,other.m_exponent) :
            log_comp::add(other.m_exponent,m_exponent);
		return *this;
    }

    //FIXME: this isn't minus clamped at zero; this is the same as difference.
    basic_lognumber& operator -= (lognumber_type const&  other)
    { m_exponent=log_comp::subtract_clamp(m_exponent,other.m_exponent); return *this; }
#ifdef LOGMATH_DISABLE_ADDITION
 public:
#endif
    basic_lognumber& operator *= (lognumber_type const&  other)
    { m_exponent += other.m_exponent; return *this; }

    basic_lognumber& operator /= (lognumber_type const&  other)
    { m_exponent -= other.m_exponent; return *this; }

    bool operator <  (lognumber_type const&  other) const
    { return log_comp::less(m_exponent,other.m_exponent); }

    bool operator == (lognumber_type const&  other) const
    { return m_exponent==other.m_exponent; }

    basic_lognumber& operator ^= (double exponent)
    { m_exponent *= exponent; return *this; }

    basic_lognumber pow(double exponent) const // redundant syntax a.pow(b) for pow(a,b) or basic_lognumber<B,T>(a,b)
    { return basic_lognumber(*this,exponent); }

/*    friend inline float_type
    log(lognumber_type const& x)
    { return x.ln(); }
    friend inline float_type
    log10(lognumber_type const& x)
    { return x.log10(); }
///////////////////////////////////////////////////////////////////////////////
///
///  in linear scale, calculates  pow(base.linear(),exp)
///  in log scale, equivalent to exp * base.log()
///
///////////////////////////////////////////////////////////////////////////////
    template <class B2, class F2, class F3>
    friend inline basic_lognumber<B2,F2>
    pow(basic_lognumber<B2,F2> const& base,F3 exp);
*/

    template <class B2, class F2>
    friend class basic_lognumber;

private:
    float_type m_exponent;

    friend class boost::serialization::access;
    template <class ArchiveT>
    void serialize(ArchiveT & ar, const unsigned int version)
    { ar & m_exponent; }
};

template <class B, class F, class D>
basic_lognumber<B,F> operator ^ (basic_lognumber<B,F> const& b, D p)
{
    basic_lognumber<B,F> ret = b;
    ret ^= p;
    return ret;
}

template <class B,class F>
inline F log(basic_lognumber<B,F> const& x)
{ return x.ln(); }

template <class B,class F>
inline F log10(basic_lognumber<B,F> const& x)
{ return x.log10(); }

template <class B,class F,class F2>
inline basic_lognumber<B,F>
pow(basic_lognumber<B,F> const& base,F2 exp)
{ return basic_lognumber<B,F>(base,exp); }

template <class B,class F,class F2>
inline basic_lognumber<B,F>
root(basic_lognumber<B,F> const& base,F2 rootexp)
{ return basic_lognumber<B,F>(base,1./rootexp); }

/// graehl/shared/accumulate.hpp
template <class B,class F>
inline void set_min_identity(basic_lognumber<B,F> & x)
{ x.set(as_infinity()); }

template <class B,class F>
inline void set_max_identity(basic_lognumber<B,F> & x)
{ x.set(as_zero()); }

template <class B,class F>
inline void set_add_identity(basic_lognumber<B,F> & x)
{ x.set(as_zero()); }

template <class B,class F>
inline void set_multiply_identity(basic_lognumber<B,F> & x)
{ x.set(as_one()); }


template <class B,class F>
inline void swap(basic_lognumber<B,F> &a,
                 basic_lognumber<B,F> &b)
{
    a.swap(b);
}

} // namespace logmath

using logmath::basic_lognumber;
using logmath::log_base_convert;
using logmath::ln_base;
using logmath::log;
using logmath::log10;
using logmath::pow;
using logmath::root;
using logmath::set_max_identity;
using logmath::set_min_identity;
using logmath::set_add_identity;
using logmath::set_multiply_identity;


typedef basic_lognumber< logmath::base_e,double > ln_number;
typedef basic_lognumber< logmath::base_10,double > log10_number;
typedef basic_lognumber< logmath::x_pow_n_neg<10,-1>,double > neglog10_number;

typedef basic_lognumber< logmath::base_e,float > ln_f_number;
typedef basic_lognumber< logmath::base_10,float > log10_f_number;
typedef basic_lognumber< logmath::x_pow_n_neg<10,-1>,float > neglog10_f_number;

////////////////////////////////////////////////////////////////////////////////
///
/// strictly provided as an homage.  the sphinx family of speech recognizers
/// used an integer based lognumber (actually logprob, since it had very
/// degraded accuracy for values above 1.0), with the base set to 1e-5,
/// most of the time. sphinx was where i first learned of lognumbers, and my
/// frustration over their akward base changes, log to linear changes,
/// and addition macros led me to create this class.
///
////////////////////////////////////////////////////////////////////////////////
typedef basic_lognumber< logmath::precision<5>,double > sphinx_number;


} //namespace sbmt

namespace std {
// oops: this is not really legal.  probably won't be used.  "partial function template specialization not supported, adding new overloads to std not allowed"
template <class B,class F>
inline void swap(sbmt::logmath::basic_lognumber<B,F> &a,
                 sbmt::logmath::basic_lognumber<B,F> &b)
{
    a.swap(b);
}

}

#include <sbmt/logmath/impl/lognumber.ipp>

#endif // SBMT_LOGMATH_LOGNUMBER_HPP
