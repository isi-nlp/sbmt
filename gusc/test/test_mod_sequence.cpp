# include <boost/test/auto_unit_test.hpp>
# include <gusc/mod_sequence.hpp>
# include <vector>

BOOST_AUTO_TEST_CASE(test_mod_sequence)
{
    int modseq[3] = {0, 0, 0};
    int expected[3] = {1, 4, 5};
    gusc::mod_sequence(145,10,&modseq[0]);
    BOOST_CHECK_EQUAL_COLLECTIONS(modseq, modseq + 3, expected, expected + 3);
}