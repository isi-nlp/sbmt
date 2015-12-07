# if ! defined(GUSC__GENERATOR__GENERATOR_FROM_ITERATOR_HPP)
# define       GUSC__GENERATOR__GENERATOR_FROM_ITERATOR_HPP

# include <boost/iterator/iterator_adaptor.hpp>
# include <iterator>
# include <boost/range.hpp>
# include <gusc/generator/peekable_generator_facade.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
///
/// convert an InputIterator range into a generator interface.
/// this generator forwards the iterator interface
///
////////////////////////////////////////////////////////////////////////////////
template <class Iterator>
class generator_from_iterator 
: public boost::iterator_adaptor< 
           generator_from_iterator<Iterator>
         , Iterator
         >
{
public:
    typedef typename std::iterator_traits<Iterator>::value_type result_type;
    
    generator_from_iterator(Iterator begin, Iterator end)
      : generator_from_iterator::iterator_adaptor_(begin)
      , end(end) {}
    
    template <class Range>
    explicit generator_from_iterator(Range const& range)
      : generator_from_iterator::iterator_adaptor_(boost::begin(range))
      , end(boost::end(range)) {}
    
    operator bool() const { return this->base() != end; }
    
    result_type operator()() { return *((this->base_reference())++); }
private:
    Iterator end;
};

////////////////////////////////////////////////////////////////////////////////

template <class Range>
class generator_from_range 
: public peekable_generator_facade< 
           generator_from_range<Range>
         , typename boost::range_value<Range>::type
         , typename boost::iterator_reference<typename boost::range_iterator<Range const>::type>::type
         >
{
public:
    typedef typename boost::range_value<Range>::type result_;
    
    explicit generator_from_range(Range const& r)
      : range(r)
      , itr(boost::begin(const_cast<Range const&>(range))) {}
      
private:    
    bool more() const { return itr != boost::end(range); }
    
    typename boost::iterator_reference<typename boost::range_iterator<Range const>::type>::type peek() const { return *itr; }
    
    void pop() { ++itr; }
    
    friend class gusc::generator_access;
    Range range;
    typename boost::range_iterator<Range const>::type itr;
};

////////////////////////////////////////////////////////////////////////////////

template <class Range>
generator_from_range<Range> generate_from_range(Range const& rng)
{ 
    return generator_from_range<Range>(rng); 
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__GENERATOR__GENERATOR_FROM_ITERATOR_HPP
