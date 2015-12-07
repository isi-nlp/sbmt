# include <iostream>
# include <boost/test/auto_unit_test.hpp>
# include <gusc/iterator/concat_iterator.hpp>
# include <gusc/iterator/reverse.hpp>

# define CHECK_EQUAL_RANGES(x,y) \
BOOST_CHECK_EQUAL_COLLECTIONS(boost::begin(x), boost::end(x), boost::begin(y), boost::end(y))


namespace {
int first[] = {1, 2, 3, 4};
int second[] = {5, 6, 7, 8};
}

BOOST_AUTO_TEST_CASE(test_concat_iterator)
{
    using gusc::concatenate_ranges;
    
    int combine12[] = {1, 2, 3, 4, 5, 6, 7, 8};
    CHECK_EQUAL_RANGES(concatenate_ranges(first,second), combine12);
    
    int combine21[] = {5, 6, 7, 8, 1, 2, 3, 4};
    CHECK_EQUAL_RANGES(concatenate_ranges(second,first), combine21);
    
    int combine11[] = {1, 2, 3, 4, 1, 2, 3, 4};
    CHECK_EQUAL_RANGES(concatenate_ranges(first,first), combine11);
}

BOOST_AUTO_TEST_CASE(test_concat_iterator_reverse)
{
    using gusc::concatenate_ranges;
    using gusc::reverse_range;
    using namespace std;
    
    typedef gusc::concat_iterator<int*> concat_t;
    typedef reverse_iterator<concat_t> reverse_t;
    
    int revcombine12[] = {8, 7, 6, 5, 4, 3, 2, 1};
    int revcombine21[] = {4, 3, 2, 1, 8, 7, 6, 5};
    int revcombine22[] = {8, 7, 6, 5, 8, 7, 6, 5};
    
    CHECK_EQUAL_RANGES(
        reverse_range(concatenate_ranges(second,first))
      , revcombine21
    );
    
    CHECK_EQUAL_RANGES(
        reverse_range(concatenate_ranges(first,second))
      , revcombine12
    );
    
    CHECK_EQUAL_RANGES(
        reverse_range(concatenate_ranges(second,second))
      , revcombine22
    );
}