# if ! defined(GUSC__GENERATOR__PEEKABLE_GENERATOR_FACADE_HPP)
# define       GUSC__GENERATOR__PEEKABLE_GENERATOR_FACADE_HPP

# include <boost/iterator/iterator_facade.hpp>
# include <boost/optional.hpp>
# include <boost/utility/result_of.hpp>

namespace gusc {
    
struct generator_access {
    template <class Result, class G>
    static Result peek(G const& g)
    {
        return g.peek();
    }
    
    template <class G>
    static bool more(G const& g)
    {
        return g.more();
    }
    
    template <class Result, class G>
    static Result next(G& g)
    {
        return g.next();
    }
    
    template <class G>
    static void pop(G& g)
    {
        g.pop();
    }
};

template <class Derived, class Value, class Reference = Value const&>
class peekable_generator_facade
  : public boost::iterator_facade<
      peekable_generator_facade<Derived,Value>
    , Value
    , std::input_iterator_tag
    , Reference
    > {
public:
    typedef peekable_generator_facade<Derived,Value> generator_facade_;
    typedef Value result_type;
    typedef Reference reference;
    operator bool() const
    {
        return generator_access::more(derived());
    }
    
    result_type operator()()
    {
        result_type ret(generator_access::peek<reference>(derived()));
        generator_access::pop(derived());
        return ret;
    }
    
private:
    friend class boost::iterator_core_access;
    
    bool equal(peekable_generator_facade const& other) const
    {
        return !generator_access::more(derived()) and
               !generator_access::more(other.derived());
    }
    
    reference dereference() const
    {
        return generator_access::peek<reference>(derived());
    }
    
    void increment()
    {
        generator_access::pop(derived());
    }
    
    Derived const& derived() const { return static_cast<Derived const&>(*this); }
    Derived& derived() { return static_cast<Derived&>(*this); }
};

////////////////////////////////////////////////////////////////////////////////



template <class Gen>
class peekable_generator 
: public peekable_generator_facade< peekable_generator<Gen>
                                  , typename boost::result_of<Gen()>::type
                                  >
{
    typedef typename boost::result_of<Gen()>::type res_type;
    
    bool more() const { return !(!val); }
    
    res_type const& peek() const { return val.get(); }
    
    void pop() 
    {
        if (gen) val = gen();
        else val = boost::none_t();
    } 
    friend class generator_access;
    Gen gen;
    boost::optional<res_type> val;
public:
    template <class U>
    explicit peekable_generator(U const& u) 
    : gen(u)
    {
        pop();
    }
};

template <class Gen>
peekable_generator<Gen>
make_peekable(Gen const& g)
{
    return peekable_generator<Gen>(g);
}

} // namespace gusc

# endif //     GUSC__GENERATOR__PEEKABLE_GENERATOR_FACADE_HPP
