# if ! defined(GUSC__GENERATOR__PRODUCT_HEAP_GENERATOR_HPP)
# define       GUSC__GENERATOR__PRODUCT_HEAP_GENERATOR_HPP

# include <boost/utility/result_of.hpp>
# include <queue>
# include <boost/tuple/tuple.hpp>
# include <iostream>
# include <gusc/functional.hpp>
# include <functional>
# include <boost/array.hpp>
# include <boost/range.hpp>
# include <boost/mpl/for_each.hpp>
# include <boost/mpl/range_c.hpp>
# include <boost/shared_ptr.hpp>

namespace gusc {
/*   
struct comb_successors {
    template <class Pos, class Heap, class End, class Apply>
    void operator()(Pos& pos, Heap& heap, End const& at_end, Apply const& apply) 
    {
        for (size_t x = 0; x != pos.size(); ++x) {
            //position_type p = pos;
            ++(pos[x]);
            if (at_end(pos))
                heap.push(boost::make_tuple(apply(pos),pos));
            --(pos[x]);
            if (pos[x] != 0) break;
        }
    }
};

struct buchse_successors {
    template <class Pos>
    int num_succ(Pos const& pos) const
    {
        
    }
    template <class Pos, class Heap, class End, class Apply>
    void operator()(Pos& pos, Heap& heap, End const& at_end, Apply const& apply) 
    {
        for (size_t x = 0; x != pos.size(); ++x) {
            //position_type p = pos;
            ++(pos[x]);
            if (at_end(pos)) {
                typename buchse_count_map::iterator 
                    bcp = buchse_counts.find(pos);
                if (bcp == buchse_counts.end()) {
                    bcp = buchse_counts.insert(std::make_pair(pos,1)).first;
                } else {
                    ++(bcp->second);
                }
                if (bcp->second == num_succ(pos)) {
                    heap.push(boost::make_tuple(apply(pos),pos));
                }
            }
            --(pos[x]);
        }
    }  
};
*/

////////////////////////////////////////////////////////////////////////////////

template < class Result, class F, class T0, class T1, class T2, class T3
         , class T4, class T5, class T6, class T7, class T8, class T9 
         >
Result
apply_at( F const& f
        , boost::tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9>& seq
        , boost::array<int,10> const& pos
        )
{
    return f( boost::get<0>(seq)[pos[0]]
            , boost::get<1>(seq)[pos[1]]
            , boost::get<2>(seq)[pos[2]]
            , boost::get<3>(seq)[pos[3]]
            , boost::get<4>(seq)[pos[4]]
            , boost::get<5>(seq)[pos[5]]
            , boost::get<6>(seq)[pos[6]]
            , boost::get<7>(seq)[pos[7]]
            , boost::get<8>(seq)[pos[8]]
            , boost::get<9>(seq)[pos[9]]
            )
            ;
}

template < class Result, class F, class T0, class T1, class T2, class T3
         , class T4, class T5, class T6, class T7, class T8
         >
Result
apply_at( F const& f
        , boost::tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8>& seq
        , boost::array<int,9> const& pos
        )
{
    return f( boost::get<0>(seq)[pos[0]]
            , boost::get<1>(seq)[pos[1]]
            , boost::get<2>(seq)[pos[2]]
            , boost::get<3>(seq)[pos[3]]
            , boost::get<4>(seq)[pos[4]]
            , boost::get<5>(seq)[pos[5]]
            , boost::get<6>(seq)[pos[6]]
            , boost::get<7>(seq)[pos[7]]
            , boost::get<8>(seq)[pos[8]]
            )
            ;
}

template < class Result, class F, class T0, class T1, class T2, class T3
         , class T4, class T5, class T6, class T7
         >
Result
apply_at( F const& f
        , boost::tuple<T0,T1,T2,T3,T4,T5,T6,T7>& seq
        , boost::array<int,8> const& pos
        )
{
    return f( boost::get<0>(seq)[pos[0]]
            , boost::get<1>(seq)[pos[1]]
            , boost::get<2>(seq)[pos[2]]
            , boost::get<3>(seq)[pos[3]]
            , boost::get<4>(seq)[pos[4]]
            , boost::get<5>(seq)[pos[5]]
            , boost::get<6>(seq)[pos[6]]
            , boost::get<7>(seq)[pos[7]]
            )
            ;
}

template < class Result, class F, class T0, class T1, class T2, class T3
         , class T4, class T5, class T6
         >
Result
apply_at( F const& f
        , boost::tuple<T0,T1,T2,T3,T4,T5,T6>& seq
        , boost::array<int,7> const& pos
        )
{
    return f( boost::get<0>(seq)[pos[0]]
            , boost::get<1>(seq)[pos[1]]
            , boost::get<2>(seq)[pos[2]]
            , boost::get<3>(seq)[pos[3]]
            , boost::get<4>(seq)[pos[4]]
            , boost::get<5>(seq)[pos[5]]
            , boost::get<6>(seq)[pos[6]]
            )
            ;
}

template < class Result, class F, class T0, class T1, class T2, class T3
         , class T4, class T5
         >
Result
apply_at( F const& f
        , boost::tuple<T0,T1,T2,T3,T4,T5>& seq
        , boost::array<int,6> const& pos
        )
{
    return f( boost::get<0>(seq)[pos[0]]
            , boost::get<1>(seq)[pos[1]]
            , boost::get<2>(seq)[pos[2]]
            , boost::get<3>(seq)[pos[3]]
            , boost::get<4>(seq)[pos[4]]
            , boost::get<5>(seq)[pos[5]]
            )
            ;
}

template < class Result, class F, class T0, class T1, class T2, class T3
         , class T4
         >
Result
apply_at( F const& f
        , boost::tuple<T0,T1,T2,T3,T4>& seq
        , boost::array<int,5> const& pos
        )
{
    return f( boost::get<0>(seq)[pos[0]]
            , boost::get<1>(seq)[pos[1]]
            , boost::get<2>(seq)[pos[2]]
            , boost::get<3>(seq)[pos[3]]
            , boost::get<4>(seq)[pos[4]]
            )
            ;
}

template <class Result, class F, class T0, class T1, class T2, class T3>
Result
apply_at( F const& f
        , boost::tuple<T0,T1,T2,T3>& seq
        , boost::array<int,4> const& pos
        )
{
    return f( boost::get<0>(seq)[pos[0]]
            , boost::get<1>(seq)[pos[1]]
            , boost::get<2>(seq)[pos[2]]
            , boost::get<3>(seq)[pos[3]]
            )
            ;
}

template <class Result, class F, class T0, class T1, class T2>
Result
apply_at( F const& f
        , boost::tuple<T0,T1,T2>& seq
        , boost::array<int,3> const& pos
        )
{
    return f( boost::get<0>(seq)[pos[0]]
            , boost::get<1>(seq)[pos[1]]
            , boost::get<2>(seq)[pos[2]]
            )
            ;
}

template <class Result, class F, class T0, class T1>
Result
apply_at( F const& f
        , boost::tuple<T0,T1>& seq
        , boost::array<int,2> const& pos
        )
{
    return f( boost::get<0>(seq)[pos[0]]
            , boost::get<1>(seq)[pos[1]]
            )
            ;
}

template <class F>
struct product_heap_result_of;

template <class F, class T0, class T1>
struct product_heap_result_of<F(boost::tuple<T0,T1>)> {
    typedef 
        typename boost::result_of<
            F( typename T0::value_type
             , typename T1::value_type
             )
        >::type type;
};

template <class F, class T0, class T1, class T2>
struct product_heap_result_of<F(boost::tuple<T0,T1,T2>)> {
    typedef 
        typename boost::result_of<
            F( typename T0::value_type
             , typename T1::value_type
             , typename T2::value_type
             )
        >::type type;
};

template <class F, class T0, class T1, class T2, class T3>
struct product_heap_result_of<F(boost::tuple<T0,T1,T2,T3>)> {
    typedef 
        typename boost::result_of<
            F( typename T0::value_type
             , typename T1::value_type
             , typename T2::value_type
             , typename T3::value_type
             )
        >::type type;
};

template <class F, class T0, class T1, class T2, class T3, class T4>
struct product_heap_result_of<F(boost::tuple<T0,T1,T2,T3,T4>)> {
    typedef 
        typename boost::result_of<
            F( typename T0::value_type
             , typename T1::value_type
             , typename T2::value_type
             , typename T3::value_type
             , typename T4::value_type
             )
        >::type type;
};

template <class F, class T0, class T1, class T2, class T3, class T4, class T5>
struct product_heap_result_of<F(boost::tuple<T0,T1,T2,T3,T4,T5>)> {
    typedef 
        typename boost::result_of<
            F( typename T0::value_type
             , typename T1::value_type
             , typename T2::value_type
             , typename T3::value_type
             , typename T4::value_type
             , typename T5::value_type
             )
        >::type type;
};

template < class F, class T0, class T1, class T2, class T3, class T4, class T5
         , class T6
         >
struct product_heap_result_of<F(boost::tuple<T0,T1,T2,T3,T4,T5,T6>)> {
    typedef 
        typename boost::result_of<
            F( typename T0::value_type
             , typename T1::value_type
             , typename T2::value_type
             , typename T3::value_type
             , typename T4::value_type
             , typename T5::value_type
             , typename T6::value_type
             )
        >::type type;
};

template < class F, class T0, class T1, class T2, class T3, class T4, class T5
         , class T6, class T7
         >
struct product_heap_result_of<F(boost::tuple<T0,T1,T2,T3,T4,T5,T6,T7>)> {
    typedef 
        typename boost::result_of<
            F( typename T0::value_type
             , typename T1::value_type
             , typename T2::value_type
             , typename T3::value_type
             , typename T4::value_type
             , typename T5::value_type
             , typename T6::value_type
             , typename T7::value_type
             )
        >::type type;
};

template < class F, class T0, class T1, class T2, class T3, class T4, class T5
         , class T6, class T7, class T8
         >
struct product_heap_result_of<F(boost::tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8>)> {
    typedef 
        typename boost::result_of<
            F( typename T0::value_type
             , typename T1::value_type
             , typename T2::value_type
             , typename T3::value_type
             , typename T4::value_type
             , typename T5::value_type
             , typename T6::value_type
             , typename T7::value_type
             , typename T8::value_type
             )
        >::type type;
};

template < class F, class T0, class T1, class T2, class T3, class T4, class T5
         , class T6, class T7, class T8, class T9
         >
struct product_heap_result_of<F(boost::tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9>)> {
    typedef 
        typename boost::result_of<
            F( typename T0::value_type
             , typename T1::value_type
             , typename T2::value_type
             , typename T3::value_type
             , typename T4::value_type
             , typename T5::value_type
             , typename T6::value_type
             , typename T7::value_type
             , typename T8::value_type
             , typename T9::value_type 
             )
        >::type type;
};

////////////////////////////////////////////////////////////////////////////////

template < class Product
         , class Compare
         , class S0
         , class S1
         , class S2 = boost::tuples::null_type
         , class S3 = boost::tuples::null_type
         , class S4 = boost::tuples::null_type
         , class S5 = boost::tuples::null_type
         , class S6 = boost::tuples::null_type
         , class S7 = boost::tuples::null_type
         , class S8 = boost::tuples::null_type
         , class S9 = boost::tuples::null_type
         >
class product_heap_generator 
 : public boost::iterator_facade<
     product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8,S9>
   , typename product_heap_result_of<
         Product(boost::tuple<S0,S1,S2,S3,S4,S5,S6,S7,S8,S9>)
     >::type const
   , std::input_iterator_tag
   >
{
    typedef boost::tuple<S0,S1,S2,S3,S4,S5,S6,S7,S8,S9> Sequences;
public:
    product_heap_generator() {}
    product_heap_generator( Product const& product
                          , Compare const& compare
                          , S0 const& s0
                          , S1 const& s1
                          , S2 const& s2 = boost::tuples::null_type()
                          , S3 const& s3 = boost::tuples::null_type()
                          , S4 const& s4 = boost::tuples::null_type()
                          , S5 const& s5 = boost::tuples::null_type()
                          , S6 const& s6 = boost::tuples::null_type()
                          , S7 const& s7 = boost::tuples::null_type()
                          , S8 const& s8 = boost::tuples::null_type()
                          , S9 const& s9 = boost::tuples::null_type()
                          ) 
      : impl(new impl_(product,compare,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9)) {}
    
    typedef 
        typename product_heap_result_of<Product(Sequences)>::type 
        result_type;
    
    result_type operator()()
    {
        result_type res = dereference();
        increment();
        return res;
    }
    operator bool() const { return bool(*impl); }
private:
    
    bool equal(product_heap_generator const& other) const
    {
        return impl->equal(*(other.impl));
    }
    
    result_type const& dereference() const { return impl->dereference(); }
    
    void increment() { impl->increment(); }
    
    friend class boost::iterator_core_access;

    typedef 
        boost::array<int,boost::tuples::length<Sequences>::value>
        position_type;
    
    typedef Sequences sequences_type;
    
    struct not_at_end {
        not_at_end(sequences_type* seq, position_type const* pos, bool* result)
            : seq(seq), pos(pos), i(i), result(result) { *result = true; }
        sequences_type* seq;
        position_type const* pos;
        int i;
        bool* result;
        template <typename I>
        void operator()(I i)
        {
            if (*result) {
                *result = 
                    (boost::begin(boost::get<I::value>(*seq)) + (*pos)[I::value] 
                    != 
                    boost::end(boost::get<I::value>(*seq)));
            }
        }
    };
    
    struct sorter {
        Compare sort;
        template <class Tuple>
        bool operator()(Tuple const& t1, Tuple const& t2) const
        {
            return sort(boost::get<0>(t1),boost::get<0>(t2));
        }
        sorter(Compare const& sort) : sort(sort) {}
    };
    
    typedef boost::tuple<result_type,position_type> elem_type;
    
    struct impl_ {
        Product product;
        sequences_type sequences;
        std::priority_queue<elem_type,std::vector<elem_type>,sorter> heap;
        
        impl_( Product const& product, Compare const& compare
             , S0 const& s0, S1 const& s1, S2 const& s2, S3 const& s3
             , S4 const& s4, S5 const& s5, S6 const& s6, S7 const& s7
             , S8 const& s8, S9 const& s9 
             ) 
          : product(product)
          , sequences(s0,s1,s2,s3,s4,s5,s6,s7,s8,s9)
          , heap(sorter(compare)) { init(); }
        
        void increment()
        {
            position_type pos = boost::get<1>(heap.top());
            heap.pop();
            for (size_t x = 0; x != pos.size(); ++x) {
                //position_type p = pos;
                ++(pos[x]);
                if (all_not_at_end(pos))
                    heap.push(boost::make_tuple(apply_at_(pos),pos));
                --(pos[x]);
                if (pos[x] != 0) break;
            }
        }
        
        void init()
        {
            position_type pos;
            pos.assign(0);
            if (all_not_at_end(pos)) {
                heap.push(boost::make_tuple(apply_at_(pos),pos));
            }
        }
        
        bool all_not_at_end(position_type const& pos)
        {
            bool result;
            not_at_end nae(&sequences,&pos,&result);
            boost::mpl::for_each< 
                boost::mpl::range_c<int,0,boost::tuples::length<Sequences>::value> 
            >(nae);
            return result;
        }
        
        result_type apply_at_(position_type const& pos)
        {
            return apply_at<result_type>(product,sequences,pos);
        }
        
        result_type const& dereference() const
        {
            return boost::get<0>(heap.top());
        }

        // undefined to compare to not-end.
        bool equal(impl_ const& other) const
        {
            return (not bool(*this)) and (not bool(other));
        }
        
        operator bool() const { return not heap.empty(); }
    };
    
    boost::shared_ptr<impl_> impl;
};

////////////////////////////////////////////////////////////////////////////////

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4, class S5, class S6, class S7, class S8, class S9
         >
product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8,S9> 
generate_product_heap( Product const& product
                     , Compare const& compare
                     , S0 const& s0
                     , S1 const& s1
                     , S2 const& s2
                     , S3 const& s3
                     , S4 const& s4
                     , S5 const& s5
                     , S6 const& s6
                     , S7 const& s7
                     , S8 const& s8
                     , S9 const& s9
                     )
{
    return 
        product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8,S9>
            (product,compare,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4, class S5, class S6, class S7, class S8
         >
product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8> 
generate_product_heap( Product const& product
                     , Compare const& compare
                     , S0 const& s0
                     , S1 const& s1
                     , S2 const& s2
                     , S3 const& s3
                     , S4 const& s4
                     , S5 const& s5
                     , S6 const& s6
                     , S7 const& s7
                     , S8 const& s8
                     )
{
    return 
        product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8>
            (product,compare,s0,s1,s2,s3,s4,s5,s6,s7,s8);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4, class S5, class S6, class S7
         >
product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7> 
generate_product_heap( Product const& product
                     , Compare const& compare
                     , S0 const& s0
                     , S1 const& s1
                     , S2 const& s2
                     , S3 const& s3
                     , S4 const& s4
                     , S5 const& s5
                     , S6 const& s6
                     , S7 const& s7
                     )
{
    return 
        product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7>
            (product,compare,s0,s1,s2,s3,s4,s5,s6,s7);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4, class S5, class S6
         >
product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6> 
generate_product_heap( Product const& product
                     , Compare const& compare
                     , S0 const& s0
                     , S1 const& s1
                     , S2 const& s2
                     , S3 const& s3
                     , S4 const& s4
                     , S5 const& s5
                     , S6 const& s6
                     )
{
    return 
        product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6>
            (product,compare,s0,s1,s2,s3,s4,s5,s6);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4, class S5
         >
product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5> 
generate_product_heap( Product const& product
                     , Compare const& compare
                     , S0 const& s0
                     , S1 const& s1
                     , S2 const& s2
                     , S3 const& s3
                     , S4 const& s4
                     , S5 const& s5
                     )
{
    return 
        product_heap_generator<Product,Compare,S0,S1,S2,S3,S4,S5>
            (product,compare,s0,s1,s2,s3,s4,s5);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4
         >
product_heap_generator<Product,Compare,S0,S1,S2,S3,S4> 
generate_product_heap( Product const& product
                     , Compare const& compare
                     , S0 const& s0
                     , S1 const& s1
                     , S2 const& s2
                     , S3 const& s3
                     , S4 const& s4
                     )
{
    return 
        product_heap_generator<Product,Compare,S0,S1,S2,S3,S4>
            (product,compare,s0,s1,s2,s3,s4);
}

template <class Product, class Compare, class S0, class S1, class S2, class S3>
product_heap_generator<Product,Compare,S0,S1,S2,S3> 
generate_product_heap( Product const& product
                     , Compare const& compare
                     , S0 const& s0
                     , S1 const& s1
                     , S2 const& s2
                     , S3 const& s3
                     )
{
    return 
        product_heap_generator<Product,Compare,S0,S1,S2,S3>
            (product,compare,s0,s1,s2,s3);
}

template <class Product, class Compare, class S0, class S1, class S2>
product_heap_generator<Product,Compare,S0,S1,S2> 
generate_product_heap( Product const& product
                     , Compare const& compare
                     , S0 const& s0
                     , S1 const& s1
                     , S2 const& s2
                     )
{
    return 
        product_heap_generator<Product,Compare,S0,S1,S2>
            (product,compare,s0,s1,s2);
}

template <class Product, class Compare, class S0, class S1>
product_heap_generator<Product,Compare,S0,S1> 
generate_product_heap( Product const& product
                     , Compare const& compare
                     , S0 const& s0
                     , S1 const& s1
                     )
{
    return 
        product_heap_generator<Product,Compare,S0,S1>
            (product,compare,s0,s1);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc
# endif //     GUSC__GENERATOR__PRODUCT_HEAP_GENERATOR_HPP
