#include <sbmt/search/cube.hpp>
#include <sbmt/edge/null_info.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>


//#include <boost/test/test_case_template.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/mpl/vector_c.hpp>

#include <iostream>

#include <sbmt/search/sorted_cube.hpp>
#include <gusc/generator/product_heap_generator.hpp>
#include <gusc/generator/any_generator.hpp>
#include <gusc/functional.hpp>

struct sum_threshold {
    sum_threshold(int threshold, std::vector<int>& out) 
    : threshold(threshold)
    , out(out) {}
    
    bool operator()(int x, int y, int z) const
    {
        bool r = x + y + z > threshold;
        if (r) {
            out.push_back(x + y + z);
        }
        return r;
    }
private:
    int threshold;
    std::vector<int>& out;
};

struct sum_threshold_print {
    sum_threshold_print(int threshold, std::vector<int>& out) 
    : threshold(threshold)
    , out(out) {}
    
    bool operator()(int x, int y, int z) const
    {
        bool r = x + y + z > threshold;
        if (r) {
            out.push_back(x + y + z);
        }
        return r;
    }
private:
    int threshold;
    std::vector<int>& out;
};

struct heap_sum_threshold
{
    typedef boost::tuple<int,int,int> tuple_type;
    bool operator()(tuple_type const& x, tuple_type const& y) const
    {
        using boost::tuples::get;
        return get<0>(x) + get<1>(x) + get<2>(x) <
               get<0>(y) + get<1>(y) + get<2>(y) ;
    }
};

void dumb_cube( std::vector<int>& ra, std::vector<int>& rb, std::vector<int>& rc
              , sum_threshold thresh )
{
    for (unsigned x = 0; x != ra.size(); ++x)
         for (unsigned y = 0; y != rb.size(); ++y)
             for (unsigned z = 0; z != rc.size(); ++z)
                thresh(ra[x],rb[y],rc[z]);
}
// ,25,23,22,20,18,12,10,8,6,4,3,2
typedef boost::mpl::vector_c<int,29> test_args;

struct firstless {
    template <class A, class B>
    bool operator()(std::pair<A,B> const& p1, std::pair<A,B> const& p2) const
    {
        return p1.first < p2.first;
    }
};

struct adder {
    typedef int result_type;
    int operator()(int x, int y, int z) const { return x + y + z; }
};

template <class A, class B, class C>
std::vector<int> 
sorted_cube_result(A const& ai, B const& bi, C const& ci)
{
    typedef sbmt::sorted_cube<int const*,int const*,int const*> cube_t;
    typedef std::pair<int,cube_t::item> pair_t;
    cube_t sc(ai,bi,ci);
    std::vector<int> ret;
    std::priority_queue<pair_t, std::vector<pair_t>, firstless> pq;
    int corner = sc.corner().x() + sc.corner().y() + sc.corner().z();
    pq.push(std::make_pair(corner,sc.corner()));
    while (!pq.empty()) {
        pair_t p = pq.top();
        pq.pop();
        ret.push_back(p.first);
        for (size_t x = 1; x <= 3; ++x) {
            if (p.second.has_successor(x)) {
                cube_t::item s = p.second.successor(x);
                int c = s.x() + s.y() + s.z();
                pq.push(std::make_pair(c,s));
            }
        }
    }
    return ret;
}

template <class A, class B, class C>
std::vector<int> 
product_heap_result(A const& ai, B const& bi, C const& ci)
{
    gusc::any_generator<int> 
        gen = gusc::generate_product_heap( adder()
                                         , gusc::less()
                                         , gusc::shared_varray<int>(ai)
                                         , gusc::shared_varray<int>(bi)
                                         , gusc::shared_varray<int>(ci)
                                         );
    std::vector<int> ret;
    while (gen) { ret.push_back(gen()); }
    return ret;
}

BOOST_AUTO_TEST_CASE(compare_product_heap_sorted_cube)
{
    int a[10] = { 3, 11, 9, 6, 4, 8, 5, 3, 1, 1 };
    int b[10] = { 15, 14, 10, 6, 9, 10, 4, 4, 7, 2 };
    int c[10] = { 20, 19, 18, 17, 5, 4, 10, 9, 8, 20 };
    
    std::vector<int> sc = sorted_cube_result(a,b,c);
    std::vector<int> ph = product_heap_result(a,b,c);
    
    BOOST_CHECK_EQUAL_COLLECTIONS(sc.begin(),sc.end(),ph.begin(),ph.end());
    BOOST_FOREACH(int x, sc)
    {
        std::cout << x << " ";
    }
    std::cout << std::endl;
}

/*
BOOST_AUTO_TEST_CASE_TEMPLATE(test_cube,test_arg,test_args)
{
    using namespace std;
    using namespace sbmt::detail::cube;

    int a[8] = {10, 9, 8, 7, 4, 3, 2, 1};
    int b[4] = {8, 4, 2, 1};
    int c[7] = {15, 12, 9, 8, 2, 1, 1};
    
    vector<int> ra(a, a+8);
    vector<int> rb(b, b+4);
    vector<int> rc(c, c+7);
    
    vector<int> out;
    sum_threshold s(test_arg::value, out);
    
    vector<int> outh;
    sum_threshold_print sh(test_arg::value, outh);
    
    cube_loop<int,int,int,sum_threshold> cu(s);
    cube_heap<int,int,int,sum_threshold_print,heap_sum_threshold> ch(sh);
    
    cu(ra, rb, rc);
    ch(ra, rb, rc);
    
    vector<int> out2;
    sum_threshold s2(test_arg::value, out2);
    dumb_cube(ra,rb,rc,s2);
    
    sort(out.begin(), out.end());
    sort(out2.begin(),out2.end());
    sort(outh.begin(),outh.end());
    
    BOOST_CHECK_EQUAL_COLLECTIONS( out.begin(),  out.end()
                                 , out2.begin(), out2.end() );
                                 
    BOOST_CHECK_EQUAL_COLLECTIONS( outh.begin(), outh.end()
                                 , out2.begin(), out2.end() );
                                 

}
*/