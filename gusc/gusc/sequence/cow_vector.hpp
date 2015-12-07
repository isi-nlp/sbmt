# if ! defined(GUSC__SEQUENCE__COW_VECTOR_HPP)
# define       GUSC__SEQUENCE__COW_VECTOR_HPP

# include <iterator> 
# include <memory> 
# include <vector> 
# include <deque> 
# include <cassert> 
# include <stdexcept> 
# include <algorithm>

namespace gusc {

template < typename Container > 
class cow_container { 
    typedef boost::shared_ptr< Container > container_ptr; 
    container_ptr the_ptr; 

    void ensure_unique() 
    { 
        if (not the_ptr.unique()) { 
            the_ptr.reset(new Container(*the_ptr)); 
        } 
    } 
    
    typedef typename Container::iterator Iterator; 
    typedef typename Container::const_iterator ConstIterator; 
public: 
    cow_container() : the_ptr(new Container()) {}
     
    template <typename A> 
    cow_container(A const& a) : the_ptr(new Container(a)) {} 

    template <typename A, typename B> 
    cow_container(A const& a, B const& b) : the_ptr(new Container(a, b)) {}
     
    typedef typename Container::value_type       value_type; 
    typedef typename Container::size_type        size_type; 
    typedef typename Container::difference_type  difference_type; 
    typedef typename Container::allocator_type   allocator_type; 

    allocator_type get_allocator() const { return the_ptr->get_allocator(); } 

    class reference; 
    class const_reference; 
    class pointer; 
    class const_pointer; 
    class iterator; 
    class const_iterator; 

    class reference { 
        friend class cow_container; 
        friend class pointer; 
        cow_container* ptr; 
        size_type pos; 
        value_type value; 
        bool written; 
  
