# if ! defined(GUSC__GENERATOR__FINITE_UNION_GENERATOR_HPP)
# define       GUSC__GENERATOR__FINITE_UNION_GENERATOR_HPP

# include <boost/foreach.hpp>
# include <boost/range.hpp>
# include <boost/utility/result_of.hpp>
# include <gusc/functional.hpp>
# include <queue>
# include <boost/shared_ptr.hpp>
# include <boost/iterator/iterator_facade.hpp>
# include <iterator>

namespace gusc {

template <class Generator, class Compare = less>
class finite_union_generator 
  : public peekable_generator_facade<
      finite_union_generator<Generator,Compare>
    , typename boost::result_of<Generator()>::type
    >
{
public:
    typedef typename boost::result_of<Generator()>::type result_type_;
    template <class Range>
    finite_union_generator( Range const& rng
                          , Compare const& cmp ) 
      : impl(new impl_type(rng,cmp)) { }
      
    finite_union_generator(finite_union_generator const& fug)
    : impl(fug.impl) {}
    
    finite_union_generator& operator=(finite_union_generator const& fug)
    {
        impl = fug.impl;
        return *this;
    }

    finite_union_generator() {}
private:
    friend class generator_access;

    void pop()
    {
        Generator g(impl->heap.top());
        impl->heap.pop();
        ++g;
        if (g) {
            impl->heap.push(g);
        }
    }

    result_type_ const& peek() const
    {
        return *(impl->heap.top());
    }

    bool more() const { return impl && !impl->heap.empty(); }

    struct compare_current {
        Compare compare;
        bool operator()(Generator const& g1, Generator const& g2) const
        {
            return compare(*g1,*g2);
        }
        compare_current(Compare const& compare) : compare(compare) {}
    };

    struct impl_type {
        template <class Range>
        impl_type( Range const& rng
                 , Compare const& compare
                 ) 
        : heap(compare_current(compare)) 
        {
            BOOST_FOREACH(Generator gen, rng) {
                if (gen) {
                    heap.push(gen);
                }
            }
        }

        std::priority_queue<Generator, std::vector<Generator>, compare_current> 
            heap;
    };

    boost::shared_ptr<impl_type> impl;
};

////////////////////////////////////////////////////////////////////////////////

template <class Compare, class Range>
finite_union_generator<typename boost::range_value<Range>::type, Compare>
generate_finite_union(Range const& rng, Compare const& cmp)
{
    return 
        finite_union_generator< 
          typename boost::range_value<Range>::type
        , Compare
        >(rng,cmp);
}

} // namespace gusc

# endif //     GUSC__GENERATOR__UNION_GENERATOR_HPP
