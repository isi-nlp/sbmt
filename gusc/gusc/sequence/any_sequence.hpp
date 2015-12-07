# if ! defined(GUSC__SEQUENCE__ANY_SEQUENCE_HPP)
# define       GUSC__SEQUENCE__ANY_SEQUENCE_HPP

# include <boost/any_iterator/any_iterator.hpp>
# include <boost/range.hpp>

namespace gusc {

template <class Value>
class any_sequence {
public:
    typedef Value value_type;
    typedef value_type const& reference;
    typedef boost::any_iterator<value_type const, boost::random_access_traversal_tag>
            iterator;
    typedef iterator const_iterator;
    
    iterator begin() const { return pimpl->begin(); }
    iterator end() const { return pimpl->end(); }
    reference operator[](size_t n) const { return pimpl->at(n); }
    
    void swap(any_sequence& other)
    {
        std::swap(pimpl,other.pimpl);
    }
    
    any_sequence() {}
    
    any_sequence(any_sequence const& other)
     : pimpl(other.pimpl->clone()) {}
     
    template <class Impl>
    explicit any_sequence(Impl const& impl)
     : pimpl(new impl_derived<Impl>(impl)) {}
    
    any_sequence& operator=(any_sequence const& other)
    {
        any_sequence(other).swap(*this);
        return *this;
    }
    
private:
    struct impl_base {
        virtual iterator begin() = 0;
        virtual iterator end() = 0;
        virtual reference at(size_t n) = 0;
        virtual impl_base* clone() = 0;
        virtual ~impl_base() {}
    };
    
    template <class Impl>
    struct impl_derived : impl_base {
        Impl impl;
        impl_derived(Impl const& impl) : impl(impl) {}
        impl_base* clone() { return new impl_derived(impl); }
        virtual iterator begin() { return iterator(boost::begin(impl)); }
        virtual iterator end() { return iterator(boost::end(impl)); }
        virtual reference at(size_t n) { return impl[n]; }
    };
    
    boost::shared_ptr<impl_base> pimpl;
};

} // namespace gusc

# endif //     GUSC__SEQUENCE__ANY_SEQUENCE_HPP
