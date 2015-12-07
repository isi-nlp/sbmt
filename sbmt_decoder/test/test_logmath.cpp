// make sure we don't disable +,-
#include <sbmt/logmath/logbase.hpp>
#include <sbmt/logmath/lognumber.hpp>
#include <sbmt/logmath.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <sstream>
#include <boost/mpl/vector.hpp>
#include "test_util.hpp"

typedef boost::mpl::vector<
            sbmt::logmath::precision<5>
          , sbmt::logmath::exp_n<1>
          , sbmt::logmath::exp_n<-2>
          , sbmt::logmath::x_pow_n<3,5>
          , sbmt::logmath::x_pow_n_neg<3,-3>
        > bases;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_lognumber_operations, BaseT, bases)
{
    using namespace sbmt;
    using namespace sbmt::logmath;
    using namespace std;

    typedef basic_lognumber<BaseT,float> lognum;

    lognum zero2=as_zero();
    lognum half=0.5;
    lognum qtr=0.25;
    lognum zero=0.0;
    lognum one=1.0;
    lognum two=2.0;
    lognum three=3.0;
    lognum four=4.0;
    lognum nine=9.0;
    lognum eleven=11.0;
    lognum eighteen=18.0;
    lognum eightyone=81;
    lognum ninety=90;
    lognum zero3(-HUGE_VAL,as_log10());

    BOOST_CHECK_EQUAL(zero,zero2);
    BOOST_CHECK_EQUAL(zero,zero3);

    lognum tolerance=0.00002;

    double linear_tolerance=.001;

    /// first, try a little addition...
    LOGNUMBER_CHECK_CLOSE(half + half, one,      tolerance);
    LOGNUMBER_CHECK_CLOSE(nine + nine, eighteen, tolerance);
    LOGNUMBER_CHECK_CLOSE(nine + two,  eleven,   tolerance);

    /// next, try a little multiplication...
    LOGNUMBER_CHECK_CLOSE(half * half, qtr,       tolerance);
    LOGNUMBER_CHECK_CLOSE(nine * nine, eightyone, tolerance);
    LOGNUMBER_CHECK_CLOSE(nine * two,  eighteen,  tolerance);

    //subtraction
    LOGNUMBER_CHECK_CLOSE(half - half,   zero,      tolerance);
    LOGNUMBER_CHECK_CLOSE(half - qtr,    qtr,       tolerance);
    LOGNUMBER_CHECK_CLOSE(one - qtr,     three*qtr, tolerance);
    LOGNUMBER_CHECK_CLOSE(eleven - nine, two,       tolerance);
    LOGNUMBER_CHECK_EQ(ninety-eightyone,   nine);
    LOGNUMBER_CHECK_EQ(ninety-nine,   eightyone);

    //clamped subtraction
    LOGNUMBER_CHECK_EQ(half - one,   zero);
    LOGNUMBER_CHECK_EQ(qtr - half,   zero);
    LOGNUMBER_CHECK_EQ(qtr - four,   zero);
    LOGNUMBER_CHECK_EQ(three - four,   zero);
    LOGNUMBER_CHECK_EQ(four - four,   zero);

    //diference
    LOGNUMBER_CHECK_CLOSE(half.difference(zero),   half,      tolerance);
    LOGNUMBER_CHECK_CLOSE(half.difference(half),   zero,      tolerance);
    LOGNUMBER_CHECK_CLOSE(half.difference(qtr),    qtr,       tolerance);
    LOGNUMBER_CHECK_CLOSE(one.difference(qtr),     three*qtr, tolerance);
    LOGNUMBER_CHECK_CLOSE(eleven.difference(nine), two,       tolerance);
    LOGNUMBER_CHECK_CLOSE(qtr.difference(half),    qtr,       tolerance);
    LOGNUMBER_CHECK_CLOSE(qtr.difference(one),     three*qtr, tolerance);
    LOGNUMBER_CHECK_CLOSE(nine.difference(eleven), two,       tolerance);
    LOGNUMBER_CHECK_EQ(eightyone.difference(ninety),   nine);


    /// and division...
    LOGNUMBER_CHECK_CLOSE(three / three,        one,        tolerance);
    LOGNUMBER_CHECK_CLOSE(half / two,           qtr,        tolerance);
    LOGNUMBER_CHECK_CLOSE(eightyone / eighteen, nine / two, tolerance);
    LOGNUMBER_CHECK_CLOSE(two / qtr,            four * two, tolerance);

    BOOST_CHECK_PREDICATE(std::less<lognum>(),(zero)(half));
    BOOST_CHECK_PREDICATE(std::less<lognum>(),(half)(one));

    /// make sure we can tell the difference between numbers....
    BOOST_CHECK_EQUAL(zero,zero2);
    BOOST_CHECK_EQUAL(zero!=zero2,false);
    BOOST_CHECK_EQUAL(three != two,         true);
//    BOOST_CHECK_EQUAL(three != lognum(3.0), false);
    BOOST_CHECK_EQUAL(four  == nine,        false);
    lognum four2=4;
    BOOST_CHECK_EQUAL(four,four2);
    BOOST_CHECK_EQUAL(four,four);
    BOOST_CHECK(!(four  != four));
    BOOST_CHECK(!(four  != four2));

    BOOST_CHECK_EQUAL((bool)(four  == four2), true);
    BOOST_CHECK_EQUAL((bool)(four  == four), true);
    BOOST_CHECK_EQUAL((bool)(four  != four), false);
    BOOST_CHECK_EQUAL((bool)(four  != four2), false);

    /// and make sure order relations are in order.
    BOOST_CHECK_EQUAL( three < four, true);
    BOOST_CHECK_EQUAL(three >= four, false);
    BOOST_CHECK_EQUAL(half >= qtr,   true);
    BOOST_CHECK_EQUAL(half < qtr,    false);

    BOOST_CHECK_EQUAL(std::max(half,qtr), half);
    BOOST_CHECK_EQUAL(std::min(half,qtr), qtr);

    BOOST_CHECK_CLOSE(pow(half,2.0).linear(),0.25,linear_tolerance);

    LOGNUMBER_CHECK_CLOSE(pow(half,-2.0) * qtr, one, tolerance * tolerance);

    lognum l_100(10,2);
    LOGNUMBER_CHECK_EQ(l_100,100);
    lognum l_p1(10,-1);
    LOGNUMBER_CHECK_EQ(l_p1,.1);
    lognum l_p5(2,-1);
    LOGNUMBER_CHECK_EQ(l_p5,.5);


    double e=exp(1.0);
    double power10=3.0;
    double tenpow_real=pow(10.0,power10);
    lognum tenpow(tenpow_real);
    double ln10=log(10.0);
    double lntenpow=log(tenpow_real);
    lognum tenpow_2(10.0,power10);
    lognum tenpow_3(power10,as_log10());

    lognum tenpow_4(lntenpow,as_ln());
    lognum tenpow_5(power10*ln10,as_ln());
    lognum tenpow_6(e,lntenpow);
    lognum tenpow_7(ln10,power10,as_log_base_ln());

    LOGNUMBER_CHECK_EQ(tenpow,tenpow_2);
    LOGNUMBER_CHECK_EQ(tenpow,tenpow_3);
    LOGNUMBER_CHECK_EQ(tenpow,tenpow_4);
    LOGNUMBER_CHECK_EQ(tenpow,tenpow_5);
    LOGNUMBER_CHECK_EQ(tenpow,tenpow_6);
    LOGNUMBER_CHECK_EQ(tenpow,tenpow_7);

    double log10tenpow_=log10(tenpow);
    double lntenpow_=log(tenpow);
    BOOST_CHECK_CLOSE(log10tenpow_,power10,linear_tolerance);
    BOOST_CHECK_CLOSE(lntenpow_,lntenpow,linear_tolerance);
    double log_10_power10=tenpow.log_base(10);
    BOOST_CHECK_CLOSE(log_10_power10,power10,linear_tolerance);

    lognum loose=.101;
    lognum exact=.1;
    lognum tight=.099;

    lognum Aa=1.1;
    lognum Ab=1.0;
    lognum Ba=1.1e-29;
    lognum Bb=1.0e-29;
    lognum Ca=1.1e29;
    lognum Cb=1.0e29;


    tolerance=.001;
    LOGNUMBER_CHECK_EQ(Aa.relative_difference(Ab),exact);
    LOGNUMBER_CHECK_EQ(Ab.relative_difference(Aa),exact);
    BOOST_CHECK(Ab.nearly_equal(Aa,loose));
    BOOST_CHECK(Aa.nearly_equal(Ab,loose));
    BOOST_CHECK(!Ab.nearly_equal(Aa,tight));
    BOOST_CHECK(!Aa.nearly_equal(Ab,tight));

    LOGNUMBER_CHECK_EQ(Ba.relative_difference(Bb),exact);
    LOGNUMBER_CHECK_EQ(Bb.relative_difference(Ba),exact);
    BOOST_CHECK(Bb.nearly_equal(Ba,loose));
    BOOST_CHECK(Ba.nearly_equal(Bb,loose));
    BOOST_CHECK(!Bb.nearly_equal(Ba,tight));
    BOOST_CHECK(!Ba.nearly_equal(Bb,tight));

    LOGNUMBER_CHECK_EQ(Ca.relative_difference(Cb),exact);
    LOGNUMBER_CHECK_EQ(Cb.relative_difference(Ca),exact);
    BOOST_CHECK(Cb.nearly_equal(Ca,loose));
    BOOST_CHECK(Ca.nearly_equal(Cb,loose));
    BOOST_CHECK(!Cb.nearly_equal(Ca,tight));
    BOOST_CHECK(!Ca.nearly_equal(Cb,tight));

    BOOST_CHECK_EQUAL(lognum(0.0) , lognum(0.0) * lognum(0.5));
}

