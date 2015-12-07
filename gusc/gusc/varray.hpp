# if ! defined(GUSC__VARRAY_HPP)
# define       GUSC__VARRAY_HPP

# include <cstdlib>
# include <algorithm>
# include <new>
# include <boost/range.hpp>
# include <boost/utility/enable_if.hpp>
# include <boost/type_traits.hpp>
# include <boost/serialization/split_member.hpp>
# include <boost/serialization/access.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/functional/hash.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
///
///  like vector, except no resizing.  in practice, saves you a wordsize, so
///  potentially useful if storing many small (possibly empty) instances.
///
////////////////////////////////////////////////////////////////////////////////
template <class T, class A = std::allocator<T> >
class varray
  : A::template rebind<T>::other
{
    
public:
    typedef typename A::template rebind<T>::other allocator_type;
    typedef T value_type;
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::const_reference const_reference;
    typedef typename allocator_type::pointer iterator;
    typedef typename allocator_type::const_pointer const_iterator;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;
    typedef iterator pointer;
    typedef const_iterator const_pointer;
    
    size_type size() const { return sz; }
    bool empty() const { return size() == 0; }
    
    iterator begin() { return array; }
    iterator end() { return array + size(); }
    const_iterator begin() const { return array; }
    const_iterator end() const { return array + size(); }
    
    reference operator[](size_type pos) { return array[pos]; }
    const_reference operator[](size_type pos) const { return array[pos]; }
    
    reference at(size_type pos) { return array[pos]; }
    const_reference at(size_type pos) const { return array[pos]; }
    
    reference front() { return array[0]; }
    reference back() { return array[sz-1]; }
    
    const_reference front() const { return array[0]; }
    const_reference back() const { return array[sz-1]; }
    
    explicit varray(A const& alloc = A()) 
      : allocator_type(alloc)
      , sz(0)
      , array(0) {}
    
    explicit varray(size_type s, const_reference t = T(), A const& alloc = A()) 
      : allocator_type(alloc)
      , sz(s)
      , array(allocator_type::allocate(s))
    {
        for (size_type i = 0; i != sz; ++i) { 
            allocator_type::construct(array + i,t); 
        }
    }
    
    template <class I>
    varray(I itr, I end, A const& alloc = A())
      : allocator_type(alloc)
      , sz(std::distance(itr,end))
      , array(allocator_type::allocate(sz))
    {
        copy_construct(itr,end,array);
    }
    
    ~varray()
    {
        for (size_type i = sz; i != 0; --i) { 
            allocator_type::destroy(array + (i - 1)); 
        }
        if (array != NULL) allocator_type::deallocate(array,sz);
    }
    
    void swap(varray& other) 
    {
        std::swap(sz,other.sz);
        std::swap(array,other.array);
    }
    
    varray(varray const& other)
      : allocator_type(other.get_allocator())
      , sz(other.sz)
      , array(allocator_type::allocate(sz))
    {
        copy_construct(other.begin(),other.end(),array);
    }
    
    varray& operator=(varray const& other)
    {
        varray(other).swap(*this);
        return *this;
    }
    
    template <class Range>
    varray( Range const& v
          , A const& alloc = A()
          , typename boost::enable_if< boost::has_range_const_iterator<Range> >::type* = 0 )
//          , typename boost::disable_if< boost::is_integral<Range> >::type* = 0)
      : allocator_type(alloc)
      , sz(boost::size(v))
      , array(allocator_type::allocate(sz))
    {
        copy_construct(boost::begin(v),boost::end(v),array);
    }
    
    template <class Range> typename 
    boost::disable_if< 
      boost::is_integral<Range>
    , varray
    >::type& operator=(Range const& other)
    {
        varray(other,allocator_type(*this)).swap(*this);
        return *this;
    }
    allocator_type const& get_allocator() const { return *this; }
private:
    friend class boost::serialization::access;
    
    template <class I1, class I2>
    void copy_construct(I1 itr, I1 end, I2 into) 
    {
        for (; itr != end; ++itr, ++into) allocator_type::construct(into,*itr);
    }
    
    template <class Archive>
    void save(Archive& ar, unsigned int version) const
    {
        ar << sz;
        for (size_type i = 0; i != sz; ++i) ar << at(i);
    }  
    

    template <class Archive>
    void load(Archive& ar, unsigned int version)
    {
        size_type s;
        ar >> s;
        varray v(s,value_type(),get_allocator());
        for (size_type i = 0; i != s; ++i) ar >> v[i];
        this->swap(v);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    size_type sz;
    pointer array;
};

////////////////////////////////////////////////////////////////////////////////

template <class T, class A>
bool operator==(varray<T,A> const& v1, varray<T,A> const& v2)
{
    return (v1.size() == v2.size()) and 
           std::equal(v1.begin(),v1.end(),v2.begin());
}

template <class T, class A>
size_t hash_value(varray<T,A> const& v)
{
    return boost::hash_range(v.begin(),v.end());
}

////////////////////////////////////////////////////////////////////////////////

template <class T,class A>
bool operator< (varray<T,A> const& v1, varray<T,A> const& v2)
{
    return std::lexicographical_compare( v1.begin()
                                       , v1.end()
                                       , v2.begin()
                                       , v2.end() );
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A = std::allocator<T> >
class shared_varray
{
public:
    typedef T value_type;
    typedef T& reference;
    typedef T const& const_reference;
    typedef T* iterator;
    typedef T const* const_iterator;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T* pointer;
    
    size_type size() const 
    {   
        if (not impl) return 0; 
        else return impl->size(); 
    }
    bool empty() const { return size() == 0; }
    
    iterator begin() 
    {
        if (not impl) return 0; 
        else return impl->begin(); 
    }
    
    iterator end() 
    { 
        if (not impl) return 0; 
        else return impl->end(); 
    }
    
    const_iterator begin() const 
    { 
        if (not impl) return 0; 
        else return impl->begin(); 
    }
    
    const_iterator end() const 
    { 
        if (not impl) return 0; 
        else return impl->end(); 
    }
    
    reference operator[](size_type pos) { return impl->operator[](pos); }
    const_reference operator[](size_type pos) const { return impl->operator[](pos); }
    
    reference at(size_type pos) { return impl->at(pos); }
    const_reference at(size_type pos) const { return impl->at(pos); }
    
    reference front() { return impl->front(); }
    reference back() { return impl->back(); }
    
    const_reference front() const { return impl->front(); }
    const_reference back() const { return impl->back(); }
    
    shared_varray() {}
    
    template <class X>
    explicit shared_varray(X const& x) : impl(new varray<T,A>(x)) {}
    
    template <class X, class Y>
    shared_varray(X const& x, Y const& y) : impl(new varray<T,A>(x,y)) {}
    
    shared_varray(shared_varray const& other) : impl(other.impl) {}
    
    template <class X>
    shared_varray& operator=(X const& other) 
    {
        shared_varray(other).swap(*this);
        return *this;
    }
    
    void swap(shared_varray& other) 
    {
        impl.swap(other.impl);
    }
    
    bool operator==(shared_varray const& other) const
    {
        if (not impl or not other.impl) return this == &other;
        else return *impl == *other.impl;
    }
    
    bool operator< (shared_varray const& other) const
    {
        if (not impl or not other.impl) return this < &other;
        else return *impl < *other.impl;
    }
    
    varray<T,A> const* get() const 
    {
        return impl.get();
    }
   
private:
    boost::shared_ptr< varray<T,A> > impl;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__VARRAY_HPP
