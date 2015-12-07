#include <boost/iterator/iterator_facade.hpp>
#include <stdexcept>
#include <sstream>

#ifdef _WIN32
#include <iso646.h>
#endif

#include <cassert>

namespace sbmt {

enum oa_entry_status { oa_entry_empty = 0
                     , oa_entry_occupied = 1
					 , oa_entry_erased = 2
					 , oa_entry_invalid = 3 };

/** \todo: I suggest a separate parallel vector for oa_entry_status, 
   for alignment reasons (2bit per?  or char).  
   but, stat/value together makes iterator more efficient?  
   but hash table iteration is not so often used.  
   separate type for find return, 
   and begin/end is best (but not in stdext::hash_map concept)
   - Jon
 */

template <class value_type,class const_pointer> 
class oa_iterator 
: public boost::iterator_facade <
	  oa_iterator<value_type,const_pointer>
	, value_type const
	, boost::bidirectional_traversal_tag
  >
{
public:
	oa_iterator() : itr(NULL) {}
	
	explicit oa_iterator(const_pointer i)
	  : itr(i) 
	{
		while (itr->get_status() == oa_entry_empty or itr->get_status() == oa_entry_erased ) ++itr;
		assert(itr->get_status() != oa_entry_empty);
		assert(itr->get_status() != oa_entry_erased);
	}
private:
	friend class boost::iterator_core_access;
		
	void increment()
	{
		if (itr->get_status() != oa_entry_invalid) do {
			++itr;
		} while (itr->get_status() == oa_entry_empty or itr->get_status() == oa_entry_erased); 
		// note: stops on oa_entry_invalid (end)
	}
	
	void decrement()
	{
		if (itr->get_status() != oa_entry_invalid) do {
			--itr;
		} while (itr->get_status() == oa_entry_empty or itr->get_status() == oa_entry_erased); 
		// note: stops on oa_entry_invalid (end)
	}
	
	value_type const& dereference() const 
	{
		// this method is inlined for MSVC compatibility
		assert(itr->get_status() != oa_entry_empty);
		assert(itr->get_status() != oa_entry_erased);
		assert(itr->get_status() != oa_entry_invalid);
		return itr->value;
	}
	
	bool equal(oa_iterator const& other) const
	{
		return itr == other.itr;
	}
	
	const_pointer itr;
};

template <class ValueT>
struct oa_entry {
    public:
        oa_entry_status get_status() const { return oa_entry_status(stat); }
        void set_status(oa_entry_status s) { stat = s;}
        ValueT value; // value first -> faster deref
        char stat;
        oa_entry();
        ~oa_entry();
    };
 
template <class ValueT, class AllocT>
class oa_table_c
{
public:
     
    ////////////////////////////////////////////////////////////////////////////
    
    typedef oa_entry<ValueT> entry;
    
    ////////////////////////////////////////////////////////////////////////////
    
    typedef ValueT                                         value_type;
    typedef typename AllocT::template rebind<entry>::other allocator_type;
    typedef oa_entry<ValueT> const*                          const_pointer;
    typedef oa_entry<ValueT>*                                pointer;
    typedef oa_entry<ValueT>&                                reference;
    typedef oa_entry<ValueT> const&                          const_reference;
    
    ////////////////////////////////////////////////////////////////////////////
    
    typedef oa_iterator<ValueT,oa_entry<ValueT> const*> iterator;
    
    ////////////////////////////////////////////////////////////////////////////
    
    oa_table_c( std::size_t max_occupied
              , std::size_t table_size
              , allocator_type a = std::allocator<entry>() );

    oa_table_c(oa_table_c const& other);
    
    oa_table_c& operator=(oa_table_c const& other);
    
    void swap(oa_table_c& other);
    
    ~oa_table_c();
    
    void set_value_at(std::size_t idx, value_type const& v);
    
    value_type const& value_at(std::size_t idx) const;   
    value_type&       value_at(std::size_t idx);
    
    iterator position_iterator(std::size_t idx) const;
    std::size_t index_at(iterator const& position) const;
    
    void     erase_value_at(iterator pos);
    void     erase_value_at(std::size_t idx);
    
    oa_entry_status status_at(std::size_t idx) const;
    oa_entry_status status_at(iterator pos) const;
    
    std::size_t count() const { return num_occupied; }
    std::size_t size() const { return table_size; }
    std::size_t non_empty_count() const { return num_non_empty; }
    std::size_t capacity() const { return max_non_empty; }
    
    iterator begin() const { return iterator(table + 1); }
    iterator end() const { return iterator(table + table_size + 1); }
    
    allocator_type get_allocator() const { return alloc; }
    
