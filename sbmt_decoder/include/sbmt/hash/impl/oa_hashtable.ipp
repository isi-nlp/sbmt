#include <string>
#include <sstream>

#include <sbmt/hash/swap.hpp>

#ifdef _WIN32
#include <iso646.h>
#endif

#include <cassert>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

// this code shamelessly ripped from g++ hashtable implementation
namespace detail {

enum { oa_num_primes = 28 };

static const std::size_t oa_prime_list[oa_num_primes] = {
    53u,         97u,         193u,       389u,       769u,
    1543u,       3079u,       6151u,      12289u,     24593u,
    49157u,      98317u,      196613u,    393241u,    786433u,
    1572869u,    3145739u,    6291469u,   12582917u,  25165843u,
    50331653u,   100663319u,  201326611u, 402653189u, 805306457u,
    1610612741u, 3221225473u, 4294967291u
};

inline std::size_t
oa_next_prime(std::size_t n)
{
    const std::size_t* first = oa_prime_list; 
    const std::size_t* last = oa_prime_list + (int)oa_num_primes;
    const std::size_t* pos = std::lower_bound(first, last, n);
    return pos == last ? *(last - 1) : *pos;
}

class double_hash_generator 
{
public:
    double_hash_generator(std::size_t hash_v, std::size_t mod)
    : h(hash_v % mod)
    , k(1 + (hash_v % (mod - 1)))
    , m(mod) 
    , last(hash_v % mod) 
    {
        assert(m > 0);
        assert(h >= 0 and h < m);
        assert(k >  0 and h < m);
    }
    
    std::size_t next() 
    {
        std::size_t retval = last;
        last = (last + k) % m;
        assert(retval >= 0 and retval < m);
        return retval;
    }
private:
    std::size_t h;
    std::size_t k;
    std::size_t m;
    std::size_t last;
};

} // namespace detail

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
oa_hashtable<V,KFV,H,E,A>::oa_hashtable( size_type n
                                       , const key_extractor& k
                                       , const hasher& hf
                                       , const key_equal& eql
                                       , const allocator_type& a )
    : key_extractor(k)
, equal_keys(eql)
, hash_key(hf)
    ,  lf(0.5)
