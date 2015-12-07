# if ! defined(GUSC__GENERATOR__BUCHSE_PRODUCT_GENERATOR_HPP)
# define       GUSC__GENERATOR__BUCHSE_PRODUCT_GENERATOR_HPP

# include <boost/utility/result_of.hpp>
# include <queue>
# include <boost/tuple/tuple.hpp>
# include <iostream>
# include <gusc/functional.hpp>
# include <functional>
# include <boost/tr1/unordered_map.hpp>
# include <boost/array.hpp>
# include <boost/range.hpp>
# include <boost/mpl/for_each.hpp>
# include <boost/mpl/range_c.hpp>
# include <gusc/generator/product_heap_generator.hpp>
# include <gusc/hash/boost_array.hpp>
# include <map>

namespace gusc {

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
class buchse_product_generator 
 : public boost::iterator_facade<
     buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8,S9>
   , typename product_heap_result_of<
         Product(boost::tuple<S0,S1,S2,S3,S4,S5,S6,S7,S8,S9>)
     >::type const
   , std::input_iterator_tag
   >
{
    typedef boost::tuple<S0,S1,S2,S3,S4,S5,S6,S7,S8,S9> Sequences;
public:
    buchse_product_generator() {}
    buchse_product_generator( Product const& product
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
    
    bool equal(buchse_product_generator const& other) const
    {
        return impl->equal(*(other.impl));
    }
    
    result_type const& dereference() const { return impl->dereference(); }
    
    void increment() { return impl->increment(); }
    
    friend class boost::iterator_core_access;

    typedef 
        boost::array<int,boost::tuples::length<Sequences>::value>
        position_type;
    
    typedef Sequences sequences_type;
    
    struct not_at_end {
        not_at_end(sequences_type* seq, position_type const* pos, bool* result)
            : seq(seq), pos(pos), result(result) { *result = true; }
        sequences_type* seq;
        position_type const* pos;
        bool* result;
        template <typename I>
        void operator()(I ignore)
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
        //typedef std::map<position_type,size_t> buchse_count_map;
        typedef std::tr1::unordered_map<position_type,size_t,boost::hash<position_type> > buchse_count_map;
        buchse_count_map buchse_counts;
        
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
                if (all_not_at_end(pos)) {
                    typename buchse_count_map::iterator 
                        bcp = buchse_counts.find(pos);
                    if (bcp == buchse_counts.end()) {
                        bcp = buchse_counts.insert(std::make_pair(pos,1)).first;
                    } else {
                        ++(bcp->second);
                    }
                    if (bcp->second == num_succ(pos)) {
                        heap.push(boost::make_tuple(apply_at_(pos),pos));
                    }
                }
                --(pos[x]);
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
        
        size_t num_succ(position_type const& pos) const
        {
            size_t ret = 0;
            for (size_t x = 0; x != size_t(boost::tuples::length<Sequences>::value); ++x) {
                if (pos[x] != 0) ++ret;
            }
            return ret;
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
buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8,S9> 
generate_buchse_product( Product const& product
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
        buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8,S9>
            (product,compare,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4, class S5, class S6, class S7, class S8
         >
buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8> 
generate_buchse_product( Product const& product
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
        buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7,S8>
            (product,compare,s0,s1,s2,s3,s4,s5,s6,s7,s8);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4, class S5, class S6, class S7
         >
buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7> 
generate_buchse_product( Product const& product
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
        buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6,S7>
            (product,compare,s0,s1,s2,s3,s4,s5,s6,s7);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4, class S5, class S6
         >
buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6> 
generate_buchse_product( Product const& product
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
        buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5,S6>
            (product,compare,s0,s1,s2,s3,s4,s5,s6);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4, class S5
         >
buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5> 
generate_buchse_product( Product const& product
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
        buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4,S5>
            (product,compare,s0,s1,s2,s3,s4,s5);
}

template < class Product, class Compare, class S0, class S1, class S2, class S3
         , class S4
         >
buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4> 
generate_buchse_product( Product const& product
                       , Compare const& compare
                       , S0 const& s0
                       , S1 const& s1
                       , S2 const& s2
                       , S3 const& s3
                       , S4 const& s4
                       )
{
    return 
        buchse_product_generator<Product,Compare,S0,S1,S2,S3,S4>
            (product,compare,s0,s1,s2,s3,s4);
}

template <class Product, class Compare, class S0, class S1, class S2, class S3>
buchse_product_generator<Product,Compare,S0,S1,S2,S3> 
generate_buchse_product( Product const& product
                       , Compare const& compare
                       , S0 const& s0
                       , S1 const& s1
                       , S2 const& s2
                       , S3 const& s3
                       )
{
    return 
        buchse_product_generator<Product,Compare,S0,S1,S2,S3>
            (product,compare,s0,s1,s2,s3);
}

template <class Product, class Compare, class S0, class S1, class S2>
buchse_product_generator<Product,Compare,S0,S1,S2> 
generate_buchse_product( Product const& product
                       , Compare const& compare
                       , S0 const& s0
                       , S1 const& s1
                       , S2 const& s2
                       )
{
    return 
        buchse_product_generator<Product,Compare,S0,S1,S2>
            (product,compare,s0,s1,s2);
}

template <class Product, class Compare, class S0, class S1>
buchse_product_generator<Product,Compare,S0,S1> 
generate_buchse_product( Product const& product
                       , Compare const& compare
                       , S0 const& s0
                       , S1 const& s1
                       )
{
    return 
        buchse_product_generator<Product,Compare,S0,S1>
            (product,compare,s0,s1);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc
# endif //     GUSC__GENERATOR__BUCHSE_PRODUCT_GENERATOR_HPP