BOOST_AUTO_TEST_CASE(test_lognumber_assignment)
{
    using namespace sbmt::logmath;

    typedef basic_lognumber< precision<5>,float > sphinx_lognum;
    typedef basic_lognumber< exp_n<1>,float > nat_lognum;

    sphinx_lognum sl1 = 0.5;
    nat_lognum nl1 = sl1;

    BOOST_CHECK_CLOSE(0.5,nl1.linear(),0.001);
}

BOOST_AUTO_TEST_CASE(test_lognumber_archive)
{
    using namespace sbmt;
    using namespace std;
    using namespace boost::archive;

    score_t x(0.1);

    stringstream strstr;
    binary_oarchive oa(strstr);

    oa & x;

    binary_iarchive ia(strstr);

    score_t y;
    ia & y;

    LOGNUMBER_CHECK_CLOSE(x,y,score_t(0.000001));
}

BOOST_AUTO_TEST_CASE(test_lognumber_vector_archive)
{
    using namespace sbmt;
    using namespace std;
    using namespace boost::archive;

    vector<score_t> x;
    x.push_back(score_t(0.1));
    x.push_back(score_t(2.5));
    x.push_back(score_t(0.5));

    stringstream strstr;
    binary_oarchive oa(strstr);

    oa & x;

    binary_iarchive ia(strstr);

    vector<score_t> y;
    ia & y;

    LOGNUMBER_CHECK_CLOSE(x[0],y[0],score_t(0.000001));
    LOGNUMBER_CHECK_CLOSE(x[1],y[1],score_t(0.000001));
    LOGNUMBER_CHECK_CLOSE(x[2],y[2],score_t(0.000001));
    BOOST_CHECK(x.size() == y.size());
}

