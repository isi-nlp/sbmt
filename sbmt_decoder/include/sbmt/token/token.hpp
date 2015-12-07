#ifndef   SBMT_TOKEN_TOKEN_HPP
#define   SBMT_TOKEN_TOKEN_HPP

#include <string>
#include <iostream>

#ifdef _WIN32
#	include <iso646.h>
#endif

namespace sbmt 
{

enum token_type_id { foreign_token     = 0
                   , native_token      = 1
                   , tag_token         = 2
                   , virtual_tag_token = 3
                   , top_token         = 4
};

#define top_token_text "TOP"
  
////////////////////////////////////////////////////////////////////////////////

template <class DerivT>
class token
{
public:
    token_type_id type() const;
private:
    friend class token_access;
    DerivT const& derived() const { return static_cast<DerivT const&>(*this); }
    DerivT&       derived()       { return static_cast<DerivT&>(*this); }
};

////////////////////////////////////////////////////////////////////////////////

class token_access 
{
public:
    template <class D>
    static token_type_id type(token<D> const& t)
    {
        return t.derived().type();
    }
    
    template <class D>
    static bool equal(token<D> const& t1, token<D> const& t2)
    {
        return t1.derived().equal(t2.derived());
    }
    
    template <class D>
    static bool less(token<D> const& t1, token<D> const& t2)
    {
        return t1.derived().less(t2.derived());
    }
    
    template <class D>
    static std::size_t hash_value(token<D> const& t)
    {
        return t.derived().hash_value();
    }
    
    template <class D>
    static void swap(token<D>& t1, token<D>& t2)
    {
        t1.derived().swap(t2.derived());
    }
    
    template <class C, class T, class D>
    static void print_token(std::basic_ostream<C,T>& os, token<D> const& t)
    {
        t.derived().print_self(os);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class D>
token_type_id token<D>::type() const { return token_access::type(*this); }

////////////////////////////////////////////////////////////////////////////////

template <class D>
void swap(token<D>& t1, token<D>& t2)
{
    token_access::swap(t1,t2);
}

////////////////////////////////////////////////////////////////////////////////

template <class D>
bool operator==(token<D> const& t1, token<D> const& t2)
{
    return token_access::equal(t1,t2);
}

template <class D>
bool operator<(token<D> const& t1, token<D> const& t2)
{
    return token_access::less(t1,t2);
}

template <class D>
bool operator<=(token<D> const& t1, token<D> const& t2)
{
    return token_access::less(t1,t2) or token_access::equal(t1,t2);
}

template <class D>
bool operator>=(token<D> const& t1, token<D> const& t2)
{
    return not token_access::less(t1,t2);
}

template <class D>
bool operator>(token<D> const& t1, token<D> const& t2)
{
    return (not token_access::less(t1,t2)) and (not token_access::equal(t1,t2));
}

////////////////////////////////////////////////////////////////////////////////

template <class D>
bool operator!=(token<D> const& t1, token<D> const& t2)
{
    return !(t1 == t2);
}

////////////////////////////////////////////////////////////////////////////////

template <class D>
std::size_t hash_value(token<D> const& t)
{
    return token_access::hash_value(t);
}

////////////////////////////////////////////////////////////////////////////////

template <class C, class T, class D>
std::basic_ostream<C,T>& 
operator << (std::basic_ostream<C,T>& os, token<D> const& tok)
{
    token_access::print_token<C,T>(os,tok);
    return os;
}

////////////////////////////////////////////////////////////////////////////////

template <class D>
inline bool is_lexical(token<D> const& t) {
    token_type_id typ = t.type();
    return (typ == foreign_token) or (typ == native_token);
}

template <class D>
inline bool is_nonterminal(token<D> const& t)
{
    return !is_lexical(t);
}

template <class D>
inline bool is_native_tag(token<D> const& t)
{
    token_type_id typ = t.type();
    return typ==tag_token || typ==top_token;
}

template <class D>
inline bool is_virtual_tag(token<D> const& t)
{
    return t.type() == virtual_tag_token;
}

template <class D>
inline bool is_native(token<D> const& t)
{
    return t.type() == native_token;
}

template <class D>
inline bool is_foreign(token<D> const& t)
{
    return t.type() == foreign_token;
}


////////////////////////////////////////////////////////////////////////////////
///
/// some datastructures are templated on token type.  fat tokens can be 
/// created out of thin air.  indexed_tokens require a dictionary.  
/// token_factory provides a uniform interface for creation of tokens.
///
////////////////////////////////////////////////////////////////////////////////
template <class DerivedT,class TokenT>
class token_factory
{
public:
    typedef TokenT token_t;
    
    std::string label(token_t const& tok) const
    { return derived().label(tok); }
    
    token_t toplevel_tag()
    { return derived().toplevel_tag(); }
    
    token_t foreign_word(std::string const& s)
    { return derived().foreign_word(s); }
    
    token_t native_word(std::string const& s)
    { return derived().native_word(s); }
    
    token_t tag(std::string const& s)
    { return derived().tag(s); }
    
    token_t virtual_tag(std::string const& s)
    { return derived().virtual_tag(s); }
private:
    DerivedT& derived() { return *static_cast<DerivedT*>(this); }
    DerivedT const& derived() const 
    { return *static_cast<DerivedT const*>(this); }
};
    
} // namespace sbmt

//#include <sbmt/token/impl/token.ipp>

#endif // SBMT_TOKEN_TOKEN_HPP
