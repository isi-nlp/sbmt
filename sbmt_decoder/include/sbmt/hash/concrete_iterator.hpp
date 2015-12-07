# if ! defined(SBMT__HASH__CONCRETE_ITERATOR_HPP)
# define       SBMT__HASH__CONCRETE_ITERATOR_HPP

# include <boost/shared_ptr.hpp>
# include <boost/iterator/iterator_facade.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template < class Value
         , class Reference = Value&
         , class Difference = std::ptrdiff_t
         >
class concrete_forward_iterator 
: 
public boost::iterator_facade<
    concrete_forward_iterator<
        Value
      , Reference
      , Difference
    >
  , Value
  , boost::forward_traversal_tag
  , Reference
  , Difference
> {
public:
    concrete_forward_iterator() {}
    
    concrete_forward_iterator(concrete_forward_iterator const& other)
     : pimpl(other.pimpl->clone()) {}
     
    concrete_forward_iterator& operator=(concrete_forward_iterator const& other)
    {
        boost::shared_ptr<iterator_iface> p(other.pimpl->clone());
        pimpl = p;
        return *this;
    }
     
    template <class ItrT>
    concrete_forward_iterator(ItrT const& itr)
      : pimpl(new iterator_impl<ItrT>(itr)) {}
      
    template <class ItrT>
    concrete_forward_iterator& operator=(ItrT const& itr)
    {
        boost::shared_ptr<iterator_iface> p(new iterator_impl<ItrT>(itr));
        pimpl = p;
        return *this;
    }
private:
    Reference dereference() const { return pimpl->dereference(); }
    void increment() { pimpl->increment(); }
    bool equal(concrete_forward_iterator const& other) const
    { return pimpl->equal(*(other.pimpl)); }
    friend class boost::iterator_core_access;
    
    ////////////////////////////////////////////////////////////////////////////
    
    struct iterator_iface {
        virtual Reference dereference() const = 0;
        virtual bool equal(iterator_iface const& other) const = 0;
        virtual void increment() = 0;
        virtual iterator_iface* clone() const = 0;
        virtual ~iterator_iface() {}
    };
    
    ////////////////////////////////////////////////////////////////////////////
    
    template <class Iterator>
    class iterator_impl : public iterator_iface {
        typedef iterator_impl<Iterator> self_t;
        Iterator itr;
    public:
        iterator_impl(Iterator itr) : itr(itr) {}
        virtual void increment() { ++itr; }
        virtual Reference dereference() const { return *itr; }
        virtual bool equal(iterator_iface const& other) const
        { return itr == static_cast<self_t const&>(other).itr; }
        virtual iterator_iface* clone() const { return new self_t(itr); }
        virtual ~iterator_impl() {}
    };
    
    ////////////////////////////////////////////////////////////////////////////
    
    boost::shared_ptr<iterator_iface> pimpl;
};

template <class It>
std::pair<
    concrete_forward_iterator<typename std::iterator_traits<It>::value_type>
  , concrete_forward_iterator<typename std::iterator_traits<It>::value_type>
>
make_concrete_range(It const& i,It const& end)
{
    typedef typename std::iterator_traits<It>::value_type value_type;
    typedef concrete_forward_iterator<value_type> iterator;
    return std::pair<iterator,iterator>(i,end);
}


template <class It>
std::pair<
    concrete_forward_iterator<typename std::iterator_traits<It>::value_type>
  , concrete_forward_iterator<typename std::iterator_traits<It>::value_type>
>
make_concrete_range(std::pair<It,It> const& range)
{
    return make_concrete_range(range.first,range.second);
}



////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__HASH__CONCRETE_ITERATOR_HPP
