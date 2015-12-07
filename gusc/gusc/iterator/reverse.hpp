# if ! defined(GUSC__ITERATOR__REVERSE_HPP)
# define       GUSC__ITERATOR__REVERSE_HPP

# include <iterator>
# include <boost/range.hpp>
# include <utility>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class Iterator>
std::reverse_iterator<Iterator> reverse(Iterator const& itr)
{
    return std::reverse_iterator<Iterator>(itr);
}

////////////////////////////////////////////////////////////////////////////////

template <class Range>
struct reverse_range_return {
    typedef 
    boost::iterator_range< 
        std::reverse_iterator<typename boost::range_result_iterator<Range>::type>
    > type;
};

template <class Range>
typename reverse_range_return<Range>::type
reverse_range(Range& rng)
{
    return boost::make_iterator_range( reverse(boost::end(rng))
                                     , reverse(boost::begin(rng))
                                     );
}

template <class Range>
boost::iterator_range< 
    std::reverse_iterator<typename boost::range_const_iterator<Range>::type>
>
reverse_range(Range const& rng)
{
    return boost::make_iterator_range( reverse(boost::end(rng))
                                     , reverse(boost::begin(rng))
                                     );
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__ITERATOR__REVERSE_HPP
