# include <sbmt/span.hpp>
# include <boost/test/auto_unit_test.hpp>
# include <boost/iterator/reverse_iterator.hpp>
# include <iostream>
# include <sstream>
# include <vector>

using namespace sbmt;
using boost::reverse_iterator;

BOOST_AUTO_TEST_CASE(test_partitions_generator)
{
    partitions_generator pg(span_t(1,5));
    partitions_generator::iterator itr = pg.begin();
    partitions_generator::iterator end = pg.end();
    
    BOOST_CHECK_EQUAL(itr->first , span_t(1,2));
    BOOST_CHECK_EQUAL(itr->second , span_t(2,5));
    ++itr;
    BOOST_CHECK(itr != end);
    
    BOOST_CHECK_EQUAL(itr->first , span_t(1,3));
    BOOST_CHECK_EQUAL(itr->second , span_t(3,5));
    ++itr;
    BOOST_CHECK(itr != end);
    
    BOOST_CHECK_EQUAL(itr->first , span_t(1,4));
    BOOST_CHECK_EQUAL(itr->second , span_t(4,5));
    ++itr;
    
    BOOST_CHECK(itr == end);
    BOOST_CHECK_EQUAL(itr->first, span_t(1,5));
    
    ++itr;
    BOOST_CHECK(itr == end);  
    BOOST_CHECK_EQUAL(itr->first, span_t(1,5));
    /*
    std::cout << itr->second << " <-- past right marker";
    
    reverse_iterator<partitions_generator::iterator> ritr(pg.end());
    reverse_iterator<partitions_generator::iterator> rend(pg.begin());
    
    BOOST_CHECK_EQUAL(ritr->first  , span_t(1,4));
    BOOST_CHECK_EQUAL(ritr->second , span_t(4,5));
    ++ritr;
    
    BOOST_CHECK_EQUAL(ritr->first  , span_t(1,3));
    BOOST_CHECK_EQUAL(ritr->second , span_t(3,5));
    ++ritr;
    
    BOOST_CHECK_EQUAL(ritr->first  , span_t(1,2));
    BOOST_CHECK_EQUAL(ritr->second , span_t(2,5));
    ++ritr;
    
    BOOST_CHECK(ritr == rend);
    
    --ritr;
    BOOST_CHECK_EQUAL(ritr->first  , span_t(1,2));
    BOOST_CHECK_EQUAL(ritr->second , span_t(2,5));
    */
    
}

BOOST_AUTO_TEST_CASE(test_span_io)
{
    using namespace std;
    stringstream sstr;
    sstr << "[0,1] [2,2]  [3,5][2,4]  ";
    vector<span_t> v(4);
    sstr >> v[0] >> v[1] >> v[2] >> v[3];
    span_t w[4] = { span_t(0,1)
                  , span_t(2,2)
                  , span_t(3,5)
                  , span_t(2,4) };
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), w, w + 4);
}

BOOST_AUTO_TEST_CASE(test_shift_generator)
{
    shift_generator gen(span_t(0,5),2);
    shift_generator::iterator itr = gen.begin();
    shift_generator::iterator end = gen.end();
    
    BOOST_CHECK_EQUAL(*itr, span_t(0,2));
    BOOST_CHECK(itr != end);
    
    ++itr;
    BOOST_CHECK_EQUAL(*itr, span_t(1,3));
    BOOST_CHECK(itr != end);
    
    ++itr;
    BOOST_CHECK_EQUAL(*itr, span_t(2,4));
    BOOST_CHECK(itr != end);
    
    ++itr;
    BOOST_CHECK_EQUAL(*itr, span_t(3,5));
    BOOST_CHECK(itr != end);
    
    ++itr;
    BOOST_CHECK_EQUAL(*itr, span_t(4,6));
    BOOST_CHECK_EQUAL(*end, span_t(4,6));
    BOOST_CHECK(itr == end);
    /*
    reverse_iterator<shift_generator::iterator> ritr(itr);
    reverse_iterator<shift_generator::iterator> rend(gen.begin());
    
    BOOST_CHECK_EQUAL(*ritr, span_t(3,5));
    BOOST_CHECK(ritr != rend);
    ++ritr;
    
    BOOST_CHECK_EQUAL(*ritr, span_t(2,4));
    BOOST_CHECK(ritr != rend);
    ++ritr;
    
    BOOST_CHECK_EQUAL(*ritr, span_t(1,3));
    BOOST_CHECK(ritr != rend);
    ++ritr;
    
    BOOST_CHECK_EQUAL(*ritr, span_t(0,2));
    BOOST_CHECK(ritr != rend);
    ++ritr;
    
    BOOST_CHECK(ritr == rend);
    
    --ritr;
    BOOST_CHECK_EQUAL(*ritr, span_t(0,2));
    */
}

