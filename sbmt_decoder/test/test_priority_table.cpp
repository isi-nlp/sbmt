#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <sbmt/hash/priority_table.hpp>
#include <iostream>

using namespace std;

namespace test {

/// hard to believe that these well-known functors are actually not in the
/// standard library.  they are just extensions of many libraries.

template <class X> struct select1st {
    typedef X result_type;
    template <class Y>
    X const& operator()(std::pair<X,Y> const& p) const { return p.first; }
};

template <class Y> struct select2nd {
    typedef Y result_type;
    template <class X>
    Y const& operator()(std::pair<X,Y> const& p) const { return p.second; }
};

}

namespace std {
ostream& operator<<(ostream& out, pair<int,int> const& p)
{
    out << "(" << p.first << " " << p.second << ")";
    return out;
}
}

struct modify_second
{
    modify_second(int x) : x(x){}
    void operator()(std::pair<int,int>& p) const
    {
        p.second = x;
    }
    int x;
};

BOOST_AUTO_TEST_CASE(test_priority_table)
{
    using std::pair;
    using std::make_pair;
    using namespace test;
    using namespace sbmt;

    typedef priority_table< pair<int,int>
                          , test::select1st<int>
                          , test::select2nd<int> >  ptable_t;

    ptable_t ptable1;
    ptable_t ptable2(ptable1);
    ptable_t ptable;
    ptable = ptable2;
    size_t ptable_size = 0;

    pair<ptable_t::iterator,bool> ret;
    ret = ptable.insert(make_pair(12,9));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK(*ret.first == make_pair(12,9));
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);

    ret = ptable.insert(make_pair(1,7));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK(*ret.first == make_pair(1,7));
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);

    ret = ptable.insert(make_pair(2,20));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK(*ret.first == make_pair(2,20));
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);


    ret = ptable.insert(make_pair(3,-2));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);

    ret = ptable.insert(make_pair(4,12));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);

    ret = ptable.insert(make_pair(5,100));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);

    ret = ptable.insert(make_pair(6,-80));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);

    ret = ptable.insert(make_pair(12,8));
    BOOST_CHECK_EQUAL(ret.second,false);
    BOOST_CHECK(*ret.first == make_pair(12,9));
    BOOST_CHECK_EQUAL(ptable.size(),ptable_size);

    ret = ptable.insert(make_pair(7,7));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);

    ptable.erase(7);
    BOOST_CHECK(ptable.find(7) == ptable.end());
    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);


    BOOST_CHECK(ptable.find(3) != ptable.end());
    BOOST_CHECK(ptable.erase(3));
    BOOST_CHECK(ptable.find(3) == ptable.end());
    //if (ptable.find(3) != ptable.end()) cerr << *(ptable.find(3)) << endl;
    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    ret = ptable.insert(make_pair(3,-2));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);

    ptable.erase(2);
    BOOST_CHECK(ptable.find(2) == ptable.end());
    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    ret = ptable.insert(make_pair(2,20));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);

    ptable.erase(5);
    BOOST_CHECK(ptable.find(5) == ptable.end());
    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    ret = ptable.insert(make_pair(5,100));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);
    BOOST_CHECK(ptable.top() == make_pair(5,100));

    ptable.erase(6);
    BOOST_CHECK(ptable.find(6) == ptable.end());
    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    ret = ptable.insert(make_pair(6,-80));
    BOOST_CHECK_EQUAL(ret.second,true);
    BOOST_CHECK_EQUAL(ptable.size(),++ptable_size);


    ptable_t::iterator pos = ptable.find(1);
    ptable.modify(pos,modify_second(1));
    BOOST_CHECK_EQUAL(*ptable.find(1),make_pair(1,1));

    pos = ptable.find(2);
    ptable.modify(pos,modify_second(2));
    BOOST_CHECK_EQUAL(*ptable.find(2), make_pair(2,2));

    pos = ptable.find(3);
    ptable.modify(pos,modify_second(7));
    BOOST_CHECK_EQUAL(*ptable.find(3), make_pair(3,7));

    pos = ptable.find(4);
    ptable.modify(pos,modify_second(3));
    BOOST_CHECK_EQUAL(*ptable.find(4), make_pair(4,3));

    pos = ptable.find(5);
    ptable.modify(pos,modify_second(0));
    BOOST_CHECK_EQUAL(*ptable.find(5), make_pair(5,0));

    pos = ptable.find(6);
    ptable.modify(pos,modify_second(5));
    BOOST_CHECK_EQUAL(*ptable.find(6), make_pair(6,5));

    pos = ptable.find(1);
    BOOST_CHECK_EQUAL(pos == ptable.end(),false);
    BOOST_CHECK_EQUAL(*ptable.find(1),make_pair(1,1));

    pos = ptable.find(22);
    BOOST_CHECK_EQUAL(pos == ptable.end(),true);

    pos = ptable.find(12);
    BOOST_CHECK(ptable.replace(pos,make_pair(12,6)) == true);
    BOOST_CHECK_EQUAL(*ptable.find(12), make_pair(12,6));

    BOOST_CHECK(ptable.replace(pos,make_pair(11,6)) == false);

    BOOST_CHECK_EQUAL(ptable.size(),ptable_size);
    BOOST_CHECK_EQUAL(ptable.top(),make_pair(3,7));
    ptable.pop();

    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    BOOST_CHECK_EQUAL(ptable.top(),make_pair(12,6));
    ptable.pop();

    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    BOOST_CHECK_EQUAL(ptable.top(),make_pair(6,5));
    ptable.pop();

    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    BOOST_CHECK_EQUAL(ptable.top(),make_pair(4,3));
    ptable.pop();

    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    BOOST_CHECK_EQUAL(ptable.top(),make_pair(2,2));
    ptable.pop();

    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    BOOST_CHECK_EQUAL(ptable.top(),make_pair(1,1));
    ptable.pop();

    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
    BOOST_CHECK_EQUAL(ptable.top(),make_pair(5,0));
    ptable.pop();

    BOOST_CHECK_EQUAL(ptable.size(),--ptable_size);
}
