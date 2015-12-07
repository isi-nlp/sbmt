# if ! defined(SBMT__HASH__REF_ARRAY_HPP)
# define       SBMT__HASH__REF_ARRAY_HPP

# include <boost/array.hpp>
# include <boost/ref.hpp>
# include <boost/iterator/iterator_facade.hpp>
# include <boost/type_traits.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  \class reference_array
///  useful for turning seperately declared objects of the same type
///  into a collection without the overhead of copying them in.
///  
///  why not use an existing solution:
///   - T& array[N] is illegal.
///   - boost::array<boost::reference_wrapper<T>,N> doesnt quite work, as the 
///     implicit conversions to underlying references arent always automatic
///     (especially when it comes to calling member functions)
///   - boost::tuple<T&,T&,...,T&> doesnt quite fit the bill, as it has no way 
///     of iterating through the member data, though its handling of 
///     reference_wrapper is completely transparent.
///
///  reference_array<T,N> is sort of a fusion of boost::tuple<T&,...,T&> and 
///  boost::array<boost::reference_wrapper<T>,N>
///
///  usage:
///   \code
///    T t1;
///    T t2;
///    ...
///    template <Container> void func(Container const& c) 
///    {
///        typename Container::const_iterator itr = c.begin(); end = c.end();
///        ...
///    }
///    ...
///    func(cref_array(t1,t2));
///   \endcode
///
////////////////////////////////////////////////////////////////////////////////
template<class T, std::size_t N>
class reference_array {
public:
    // fixed-size array of elements of type T& (sort-of)
     typedef boost::reference_wrapper<T> ref_type;
     ref_type elems[N];
     typedef typename boost::add_const<T>::type const_type;
public:
    
    typedef typename boost::remove_cv<T>::type value_type;
    typedef T&                                 reference;
    typedef const_type&                        const_reference;
    typedef std::size_t                        size_type;
    typedef std::ptrdiff_t                     difference_type;
    
    template <class TT>
    class reference_iterator 
      : public boost::iterator_facade<
            reference_iterator<TT>
          , TT
          , boost::random_access_traversal_tag
        > {
        ref_type const* ptr;
        TT& dereference() const { return ptr->get(); }
        void advance(difference_type n) { ptr += n; }
        void increment() { ++ptr; }
        void decrement() { --ptr; }
        bool equal(reference_iterator const& o) const { return o.ptr == ptr; }
        friend class boost::iterator_core_access;
    public:
        reference_iterator(ref_type const* ptr) : ptr(ptr) {}
    };
    // type definitions
    
    typedef reference_iterator<T>          iterator;
    typedef reference_iterator<const_type> const_iterator;

    // iterator support
    iterator begin() { return elems; }
    const_iterator begin() const { return elems; }
    iterator end() { return elems+N; }
    const_iterator end() const { return elems+N; }

    // reverse iterator support
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    // operator[]
    reference operator[](size_type i) 
    { 
        BOOST_ASSERT( i < N && "out of range" ); 
        return elems[i].get();
    }
    
    const_reference operator[](size_type i) const 
    {     
        BOOST_ASSERT( i < N && "out of range" ); 
        return elems[i].get(); 
    }

    // at() with range check
    reference at(size_type i) { rangecheck(i); return elems[i].get(); }
    const_reference at(size_type i) const { rangecheck(i); return elems[i].get(); }

    // front() and back()
    reference front() 
    { 
        return elems[0].get(); 
    }
    
    const_reference front() const 
    {
        return elems[0].get();
    }
    
    reference back() 
    { 
        return elems[N-1].get(); 
    }
    
    const_reference back() const 
    { 
        return elems[N-1].get(); 
    }

    // size is constant
    static size_type size() { return N; }
    
    static bool empty() { return false; }
    
    static size_type max_size() { return N; }
    
    enum { static_size = N };

    // swap (note: linear complexity)
    void swap (reference_array<T,N>& y) {
        std::swap_ranges(elems,elems + N,y.elems);
    }

    // assignment with type conversion
    template <typename T2>
    reference_array<T,N>& operator= (const reference_array<T2,N>& rhs) {
        std::copy(rhs.elems,rhs.elems + N, elems);
        return *this;
    }

    // check range (may be private because it is static)
    static void rangecheck (size_type i) {
        if (i >= size()) { 
            throw std::range_error("array<>: index out of range");
        }
    }
    
    friend class reference_iterator<T>;
    friend class reference_iterator<const_type>;

};

template <class T, std::size_t N>
std::size_t hash_value(reference_array<T,N> const& v)
{
    return boost::hash_range(v.begin(),v.end());
}

