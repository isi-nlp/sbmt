#ifndef   SBMT_HASH_FUNCTORS_HPP
#define   SBMT_HASH_FUNCTORS_HPP

#include <functional>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  it is very annoying to me that these useful functors are
///  not part of the standard.
///
////////////////////////////////////////////////////////////////////////////////

template <class X> struct identity 
: public std::unary_function<X,X>
{
    X const& operator()(X const& x) const { return x; }
};

////////////////////////////////////////////////////////////////////////////////

template <class X, class Y> struct first
: public std::unary_function< std::pair<X,Y>, X >
{
    X const& operator()(std::pair<X,Y> const& p) const
    {
        return p.first;
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class X, class Y> struct second
: public std::unary_function< std::pair<X,Y>, Y >
{
    Y const& operator()(std::pair<X,Y> const& p) const
    {
        return p.second;
    }
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT_HASH_FUNCTORS_HPP
