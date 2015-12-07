# if !defined(GUSC__SHARED_ANY_HPP)
# define      GUSC__SHARED_ANY_HPP

# include <boost/shared_ptr.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
///
///  just like boost::any, but optimized for const objects.  ownership is shared
///  your const object better really be const to guarantee that shared ownership
///  is okay.
///  
///  also, does not use typeinfo.  unsafe casts are the default.
///
////////////////////////////////////////////////////////////////////////////////
class const_any {
public:
    const_any() {}
    
    const_any(const_any const& c) : pv(c.pv) {}
    
    const_any& operator=(const_any const& c)
    {
        pv = c.pv;
        return *this;
    }
    
    void swap(const_any& c)
    {
        std::swap(pv,c.pv);
    }
    
    template <class Value>
    const_any(Value const& v) : pv(new holder<Value>(v)) {}
    
    template <class Value>
    const_any& operator=(Value const& v) 
    {
        const_any(v).swap(*this);
        return *this;
    }
    
    bool empty() const { return !(pv); }
private:
    template <class Value>
    friend Value const& any_cast(const_any const&);
    
    struct placeholder {};
    
    template <class Value>
    struct holder : placeholder {
        holder(Value const& v) : v(v) {}
        Value v;
    };
    
    boost::shared_ptr<placeholder> pv;
};

////////////////////////////////////////////////////////////////////////////////

template <class Value>
Value const& any_cast(const_any const& c)
{
    return static_cast<const_any::holder<Value> const*>(c.pv.get())->v;
}

template <class Value>
Value const* any_cast(const_any const* c)
{
    return &any_cast<Value>(*c);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif  //   GUSC__SHARED_ANY_HPP