        reference(cow_container* adr, size_type where) 
        : ptr(adr) 
        , pos(where) 
        , value(ptr->the_ptr->operator[](pos)) 
        , written ( false ) 
  {} 
public: 
  operator value_type & ( void ) { 
    written = true; 
    return ( value ); 
  } 
  operator value_type const & ( void ) const { 
    return ( value ); 
  } 
  reference & operator= ( value_type const & rhs ) { 
    written = true; 
    value = rhs; 
    return ( *this ); 
  } 
  reference & operator= ( reference const & ref ) { 
    ptr = ref.ptr; 
    pos = ref.pos; 
    value = ref.value; 
    written = true; 
    return ( *this ); 
  } 
  ~reference ( void ) { 
    if ( written ) { 
      ptr->ensure_unique(); 
      ptr->the_ptr->operator[]( pos ) = value; 
    } 
  } 
  friend 
  bool operator== ( reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             == 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator== ( const_reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             == 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator!= ( reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             != 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator!= ( const_reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             != 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator< ( reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             < 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator< ( const_reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             < 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator<= ( reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             <= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator<= ( const_reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             <= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator> ( reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             > 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator> ( const_reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             > 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator>= ( reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             >= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator>= ( const_reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             >= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator== ( reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             == 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator== ( value_type const & lhs, 
                    reference const & rhs ) { 
    return ( lhs 
             == 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator== ( reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             == 
             rhs ); 
  } 
  friend 
  bool operator!= ( reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             != 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator!= ( value_type const & lhs, 
                    reference const & rhs ) { 
    return ( lhs 
             != 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator!= ( reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             != 
             rhs ); 
  } 
  friend 
  bool operator< ( reference const & lhs, 
                   reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             < 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator< ( value_type const & lhs, 
                    reference const & rhs ) { 
    return ( lhs 
             < 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator< ( reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             < 
             rhs ); 
  } 
  friend 
  bool operator<= ( reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             <= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator<= ( value_type const & lhs, 
                    reference const & rhs ) { 
    return ( lhs 
             <= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator<= ( reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             <= 
             rhs ); 
  } 
  friend 
  bool operator> ( reference const & lhs, 
                   reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             > 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator> ( value_type const & lhs, 
                    reference const & rhs ) { 
    return ( lhs 
             > 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator> ( reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             > 
             rhs ); 
  } 
  friend 
  bool operator>= ( reference const & lhs, 
                    reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             >= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator>= ( value_type const & lhs, 
                    reference const & rhs ) { 
    return ( lhs 
             >= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator>= ( reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             >= 
             rhs ); 
  } 
  pointer operator& ( void ) const { 
    return ( pointer( ptr, pos ) ); 
  } 
}; // reference 
class pointer { 
  friend class reference; 
  friend class iterator; 
  reference ref; 
  pointer ( cow_container * adr, size_type where ) 
    : ref ( adr, where ) 
  {} 
public: 
  reference operator* ( void ) const { 
    return ( ref ); 
  } 
  value_type * operator-> ( void ) { 
    return ( & ref.value ); 
  } 
}; // pointer 
class const_reference { 
  friend class cow_container; 
  cow_container const * ptr; 
  size_type             pos; 
  const_reference ( cow_container const * adr, size_type where ) 
    : ptr ( adr ) 
    , pos ( where ) 
  {} 
public: 
  operator value_type const & ( void ) const { 
    return ( ptr->the_ptr->operator[]( pos ) ); 
  } 
  friend 
  bool operator== ( const_reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             == 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator== ( value_type const & lhs, 
                    const_reference const & rhs ) { 
    return ( lhs 
             == 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator== ( const_reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             == 
             rhs ); 
  } 
  friend 
  bool operator!= ( const_reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             != 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator!= ( value_type const & lhs, 
                    const_reference const & rhs ) { 
    return ( lhs 
             != 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator!= ( const_reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             != 
             rhs ); 
  } 
  friend 
  bool operator< ( const_reference const & lhs, 
                   const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             < 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator< ( value_type const & lhs, 
                    const_reference const & rhs ) { 
    return ( lhs 
             < 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator< ( const_reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             < 
             rhs ); 
  } 
  friend 
  bool operator<= ( const_reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             <= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator<= ( value_type const & lhs, 
                    const_reference const & rhs ) { 
    return ( lhs 
             <= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator<= ( const_reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             <= 
             rhs ); 
  } 
  friend 
  bool operator> ( const_reference const & lhs, 
                   const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             > 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator> ( value_type const & lhs, 
                   const_reference const & rhs ) { 
    return ( lhs 
             > 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator> ( const_reference const & lhs, 
                   value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             > 
             rhs ); 
  } 
  friend 
  bool operator>= ( const_reference const & lhs, 
                    const_reference const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             >= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator>= ( value_type const & lhs, 
                    const_reference const & rhs ) { 
    return ( lhs 
             >= 
             static_cast< value_type const & >( rhs ) ); 
  } 
  friend 
  bool operator>= ( const_reference const & lhs, 
                    value_type const & rhs ) { 
    return ( static_cast< value_type const & >( lhs ) 
             >= 
             rhs ); 
  } 
  const_pointer operator& ( void ) const { 
    return ( const_pointer( ptr, pos ) ); 
  } 
}; // const_reference 
class const_pointer { 
  friend class const_reference; 
  const_reference ref; 
  const_pointer ( cow_container const * adr, size_type where ) 
    : ref ( adr, where ) 
  {} 
public: 
  const_reference operator* ( void ) const { 
    return ( ref ); 
  } 
  value_type const * operator-> ( void ) const { 
    return ( &( static_cast< value_type const & >( ref ) ) ); 
  } 
}; // const_pointer 
reference operator[] ( size_type where ) { 
  return ( reference( this, where ) ); 
} 
const_reference operator[] ( size_type where ) const { 
  return ( const_reference( this, where ) ); 
} 
reference at ( size_type where ) { 
  if ( size() <= where ) { 
    throw ( std::out_of_range("cow_container") ); 
  } 
  return ( reference( this, where ) ); 
} 
const_reference at ( size_type where ) const { 
  if ( size() <= where ) { 
    throw ( std::out_of_range("cow_container") ); 
  } 
  return ( const_reference( this, where ) ); 
} 
size_type size ( void ) const { 
  return ( the_ptr->size() ); 
} 
size_type max_size ( void ) const { 
  return ( the_ptr->max_size() ); 
} 
void resize ( size_type new_size, 
              value_type const & t = value_type() ) { 
  ensure_unique(); 
  the_ptr->resize( new_size, t ); 
} 
bool empty ( void ) const { 
  return ( the_ptr->empty() ); 
} 
class iterator 
  : public std::iterator< 
    typename std::iterator_traits<Iterator>::iterator_category, 
    typename std::iterator_traits<Iterator>::value_type, 
    typename std::iterator_traits<Iterator>::difference_type, 
    pointer, 
    reference 
  > 
{ 
  cow_container * ptr; 
  size_type       pos; 
  friend class cow_container; 
  friend class const_iterator; 
  iterator ( cow_container * adr, size_type where ) 
    : ptr ( adr ) 
    , pos ( where ) 
  {} 
public: 
  reference operator* ( void ) const { 
    return ( reference( ptr, pos ) ); 
  } 
  pointer operator-> ( void ) const { 
    return ( pointer( ptr, pos ) ); 
  } 
  reference operator[] ( difference_type displ ) const { 
    return ( reference( ptr, pos+displ ) ); 
  } 
  iterator ( void ) 
    : ptr ( 0 ) 
    , pos ( 0 ) 
  {} 
  friend 
  iterator operator+ ( difference_type displ, iterator where ) { 
    return ( iterator( where.ptr, where.pos+displ ) ); 
  } 
  friend 
  iterator operator+ ( iterator where, difference_type displ ) { 
    return ( iterator( where.ptr, where.pos+displ ) ); 
  } 
  friend 
  iterator operator- ( iterator where, difference_type displ ) { 
    return ( iterator( where.ptr, where.pos - displ ) ); 
  } 
  iterator & operator+= ( difference_type displ ) { 
    pos += displ; 
    return ( *this ); 
  } 
  iterator & operator-= ( difference_type displ ) { 
    pos -= displ; 
    return ( *this ); 
  } 
  difference_type operator- ( iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos - other.pos ); 
  } 
  iterator & operator++ ( void ) { 
    ++ pos; 
    return ( *this ); 
  } 
  iterator & operator-- ( void ) { 
    -- pos; 
    return ( *this ); 
  } 
  iterator operator++ ( int ) { 
    iterator dummy ( *this ); 
    ++ pos; 
    return ( dummy ); 
  } 
  iterator operator-- ( int ) { 
    iterator dummy ( *this ); 
    -- pos; 
    return ( dummy ); 
  } 
  bool operator< ( iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos < other.pos ); 
  } 
  bool operator<= ( iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos <= other.pos ); 
  } 
  bool operator> ( iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos > other.pos ); 
  } 
  bool operator>= ( iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos >= other.pos ); 
  } 
  bool operator== ( iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos == other.pos ); 
  } 
  bool operator!= ( iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos != other.pos ); 
  } 
}; // iterator 
iterator begin ( void ) { 
  return ( iterator( this, 0u ) ); 
} 
iterator end ( void ) { 
  return ( iterator( this, size() ) ); 
} 
class const_iterator 
  : public std::iterator< 
    typename std::iterator_traits<ConstIterator>::iterator_category, 
    typename std::iterator_traits<ConstIterator>::value_type, 
    typename std::iterator_traits<ConstIterator>::difference_type, 
    const_pointer, 
    const_reference 
  > 
{ 
  cow_container const * ptr; 
  size_type             pos; 
  friend class cow_container; 
  const_iterator ( cow_container const * adr, size_type where ) 
    : ptr ( adr ) 
    , pos ( where ) 
  {} 
public: 
  const_iterator ( void ) 
    : ptr ( 0 ) 
    , pos ( 0 ) 
  {} 
  const_iterator ( iterator iter ) 
    : ptr ( iter.ptr ) 
    , pos ( iter.pos ) 
  {} 
  const_reference operator* ( void ) const { 
    return ( const_reference( ptr, pos ) ); 
  } 
  const_pointer operator-> ( void ) const { 
    return ( const_pointer( ptr, pos ) ); 
  } 
  const_reference operator[] ( difference_type displ ) const { 
    return ( const_reference( ptr, pos+displ ) ); 
  } 
  friend 
  const_iterator operator+ ( difference_type displ, 
                             const_iterator where ) { 
    return ( const_iterator( where.ptr, where.pos+displ ) ); 
  } 
  friend 
  const_iterator operator+ ( const_iterator where, 
                             difference_type displ ) { 
    return ( const_iterator( where.ptr, where.pos+displ ) ); 
  } 
  friend 
  const_iterator operator- ( const_iterator where, 
                             difference_type displ ) { 
    return ( const_iterator( where.ptr, where.pos - displ ) ); 
  } 
  const_iterator & operator+= ( difference_type displ ) { 
    pos += displ; 
    return ( *this ); 
  } 
  const_iterator & operator-= ( difference_type displ ) { 
    pos -= displ; 
    return ( *this ); 
  } 
  difference_type operator- ( const_iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos - other.pos ); 
  } 
  const_iterator & operator++ ( void ) { 
    ++ pos; 
    return ( *this ); 
  } 
  const_iterator & operator-- ( void ) { 
    -- pos; 
    return ( *this ); 
  } 
  const_iterator operator++ ( int ) { 
    const_iterator dummy ( *this ); 
    ++ pos; 
    return ( dummy ); 
  } 
  const_iterator operator-- ( int ) { 
    const_iterator dummy ( *this ); 
    -- pos; 
    return ( dummy ); 
  } 
  bool operator< ( const_iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos < other.pos ); 
  } 
  bool operator<= ( const_iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos <= other.pos ); 
  } 
  bool operator> ( const_iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos > other.pos ); 
  } 
  bool operator>= ( const_iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos >= other.pos ); 
  } 
  bool operator== ( const_iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos == other.pos ); 
  } 
  bool operator!= ( const_iterator other ) const { 
    assert( ptr == other.ptr ); 
    return ( pos != other.pos ); 
  } 
}; // const_iterator 
const_iterator begin ( void ) const { 
  return ( const_iterator( this, 0u ) ); 
} 
const_iterator end ( void ) const { 
  return ( const_iterator( this, size() ) ); 
} 
typedef std::reverse_iterator< iterator >         reverse_iterator; 
typedef std::reverse_iterator< const_iterator >   
const_reverse_iterator; 
reverse_iterator rbegin ( void ) { 
  return ( reverse_iterator( end() ) ); 
} 
reverse_iterator rend ( void ) { 
  return ( reverse_iterator( begin() ) ); 
} 
const_reverse_iterator rbegin ( void ) const { 
  return ( const_reverse_iterator( end() ) ); 
} 
const_reverse_iterator rend ( void ) const { 
  return ( const_reverse_iterator( begin() ) ); 
} 
iterator insert ( iterator where, value_type const & t ) { 
  ensure_unique(); 
  the_ptr->insert( the_ptr->begin()+where.pos, t ); 
  return ( where ); 
} 
void insert ( iterator where, size_type n, value_type const & t ) { 
  ensure_unique(); 
  the_ptr->insert( the_ptr->begin()+where.pos, n, t ); 
} 
template < typename Iter > 
void insert ( iterator where, Iter from, Iter to ) { 
  ensure_unique(); 
  the_ptr->insert( the_ptr->begin()+where.pos, from, to ); 
} 
iterator erase ( iterator where ) { 
  ensure_unique(); 
  the_ptr->erase( the_ptr->begin()+where.pos ); 
  return ( where ); 
} 
iterator erase ( iterator from, iterator to ) { 
  ensure_unique(); 
  the_ptr->erase( the_ptr->begin()+from.pos, 
                  the_ptr->begin()+to.pos ); 
  return ( from ); 
} 
template < typename A, typename B > 
void assign ( A const & a, B const & b ) { 
  ensure_unique(); 
  the_ptr->assign( a, b ); 
} 
void clear ( void ) { 
  ensure_unique(); 
  the_ptr->clear(); 
} 
reference front ( void ) { 
  return ( reference( this, 0 ) ); 
} 
reference back ( void ) { 
  return ( reference( this, size()-1 ) ); 
} 
const_reference front ( void ) const { 
  return ( const_reference( this, 0 ) ); 
} 
const_reference back ( void ) const { 
  return ( const_reference( this, size()-1 ) ); 
} 
void push_back ( value_type const & value ) { 
  ensure_unique(); 
  the_ptr->push_back( value ); 
} 
void push_front ( value_type const & value ) { 
  ensure_unique(); 
  the_ptr->push_front( value ); 
} 
void pop_back ( void ) { 
  ensure_unique(); 
  the_ptr->pop_back(); 
} 
void push_front ( void ) { 
  ensure_unique(); 
  the_ptr->pop_front(); 
} 
friend 
bool operator== ( cow_container const & lhs, 
                  cow_container const & rhs ) { 
  return ( lhs.size() == rhs.size() 
           && 
           std::equal( lhs.begin(), lhs.end(), rhs.begin() ) ); 
} 
friend 
bool operator!= ( cow_container const & lhs, 
                  cow_container const & rhs ) { 
  return ( ! ( lhs == rhs ) ); 
} 
friend 
bool operator< ( cow_container const & lhs, 
                 cow_container const & rhs ) { 
  return ( std::lexicographical_compare 
           ( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() ) ); 
} 
friend 
bool operator> ( cow_container const & lhs, 
                 cow_container const & rhs ) { 
  return ( rhs < lhs ); 
} 
friend 
bool operator<= ( cow_container const & lhs, 
                  cow_container const & rhs ) { 
  return ( ! ( lhs > rhs ) ); 
} 
friend 
bool operator>= ( cow_container const & lhs, 
                  cow_container const & rhs ) { 
  return ( ! ( lhs < rhs ) ); 
} 
void swap ( cow_container & rhs ) { 
  swap( the_ptr, rhs.the_ptr ); 
} 
}; // cow_container 
template < typename T, typename Alloc = std::allocator<T> > 
class cow_vector 
: public cow_container< std::vector< T, Alloc > > 
{ 
typedef cow_container< std::vector< T, Alloc > > base; 
public: 
cow_vector ( void ) 
  : base () 
{} 
template < typename A > 
cow_vector ( A const & a ) 
  : base ( a ) 
{} 
template < typename A, typename B > 
cow_vector ( A const & a, B const & b ) 
  : base ( a, b ) 
{} 
typedef typename base::value_type value_type; 
typedef typename base::size_type size_type; 
typedef typename base::difference_type difference_type; 
typedef typename base::allocator_type  allocator_type; 
typedef typename base::pointer pointer; 
typedef typename base::const_pointer const_pointer; 
typedef typename base::reference reference; 
typedef typename base::const_reference const_reference; 
typedef typename base::iterator iterator; 
typedef typename base::const_iterator const_iterator; 
typedef typename base::reverse_iterator reverse_iterator; 
typedef typename base::const_reverse_iterator const_reverse_iterator; 
}; // cow_vector 
template < typename T, typename Alloc = std::allocator<T> > 
class cow_deque 
: public cow_container< std::deque< T, Alloc > > 
{ 
typedef cow_container< std::deque< T, Alloc > > base; 
public: 
cow_deque ( void ) 
  : base () 
{} 
template < typename A > 
cow_deque ( A const & a ) 
  : base ( a ) 
{} 
template < typename A, typename B > 
cow_deque ( A const & a, B const & b ) 
  : base ( a, b ) 
{} 
typedef typename base::value_type value_type; 
typedef typename base::size_type size_type; 
typedef typename base::difference_type difference_type; 
typedef typename base::allocator_type  allocator_type; 
typedef typename base::pointer pointer; 
typedef typename base::const_pointer const_pointer; 
typedef typename base::reference reference; 
typedef typename base::const_reference const_reference; 
typedef typename base::iterator iterator; 
typedef typename base::const_iterator const_iterator; 
typedef typename base::reverse_iterator reverse_iterator; 
typedef typename base::const_reverse_iterator const_reverse_iterator; 
}; // cow_deque 
} // namespace kubux 


} // namespace gusc

# endif //     GUSC__SEQUENCE__COW_VECTOR_HPP
