# if ! defined(SBMT__SEARCH__LAZY__INDEXED_VARRAY_HPP)
# define       SBMT__SEARCH__LAZY__INDEXED_VARRAY_HPP

# include <gusc/varray.hpp>
# include <sbmt/hash/oa_hashtable.hpp>
# include <boost/shared_array.hpp>
# include <boost/type_traits.hpp>

namespace sbmt { namespace lazy {

////////////////////////////////////////////////////////////////////////////////

template < class Value
         , class KeyExtract
         , class Hasher = boost::hash<
                              typename boost::remove_reference<
                                  typename boost::result_of<
                                      KeyExtract(Value)
                                  >::type
                              >::type
                          >
         , class Equal = gusc::equal_to
         >
class indexed_varray
{
    struct key_extract {
        typedef typename boost::result_of<KeyExtract(Value)>::type result_type;
        KeyExtract kev;
        gusc::shared_varray<Value> v;
        result_type operator()(size_t x) const { return kev(v[x]); }
        key_extract(KeyExtract const& kev, gusc::shared_varray<Value> const& v)
        : kev(kev)
        , v(v) {}
    };
    
    typedef gusc::shared_varray<Value> impl_;
    typedef oa_hashtable<size_t,key_extract,Hasher,Equal> index_;
    
    impl_ v;
    index_ h;
    
    typedef typename boost::remove_reference<
                       typename boost::remove_const<
                         typename boost::result_of<KeyExtract(Value)>::type
                       >::type
                     >::type key_type;
                     
public:
    typedef typename impl_::value_type value_type;
    typedef typename impl_::const_reference reference;
    typedef reference const_reference;
    typedef typename impl_::const_iterator iterator;
    typedef iterator const_iterator;
    typedef typename impl_::size_type size_type;
    typedef typename impl_::difference_type difference_type;
    typedef typename impl_::pointer pointer;
    
    size_type size() const { return v.size(); }
    bool empty() const { return v.empty(); }
    
    const_iterator begin() const { return v.begin(); }
    const_iterator end() const { return v.end(); }
    
    const_reference operator[](size_type pos) const { return v[pos]; }
    
    template <class Key>
    typename boost::enable_if< boost::is_same<Key,key_type>, const_reference >::type
        operator[](Key const& key) const { return v[*(h.find(key))]; }
        
    template <class Key>
    const_iterator find(Key const& key) const
    {
        typename index_::const_iterator pos = h.find(key);
        if (pos == h.end()) return end();
        else return begin() + *pos;
    }
    
    const_reference at(size_type pos) const { return v.at(pos); }
    
    const_reference front() const { return v.front(); }
    const_reference back() const { return v.back(); }
    
    explicit indexed_varray( gusc::shared_varray<Value> const& v
                           , KeyExtract const& kev = KeyExtract()
                           , Hasher const& hasher = Hasher()
                           , Equal const& eql = Equal()
                           )
    : v(v) 
    , h(v.size(),key_extract(kev,v),hasher,eql) 
    {
        for (size_t x = 0; x != v.size(); ++x) h.insert(x);
        assert(h.size() == v.size());
    }
};

////////////////////////////////////////////////////////////////////////////////

template < class Value
         , class KeyExtract
         , class Hasher = boost::hash<
                               typename boost::remove_reference<
                                   typename boost::result_of<
                                       KeyExtract(Value)
                                   >::type
                               >::type
                           >
         , class Equal = gusc::equal_to
         >
class shared_indexed_varray
{
    typedef indexed_varray<Value,KeyExtract,Hasher,Equal> impl_;
    boost::shared_ptr<impl_> impl;
public:
    typedef typename impl_::value_type value_type;
    typedef typename impl_::const_reference const_reference;
    typedef typename impl_::reference reference;
    typedef typename impl_::const_iterator const_iterator;
    typedef typename impl_::iterator iterator;
    typedef typename impl_::size_type size_type;
    typedef typename impl_::difference_type difference_type;
    typedef typename impl_::pointer pointer;
    
    size_type size() const { return bool(impl) ? impl->size() : 0; }
    bool empty() const { return bool(impl) ? impl->empty() : true; }
    
    const_iterator begin() const { return bool(impl) ? impl->begin() : NULL; }
    const_iterator end() const { return bool(impl) ? impl->end() : NULL; }
    
    const_reference operator[](size_type pos) const { return (*impl)[pos]; }
    
    template <class Key>
    const_reference
    operator[](Key const& key) const { return (*impl)[key]; }
        
    template <class Key>
    const_iterator find(Key const& key) const { return bool(impl) ? impl->find(key) : NULL; }
    
    const_reference at(size_type pos) const { return impl->at(pos); }
    
    const_reference front() const { return impl->front(); }
    const_reference back() const { return impl->back(); }
    
    explicit shared_indexed_varray( gusc::shared_varray<Value> const& v
                                  , KeyExtract const& kev = KeyExtract()
                                  , Hasher const& hasher = Hasher()
                                  , Equal const& eql = Equal()
                                  )
    : impl(new impl_(v,kev,hasher,eql)){}
    
    template <class Container>
    explicit shared_indexed_varray( Container const& v
                                  , KeyExtract const& kev = KeyExtract()
                                  , Hasher const& hasher = Hasher()
                                  , Equal const& eql = Equal()
                                  )
    : impl(new impl_(gusc::shared_varray<Value>(v),kev,hasher,eql)){}
    
    shared_indexed_varray() {}
};

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::lazy

# endif //     SBMT__SEARCH__LAZY__INDEXED_VARRAY_HPP
