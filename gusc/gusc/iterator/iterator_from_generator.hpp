# if ! defined(GUSC__ITERATOR__ITERATOR_FROM_GENERATOR_HPP)
# define       GUSC__ITERATOR__ITERATOR_FROM_GENERATOR_HPP

# include <boost/iterator/iterator_facade.hpp>
# include <boost/range.hpp>
# include <boost/optional.hpp>
# include <boost/shared_ptr.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
///
///  convert a generator into an input iterator.
///  memoizes current value for repeated dereferencing
///
////////////////////////////////////////////////////////////////////////////////
template <class Generator>
class iterator_from_generator
  : public boost::iterator_facade<
               iterator_from_generator<Generator>
             , typename boost::result_of<Generator()>::type
             , std::input_iterator_tag
             , typename boost::result_of<Generator()>::type const&
           > {
public:
    typedef typename Generator::result_type result_type;
private:
  struct state {
        state(Generator const& g)
          : generator(g)
          , result(generator()) {}
        Generator generator;
        result_type result;
  };
public:
    iterator_from_generator() : s(boost::none) {}
  iterator_from_generator(Generator const& g) : s(boost::none)
  {
    if (g) s=state(g);
  }

    operator bool() const { return not at_end(); }
    result_type operator()()
    {
        result_type res(s.get().result);
        increment();
        return res;
    }

private:
    void increment()
    {
        if (bool(s.get().generator)) {
            s.get().result = s.get().generator();
        }
        else s = boost::none;
    }

    bool at_end() const
    {
        return (not bool(s));
    }

    // comparing two not-at-end iterators is undefined
    bool equal(iterator_from_generator const& other) const
    {
        return at_end() == other.at_end();
    }

    result_type const& dereference() const { return s.get().result; }

    boost::optional<state> s;

    friend class boost::iterator_core_access;
};

template <class Generator>
iterator_from_generator<Generator>
generator_as_iterator(Generator const& gen)
{
    return iterator_from_generator<Generator>(gen);
}

template <class Generator>
std::pair< iterator_from_generator<Generator>, iterator_from_generator<Generator> >
range_from_generator(Generator const& gen) //a more direct type of range might be more efficient.
{
    return std::make_pair( iterator_from_generator<Generator>(gen)
                         , iterator_from_generator<Generator>() );
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__ITERATOR__ITERATOR_FROM_GENERATOR_HPP
