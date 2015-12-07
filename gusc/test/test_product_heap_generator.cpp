# include <gusc/generator/product_heap_generator.hpp>
# include <gusc/generator/lazy_sequence.hpp>
# include <boost/test/auto_unit_test.hpp>
# include <iostream>
# include <boost/lexical_cast.hpp>
# include <boost/tuple/tuple_comparison.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <boost/foreach.hpp>

struct geometric_generator {
    typedef double result_type;
    double curr;
    double factor;
    
    geometric_generator(double f) : curr(1.0), factor(f) {}
    operator bool() const { return true; }
    double operator()() 
    {
        double c = curr;
        curr *= factor;
        return c;
    } 
};

struct inc {
    typedef int result_type;
    int curr;
    
    inc(int f = 0) : curr(f) {}
    operator bool() const { return true; }
    int operator()() 
    {
        int c = curr;
        curr += 1;
        return c;
    } 
};

struct make_tuple_f {
    template <class X> struct result {};
    template <class A, class B>
    struct result<make_tuple_f(A,B)> {
        typedef boost::tuple<A,B> type;
    };
    
    template <class A, class B>
    boost::tuple<A,B> operator()(A const& a, B const& b) const
    {
        return boost::make_tuple(a,b);
    }
};

struct intstring {
    typedef std::string result_type;
    operator bool() const { return true; }
    intstring() : curr(-1) {}
    int curr;
    std::string operator()() { curr++; return boost::lexical_cast<std::string>(curr); }
};

struct sum_order {
typedef bool result_type;
template <class X, class Y>
bool operator()(boost::tuple<X,Y> const& x, boost::tuple<X,Y> const& y) const
{
    using boost::make_tuple;
    using boost::lexical_cast;
    using boost::get;
    int sx = lexical_cast<int>(get<0>(x)) + lexical_cast<int>(get<1>(x)); 
    int sy = lexical_cast<int>(get<0>(y)) + lexical_cast<int>(get<1>(y));
    return make_tuple(sx,lexical_cast<int>(get<0>(x)),lexical_cast<int>(get<1>(x))) >
           make_tuple(sy,lexical_cast<int>(get<0>(y)),lexical_cast<int>(get<1>(y))) ;
}
};

BOOST_AUTO_TEST_CASE(test_structured_product_heap)
{
    using boost::make_tuple;
    using namespace gusc;
    // product_heap:
    //   arg1 = inc = 0, 1, 2, 3, 4,....
    //   arg2 = intstring = "0", "1", "2", "3", "4",...
    //   product = make_tuple = n x "m" --> (n,"m")
    //   sort = sum_order = (n1,"m1") < (n2,"m2") <==> n1 + m1 < n2 + m2
    typedef product_heap_generator< 
                make_tuple_f
              , sum_order
              , lazy_sequence<inc>
              , lazy_sequence<intstring> > sum_t; 
    lazy_sequence<sum_t> 
        sum(sum_t(make_tuple_f(),sum_order(),make_lazy_sequence(inc(0)),make_lazy_sequence(intstring())));

    boost::tuple<int,std::string> compare[] = { make_tuple(0,"0")
                                              , make_tuple(0,"1") 
                                              , make_tuple(1,"0") 
                                              , make_tuple(0,"2")
                                              , make_tuple(1,"1")
                                              , make_tuple(2,"0")
                                              , make_tuple(0,"3")
                                              , make_tuple(1,"2")
                                              , make_tuple(2,"1")
                                              , make_tuple(3,"0") 
                                              , make_tuple(0,"4")
                                              , make_tuple(1,"3")
                                              , make_tuple(2,"2")
                                              , make_tuple(3,"1")
                                              , make_tuple(4,"0") };
    BOOST_CHECK_EQUAL_COLLECTIONS( compare, compare + 15
                                 , sum.begin(), sum.begin() + 15 );
    
}