BOOST_AUTO_TEST_CASE(test_span_iterator)
{
    shift_generator sgen(span_t(0,5),3);
    span_iterator sitr = sgen.begin(), send = sgen.end();
    BOOST_CHECK_EQUAL_COLLECTIONS(sitr,send,sgen.begin(),sgen.end());
}

BOOST_AUTO_TEST_CASE(test_combining_spans)
{
    BOOST_CHECK_EQUAL(span_t(0,5) , combine(span_t(0,2),span_t(2,5)));
}

BOOST_AUTO_TEST_CASE(test_limit_diff)
{
    using namespace std;
    
    string expected_partitions =
    "[0,1] "
    "[1,2] "
    "[2,3] "
    "[3,4] "
    "[4,5] "
    "[5,6] "
    "[0,2] [0,1] [1,2] "
    "[1,3] [1,2] [2,3] "
    "[2,4] [2,3] [3,4] "
    "[3,5] [3,4] [4,5] "
    "[4,6] [4,5] [5,6] "
    "[0,3] [0,1] [1,3] [0,2] [2,3] "
    "[1,4] [1,2] [2,4] [1,3] [3,4] "
    "[2,5] [2,3] [3,5] [2,4] [4,5] "
    "[3,6] [3,4] [4,6] [3,5] [5,6] "
    "[0,4] [0,1] [1,4] [0,2] [2,4] [0,3] [3,4] "
    "[1,5] [1,3] [3,5] "
    "[2,6] [2,4] [4,6] "
    "[0,5] [0,1] [1,5] [0,2] [2,5] [0,3] [3,5] [0,4] [4,5] "
    "[1,6] [1,3] [3,6] [1,4] [4,6] "
    "[0,6] [0,1] [1,6] [0,2] [2,6] [0,3] [3,6] [0,4] [4,6] [0,5] [5,6] ";
    stringstream sstr(expected_partitions);
    vector<span_t> spans;
    spans.insert( spans.begin()
                , istream_iterator<span_t>(sstr)
                , istream_iterator<span_t>() );
    
    limit_split_difference_generator gen(1);
    
    std::vector<span_t>::iterator vitr = spans.begin(), vend = spans.end();
    
    for (size_t x = 1; x != 7; ++x) {
        span_iterator shiftx, shiftend;
        boost::tie(shiftx,shiftend) = gen.shifts(span_t(0,6),x);
        for (; shiftx != shiftend; ++shiftx) {
            BOOST_REQUIRE(vitr != vend);
            BOOST_CHECK_EQUAL(*vitr, *shiftx);
            ++vitr;
            //cerr << "span: " << *shiftx << " [" << flush;
            partition_iterator splitx, splitend;
            boost::tie(splitx,splitend) = gen.partitions(*shiftx);
            
            for (; splitx != splitend; ++splitx) {
                //cerr << "("<< splitx->first << " , " << splitx->second << ")" << flush;
                BOOST_REQUIRE(vitr != vend);
                BOOST_CHECK_EQUAL(*vitr, splitx->first);
                ++vitr;
                BOOST_REQUIRE(vitr != vend);
                BOOST_CHECK_EQUAL(*vitr, splitx->second);
                ++vitr;
            }
            //cerr << "]" << endl;
        }
    }
}
