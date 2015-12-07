/*

#include <sbmt/hash/oa_hashtable.hpp>

#include <boost/test/test_case_template.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/random.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <set>
#include <functional>
#include <string>

using namespace boost;
using namespace boost::multi_index;
using namespace sbmt;

template <class T>
class random_item {};

template <>
class random_item<int>
{
public:
    int operator()() 
    {
        return r();
    }
    random_item()
    : gen(time(0))
    , r(gen,uniform_int<>(0,1000)){}
public:  
    mt19937 gen;
    variate_generator<mt19937&, uniform_int<> > r;
};

template <>
class random_item<std::string>
{
public:
    std::string operator()()
    {
        str[0] = r();
        str[1] = r();
        return std::string(str);
    }
    
    random_item()
    : gen(time(0))
    , r(gen,uniform_int<>(65,89)) { str[2] = 0; }
private:
    mt19937 gen;
    variate_generator<mt19937&, uniform_int<> > r;
    char str[3];
};

template <>
class random_item< tuple<int,int> >
{
public:
    tuple<int,int> operator()() 
    {
        return make_tuple(r(),r());
    }
    random_item()
    : gen(time(0)), r(gen,uniform_int<>(0,500)){}
public:  
    mt19937 gen;
    variate_generator<mt19937&, uniform_int<> > r;
};

struct first_of_tuple
{
    typedef int result_type;
    int operator()(tuple<int,int> const& p) const { return p.get<0>(); }
};

////////////////////////////////////////////////////////////////////////////////

template <class ValueT, class KeyFromValF, class KeyHashF, class KeyEqF>
struct test_oa_hash_params 
{
    typedef ValueT                            value_type;
    typedef typename KeyFromValF::result_type key_type;
    typedef KeyFromValF                       key_extractor;
    typedef KeyHashF                          hasher;
    typedef KeyEqF                            key_equal;
};

////////////////////////////////////////////////////////////////////////////////

typedef boost::mpl::vector<
    test_oa_hash_params<
        int
      , identity<int>
      , boost::hash<int>
      , std::equal_to<int>
    >
  , test_oa_hash_params<
        tuple<int,int>
      , first_of_tuple
      , boost::hash<int>
      , std::equal_to<int>
    >
  , test_oa_hash_params<
        std::string
      , identity<std::string>
      , boost::hash<std::string>
      , std::equal_to<std::string>
    >
> test_args;

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE_TEMPLATE(test_oa_hashtable, test_arg, test_args)
{
    typedef typename test_arg::value_type    value_type;
    typedef typename test_arg::key_extractor key_extractor;
    typedef typename test_arg::hasher        hasher;
    typedef typename test_arg::key_equal     key_equal;
    
    typedef oa_hashtable<value_type,key_extractor,hasher,key_equal> 
            test_hash_table_type;

    typedef multi_index_container< 
                value_type
              , indexed_by< hashed_unique<key_extractor, hasher, key_equal> >
            > ref_hash_table_type;
//    typedef ref_hash_table_type test_hash_table_type;
            
    random_item<value_type> X;
    test_hash_table_type test_hash_table(10,0.5);
    ref_hash_table_type  ref_hash_table;
    
    key_extractor extract_key;
    size_t num_yes_insert(0), 
           num_no_insert(0), 
           num_yes_erase(0), 
           num_no_erase(0); 
    
    for (int i = 0; i != 25000; ++i) {
        value_type v = X();
        std::stringstream str;
        if (i % 2 == 0) {
            str << "unexpected behaviour inserting '"<< v << "'"<< std::endl;
            bool ti = test_hash_table.insert(v).second;
            bool ri = ref_hash_table.insert(v).second;
            BOOST_CHECK_MESSAGE(
                ti == ri
              , str.str()
            );
            if (ti) ++num_yes_insert;
            else ++num_no_insert;
        } else {
            str << "unexpected behaviour removing '" << v << "'"<< std::endl;
            bool tr = test_hash_table.erase(extract_key(v));
            bool rr = ref_hash_table.erase(extract_key(v));
            BOOST_CHECK_MESSAGE(
                tr == rr
              , str.str()
            );
            if (tr) ++num_yes_erase;
            else ++num_no_erase;
        }
        BOOST_CHECK_EQUAL(test_hash_table.size(), ref_hash_table.size());
        if (i % 10 == 0) {
            typedef std::set<value_type> value_set;
            value_set test_values(test_hash_table.begin(), test_hash_table.end());
            value_set ref_values(ref_hash_table.begin(), ref_hash_table.end());

            BOOST_CHECK_EQUAL_COLLECTIONS( test_values.begin(), test_values.end()
                                         , ref_values.begin(),  ref_values.end() );
        }
    }
    
//    std::clog << "successful erase: "<< num_yes_erase << std::endl;
//    std::clog << "failed erase: " << num_no_erase << std::endl;
//    std::clog << "successful insert: "<< num_yes_insert << std::endl;
//    std::clog << "failed insert: " << num_no_insert << std::endl;
    
    typedef std::set<value_type> value_set;
    value_set test_values(test_hash_table.begin(), test_hash_table.end());
    value_set ref_values(ref_hash_table.begin(), ref_hash_table.end());
    
    BOOST_CHECK_EQUAL_COLLECTIONS( test_values.begin(), test_values.end()
                                 , ref_values.begin(),  ref_values.end() );
                                 
    BOOST_CHECK(test_hash_table.empty() == false);
    test_hash_table.clear();
    BOOST_CHECK(test_hash_table.empty() == true);
    
    typename ref_hash_table_type::iterator itr = ref_hash_table.begin();
    typename ref_hash_table_type::iterator end = ref_hash_table.end();
    for (; itr != end; ++itr) {
        BOOST_CHECK(test_hash_table.insert(*itr).second == true);
        BOOST_CHECK(test_hash_table.insert(*itr).second == false);
        typename test_hash_table_type::iterator pos = 
            test_hash_table.find(extract_key(*itr));
        BOOST_CHECK(pos != test_hash_table.end());
        BOOST_CHECK(*pos == *itr);
        test_hash_table.erase(pos);
        BOOST_CHECK(test_hash_table.insert(*itr).second == true);
    }
    test_values.clear();
    test_values.insert(test_hash_table.begin(),test_hash_table.end());
    BOOST_CHECK_EQUAL_COLLECTIONS( test_values.begin(), test_values.end()
                                 , ref_values.begin(),  ref_values.end() );
                                 
    test_hash_table_type new_test_hash_table(test_hash_table);
    
    test_values.clear();
    test_values.insert(new_test_hash_table.begin(),new_test_hash_table.end());
    BOOST_CHECK_EQUAL_COLLECTIONS( test_values.begin(), test_values.end()
                                 , ref_values.begin(),  ref_values.end() );
                                 
    new_test_hash_table = test_hash_table;

    test_values.clear();
    test_values.insert(new_test_hash_table.begin(),new_test_hash_table.end());
    BOOST_CHECK_EQUAL_COLLECTIONS( test_values.begin(), test_values.end()
                              , ref_values.begin(),  ref_values.end() );
                              
    test_hash_table_type another_table;
    another_table.swap(test_hash_table);
    
    test_values.clear();
    test_values.insert(another_table.begin(),another_table.end());
    BOOST_CHECK_EQUAL_COLLECTIONS( test_values.begin(), test_values.end()
                                 , ref_values.begin(),  ref_values.end() );
                                 
//    std::cout << "load_factor: "<<test_hash_table.load_factor();
}
                                                             
template <class K,class V,class KeyHashF=boost::hash<K>, class KeyEqF=std::equal_to<K> >
struct test_oa_hash_map_params 
{
    typedef V                            data_type;
    typedef K key_type;
    typedef KeyHashF                          hasher;
    typedef KeyEqF                            key_equal;
};

typedef boost::mpl::vector<
    test_oa_hash_map_params<
        int
      , std::string
    >
  , test_oa_hash_map_params<
        std::string,
        int
    >
> test_map_args;

template <class K>
void  from_seed(K &k,int i) 
{
    int prime=1610612741;
    k=boost::lexical_cast<K>(i*prime); // uniqueness!
}

    
//BOOST_AUTO_TEST_CASE_TEMPLATE(test_oa_hash_map, test_map_arg, test_map_args)
BOOST_AUTO_TEST_CASE(test_oa_hash_map)
{

    size_t N=10000;
    size_t M=4000;
    typedef test_oa_hash_map_params<std::string,int> test_map_arg;
#define typename    
    typedef typename test_map_arg::key_type K;
    typedef typename test_map_arg::data_type V;
    typedef typename test_map_arg::hasher H;
    typedef typename test_map_arg::key_equal E;
#undef typename
    typedef oa_hash_map<K,V,H,E> hash_map;
    hash_map table;
    for (size_t i=0;i<N;++i) {
        int s = i % M;
        K k;
        V v;
        from_seed(k,s);
        from_seed(v,s);
        table[k]=v;
    }
    BOOST_CHECK_EQUAL(M,table.size());
    for (int i=0;i<M;++i) {
        K k;
        V v;
        from_seed(k,i);
        from_seed(v,i);
        BOOST_CHECK_EQUAL(table[k],v);
    }
    random_item<V> RV;
    for (int i=0;i<M;++i) {
        K k;
        from_seed(k,i);
        V v=RV();
        BOOST_CHECK_EQUAL(table.insert(k,v).second,false);
        BOOST_CHECK_EQUAL(table.erase(k),1u);
        BOOST_CHECK_EQUAL(table.insert(k,v).second,true);
        BOOST_CHECK_EQUAL(table[k],v);
    }
    table.clear();
    BOOST_CHECK_EQUAL(table.size(),0u);
}
*/
