#ifndef   SBMT_UTIL_PRIORITY_TABLE_HPP
#define   SBMT_UTIL_PRIORITY_TABLE_HPP

#include <memory>
#include <functional>
#include <algorithm>
#include <boost/call_traits.hpp>
#include <sbmt/hash/oa_hashtable.hpp>
#include <sbmt/hash/swap.hpp>
#include <boost/functional/hash.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
/// this probably isnt the most generic queue table you are looking for.
/// you cant delete elements from the table other than by popping them,
/// and you can only modify them in a way that does not change the key, only the
/// priority.
///
/// it should however be good for queueing up edge equivalents.
///
/// missing functionality of course could be added later.
///
/// there are certainly a lot of template arguments here but in most cases
/// you should only need the first three:
/// 1 - specify what you are storing
/// 2 - specify how to get the key (identifier) out of the thing you are storing
/// 3 - specify how to get the priority out of what you are storing
///
////////////////////////////////////////////////////////////////////////////////
template< class ValueT
        , class KeyFromValueF
        , class PriorityFromValueF
        , class KeyHasherF = 
                    boost::hash<typename KeyFromValueF::result_type>
        , class KeyEqualsF = 
                    std::equal_to<typename KeyFromValueF::result_type>
        , class PriorityComparatorF = 
                   std::less<typename PriorityFromValueF::result_type>
        , class AllocatorT = std::allocator<ValueT> 
        >
class priority_table
{
public:
    typedef ValueT                                   value_type;
    typedef typename KeyFromValueF::result_type      key_type;
    typedef typename PriorityFromValueF::result_type priority_type;
    typedef KeyFromValueF                            key_extractor;
    typedef PriorityFromValueF                       priority_extractor;
    typedef KeyHasherF                               hasher;
    typedef KeyEqualsF                               key_equal;
    typedef PriorityComparatorF                      priority_compare;
    typedef AllocatorT                               allocator_type;
    typedef std::size_t                              size_type;

private:    
    typedef std::vector <
                value_type
              , typename allocator_type::template rebind<value_type>::other
            > priority_vec_t;
            
    struct extract_key_from_entry {
        
        extract_key_from_entry( priority_vec_t const* pheap
                              , key_extractor  const& ek )
        : pheap(pheap)
        , extract_key(ek) {}
        
        extract_key_from_entry& operator=(const extract_key_from_entry& o)
        {
            extract_key = o.extract_key;
            return *this;
        }
        
        typedef key_type result_type;
        
        key_type operator()(size_type index) const
        {
            return extract_key((*pheap)[index]);
        }
        priority_vec_t const* pheap;
        key_extractor         extract_key;
    };

    typedef oa_hashtable< 
                std::size_t
              , extract_key_from_entry
              , hasher
              , key_equal
              , typename allocator_type::template rebind<std::size_t>::other
            > hashtable_t;
public:

    /// moves through all the items in table in no particular order
    class iterator
    : public boost::iterator_facade<
                 iterator
               , value_type const
               , std::bidirectional_iterator_tag
             >
    {
    public:
        iterator()
        : pheap(NULL) {}
    private:
        void increment() {++itr;}
        void decrement() {--itr;}
        value_type const& dereference() const { return (*pheap)[*itr]; }
        bool equal(iterator const& other) const { return itr == other.itr; }
        iterator( typename hashtable_t::iterator itr
                , priority_vec_t const* pheap )
        : itr(itr)
        , pheap(pheap) {}
        typename hashtable_t::iterator itr;
        priority_vec_t const*          pheap;
        friend class priority_table;
        friend class boost::iterator_core_access;
    };
    
    priority_table( key_extractor           extract_key = key_extractor()
                  , priority_extractor extract_priority = priority_extractor()
                  , hasher                     hash_key = hasher()
                  , key_equal                equal_keys = key_equal()
                  , priority_compare compare_priorities = priority_compare() );
    
    priority_table(priority_table const& other);
    
    std::pair<iterator,bool> insert(value_type const& val);
    iterator                 find(typename boost::call_traits<key_type>::param_type val) const;
    
    iterator begin() const;
    iterator end() const;
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// returns false and no replacement takes place if the new value 
    /// has a different key than the old value.
    /// modifying the priority is fine.
    ///
    ////////////////////////////////////////////////////////////////////////////
    bool replace(iterator pos, value_type const& val);
    
    void erase(iterator pos);
    bool erase(typename boost::call_traits<key_type>::param_type val);
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// ModifyFunctor f is callable like so:
    /// \code
    ///     value_type v;
    ///     f(v);
    /// \endcode
    ///
    /// if your modify functor changes the key of the value you are 
    /// modifying, game-over man.  this queue is now invalid.  modifying the 
    /// priority is fine.
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class ModifyFunctor>
        void modify(iterator pos, ModifyFunctor f);
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// highest priority item.  compare_priorities(top(),x) fails for all other
    /// x in the table
    ///
    ////////////////////////////////////////////////////////////////////////////
    value_type const& top() const;
    void pop();
    
    size_type size() const;
    bool      empty() const { return size() == 0; }
    
private:        
    void erase(typename priority_vec_t::iterator itr);
    priority_type priority(typename priority_vec_t::iterator itr);
    
    void swap( typename priority_vec_t::iterator itr1
             , typename priority_vec_t::iterator itr2 );
             
    void percolate_up(typename priority_vec_t::iterator itr);
    
    void percolate_down(typename priority_vec_t::iterator itr);
    
    typename priority_vec_t::iterator 
        parent(typename priority_vec_t::iterator itr);
    
    typename priority_vec_t::iterator 
        left_child(typename priority_vec_t::iterator itr);
    
    typename priority_vec_t::iterator 
        right_child(typename priority_vec_t::iterator itr);
    
    bool sanity();
    
    priority_vec_t     priority_heap;
    hashtable_t        entry_index;
    priority_compare   compare_priorities;
    priority_extractor extract_priority;
    key_extractor      extract_key;
    key_equal          equal_keys;
};

template<class VT,class KVF,class PVF,class KHF,class KEF,class PCF,class AT>
void swap(
           typename 
           priority_table<VT,KVF,PVF,KHF,KEF,PCF,AT>::extract_key_from_entry& f1
         , typename 
           priority_table<VT,KVF,PVF,KHF,KEF,PCF,AT>::extract_key_from_entry& f2
         ) { resolve_swap(f1.extract_key, f2.extract_key); }

} // namespace sbmt

#include <sbmt/hash/impl/priority_table.ipp>

#endif // SBMT_UTIL_PRIORITY_TABLE_HPP
