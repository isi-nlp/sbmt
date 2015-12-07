# include <boost/test/auto_unit_test.hpp>
# include <gusc/varray.hpp>
# include <vector>

BOOST_AUTO_TEST_CASE(varray_copyable)
{
    gusc::varray<int> v(5);
    int c[5] = {1,2,3,4,5};
    std::vector<int> C(c,c+5);
    std::copy(c, c + 5, v.begin());
    BOOST_CHECK_EQUAL_COLLECTIONS(c, c + 5, v.begin(), v.end());
    gusc::varray<int> t(v);
    BOOST_CHECK_EQUAL_COLLECTIONS(c, c + 5, t.begin(), t.end());
    gusc::varray<int> s;
    s = v;
    BOOST_CHECK_EQUAL_COLLECTIONS(c, c + 5, s.begin(), s.end());
    //s = c;
    //BOOST_CHECK_EQUAL_COLLECTIONS(c, c + 5, s.begin(), s.end());
    //s = C;
    //BOOST_CHECK_EQUAL_COLLECTIONS(c, c + 5, s.begin(), s.end());
    gusc::varray<int> V(c);
}

BOOST_AUTO_TEST_CASE(varray_equality_comparable)
{
    
}