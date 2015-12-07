# if ! defined(SBMT__EDGE__CONSTITUENT_HPP)
# define       SBMT__EDGE__CONSTITUENT_HPP

# include <sbmt/span.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/token.hpp>
# include <boost/functional/hash.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  a reduced view of a partial derivation.
///  the info type is held by pointer, so it must exist somewhere else
///
////////////////////////////////////////////////////////////////////////////////
template <class Info>
class constituent {
private:
    Info const* i;
    indexed_token tok;
    span_t spn;
public:
    typedef Info info_type;
    
    template <class I> friend class constituent;
    
    constituent() : i(NULL) {}
    constituent(Info const* i, indexed_token tok, span_t const& spn)
      : i(i)
      , tok(tok)
      , spn(spn) {}
    //constituent(indexed_token tok)
    //  : i(NULL), tok(tok) {}
    indexed_token root() const { return tok; }
    Info const* info() const { return i; }
    span_t span() const { return spn; }
};

struct constituent_info {
    template <class X>
    struct result {};
    
    template <class Info>
    struct result<constituent_info(constituent<Info>)> {
        typedef Info const& type;
    };
    template <class Info>
    Info const& operator()(constituent<Info> const& c) const
    {
        return *c.info();
    }
};

struct constituent_root {
    typedef indexed_token result_type;
    
    template <class Info>
    indexed_token operator()(constituent<Info> const& c) const
    {
        return c.root();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class CT, class CI, class TF, class Info>
void print( std::basic_ostream<CT,CI>& out
          , constituent<Info> const& c
          , TF const& tf )
{
    out << "(" 
        << print(c.root(),tf) 
        << " " 
        << print(*c.info(),tf) 
        << " "
        << c.span().left() 
        << " " 
        << c.span().right() 
        << ")";
}

////////////////////////////////////////////////////////////////////////////////

template <class Info>
constituent<Info> make_constituent( Info const* info
                                  , indexed_token tok
                                  , span_t const& spn
                                  )
{
    return constituent<Info>(info, tok, spn);
}

////////////////////////////////////////////////////////////////////////////////

template <class Info>
bool operator == (constituent<Info> const& c1, constituent<Info> const& c2)
{
    return c1.root() == c2.root() 
       and *c1.info() == *c2.info() 
       and c1.span() == c2.span();
}

template <class Info>
bool operator != (constituent<Info> const& c1, constituent<Info> const& c2)
{
    return !(c1 == c2);
}

////////////////////////////////////////////////////////////////////////////////

template <class Info>
std::size_t hash_value(constituent<Info> const& c)
{
    std::size_t ret = 0;
    boost::hash_combine(ret,c.root());
    boost::hash_combine(ret,*c.info());
    boost::hash_combine(ret,c.span());
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__EDGE__CONSTITUENT_HPP
