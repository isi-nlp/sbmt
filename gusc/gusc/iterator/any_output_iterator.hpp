# if ! defined(GUSC__ITERATOR__ANY_OUTPUT_ITERATOR_HPP)
# define       GUSC__ITERATOR__ANY_OUTPUT_ITERATOR_HPP

# include <boost/iterator/iterator_facade.hpp>
# include <boost/function.hpp>
# include <boost/shared_ptr.hpp>

namespace gusc {

template <class Value>
struct output_proxy {
    template <class F>
    explicit output_proxy(F const& f) : f(f) {}
    output_proxy& operator=(Value const& v)
    {
        f(v);
        return *this;
    }
    boost::function<void(Value const&)> f;
};

template <class Value>
class any_output_iterator 
  : public boost::iterator_facade< any_output_iterator<Value>
                                 , void
                                 , std::output_iterator_tag
                                 , output_proxy<Value>
                                 > {
public:
    any_output_iterator() {}

    template <class Iterator>
    explicit any_output_iterator(Iterator const& itr)
      : pimpl(new itr_holder<Iterator>(itr)) {}
      
    any_output_iterator(any_output_iterator const& other)
      : pimpl(other.pimpl ? other.pimpl->clone() : 0) {}
      
    void swap(any_output_iterator& other) { other.pimpl.swap(pimpl); }

    any_output_iterator& operator=(any_output_iterator const& other)
    {
        any_output_iterator(other).swap(*this);
        return *this;
    }
    
    template <class Iterator>
    Iterator any_iterator_cast() const 
    { return static_cast<itr_holder<Iterator> const*>(pimpl.get())->itr; }
private:
    friend class boost::iterator_core_access;
    
    struct itr_placeholder {
        virtual output_proxy<Value> dereference() = 0;
        virtual void increment() = 0;
        virtual itr_placeholder* clone() const = 0;
        virtual ~itr_placeholder() {}
    };
    
    template <class Iterator> 
    struct itr_out_func {
        Iterator* itr;
        typedef void result_type;
        void operator()(Value const& v)
        {
            *(*itr) = v;
        }
        itr_out_func(Iterator& itr) : itr(&itr) {}
    };
    
    template <class Iterator> struct itr_holder : itr_placeholder {
        Iterator itr;
        virtual output_proxy<Value> dereference()
        {
            return output_proxy<Value>(itr_out_func<Iterator>(itr));
        }
        explicit itr_holder(Iterator const& itr) : itr(itr) {}
        virtual void increment() { ++itr; }
        virtual itr_placeholder* clone() const { return new itr_holder<Iterator>(*this); }
    };
    
    boost::shared_ptr<itr_placeholder> pimpl;
    
    output_proxy<Value> dereference() const { return pimpl->dereference(); }
    void increment() { return pimpl->increment(); }
};

} // namespace gusc

# endif //     GUSC__ITERATOR__ANY_OUTPUT_ITERATOR_HPP