BOOST_AUTO_TEST_CASE(test_structured_product_heap2)
{
    using boost::make_tuple;
    using namespace gusc;
    typedef product_heap_generator<
               make_tuple_f
             , sum_order
             , lazy_sequence<intstring>
             , lazy_sequence<inc> > sum_t; 
    lazy_sequence<sum_t> 
        sum(sum_t(make_tuple_f(), sum_order(), make_lazy_sequence(intstring()),make_lazy_sequence(inc(0))));

    boost::tuple<std::string,int> compare[] = { make_tuple("0",0)
                                              , make_tuple("0",1) 
                                              , make_tuple("1",0) 
                                              , make_tuple("0",2)
                                              , make_tuple("1",1)
                                              , make_tuple("2",0)
                                              , make_tuple("0",3)
                                              , make_tuple("1",2)
                                              , make_tuple("2",1)
                                              , make_tuple("3",0) 
                                              , make_tuple("0",4)
                                              , make_tuple("1",3)
                                              , make_tuple("2",2)
                                              , make_tuple("3",1)
                                              , make_tuple("4",0) };
    BOOST_CHECK_EQUAL_COLLECTIONS( compare, compare + 15
                                 , sum.begin(), sum.begin() + 15 );
    
}

# include <gusc/generator/single_value_generator.hpp>

BOOST_AUTO_TEST_CASE(test_single_value_product)
{
    using namespace gusc;
    typedef single_value_generator<int> si;
    si s3(3);
    si s4(4);
    typedef product_heap_generator< gusc::times, gusc::less, lazy_sequence<si>,lazy_sequence<si> > prod_t;
    lazy_sequence<prod_t> 
        prod(prod_t(gusc::times(),gusc::less(),make_lazy_sequence(s3),make_lazy_sequence(s4)));
    
    int compare[] = { 12 };
    
    BOOST_CHECK_EQUAL_COLLECTIONS( prod.begin(), prod.end(), compare, compare + 1);
}

# include <gusc/generator/any_generator.hpp>
# include <gusc/generator/generator_from_iterator.hpp>

int c0[0] = {};
int c1[3] = {3, 2, 1};
int c2[1] = {2};
int c3[2] = {4, 3};
int c12[3] = {6, 4, 2};
int c123[6] = {24, 18, 16, 12, 8, 6};
    
BOOST_AUTO_TEST_CASE(test_corners)
{
    using namespace gusc;
    
    typedef any_generator<int> gen_t;
    typedef lazy_sequence<gen_t> seq_t;
    
    // product_heap
    //    arg1: 3, 2, 1
    //    arg2: 2
    //    product = m,n -> m*n
    //    sort = m,n -> m < n
    // expected output: 6, 4, 2
    gen_t g12 = generate_product_heap(
                    gusc::times()
                  , gusc::less()
                  , boost::make_iterator_range(c1)
                  , boost::make_iterator_range(c2)
                );
    seq_t s12(g12);
    
    BOOST_CHECK_EQUAL_COLLECTIONS( c12
                                 , c12 + 3
                                 , s12.begin()
                                 , s12.end() );
}

BOOST_AUTO_TEST_CASE(test_corners2)
{
    using namespace gusc;
    
    typedef any_generator<int> gen_t;
    typedef lazy_sequence<gen_t> seq_t;
    
    // product_heap
    //     arg1: null
    //     arg2: 4, 3
    // expected output: null
    gen_t g03 = generate_product_heap(
                    gusc::times()
                  , gusc::less()
                  , boost::make_iterator_range(c0,c0)
                  , boost::make_iterator_range(c3)
                );
    seq_t s03(g03);
    
    BOOST_CHECK_EQUAL_COLLECTIONS(c0,c0,s03.begin(),s03.end());
    
}

BOOST_AUTO_TEST_CASE(test_corners3)
{
    using namespace gusc;
    
    typedef any_generator<int> gen_t;
    typedef lazy_sequence<gen_t> seq_t;
    
    gen_t g12 = generate_product_heap(
                    times()
                  , less()
                  , boost::make_iterator_range(c2)
                  , boost::make_iterator_range(c1)
                );
    seq_t s12(g12);
    
    BOOST_CHECK_EQUAL_COLLECTIONS( c12
                                 , c12 + 3
                                 , s12.begin()
                                 , s12.end() );
}

struct times3
{
    template <class F> struct result;
    template <class X> struct result<times3(X,X,X)> {
        typedef X type;
    };
    
    template <class X>
    X operator()(X const& x1, X const& x2, X const& x3) const
    {
        return x1 * x2 * x3;
    }
};