    void clear();

private:
    std::size_t    real_index(std::size_t ixd) const;
    allocator_type alloc;
    std::size_t    table_size;
    std::size_t    max_non_empty;
    std::size_t    num_occupied;
    std::size_t    num_non_empty;
    entry*         table;
};

////////////////////////////////////////////////////////////////////////////////

template <class V, class A>
oa_table_c<V,A>::oa_table_c( std::size_t max_occupied
                           , std::size_t table_size
                           , allocator_type a )
: alloc(a)
, table_size(table_size)
, max_non_empty(max_occupied)
, num_occupied(0)
, num_non_empty(0)
, table(alloc.allocate(table_size + 2)) 
{
    if (table_size <= max_occupied or table_size <= 0) {
        alloc.deallocate(table,table_size + 2);
        std::stringstream sstr;
        sstr << "table cant hold more items than space: "
             << "was load-factor >= 1?"
             << " max_occupied: " << max_occupied
             << " table_size: " << table_size;
        std::invalid_argument e(sstr.str());
        throw e;
    }
    
    table[0].set_status(oa_entry_invalid);
    table[table_size + 1].set_status(oa_entry_invalid); /// these are the iterator guards.
    
    for (std::size_t idx = 1; idx != table_size + 1; ++idx) {
        table[idx].set_status(oa_entry_empty);
    }
}

template <class V, class A>
oa_table_c<V,A>::oa_table_c( oa_table_c const& other)
: alloc(other.alloc)
, table_size(other.table_size)
, max_non_empty(other.max_non_empty)
, num_occupied(other.num_occupied)
, num_non_empty(other.num_non_empty)
, table(alloc.allocate(table_size + 2))
{
    table[0].set_status(oa_entry_invalid);
    table[table_size + 1].set_status(oa_entry_invalid); /// these are the iterator guards.
    std::size_t idx;
    try {
        for (idx = 1; idx != table_size + 1; ++idx) {
            table[idx].set_status(other.table[idx].get_status());
            if(other.table[idx].get_status() == oa_entry_occupied) {
                new(&(table[idx].value)) value_type(other.table[idx].value);
            }
        }       
    } catch(...) {
        --idx;
        for (; idx != 0; --idx) {
            if(table[idx].get_status() == oa_entry_occupied) table[idx].value.~value_type();
        }
        alloc.deallocate(table,table_size + 2);
        throw;
    }
}

template <class V, class A>
oa_table_c<V,A>& oa_table_c<V,A>::operator=( oa_table_c<V,A> const& other)
{
    if (this != &other) {
        oa_table_c<V,A> new_table(other);
        this->swap(new_table);
    }
    return *this;
}

template <class V, class A>
void oa_table_c<V,A>::swap(oa_table_c<V,A>& other)
{
    //need to deep copy if this fails...
    //but im too lazy to write the code right now
    assert(alloc == other.alloc); 
    
    std::swap(table_size,    other.table_size);
    std::swap(max_non_empty, other.max_non_empty);
    std::swap(num_occupied,  other.num_occupied);
    std::swap(num_non_empty, other.num_non_empty);
    std::swap(table,         other.table);
}

template <class V, class A>
oa_table_c<V,A>::~oa_table_c()
{
    std::size_t idx = 1;
    for(; idx != table_size + 1; ++idx) {
        if (table[idx].get_status() == oa_entry_occupied) table[idx].value.~value_type();
    }
    alloc.deallocate(table,table_size + 2);
}

////////////////////////////////////////////////////////////////////////////////

template <class V, class A>
void oa_table_c<V,A>::set_value_at(std::size_t idx, value_type const& v)
{
    std::size_t real_idx = real_index(idx);
    bool do_construct = table[real_idx].get_status() == oa_entry_empty or
                        table[real_idx].get_status() == oa_entry_erased;
    if (do_construct) {
        new(&(table[real_idx].value)) value_type(v);
        ++num_occupied;
        if (table[real_idx].get_status() == oa_entry_empty) ++num_non_empty;
    } else {
        assert(table[real_idx].get_status() == oa_entry_occupied);
        table[real_idx].value = v;
    }
    table[real_idx].set_status(oa_entry_occupied);
}

template <class V, class A>
typename oa_table_c<V,A>::value_type const& 
oa_table_c<V,A>::value_at(std::size_t idx) const
{
    std::size_t real_idx = real_index(idx);
    assert(table[real_idx].get_status() == oa_entry_occupied);
    return table[real_idx].value;
}

template <class V, class A>
typename oa_table_c<V,A>::value_type& 
oa_table_c<V,A>::value_at(std::size_t idx)
{
    std::size_t real_idx = real_index(idx);
    assert(table[real_idx].get_status() == oa_entry_occupied);
    return table[real_idx].value;
}

template <class V, class A>
typename oa_table_c<V,A>::iterator 
oa_table_c<V,A>::position_iterator(std::size_t idx) const
{
    return iterator(table + real_index(idx));
}

template <class V, class A>
void oa_table_c<V,A>::erase_value_at(iterator pos)
{
    entry* p = table + (pos.itr - table);
    if (p->get_status() == oa_entry_empty) return; //shouldnt happen
    if (p->get_status() == oa_entry_occupied) {
        p->value.~value_type();
        --num_occupied;
        p->set_status(oa_entry_erased);
    }
}

template <class V, class A>
void oa_table_c<V,A>::erase_value_at(std::size_t idx)
{
    std::size_t real_idx = real_index(idx);
    if (table[real_idx].get_status() == oa_entry_occupied) {
        table[real_idx].value.~value_type();
        --num_occupied;
        table[real_idx].set_status(oa_entry_erased);
    }
}

template <class V, class A>
oa_entry_status 
oa_table_c<V,A>::status_at(std::size_t idx) const
{
    return table[real_index(idx)].get_status();
}

template <class V, class A>
oa_entry_status 
oa_table_c<V,A>::status_at(iterator pos) const
{
    return pos->get_status();
}

template <class V, class A>
void oa_table_c<V,A>::clear()
{
    for (std::size_t idx = 1; idx != table_size + 1; ++idx) {
        if(table[idx].get_status() == oa_entry_occupied) table[idx].value.~value_type();
        table[idx].set_status(oa_entry_empty);
    }
    num_non_empty = 0;
    num_occupied  = 0;
}

template <class V, class A>
std::size_t oa_table_c<V,A>::real_index(std::size_t idx) const
{
    assert(idx < table_size);
    return idx + 1;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
