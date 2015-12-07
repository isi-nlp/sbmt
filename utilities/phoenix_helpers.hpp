# if ! defined(UTILITIES__PHOENIX_HELPERS_HPP)
# define       UTILITIES__PHOENIX_HELPERS_HPP

# include <boost/spirit/include/phoenix1_functions.hpp>

////////////////////////////////////////////////////////////////////////////////

namespace phelper {
    
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

namespace {
    
phoenix::function<push_back_impl> push_back_ = push_back_impl();
phoenix::function<begin_impl> begin_ = begin_impl();
phoenix::function<end_impl> end_ = end_impl();
phoenix::function<cbegin_impl> cbegin_ = cbegin_impl();
phoenix::function<cend_impl> cend_ = cend_impl();
phoenix::function<insert_impl> insert_ = insert_impl();
phoenix::function<lex_cast_impl> lex_cast_ = lex_cast_impl();
phoenix::function<first_impl> first_ = first_impl();
phoenix::function<second_impl> second_ = second_impl();
phoenix::function<size_impl> size_ = size_impl();

} // anonymous namespace

} // namespace phelper

# endif //     UTILITIES__PHOENIX_HELPERS_HPP
