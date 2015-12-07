# include <gusc/generator/lazy_sequence.hpp>
# include <boost/test/auto_unit_test.hpp>


struct kth {
    kth(int start, int end, int k) : curr(start), end(end), k(k) {}
    int curr;
    int end;
    int k;
    
    typedef int result_type;
    operator bool() const { return curr <= end; }
    int operator()() { int ret = curr; curr += k; return ret; }
};

struct open_kth {
    open_kth(int start, int k) : curr(start), k(k) {}
    int curr;
    int k;
    
    typedef int result_type;
    operator bool() const { return true; }
    int operator()() { int ret = curr; curr += k; return ret; }
};

struct empty_seq {
    typedef int result_type;
    operator bool() const { return false; }
    int operator()() { throw std::runtime_error("dont access"); }
};

BOOST_AUTO_TEST_CASE(test_empty_sequence)
{
    gusc::lazy_sequence<empty_seq> seq;
    BOOST_CHECK(seq.begin() == seq.end());
}

BOOST_AUTO_TEST_CASE(test_lazy_sequence_index)
{
    gusc::lazy_sequence<kth> kth_seq(kth(0,20000,2));
    
    BOOST_CHECK_EQUAL(kth_seq[10], 20);
    BOOST_CHECK_EQUAL(kth_seq[10000], 20000);
    BOOST_CHECK_THROW(kth_seq[10001], std::out_of_range);
    
    gusc::lazy_sequence<open_kth> open_kth_seq(open_kth(0,2));
    for (size_t x = 0; x != 20u; ++x) {
        BOOST_CHECK_EQUAL(open_kth_seq[x], 2*int(x));
    }
}

BOOST_AUTO_TEST_CASE(test_lazy_sequence_bidirectional_iterator)
{
    typedef gusc::lazy_sequence<open_kth> seq_t;
    seq_t seq(open_kth(0,2));
    
    seq_t::iterator beg = seq.begin(), end = seq.end();
    
    BOOST_CHECK(beg != end);
    int x = 0;
    for (seq_t::iterator itr = beg; itr != end and x != 10; ++itr, ++x) {
        BOOST_CHECK_EQUAL(*itr, 2*x);
        int y = x;
        for (seq_t::iterator jtr = itr; jtr != beg - 1; --jtr, --y) {
            BOOST_CHECK_EQUAL(*jtr, 2*y);
        }
        BOOST_CHECK_EQUAL(y,-1);
    }
    BOOST_CHECK_EQUAL(x,10);
}

BOOST_AUTO_TEST_CASE(test_lazy_sequence_bidirectional_iterator_end)
{
    typedef gusc::lazy_sequence<kth> seq_t;
    seq_t seq(kth(0,6,2));
    
    seq_t::iterator beg = seq.begin(), end = seq.end();
    
    BOOST_CHECK(beg != end);
    int x = 0;
    for (seq_t::iterator itr = beg; itr != end and x != 10; ++itr, ++x) {
        BOOST_CHECK_EQUAL(*itr, 2*x);
        int y = x;
        for (seq_t::iterator jtr = itr; jtr != beg - 1; --jtr, --y) {
            BOOST_CHECK_EQUAL(*jtr, 2*y);
        }
        BOOST_CHECK_EQUAL(y,-1);
    }
    BOOST_CHECK_EQUAL(x,4);
}

BOOST_AUTO_TEST_CASE(test_lazy_sequence_random_access)
{
    typedef gusc::lazy_sequence<open_kth> seq_t;
    
    int ordering[] = {0, 1, 2, 3, 4, 5, 6 };
    while (std::next_permutation(ordering, ordering + 7)) {
        seq_t seq(open_kth(0,2));
        int* itr = ordering; int* end = ordering + 7;
        for (; itr != end; ++itr) {
            BOOST_CHECK_EQUAL(seq[*itr], 2*(*itr));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_lazy_sequence_random_access2)
{
    typedef gusc::lazy_sequence<open_kth> seq_t;
    
    int ordering[] = {0, 1, 2, 3, 4, 5, 6 };
    while (std::next_permutation(ordering, ordering + 7)) {
        seq_t seq(open_kth(0,2));
        int* itr = ordering; int* end = ordering + 7;
        for (; itr != end; ++itr) {
            BOOST_CHECK_EQUAL(*(seq.begin() + *itr), 2*(*itr));
        }
    }
}
