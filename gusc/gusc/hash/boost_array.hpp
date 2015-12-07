# if !defined( GUSC__HASH__BOOST_ARRAY_HPP)
# define       GUSC__HASH__BOOST_ARRAY_HPP

# include <boost/array.hpp>

# if BOOST_VERSION < 105500
namespace boost {
    template <class T, std::size_t N>
    std::size_t hash_value(array<T,N> const& v)
    {
        return hash_range(v.begin(),v.end());
    }
} // namespace boost
# endif
# endif //     GUSC__HASH__BOOST_ARRAY_HPP
