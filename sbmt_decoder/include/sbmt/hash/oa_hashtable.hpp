#ifndef   SBMT_HASH_OA_HASHTABLE_HPP
#define   SBMT_HASH_OA_HASHTABLE_HPP
#include <boost/type_traits/remove_reference.hpp>
#include <boost/functional/hash.hpp>
#include <boost/call_traits.hpp>
#include <boost/utility/result_of.hpp>
#include <sbmt/hash/impl/oa_table_c.ipp>
#include <graehl/shared/map_from_set.hpp>
#include <utility>
#include <gusc/functional.hpp>
#include <boost/utility/result_of.hpp>

namespace sbmt {

///OA: Open Addressing.
template < class ValueT
         , class KeyFromValF
         , class HashF       = boost::hash<
                                 typename boost::remove_reference<
                                     typename boost::result_of<
                                         KeyFromValF(ValueT)
                                     >::type
                                 >::type
                               >
         , class EqualToF    = gusc::equal_to
         , class AllocT      = std::allocator<ValueT>
         >
class oa_hashtable : private KeyFromValF
{
    typedef oa_table_c<ValueT,AllocT> table_type;
public:
    typedef KeyFromValF key_extractor;
    typedef ValueT value_type;
    typedef typename boost::result_of<KeyFromValF(ValueT)>::type key_type;
    typedef typename table_type::iterator iterator;
    typedef iterator const_iterator;
    typedef HashF hasher;
    typedef EqualToF key_equal;
    typedef typename table_type::allocator_type allocator_type;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::const_reference const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    explicit oa_hashtable( size_type n = 0
                         , key_extractor const& = key_extractor()
                         , hasher const& hf = hasher()
                         , key_equal const& eql = key_equal()
                         , allocator_type const& a = allocator_type() );

    template <class InputIterator>
        oa_hashtable( InputIterator f, InputIterator l
                    , size_type n = 0
                    , key_extractor const& = key_extractor()
                    , hasher const& hf = hasher()
                    , key_equal const& eql = key_equal()
                    , allocator_type const& a = allocator_type() );

    explicit oa_hashtable( size_type n
                         , float load_factor
                         , key_extractor const& = key_extractor()
                         , hasher const& hf = hasher()
                         , key_equal const& eql = key_equal()
                         , allocator_type const& a = allocator_type() );

    template <class InputIterator>
        oa_hashtable( InputIterator f, InputIterator l
                    , size_type n
                    , float load_factor
                    , key_extractor const& = key_extractor()
                    , hasher const& hf = hasher()
                    , key_equal const& eql = key_equal()
                    , allocator_type const& a = allocator_type() );

    allocator_type get_allocator() const;

    bool      empty() const;
    size_type size() const;
    size_type max_size() const;

    iterator begin() const;
    iterator end() const;

    std::pair<iterator,bool> insert(value_type const& obj);
    iterator insert(iterator hint, value_type const& obj);
    template <class InputIterator>
        void insert(InputIterator first, InputIterator last)
        {
            for(; first != last; ++first) insert(*first);
        }

    void      erase(iterator position);
    size_type erase(typename boost::call_traits<key_type>::param_type k);
    void      erase(iterator first, iterator last);
    
    std::pair<iterator,bool> replace(iterator position, value_type const& v);
    
    void      clear();

    void swap(oa_hashtable& other);

    hasher    hash_function() const;
    key_equal key_eq() const;

    iterator find(typename boost::call_traits<key_type>::param_type k) const;

    /// number of elements / table size
    float load_factor() const;
    
    /// max load_factor befor table resizes
    float max_load_factor() const;
    
    void  rehash(size_type n);
    
    std::string print_table() const;

    key_extractor const& extract_key() const 
    { return *this; }
private:
    key_extractor & extract_key_()
    { return *this; }
    size_type find_value( typename boost::call_traits<key_type>::param_type k, table_type const& tbl ) const;
    size_type find_insertion( typename boost::call_traits<key_type>::param_type k, table_type const& tbl ) const;
    
