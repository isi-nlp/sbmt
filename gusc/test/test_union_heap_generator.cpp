# include <gusc/generator/union_heap_generator.hpp>
# include <gusc/generator/any_generator.hpp>
# include <boost/test/auto_unit_test.hpp>
# include <gusc/iterator/iterator_from_generator.hpp>
# include <gusc/functional.hpp>
# include <gusc/generator/lazy_sequence.hpp>
# include <gusc/generator/generator_from_iterator.hpp>
# include <boost/tuple/tuple.hpp>

struct mod_k_generator {
    typedef int result_type;
    
    mod_k_generator(int m, int k) : m(m), k(k), i(0) {}
    
    operator bool() const { return true; }
    
    int operator()() 
    {
        return m * (i++) + k;
    }
private:
    int m, k, i;
};

struct mod_generator {
    typedef gusc::iterator_from_generator<mod_k_generator> result_type;
    mod_generator(int m) : m(m), k(0) {}
    operator bool() const { return m != k; }
    result_type operator()() {
        return boost::begin(gusc::range_from_generator(mod_k_generator(m,k++)));
    }
private:
    int m, k;
};

struct mod_loop_generator {
    typedef gusc::iterator_from_generator<mod_k_generator> result_type;
    mod_loop_generator(int m) : m(m), k(0) {}
    operator bool() const { return true; }
    result_type operator()() {
        int kk = k++;
        if (k == m) k = 0;
        return boost::begin(gusc::range_from_generator(mod_k_generator(m,kk)));
    }
private:
    int m, k;
};

struct inc {
    typedef int result_type;
    explicit inc(int i) : i(i) {}
    
    operator bool() const { return true; }
    int operator()() {
        return i++;
    }
private:
    int i;
};

struct loop_inc {
    typedef gusc::iterator_from_generator<inc> result_type;
    loop_inc(int i) : i(i) {}
    operator bool() const { return true; }
    result_type operator()() const { 
        return boost::begin(gusc::range_from_generator(inc(i))); 
    }
private:
    int i;
};

struct const_gen {
    typedef int result_type;
    const_gen(int c) : c(c) {}
    operator bool() const { return true; }
    result_type operator()() const { return c; }
private:
    int c;
};

BOOST_AUTO_TEST_CASE(test_union_heap)
{
    using gusc::range_from_generator;
    using gusc::greater;
    using gusc::lazy_sequence;
    using gusc::generate_union_heap;
    using gusc::any_generator;
    
    any_generator<int> 
        u = generate_union_heap(boost::begin(range_from_generator(mod_generator(5))),greater(),2);
    
    inc i(0);
    
    lazy_sequence< any_generator<int> > lzu(u);
    lazy_sequence<inc> lzi(i);
    
    BOOST_CHECK_EQUAL_COLLECTIONS(
        lzu.begin(), lzu.begin() + 100,
        lzi.begin(), lzi.begin() + 100
    )
    ;
}

BOOST_AUTO_TEST_CASE(inf_union_of_same_is_constant)
{
    using namespace gusc;
    
    any_generator<int> 
        u = generate_union_heap(boost::begin(range_from_generator(loop_inc(2))),greater(),4);
    lazy_sequence< any_generator<int> > lzu(u);
    
    const_gen c(2);
    lazy_sequence<const_gen> lzc(c);
    
    BOOST_CHECK_EQUAL_COLLECTIONS(
        lzu.begin(), lzu.begin() + 100, 
        lzc.begin(), lzc.begin() + 100
    );
}


BOOST_AUTO_TEST_CASE(union_heap_with_generator_from_iterator)
{
    using namespace gusc;
    int m0[] = {0, 3, 6, 9, 12, 15};
    int m1[] = {1, 4, 7, 10, 13};
    int m2[] = {2, 5, 8, 11, 14};
    
    generator_from_iterator<int*> g0(m0,m0 + 6), g1(m1,m1 + 5), g2(m2,m2 + 5);
    generator_from_iterator<int*>
        coll[] = { g0, g1, g2 };
        
    generator_from_iterator<generator_from_iterator<int*>*> g(coll, coll + 3);
    any_generator<int>
        u = generate_union_heap(g,greater());
        
    int m[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    lazy_sequence< any_generator<int> > lzu(u);
    
    BOOST_CHECK_EQUAL_COLLECTIONS(lzu.begin(),lzu.end(),m,m+16);
}

BOOST_AUTO_TEST_CASE(union_heap_with_generator_from_iterator2)
{
    using namespace gusc;
    int m0[] = {0, 3, 6, 9, 12, 15};
    int m1[] = {1, 4, 7, 10, 13};
    int m2[] = {2, 5, 8, 11, 14};
    
    generator_from_iterator<int*> g0(m0,m0 + 6), g1(m1,m1 + 5), g2(m2,m2 + 5);
    generator_from_iterator<int*>
        coll[] = { g0, g1, g2 };
        
    generator_from_iterator<generator_from_iterator<int*>*> g(coll, coll + 3);
    any_generator<int>
        u = generate_union_heap(g,greater());
        
    int m[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    iterator_from_generator< any_generator<int> > gitr, gend;
    
    boost::tie(gitr,gend) = range_from_generator(u);
    
    BOOST_CHECK_EQUAL_COLLECTIONS(gitr,gend,m,m+16);
}

# include <gusc/generator/finite_union_generator.hpp>

BOOST_AUTO_TEST_CASE(test_finite_union)
{
    using namespace gusc;
    int m0[] = {0, 3, 6, 9, 12, 15};
    int m1[] = {1, 4, 7, 10, 13};
    int m2[] = {2, 5, 8, 11, 14};
    
    generator_from_iterator<int*> g0(m0,m0 + 6), g1(m1,m1 + 5), g2(m2,m2 + 5);
    generator_from_iterator<int*>
        coll[] = { g0, g1, g2 };
        
    any_generator<int>
        u = generate_finite_union(coll,greater());
        
    int m[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    iterator_from_generator< any_generator<int> > gitr, gend;
    
    boost::tie(gitr,gend) = range_from_generator(u);
    
    BOOST_CHECK_EQUAL_COLLECTIONS(gitr,gend,m,m+16);
}
