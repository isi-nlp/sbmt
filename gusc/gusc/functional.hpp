# if ! defined(GUSC__FUNCTIONAL_HPP)
# define       GUSC__FUNCTIONAL_HPP

# include <boost/call_traits.hpp>
# include <boost/version.hpp>
# include <cmath>
#if BOOST_VERSION >= 104000
#include <boost/property_map/property_map.hpp>
#else
#include <boost/property_map.hpp>
#endif

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
//
//  these are 'nicer' functors than the ones provided by std library, because
//  they dont need template arguments to instantiate them
//
////////////////////////////////////////////////////////////////////////////////
struct power {
    template <class A> struct result {};
    
    template <class A> struct result<power(A,A)> { typedef A type; };
    
    template <class A>
    A operator()(A const& a, A const& b) const { return pow(a,b); }
};

struct times {
    template <class A>
    struct result {};

    template <class A> struct result<times(A,A)> { typedef A type; };

    template <class A>
    A operator()(A const& a, A const& b) const { return a * b; }
};

struct times_eq {
    template <class A>
    struct result {};

    template <class A, class B> struct result<times_eq(A,B)> { typedef A type; };

    template <class A, class B>
    A& operator()(A& a, B const& b) const { return a *= b; }

    template <class A, class B>
    A operator()(A const& a, B const& b) const { A aa=a; return aa *= b; }
};

struct divide {
    template <class A>
    struct result {};

    template <class A> struct result<divide(A,A)> { typedef A type; };

    template <class A>
    A operator()(A const& a, A const& b) const { return a / b; }
};

struct divide_eq {
    template <class A>
    struct result {};

    template <class A, class B> struct result<divide_eq(A,B)> { typedef A type; };

    template <class A, class B>
    A& operator()(A& a, B const& b) const { return a /= b; }

    template <class A, class B>
    A operator()(A a, B const& b) const { return a /= b; }
};

struct plus {
    template <class A> struct result {};

    template <class A> struct result<plus(A,A)> { typedef A type; };

    template <class A>
    A operator()(A const& a, A const& b) const { return a + b; }

};

struct plus_eq {
    template <class A>
    struct result {};

    template <class A, class B> struct result<plus_eq(A,B)> { typedef A type; };

    template <class A, class B>
    A& operator()(A& a, B const& b) const { return a += b; }

    template <class A, class B>
    A operator()(A const& a, B const& b) const { A aa=a; return aa += b; }
};

struct minus {
    template <class A> struct result {};

    template <class A> struct result<minus(A,A)> { typedef A type; };

    template <class A>
    A operator()(A const& a, A const& b) const { return a - b; }
};

struct minus_eq {
    template <class A>
    struct result {};

    template <class A, class B> struct result<minus_eq(A,B)> { typedef A type; };

    template <class A, class B>
    A& operator()(A& a, B const& b) const { return a -= b; }

    template <class A, class B>
    A operator()(A const& a, B const& b) const { A aa=a; return aa -= b; }
};

struct less {
    typedef bool result_type;
    template <class A, class B>
    bool operator()(A const& a, B const& b) const { return a < b; }
};

struct greater {
    typedef bool result_type;
    template <class A, class B>
    bool operator()(A const& a, B const& b) const { return a > b; }
};

struct equal_to {
    typedef bool result_type;
    template <class A, class B>
    bool operator()(A const& a, B const& b) const { return a == b; }
};

struct identity {
    template <class X> struct result {};

    template <class X>
    struct result<identity(X)> {
        typedef X type;
    };

    template <class X>
    typename result<identity(X)>::type const&
    operator()(X const& x) const
    {
        return x;
    }
};

struct always_false {
    typedef bool result_type;
    template <class X>
    bool operator()(X const& x) const { return false; }
};

struct always_true {
    typedef bool result_type;
    template <class X>
    bool operator()(X const& x) const { return true; }
};

////////////////////////////////////////////////////////////////////////////////

template <class PropertyMap>
struct property_map_transform {
    typedef typename boost::property_traits<PropertyMap>::value_type result_type;

    result_type
    operator()(typename boost::property_traits<PropertyMap>::key_type const& x) const
    {
        return get(pmap,x);
    }
    PropertyMap pmap;
    property_map_transform(PropertyMap const& pmap) : pmap(pmap) {}
};

////////////////////////////////////////////////////////////////////////////////

template <class PMap>
property_map_transform<PMap>
pmap_transform(PMap pmap) { return property_map_transform<PMap>(pmap); }

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__FUNCTIONAL_HPP