    key_equal     equal_keys;
    hasher        hash_key;
    float         lf;
    table_type    table;
};

template < class K
         , class V
         , class H
         , class KE
         , class A
>
struct oa_hash_map_base {
    typedef oa_hashtable<std::pair<K,V>,graehl::first_<K>,H,KE,A> type;   
};	
template < class K
         , class V
         , class hasher = boost::hash<K>
         , class key_equal = std::equal_to<K>
         , class allocator_type = std::allocator<std::pair<K,V> >
> class oa_hash_map;
//TODO: test
template < class K
         , class V
         , class hasher
         , class key_equal
         , class allocator_type
>
class oa_hash_map 
: public oa_hash_map_base<K,V,hasher,key_equal,allocator_type>::type                                        
{
 public:
    typedef std::pair<K,V> value_type;
    typedef K key_type;
    typedef V mapped_type;
    typedef V data_type;
 private:
    typedef graehl::first_<K> KE;
    typedef typename oa_hash_map_base<K,V,hasher,key_equal,allocator_type>::type super;
    typedef oa_hash_map self_type;
    typedef typename super::size_type size_type;
 public:
    oa_hash_map() {}
	
    explicit oa_hash_map( size_type n
                        , hasher const& hf = hasher()
                        , key_equal const& eql = key_equal()
                        , allocator_type const& a = allocator_type() )
        : oa_hash_map::super(n,KE(),hf,eql,a) {}

    template <class InputIterator>
        oa_hash_map( InputIterator f, InputIterator l
                    , size_type n = 0
                    , hasher const& hf = hasher()
                    , key_equal const& eql = key_equal()
                    , allocator_type const& a = allocator_type() )
            : oa_hash_map::super(f,l,n,KE(),hf,eql,a) {}

    explicit oa_hash_map( size_type n
                        , float load_factor
                        , hasher const& hf = hasher()
                        , key_equal const& eql = key_equal()
                        , allocator_type const& a = allocator_type() )
        : oa_hash_map::super(n,load_factor,KE(),hf,eql,a) {}
    
    template <class InputIterator>
    oa_hash_map( InputIterator f, InputIterator l
               , size_type n
               , float load_factor
               , hasher const& hf = hasher()
               , key_equal const& eql = key_equal()
               , allocator_type const& a = allocator_type() )
    : oa_hash_map::super(f,l,n,load_factor,hf,eql,a) {}

    V & operator[](K const& key) {
        return const_cast<V&>(this->insert(value_type(key,V())).first->second);
    }
    V &at_default(K const& key,V const& def=V()) 
    {
        return const_cast<V&>(this->insert(value_type(key,def)).first->second);
    }
    V &at_throw(K const& key) const
    {
        typename super::iterator i = this->find(key);
        if (i==this->end())
            throw std::runtime_error("Tried to get a key from oa_hash_map that isn't there");
        return const_cast<V&>(i->second);
    }
    V &at(K const& key) const
    {
        typename super::iterator i=this->find(key);
        assert(i!=this->end());
        return const_cast<V&>(i->second);
    }
/*
    void add_new(K const& key,V const &value=V())
    {
        std::pair<typename HT::iterator,bool> r=ht().insert(value_type(key,value));
        if (!r.second)
            throw std::runtime_error("Key already existed in oa_hash_map::add_new(key,val)");
    }    
    void set(K const& key,V const &value=V())
    {
        std::pair<typename HT::iterator,bool> r=ht().insert(value_type(key,value));
        if (!r.second)
            const_cast<V&>(r.first->second)=value;
    }
	
    V * find_second(K const& key)
    {
        typename HT::iterator i=ht().find(key);
        return i==ht().end() ? 0 : &const_cast<V&>(i->second);
    }
    V const* find_second(K const& key) const
    {
        return const_cast<self_type *>(this)->find_second(key);
    }
*/
    void swap(self_type &o) 
    {
        static_cast<super*>(this)->swap(static_cast<super&>(o));
    }
    
};


////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
void swap(oa_hashtable<V,KFV,H,E,A>& x, oa_hashtable<V,KFV,H,E,A>& y)
{ return x.swap(y); }

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/hash/impl/oa_hashtable.ipp>

#endif // SBMT_HASH_OA_HASHTABLE_HPP
