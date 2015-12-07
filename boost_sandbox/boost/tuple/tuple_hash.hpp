# ifndef   BOOST__TUPLE__TUPLE_HASH_HPP
# define   BOOST__TUPLE__TUPLE_HASH_HPP

# include <boost/tuple/tuple.hpp>
# include <boost/functional/hash.hpp>

namespace boost { namespace tuples {

inline size_t hash_value(const null_type&)
{
    return 0;
}

template<class T1, class T2>
inline size_t hash_value(cons<T1,T2> const& tpl)
{
    size_t seed(0);
    ::boost::hash_combine(seed,tpl.get_head());
    ::boost::hash_combine(seed,hash_value(tpl.get_tail()));
    return seed;
}

}} // namespace boost::tuples

# endif // BOOST__TUPLE__TUPLE_HASH_HPP