// comparisons
template<class T, std::size_t N>
bool operator== (const reference_array<T,N>& x, const reference_array<T,N>& y) {
    return std::equal(x.begin(), x.end(), y.begin());
}
template<class T, std::size_t N>
bool operator< (const reference_array<T,N>& x, const reference_array<T,N>& y) {
    return std::lexicographical_compare(x.begin(),x.end(),y.begin(),y.end());
}
template<class T, std::size_t N>
bool operator!= (const reference_array<T,N>& x, const reference_array<T,N>& y) {
    return !(x==y);
}
template<class T, std::size_t N>
bool operator> (const reference_array<T,N>& x, const reference_array<T,N>& y) {
    return y<x;
}
template<class T, std::size_t N>
bool operator<= (const reference_array<T,N>& x, const reference_array<T,N>& y) {
    return !(y<x);
}
template<class T, std::size_t N>
bool operator>= (const reference_array<T,N>& x, const reference_array<T,N>& y) {
    return !(x<y);
}

// global swap()
template<class T, std::size_t N>
inline void swap (reference_array<T,N>& x, reference_array<T,N>& y) {
    x.swap(y);
}

template <class T> 
inline reference_array<T const,10> 
cref_array( T const& t0
          , T const& t1
          , T const& t2
          , T const& t3
          , T const& t4
          , T const& t5
          , T const& t6 
          , T const& t7
          , T const& t8
          , T const& t9 )
{
    reference_array<T const,10>
        retval = {{ boost::cref(t0) 
                  , boost::cref(t1)
                  , boost::cref(t2)
                  , boost::cref(t3)
                  , boost::cref(t4)
                  , boost::cref(t5)
                  , boost::cref(t6)
                  , boost::cref(t7)
                  , boost::cref(t8)
                  , boost::cref(t9) }};
    return retval;
}

