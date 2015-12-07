# include <gusc/iterator/iterator_from_generator.hpp>
# include <boost/test/auto_unit_test.hpp>

# include <stdexcept>

template <class Value>
struct empty {
    typedef Value result_type;
    result_type operator()() const { throw std::runtime_error("empty!"); }
    operator bool() const { return false; }
};

struct xrange {
    typedef int result_type;
    xrange(int b, int e, int i = 1) : x(b), e(e), i(i) {}
    operator bool() const { return x <= e; }
    int operator()() { int ret = x; x += i; return ret; } 
};

BOOST_AUTO_TEST_CASE(test_iterator_from_empty_generator)
{
    using namespace gusc;
    iterator_from_generator<empty<int> > itr(empty()), end;
    int x[] = {}
    BOOST_CHECK_EQUAL_RANGE(itr,end,x,x);
}

BOOST_AUTO_TEST_CASE(test_iterator_from_range)
{
    using namespace gusc;
    int rng = { 0, 1, 2, 3, 4, 5 };
    iterator_from_generator<xrange> itr(xrange(0,5)), end;
    BOOST_CHECK_EQUAL_RANGE(itr,end,rng,rng + 6);
}