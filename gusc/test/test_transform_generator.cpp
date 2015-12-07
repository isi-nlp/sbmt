# include <iostream>
# include <boost/test/auto_unit_test.hpp>
# include <gusc/generator/transform_generator.hpp>

struct autoinc {
    autoinc() : x(0) {}
    operator bool() const { return true; }
    typedef int result_type;
    int operator()() { return x++; }
    int x;
};

struct mult {
    double x;
    mult(double x) : x(x) {}
    typedef double result_type;
    template <class X>
    double operator()(X y) const { return x * y; }
};

BOOST_AUTO_TEST_CASE(test_transform_generator)
{
    gusc::transform_generator<autoinc,mult> gen(autoinc(),mult(0.5));
    for (int x = 0; x != 5; ++x) {
        BOOST_CHECK_EQUAL(x * 0.5 , gen());
    }
}