namespace sbmt { namespace logmath {
    template <class B, class F>
    basic_lognumber<B,F> abs(basic_lognumber<B,F> const& x)
    {
        return x;
    }

    template <class B, class F>
    basic_lognumber<B,F> sqrt(basic_lognumber<B,F> const& x)
    {
        return basic_lognumber<B,F>(x.log()/2.0,as_log());
    }
} }

# include <boost/numeric/ublas/vector_sparse.hpp>
# include <boost/numeric/ublas/io.hpp>
# include <sbmt/hash/hash_map.hpp>
# include <complex>
# include <sbmt/feature/feature_vector.hpp>

BOOST_AUTO_TEST_CASE(test_weight_vector_ops)
{
    sbmt::weight_vector w1, w2, w3, w4;
    w1[2] = 1;
    w1[5] = 1;
    w1[9] = 1;

    for (int x = 1; x != 10; ++x) w2[x] = x;

    BOOST_CHECK_EQUAL(dot(w1,w2), 2 + 5 + 9);
    BOOST_CHECK_EQUAL(dot(w2,w1), 2 + 5 + 9);

    for (int x = 1; x != 10; ++x) w3[x] = 1;

    BOOST_CHECK_EQUAL(dot(w2,w3), (9*10)/2);
    BOOST_CHECK_EQUAL(dot(w3,w2), (9*10)/2);

    BOOST_CHECK_EQUAL(w2 - w2, sbmt::weight_vector());

    BOOST_CHECK_EQUAL(w1 + w2, w2 + w1);

    //BOOST_CHECK_EQUAL(w1 + w1, 2 * w1);

    BOOST_CHECK_EQUAL(sbmt::weight_vector() - w2, -w2);
}