BOOST_AUTO_TEST_CASE(test_chain)
{
    using namespace gusc;
    
    typedef any_generator<int> gen_t;
    typedef lazy_sequence<gen_t> seq_t;
    
    // n-ary heap producting can be accomplished by chaining:
    // ie prod(prod(g1,g2),g3)
    //
    // of course you can think of it as prod(g1,g2,g3) provided your 
    // multiplication is associative.  

    gen_t g123 = 
        generate_product_heap(
            times3()
          , less()
          , boost::make_iterator_range(c1)
          , boost::make_iterator_range(c2)
          , boost::make_iterator_range(c3)
        )
        ;
    
    // product_heap:
    //    
    seq_t s123(g123);
    BOOST_CHECK_EQUAL_COLLECTIONS( c123
                                 , c123 + 6
                                 , s123.begin()
                                 , s123.end() );
}


BOOST_AUTO_TEST_CASE(test_lazy_inc)
{
    gusc::lazy_sequence<inc> seq;
    BOOST_CHECK_EQUAL(seq[0],*seq.begin());
    
    gusc::lazy_sequence<inc> seq2;
    BOOST_CHECK_EQUAL(*seq2.begin(),seq2[0]);
} 

struct plusN
{
    template <class F> struct result;
    template <class X> struct result<plusN(X,X,X,X)> {
        typedef X type;
    };
    template <class X> struct result<plusN(X,X,X)> {
        typedef X type;
    };
    template <class X> struct result<plusN(X,X)> {
        typedef X type;
    };
    
    template <class X>
    X operator()(X const& x1, X const& x2) const
    {
        return x1 + x2;
    }
    
    template <class X>
    X operator()(X const& x1, X const& x2, X const& x3) const
    {
        return x1 + x2 + x3;
    }
    
    template <class X>
    X operator()(X const& x1, X const& x2, X const& x3, X const& x4) const
    {
        return x1 + x2 + x3 + x4;
    }
};

std::vector<int> nary_sorted_product(int arity, int depth)
{
    std::vector<int> s(1); 
    s[0] = 0;
    for (int x = 0; x != arity; ++x) {
        std::vector<int> ss;
        for (int y = 0; y != depth; ++y) ss.push_back(y);
        std::vector<int> result;
        BOOST_FOREACH(int x, s)
        {
            BOOST_FOREACH(int y, ss)
            {
                result.push_back(x + y);
            }
        }
        s = result;
    }
    sort(s.begin(),s.end());
    return s;
}

BOOST_AUTO_TEST_CASE(test_product_heap_unlimited)
{
    using namespace gusc;
    typedef any_generator<int> gen_t;
    typedef lazy_sequence<gen_t> seq_t;
    gen_t gen =
        generate_product_heap( 
            plusN()
          , greater()
          , make_lazy_sequence(inc())
          , make_lazy_sequence(inc())
        )
        ;
    seq_t sum(gen);

    int compare[] = { 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4 };
    BOOST_CHECK_EQUAL_COLLECTIONS( 
        compare, compare + 15
      , sum.begin(), sum.begin() + 15 
      );
                                 
    std::vector<int> s = nary_sorted_product(3,50); 
    
    gen_t gen3 =
    generate_product_heap( 
        plusN()
      , greater()
      , make_lazy_sequence(inc())
      , make_lazy_sequence(inc())
      , make_lazy_sequence(inc())
    )
    ;
    
    seq_t sum3(gen3);
    BOOST_CHECK_EQUAL_COLLECTIONS( 
        s.begin(), s.begin() + 50
      , sum3.begin(), sum3.begin() + 50 
      );
      
    s = nary_sorted_product(4,20);
    
    gen_t gen4 =
    generate_product_heap( 
        plusN()
      , greater()
      , make_lazy_sequence(inc())
      , make_lazy_sequence(inc())
      , make_lazy_sequence(inc())
      , make_lazy_sequence(inc())
    )
    ;
    
    seq_t sum4(gen4);
    BOOST_CHECK_EQUAL_COLLECTIONS( 
        s.begin(), s.begin() + 20
      , sum4.begin(), sum4.begin() + 20 
      );
      
} 