, table(size_type(0.5*detail::oa_next_prime(n)), detail::oa_next_prime(n+1), a)
{}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
template <class InputIterator>
oa_hashtable<V,KFV,H,E,A>::oa_hashtable( InputIterator f, InputIterator l
                                       , size_type n
                                       , const key_extractor& k
                                       , const hasher& hf
                                       , const key_equal& eql
                                       , const allocator_type& a )
    : key_extractor(k)
    , equal_keys(eql)
    , hash_key(hf) 
    ,  lf(0.5)
    , table(size_type(0.5*detail::oa_next_prime(n)), detail::oa_next_prime(n+1), a)
{
    for (; f != l; ++f) insert(*f);
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
oa_hashtable<V,KFV,H,E,A>::oa_hashtable( size_type n
                                       , float lft
                                       , const key_extractor& k
                                       , const hasher& hf
                                       , const key_equal& eql
                                       , const allocator_type& a )
    : key_extractor(k)
, equal_keys(eql)
, hash_key(hf)
    ,  lf(lft)
, table(size_type(lft*detail::oa_next_prime(n)), detail::oa_next_prime(n+1), a)
{}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
template <class InputIterator>
oa_hashtable<V,KFV,H,E,A>::oa_hashtable( InputIterator f, InputIterator l
                                       , size_type n
                                       , float lft
                                       , const key_extractor& k
                                       , const hasher& hf
                                       , const key_equal& eql
                                       , const allocator_type& a )
    : key_extractor(k)
, equal_keys(eql)
, hash_key(hf) 
    ,  lf(lft)
, table(size_type(lft*detail::oa_next_prime(n)), detail::oa_next_prime(n+1), a)
{
    for (; f != l; ++f) insert(*f);
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::allocator_type 
oa_hashtable<V,KFV,H,E,A>::get_allocator() const
{
    return table.get_allocator();
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
bool oa_hashtable<V,KFV,H,E,A>::empty() const
{
    return table.count() == 0;
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::size_type 
oa_hashtable<V,KFV,H,E,A>::size() const
{
    return table.count();
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::size_type 
oa_hashtable<V,KFV,H,E,A>::max_size() const
{
    return table.capacity();
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::iterator 
oa_hashtable<V,KFV,H,E,A>::begin() const
{
    return table.begin();
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::iterator 
oa_hashtable<V,KFV,H,E,A>::end() const
{
    return table.end();
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
std::pair<typename oa_hashtable<V,KFV,H,E,A>::iterator, bool>
oa_hashtable<V,KFV,H,E,A>::insert(const value_type& obj)
{
    size_type pos = find_insertion(extract_key()(obj),table);
    bool inserting = table.status_at(pos) != oa_entry_occupied;
    
    if (inserting) {
        table.set_value_at(pos,obj);
    
        if (table.non_empty_count() > table.capacity()) {
            // the conditional is an attempt to avoid the effects of lots of 
            // lazy deletions:  if the table can be rehashed at current size while
            // only being "half" full, then dont double the capacity, just rehash
            if ( table.count() < table.capacity() and 
                 float(table.count())/float(table.size()) <= lf/2
                ) rehash(table.capacity());
            else rehash(2 * table.capacity() + 1);
            pos = find_value(extract_key()(obj),table);
        }
    }
    
    return std::make_pair(table.position_iterator(pos),inserting);
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
void oa_hashtable<V,KFV,H,E,A>::erase(iterator position)
{
    table.erase_value_at(position);
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::size_type 
oa_hashtable<V,KFV,H,E,A>::erase(typename boost::call_traits<key_type>::param_type k) 
{
    size_type pos = find_value(k, table);
    if (table.status_at(pos) == oa_entry_occupied) {
        table.erase_value_at(pos);
        return 1;
    } else return 0;
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
std::pair<typename oa_hashtable<V,KFV,H,E,A>::iterator, bool>
oa_hashtable<V,KFV,H,E,A>::replace(iterator position, value_type const& v)
{
    size_type pos = table.index_at(position);
    if (equal_keys(extract_key()(v),extract_key()(table.value_at(pos)))) {
        table.set_value_at(pos,v);
        return std::make_pair(position,true);
    } else {
        size_type  new_pos = find_insertion(extract_key()(v),table);
        if(table.status_at(new_pos) == oa_entry_occupied) {
            return std::make_pair(table.position_iterator(new_pos),false);
        } else {
            table.insert_value_at(new_pos,v);
            table.erase_value_at(pos);
            return std::make_pair(table.position_iterator(new_pos),true);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
void oa_hashtable<V,KFV,H,E,A>::clear()
{
    table.clear();
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
void oa_hashtable<V,KFV,H,E,A>::swap(oa_hashtable& other)
{
    std::resolve_swap(equal_keys,  other.equal_keys);
    std::resolve_swap(extract_key_(), other.extract_key_());
    std::resolve_swap(hash_key,    other.hash_key);
    std::resolve_swap(table,       other.table);
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::hasher
oa_hashtable<V,KFV,H,E,A>::hash_function() const { return hash_key; }

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::key_equal 
oa_hashtable<V,KFV,H,E,A>::key_eq() const { return equal_keys; }

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::size_type
oa_hashtable<V,KFV,H,E,A>::find_value( typename boost::call_traits<key_type>::param_type k
                                     , table_type const& tbl ) const
{
    detail::double_hash_generator hg(hash_key(k),tbl.size());
    size_type idx = hg.next();
    while (tbl.status_at(idx) != oa_entry_empty) {
        if (tbl.status_at(idx) == oa_entry_occupied and 
            equal_keys(extract_key()(tbl.value_at(idx)), k)) break;
        idx = hg.next();
    }
    return idx;
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::size_type
oa_hashtable<V,KFV,H,E,A>::find_insertion( typename boost::call_traits<key_type>::param_type k
                                         , table_type const& tbl ) const
{
    detail::double_hash_generator hg(hash_key(k),tbl.size());
    size_type idx = hg.next();
    for(;tbl.status_at(idx) != oa_entry_empty;idx = hg.next()) {
        if (tbl.status_at(idx) == oa_entry_occupied && 
            equal_keys(extract_key()(tbl.value_at(idx)), k))
            return idx;
        if (tbl.status_at(idx) == oa_entry_erased) {
            size_type blank_pos = idx;
            for(;tbl.status_at(idx) != oa_entry_empty;idx = hg.next()) {
                if (tbl.status_at(idx) == oa_entry_occupied and 
                    equal_keys(extract_key()(tbl.value_at(idx)), k))
                    return idx;
            }
            return blank_pos;
        }
    }
    return idx;
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
typename oa_hashtable<V,KFV,H,E,A>::iterator 
oa_hashtable<V,KFV,H,E,A>::find(typename boost::call_traits<key_type>::param_type k) const
{
    size_type pos = find_value(k, table);
    if (table.status_at(pos) == oa_entry_occupied) 
        return table.position_iterator(pos);
    else return table.end();
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
float oa_hashtable<V,KFV,H,E,A>::load_factor() const
{
    return float(table.non_empty_count()) / float(table.size());
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
float oa_hashtable<V,KFV,H,E,A>::max_load_factor() const 
{ 
    return float(table.capacity()) / float(table.size());
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class KFV, class H, class E, class A>
void oa_hashtable<V,KFV,H,E,A>::rehash(size_type n)
{
    size_type new_capacity = n;
    size_type new_table_size;
    /// dont rehash if the new-capacity request is smaller than what is 
    /// already in the table.
    if (new_capacity < table.count()) return;
    if (n == table.capacity()) {
        new_table_size = table.size();
    } else {
        new_table_size = detail::oa_next_prime(size_type(float(n) / lf + 1));
        if (new_table_size <= new_capacity) 
            new_table_size = detail::oa_next_prime(new_capacity + 1);
    }
    
    table_type new_table(new_capacity,new_table_size,get_allocator());
    
    iterator itr = table.begin();
    iterator end = table.end();
    for (; itr != end; ++itr) {
        size_type pos = find_insertion(extract_key()(*itr),new_table);
        assert(new_table.status_at(pos) == oa_entry_empty);
        new_table.set_value_at(pos,*itr);
    }
    
    table.swap(new_table);
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class A>
std::string status_string(typename oa_table_c<V,A>::status c)
{
    typedef oa_table_c<V,A> table_t;
    switch(c) {
        case table_t::empty:    return "empty";
        case table_t::occupied: return "occupied";
        case table_t::erased:   return "erased";
        case table_t::invalid:  return "invalid";
    }
}

template <class V, class KFV, class H, class E, class A>
std::string oa_hashtable<V,KFV,H,E,A>::print_table() const
{
    std::stringstream out;
    for (std::size_t x = 0; x != table.size(); ++x) {
        if (table.status_at(x) == oa_entry_occupied)
            out << x << " : " << table.value_at(x) << " : " 
                << status_string<V,A>(table.status_at(x)) << std::endl;
        else out << x << " : __ : " 
                 << status_string<V,A>(table.status_at(x)) << std::endl;
    }
    return out.str();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
