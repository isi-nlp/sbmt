# if ! defined(GUSC__GENERATOR__ANY_GENERATOR_HPP)
# define       GUSC__GENERATOR__ANY_GENERATOR_HPP

# include <boost/shared_ptr.hpp>
# include <boost/iterator/iterator_facade.hpp>
# include <boost/utility/enable_if.hpp>
# include <boost/type_traits/is_same.hpp>

namespace gusc {

struct generator_tag {};
struct iterator_tag {};

template <class Derived, class Value, class Tag>
struct any_generator_base {};

template <class Derived, class Value>
class any_generator_base<Derived,Value,iterator_tag>
: public boost::iterator_facade<
    Derived
  , Value
  , std::input_iterator_tag
  , Value const&>
{
    friend class boost::iterator_core_access;

    void increment()
    {
        static_cast<Derived*>(this)->pimpl->pop();
    }

    Value const& dereference() const
    {
        return static_cast<Derived const*>(this)->pimpl->peek();
    }

    bool equal(any_generator_base const& other) const
    {
        return (not static_cast<Derived const&>(*this)) and
               (not static_cast<Derived const&>(other));
    }
};

namespace detail {

}

////////////////////////////////////////////////////////////////////////////////
///
///  any_generator<Value,Tag = generator_tag> is type erasure of generator
///  concept, returning values of type Value.
///  if Tag == iterator_tag, then any_generator is additionally an input
///  iterator that dereferences Value const&.  in this case, underlying generator
///  must also be a compatible input iterator.
///
////////////////////////////////////////////////////////////////////////////////
template <class Value, class Tag = generator_tag>
class any_generator
: public any_generator_base<any_generator<Value,Tag>, Value, Tag> {
public:
    typedef Value result_type;

    any_generator() {}

    any_generator(any_generator const& o) : pimpl(o.pimpl) {}

    any_generator& operator=(any_generator const& o)
    {
        pimpl = o.pimpl;
        return *this;
    }

    template <class Generator>
    any_generator(Generator const& generator)
      : pimpl(new holder<Value,Tag,Generator>(generator)) {}

    template <class Generator>
    any_generator& operator=(Generator const& g)
    {
        any_generator(g).swap(*this);
        return *this;
    }

    operator bool() const { return bool(pimpl) && pimpl->more(); }
    Value  operator()() { return pimpl->next(); }

    void swap(any_generator& o)
    {
        pimpl.swap(o.pimpl);
    }

private:

    template <class V>
    struct placeholder_base {
        virtual bool more() const = 0;
        virtual V next() = 0;
        virtual ~placeholder_base() {}
    };

    template <class V, class T>
    struct placeholder : placeholder_base<V> {};

    template <class V>
    struct placeholder<V,iterator_tag> : placeholder_base<V> {
        virtual void pop() = 0;
        virtual V const& peek() const = 0;
    };

    template <class V, class T, class G>
    struct holder_base : placeholder<V,T> {
        typedef holder_base<V,T,G> holder_base_;
        virtual bool more() const { return bool(g); }
        virtual V next() { return g(); }
        holder_base(G const& g)
        : g(g) {}
        G g;
    };

    template <class V, class T, class G>
    struct holder : holder_base<V,T,G> {
        holder(G const& g) : holder_base<V,T,G>(g) {}
    };

    template <class V,class G>
    struct holder<V,iterator_tag,G> : holder_base<V,iterator_tag,G> {
        holder(G const& g) : holder_base<V,iterator_tag,G>(g) {}
        virtual void pop() { ++holder_base<V,iterator_tag,G>::g; }
        virtual V const& peek() const { return *holder_base<V,iterator_tag,G>::g; }
    };

    template <class D, class V, class T>
    friend class any_generator_base;

  boost::shared_ptr< placeholder<Value,Tag> > pimpl; //TODO: intrusive refcount?
};

////////////////////////////////////////////////////////////////////////////////

template <class V, class T>
void swap(any_generator<V,T>& g1, any_generator<V,T>& g2)
{
//FIXME: why is this empty?
}

template <class V> struct null_generator {
  //may be iterator_tag or generator_tag.
  operator bool() const {
    return false;
  }
  V operator()() const {
    return V();
  }
  V operator*() const {
    return V();
  }
  void operator++() {  }
};

template <class V>
any_generator<V,generator_tag> make_null_generator() {
  return null_generator<V>();
}

} // namespace gusc

# endif //     GUSC__GENERATOR__ANY_GENERATOR_HPP
