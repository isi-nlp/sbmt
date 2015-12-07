# if ! defined(GUSC__PHOENIX_HELPERS_HPP)
# define       GUSC__PHOENIX_HELPERS_HPP

# include <boost/spirit/include/phoenix1_functions.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/utility/enable_if.hpp>
# include <boost/type_traits.hpp>
# include <boost/tuple/tuple.hpp>

////////////////////////////////////////////////////////////////////////////////

namespace gusc {
    
struct insert_impl
{
    template <class C, class P>
    struct result { typedef void type; };
    
    template <class C, class P>
    void operator()(C& container, P const& p) const
    {
        container.insert(p);
    }
};

struct first_impl
{
    template <class P>
    struct result { typedef typename P::first_type& type; };
    
    template <class P>
    typename result<P>::type operator()(P& p) const
    {
        return p.first;
    }
};

struct second_impl
{
    template <class P>
    struct result { typedef typename P::second_type& type; };
    
    template <class P>
    typename result<P>::type operator()(P& p) const
    {
        return p.second;
    }
};

struct lex_cast_impl
{
    template <class T, class S>
    struct result { typedef void type; };
    
    template <class T, class S>
    void operator()(T& target, S const& source) const
    {
        target = boost::lexical_cast<T,S>(source);
    }
};

struct size_impl
{
    template <class S>
    struct result { typedef std::size_t type; };
    
    template <class S>
    std::size_t operator()(S const& s) const
    {
        return s.size();
    }
};

struct begin_impl
{
    template <class C, class Enable = void>
    struct result {};
    
    template <class C>
    struct result<C,typename boost::enable_if< boost::is_const<C> >::type >
    { typedef typename C::const_iterator type; };
    
    template <class C>
    struct result<C,typename boost::disable_if< boost::is_const<C> >::type >
    { typedef typename C::iterator type; };
    
    template <class C>
    typename result<C>::type operator()(C& c) const
    { return c.begin(); }
};

struct cbegin_impl
{
    template <class C>
    struct result{ 
        typedef typename boost::remove_cv<C>::type::const_iterator type; 
    };
    
    template <class C>
    typename C::const_iterator operator()(C const& c) const
    { return c.begin(); } 
};

struct cend_impl
{
    template <class C>
    struct result{ 
        typedef typename boost::remove_cv<C>::type::const_iterator type; 
    };
    
    template <class C>
    typename C::const_iterator operator()(C const& c) const
    { return c.end(); } 
};

struct end_impl
{
    template <class C, class Enable = void>
    struct result {};
    
    template <class C>
    struct result<C,typename boost::enable_if< boost::is_const<C> >::type>
    { typedef typename C::const_iterator type; };
    
    template <class C>
    struct result<C,typename boost::disable_if< boost::is_const<C> >::type>
    { typedef typename C::iterator type; };
    
    template <class C>
    typename result<C>::type operator()(C& c) const
    { return c.end(); }
};

struct push_back_impl
{
    template <class C, class E>
    struct result { typedef void type; };
    
    template <class C, class E>
    void operator()(C& c, E const& e) const
    { c.push_back(e); }
};

struct push_front_impl
{
    template <class C, class E>
    struct result { typedef void type; };
    
    template <class C, class E>
    void operator()(C& c, E const& e) const
    { c.push_front(e); }
};

struct pop_back_impl
{
    template <class C, class E>
    struct result { typedef void type; };
    
    template <class C, class E>
    void operator()(C& c, E const& e) const
    { c.pop_back(e); }
};

struct pop_front_impl
{
    template <class C, class E>
    struct result { typedef void type; };
    
    template <class C, class E>
    void operator()(C& c, E const& e) const
    { c.pop_front(e); }
};

struct push_impl
{
    template <class C, class E>
    struct result { typedef void type; };
    
    template <class C, class E>
    void operator()(C& c, E const& e) const
    { c.push(e); }
};

struct pop_impl
{
    template <class C, class E>
    struct result { typedef void type; };
    
    template <class C, class E>
    void operator()(C& c, E const& e) const
    { c.pop(e); }
};

struct front_impl
{
    template <class C>
    struct result { typedef typename C::const_reference type; };
    
    template <class C>
    void operator()(C& c) const
    { c.front(); }
};

struct back_impl
{
    template <class C>
    struct result { typedef typename C::const_reference type; };
    
    template <class C>
    void operator()(C& c) const
    { c.back(); }
};

struct top_impl
{
    template <class C>
    struct result { typedef typename C::const_reference type; };
    
    template <class C>
    void operator()(C& c) const
    { c.top(); }
};

struct tuple_get_impl
{
    template <class N, class T>
    struct result { 
        typedef typename boost::tuples::element<N::value,T&>::type type; 
    };
    
    template <class N, class T>
    typename result<N,T>::type operator()(N n, T& t) const
    {
        return t.template get<N::value>();
    }
};

template <unsigned int N>
struct nth {
    enum { value = N };
};

namespace {

nth<0> _i0 = nth<0>();
nth<1> _i1 = nth<1>();
nth<2> _i2 = nth<2>();
nth<3> _i3 = nth<3>();
nth<4> _i4 = nth<4>();
nth<5> _i5 = nth<5>();
nth<6> _i6 = nth<6>();
nth<7> _i7 = nth<7>();
nth<8> _i8 = nth<8>();
nth<9> _i9 = nth<9>();

/// lambda expressions for common standard library and boost library 
/// manipulations
///\{
phoenix::function<push_back_impl> push_back_ = push_back_impl();
phoenix::function<push_front_impl> push_front_ = push_front_impl();
phoenix::function<pop_back_impl> pop_back_ = pop_back_impl();
phoenix::function<pop_front_impl> pop_front_ = pop_front_impl();
phoenix::function<push_impl> push_ = push_impl();
phoenix::function<pop_impl> pop_ = pop_impl();
phoenix::function<back_impl> back_ = back_impl();
phoenix::function<front_impl> front_ = front_impl();
phoenix::function<top_impl> top_ = top_impl();
phoenix::function<begin_impl> begin_ = begin_impl();
phoenix::function<end_impl> end_ = end_impl();
phoenix::function<cbegin_impl> cbegin_ = cbegin_impl();
phoenix::function<cend_impl> cend_ = cend_impl();
phoenix::function<insert_impl> insert_ = insert_impl();
phoenix::function<lex_cast_impl> lex_cast_ = lex_cast_impl();
phoenix::function<first_impl> first_ = first_impl();
phoenix::function<second_impl> second_ = second_impl();
phoenix::function<size_impl> size_ = size_impl();
phoenix::function<tuple_get_impl> tuple_get_ = tuple_get_impl();
///\}

} // anonymous namespace

} // namespace gusc

# endif //     GUSC__PHOENIX_HELPERS_HPP