BOOST_AUTO_TEST_CASE(test_compressed_vector_score)
{
    sbmt::in_memory_token_storage dict;
    sbmt::feature_vector f1,f2,f3;
    sbmt::weight_vector w1,w2;

    get(f1,dict,"a") *= 1e-1;
    get(f1,dict,"b") *= 1e-2;
    get(f1,dict,"c") *= 1e-3;

    get(f2,dict,"c") = 1e-3;
    get(f2,dict,"d") = 1e-4;
    get(f2,dict,"a") = 1e-1;

    get(f3,dict,"a") = 1e-2;
    get(f3,dict,"b") = 1e-2;
    get(f3,dict,"c") = 1e-6;
    get(f3,dict,"d") = 1e-4;

    BOOST_CHECK_EQUAL(f1 * f2, f3);

    sbmt::fat_weight_vector w;
    w["a"] = 3.0;
    w["b"] += 1.5;
    w["c"] -= -1.0;

    w1 = index(w,dict);

    BOOST_CHECK_EQUAL(geom(f1,w1), 1e-9);
}

BOOST_AUTO_TEST_CASE(test_lognumber_lexical_cast)
{
    using namespace sbmt;
    using namespace sbmt::logmath;
    using namespace std;

    ln_number tolerance(0.0001);

    string e_inv_str("e^-1");
    double e_inv = exp(-1.0);

    BOOST_CHECK_THROW(boost::lexical_cast<ln_number>("A"),boost::bad_lexical_cast);
    BOOST_CHECK_THROW(boost::lexical_cast<ln_number>("10a"),boost::bad_lexical_cast);
    BOOST_CHECK_THROW(boost::lexical_cast<ln_number>("10^^"),boost::bad_lexical_cast);

    {
        ln_number n(10,4);
        stringstream s;
        s << log_scale << log_base_10;
        s << n;
        LOGNUMBER_CHECK_CLOSE(logmath::lexical_cast<ln_number>(s.str()),n,tolerance);
    }

    {
        ln_number n(10,100000);
        stringstream s;
        s << log_scale << log_base_10;
        s << n;
        LOGNUMBER_CHECK_EXP_CLOSE(logmath::lexical_cast<ln_number>(s.str()),n,tolerance);
    }

    score_t x(1e-4);
    stringstream sstr;
    sstr << x;
    BOOST_CHECK_EQUAL(x, boost::lexical_cast<score_t>(sstr.str()));

    {
        ln_number n(-100000,as_ln());
        stringstream s;
        s << n;
        LOGNUMBER_CHECK_EXP_CLOSE(logmath::lexical_cast<ln_number>(s.str()),n,tolerance);
    }

    string ten_inv_str("10^-2");
    BOOST_CHECKPOINT("before cast 10^-2");
    ln_number x0 = logmath::lexical_cast<ln_number>(ten_inv_str);
    LOGNUMBER_CHECK_CLOSE(ln_number(0.01),x0,tolerance)
    BOOST_CHECKPOINT("after cast 10^-2");

    ten_inv_str = "10^0";
    BOOST_CHECKPOINT("before cast 10^0");
    x0 = logmath::lexical_cast<ln_number>(ten_inv_str);
    LOGNUMBER_CHECK_CLOSE(ln_number(1.0),x0,tolerance)
    BOOST_CHECKPOINT("after cast 10^0");

    BOOST_CHECKPOINT("before cast 10^-0");
    ten_inv_str = "10^-0";
    x0 = logmath::lexical_cast<ln_number>(ten_inv_str);
    LOGNUMBER_CHECK_CLOSE(ln_number(1.0),x0,tolerance)
    BOOST_CHECKPOINT("after cast 10^-0");

#ifdef  OLD_LOGMATH_LEXICAL_CAST
    BOOST_CHECK(boost::regex_search(e_inv_str,logmath::match_e_carat));

    BOOST_CHECK_EQUAL(
        string("-1"),
        boost::regex_replace(e_inv_str,logmath::match_e_carat,"")
    );
#endif
    ln_number x1 = logmath::lexical_cast<ln_number>(e_inv_str);
    ln_number x2(-1,as_log());
    ln_number x3 = e_inv;

    ln_number x5(10,2);
    ln_number x6 = 100.0;

    LOGNUMBER_CHECK_CLOSE(x5,x6,tolerance);

    double logbase = ::log(10.0);
    x5 = ln_number(logbase,2.0,as_log_base_ln());

    LOGNUMBER_CHECK_CLOSE(x5,x6,tolerance);

    BOOST_CHECK_EQUAL(x1,x2);
    BOOST_CHECK_EQUAL(x2,x3);
}