template <class T> 
inline reference_array<T,10> 
ref_array ( T& t0
          , T& t1
          , T& t2
          , T& t3
          , T& t4
          , T& t5
          , T& t6 
          , T& t7
          , T& t8
          , T& t9 )
{
    reference_array<T,10>
        retval = {{ boost::ref(t0) 
                  , boost::ref(t1)
                  , boost::ref(t2)
                  , boost::ref(t3)
                  , boost::ref(t4)
                  , boost::ref(t5)
                  , boost::ref(t6)
                  , boost::ref(t7)
                  , boost::ref(t8)
                  , boost::ref(t9) }};
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline reference_array<T const,9> 
cref_array( T const& t0
          , T const& t1
          , T const& t2
          , T const& t3
          , T const& t4
          , T const& t5
          , T const& t6 
          , T const& t7
          , T const& t8 )
{
    reference_array<T const,9>
        retval = {{ boost::cref(t0) 
                  , boost::cref(t1)
                  , boost::cref(t2)
                  , boost::cref(t3)
                  , boost::cref(t4)
                  , boost::cref(t5)
                  , boost::cref(t6)
                  , boost::cref(t7)
                  , boost::cref(t8) }};
    return retval;
}

template <class T> 
inline reference_array<T,9> 
ref_array ( T& t0
          , T& t1
          , T& t2
          , T& t3
          , T& t4
          , T& t5
          , T& t6 
          , T& t7
          , T& t8 )
{
    reference_array<T,9>
        retval = {{ boost::ref(t0) 
                  , boost::ref(t1)
                  , boost::ref(t2)
                  , boost::ref(t3)
                  , boost::ref(t4)
                  , boost::ref(t5)
                  , boost::ref(t6)
                  , boost::ref(t7)
                  , boost::ref(t8) }};
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline reference_array<T const,8> 
cref_array( T const& t0
          , T const& t1
          , T const& t2
          , T const& t3
          , T const& t4
          , T const& t5
          , T const& t6 
          , T const& t7 )
{
    reference_array<T const,8>
        retval = {{ boost::cref(t0) 
                  , boost::cref(t1)
                  , boost::cref(t2)
                  , boost::cref(t3)
                  , boost::cref(t4)
                  , boost::cref(t5)
                  , boost::cref(t6)
                  , boost::cref(t7) }};
    return retval;
}

template <class T> 
inline reference_array<T,8> 
ref_array ( T& t0
          , T& t1
          , T& t2
          , T& t3
          , T& t4
          , T& t5
          , T& t6 
          , T& t7 )
{
    reference_array<T,8>
        retval = {{ boost::ref(t0) 
                  , boost::ref(t1)
                  , boost::ref(t2)
                  , boost::ref(t3)
                  , boost::ref(t4)
                  , boost::ref(t5)
                  , boost::ref(t6)
                  , boost::ref(t7) }};
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline reference_array<T const,7> 
cref_array( T const& t0
          , T const& t1
          , T const& t2
          , T const& t3
          , T const& t4
          , T const& t5
          , T const& t6 )
{
    reference_array<T const,7>
        retval = {{ boost::cref(t0) 
                  , boost::cref(t1)
                  , boost::cref(t2)
                  , boost::cref(t3)
                  , boost::cref(t4)
                  , boost::cref(t5)
                  , boost::cref(t6) }};
    return retval;
}

template <class T> 
inline reference_array<T,7> 
ref_array ( T& t0
          , T& t1
          , T& t2
          , T& t3
          , T& t4
          , T& t5
          , T& t6 )
{
    reference_array<T,7>
        retval = {{ boost::ref(t0) 
                  , boost::ref(t1)
                  , boost::ref(t2)
                  , boost::ref(t3)
                  , boost::ref(t4)
                  , boost::ref(t5)
                  , boost::ref(t6) }};
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline reference_array<T const,6> 
cref_array( T const& t0
          , T const& t1
          , T const& t2
          , T const& t3
          , T const& t4
          , T const& t5 )
{
    reference_array<T const,6>
        retval = {{ boost::cref(t0) 
                  , boost::cref(t1)
                  , boost::cref(t2)
                  , boost::cref(t3)
                  , boost::cref(t4)
                  , boost::cref(t5) }};
    return retval;
}

template <class T> 
inline reference_array<T,6> 
ref_array ( T& t0
          , T& t1
          , T& t2
          , T& t3
          , T& t4
          , T& t5 )
{
    reference_array<T,6>
        retval = {{ boost::ref(t0) 
                  , boost::ref(t1)
                  , boost::ref(t2)
                  , boost::ref(t3)
                  , boost::ref(t4)
                  , boost::ref(t5) }};
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline reference_array<T const,5> 
cref_array( T const& t0
          , T const& t1
          , T const& t2
          , T const& t3
          , T const& t4 )
{
    reference_array<T const,5>
        retval = {{ boost::cref(t0) 
                  , boost::cref(t1)
                  , boost::cref(t2)
                  , boost::cref(t3)
                  , boost::cref(t4) }};
    return retval;
}

template <class T> 
inline reference_array<T,5> 
ref_array ( T& t0
          , T& t1
          , T& t2
          , T& t3
          , T& t4 )
{
    reference_array<T,5>
        retval = {{ boost::ref(t0) 
                  , boost::ref(t1)
                  , boost::ref(t2)
                  , boost::ref(t3)
                  , boost::ref(t4) }};
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline reference_array<T const,4> 
cref_array( T const& t0
          , T const& t1
          , T const& t2
          , T const& t3 )
{
    reference_array<T const,4>
        retval = {{ boost::cref(t0) 
                  , boost::cref(t1)
                  , boost::cref(t2)
                  , boost::cref(t3) }};
    return retval;
}

template <class T> 
inline reference_array<T,4> 
ref_array ( T& t0
          , T& t1
          , T& t2
          , T& t3 )
{
    reference_array<T,4>
        retval = {{ boost::ref(t0) 
                  , boost::ref(t1)
                  , boost::ref(t2)
                  , boost::ref(t3) }};
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline reference_array<T const,3> 
cref_array( T const& t0
          , T const& t1
          , T const& t2 )
{
    reference_array<T const,3>
        retval = {{ boost::cref(t0) 
                  , boost::cref(t1)
                  , boost::cref(t2) }};
    return retval;
}

template <class T> 
inline reference_array<T,3> 
ref_array ( T& t0
          , T& t1
          , T& t2 )
{
    reference_array<T,3>
        retval = {{ boost::ref(t0) 
                  , boost::ref(t1)
                  , boost::ref(t2) }};
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline reference_array<T const,2> 
cref_array( T const& t0
          , T const& t1 )
{
    reference_array<T const,2>
        retval = {{ boost::cref(t0), boost::cref(t1) }};
    return retval;
}

template <class T> 
inline reference_array<T,2> 
ref_array ( T& t0
          , T& t1 )
{
    reference_array<T,2>
        retval = {{ boost::ref(t0), boost::ref(t1) }};
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline reference_array<T const,1> 
cref_array( T const& t0 )
{
    reference_array<T const,1>
        retval = {{ boost::cref(t0) }};
    return retval;
}

template <class T> 
inline reference_array<T,1> 
ref_array ( T& t0 )
{
    reference_array<T,1>
        retval = {{ boost::ref(t0) }}; //FIXME: ref_array.hpp:577: warning: missing braces around initializer for 'boost::reference_wrapper<sbmt::indexed_token> [1]'

    return retval;
}

////////////////////////////////////////////////////////////////////////////////
 
} // namespace sbmt
# endif //     SBMT__HASH__REF_ARRAY_HPP

