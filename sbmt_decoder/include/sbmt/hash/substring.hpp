# if ! defined(SBMT__HASH__SUBSTRING_MATCH_HPP)
# define       SBMT__HASH__SUBSTRING_MATCH_HPP

# include <boost/multi_index_container.hpp>
# include <boost/multi_index/ordered_index.hpp>
# include <boost/multi_index/identity.hpp>

namespace sbmt {

struct lex_order {
    template <class I1, class I2>
    bool operator()( std::pair<I1,I1> const& r1
                   , std::pair<I2,I2> const& r2 ) const ;
} ;

////////////////////////////////////////////////////////////////////////////////
///
/// determines whether a fixed string contains another string
/// the fixed string is passed in at construction time.
/// the searched-for string is passed through the function-call
/// operator.
///
/// \todo this is essentially a suffix-tree cobbled together from
/// standard parts.  unfortunately, since its 
/// not using a full-fledged suffix-array or suffix-tree interface
/// its not quite as efficient as it could be.
///
////////////////////////////////////////////////////////////////////////////////
template <class TokenT>
class substring_match
{
public:
    template <class ItrT>
    substring_match(ItrT begin, ItrT end) ;
    
    template <class ItrT>
    bool operator() (ItrT begin, ItrT end) const ;
    
    substring_match(substring_match const&) ;
    substring_match& operator= (substring_match const&) ;
private:
    typedef std::vector<TokenT> vector_t ;
    typedef std::pair< typename vector_t::iterator
                     , typename vector_t::iterator > range_t ;
    
    vector_t text ;
    typedef boost::multi_index_container <
                range_t
              , boost::multi_index::indexed_by<
                    boost::multi_index::ordered_unique<
                        boost::multi_index::identity<range_t>
                      , lex_order
                    >
                >
            > suffix_container_t ;
    suffix_container_t suffix_c ;
    void init() ;
public:
    std::pair< typename vector_t::const_iterator
             , typename vector_t::const_iterator > 
    get_text() const { return std::make_pair(text.begin(),text.end()); }
} ;

template <class T>
void substring_match<T>::init()
{
    typename vector_t::iterator itr = text.begin() ,
                                end = text.end() ;
    for (; itr != end; ++itr) {
        suffix_c.insert(range_t(itr,end)) ;
    }
}

template <class T>
substring_match<T>::substring_match(substring_match const& o) 
: text(o.text)
{ init(); }

template <class T>
substring_match<T>& 
substring_match<T>::operator= (substring_match const& o) 
{
    if (this != &o) {
        text = o.text ;
        suffix_c.clear() ;
        init() ;
    } 
    return *this ;
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class I>
substring_match<T>::substring_match(I b, I e)
: text(b,e) 
{
    init() ;
}

////////////////////////////////////////////////////////////////////////////////

template <class I1, class I2>
bool is_prefix( std::pair<I1,I1> const& r1
              , std::pair<I2,I2> const& r2 )
{
    I1 r1itr = r1.first , r1end = r1.second ;
    I2 r2itr = r2.first , r2end = r2.second ;

    for (; r1itr != r1end and r2itr != r2end; ++r1itr, ++r2itr) {
        if (!(*r1itr == *r2itr)) return false ;
    }
    return r1itr == r1end ;
}

////////////////////////////////////////////////////////////////////////////////

template <class I1, class I2>
bool lex_order::operator()( std::pair<I1,I1> const& r1
                          , std::pair<I2,I2> const& r2 ) const
{
    I1 r1itr = r1.first , r1end = r1.second ;
    I2 r2itr = r2.first , r2end = r2.second ;

    for (; r1itr != r1end and r2itr != r2end; ++r1itr, ++r2itr) {
        if (!(*r1itr == *r2itr)) { return *r1itr < *r2itr ; }
    }
    if (r1itr == r1end) return r2itr != r2end ;
    else return false ;
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class I>
bool substring_match<T>::operator()(I begin, I end) const
{
    std::pair<I,I> p = std::make_pair(begin,end) ;
    typename suffix_container_t::iterator pos = suffix_c.lower_bound(p) ;
    return pos != suffix_c.end() and is_prefix(p,*pos) ;
}

} // namespace sbmt 

# endif // SBMT__HASH__SUBSTRING_MATCH_HPP
