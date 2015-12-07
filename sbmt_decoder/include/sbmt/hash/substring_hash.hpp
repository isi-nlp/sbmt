# if ! defined(SBMT__HASH__SUBSTRING_HASH_MATCH_HPP)
# define       SBMT__HASH__SUBSTRING_HASH_MATCH_HPP

# include <boost/multi_index_container.hpp>
# include <boost/multi_index/hashed_index.hpp>
# include <boost/multi_index/identity.hpp>
# include <sbmt/span.hpp>
# include <sbmt/hash/functors.hpp>

namespace sbmt {

struct lex_hash {
    template <class I>
    std::size_t operator()(std::pair<I,I> const& r) const ;
} ;

struct lex_equal {
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
////////////////////////////////////////////////////////////////////////////////
template <class TokenT>
class substring_hash_match
{
public:
    substring_hash_match() {}
    
    template <class ItrT>
    substring_hash_match(ItrT begin, ItrT end) ;
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  \pre ItrT is a forward-iterator over a collection of TokenT
    ///  \return true if [begin,end) is a substring of the string specified in
    ///  the constructor.
    ///
    ///  \remark [x,x) is always returns true.  that is, empty strings 
    ///  always match.
    ///
    ///////////////////////////////////////////////////////////////////////////
    template <class ItrT>
    bool operator() (ItrT begin, ItrT end) const ;
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    ///  \pre ItrT is a forward-iterator over a collection of TokenT
    ///  \pre InsertItrT is an insertion-iterator for a collection of span_t
    ///
    ///  \post the span of every substring match is placed in inserter
    ///
    ///  \return true if [begin,end) is a substring of the string specified in
    ///  the constructor.
    ///
    ///  \remark [x,x) is always returns true.  that is, empty strings 
    ///  always match.  in this case span_t(0,0) is always inserted.
    ///
    //////////////////////////////////////////////////////////////////////////
    template <class ItrT, class InsertItrT>
    bool operator() (ItrT begin, ItrT end, InsertItrT inserter) const ;
    substring_hash_match(substring_hash_match const&) ;
    substring_hash_match& operator= (substring_hash_match const&) ;
private:
    typedef std::vector<TokenT> vector_t ;
    typedef std::pair< typename vector_t::iterator
                     , typename vector_t::iterator > range_t ;
    
    vector_t text ;
    typedef boost::multi_index_container <
                std::pair<range_t, span_t>
              , boost::multi_index::indexed_by<
                    boost::multi_index::hashed_non_unique<
                        first<range_t, span_t>
                      , lex_hash
                      , lex_equal
                    >
                >
            > suffix_container_t ;
    suffix_container_t suffix_c ;
    
    void init();
public:
    std::pair< typename vector_t::const_iterator
             , typename vector_t::const_iterator > 
             get_text() const { return std::make_pair(text.begin(),text.end()); }
} ;

////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class I>
substring_hash_match<T>::substring_hash_match(I b, I e)
: text(b,e) 
{
   init() ;
}

template <class T>
void substring_hash_match<T>::init()
{
    using std::make_pair ;
    typename vector_t::iterator itr = text.begin() ,
                                end = text.end() ;

    suffix_c.insert(make_pair(range_t(itr,itr),span_t(0,0))) ;
    
    for (int left = 0; itr != end; ++itr, ++left) {
        typename vector_t::iterator jitr = itr ;
        if (itr != end) { 
            ++jitr ;
            int right = left + 1;
            for (;jitr != end; ++jitr, ++right) {
                //std::cerr << "inserting: " << span_t(left,right) << ":";
                //for (typename vector_t::iterator i = itr; i != jitr; ++i) std::cerr << *i ;
                //std::cerr << std::endl ; 
                suffix_c.insert(make_pair( range_t(itr,jitr)
                                         , span_t(left,right) )) ;
                assert(suffix_c.find(range_t(itr,jitr)) != suffix_c.end());
            }
            suffix_c.insert(make_pair(range_t(itr,end),span_t(left,right))) ;
        }
    }
}

template <class T>
substring_hash_match<T>::substring_hash_match(substring_hash_match const& o) 
: text(o.text)
{ init(); }

template <class T>
substring_hash_match<T>& 
substring_hash_match<T>::operator= (substring_hash_match const& o) 
{
    if (this != &o) {
        text = o.text ;
        suffix_c.clear();
        init() ;
    } 
    return *this ;
}

////////////////////////////////////////////////////////////////////////////////

template <class I>
std::size_t lex_hash::operator()(std::pair<I,I> const& r) const
{
    I ritr = r.first , rend = r.second ;
    std::size_t retval(0) ;
    for (; ritr != rend; ++ritr) {
        boost::hash_combine(retval,*ritr) ;
    }
    return retval ;
}

////////////////////////////////////////////////////////////////////////////////

template <class I1, class I2>
bool lex_equal::operator()( std::pair<I1,I1> const& r1
                          , std::pair<I2,I2> const& r2 ) const
{
    I1 r1itr = r1.first , r1end = r1.second ;
    I2 r2itr = r2.first , r2end = r2.second ;

    for (; r1itr != r1end and r2itr != r2end; ++r1itr, ++r2itr) {
        if (!(*r1itr == *r2itr)) { return false ; }
    }
    return (r1itr == r1end and r2itr == r2end) ; 
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class I>
bool substring_hash_match<T>::operator()(I begin, I end) const
{
    std::pair<I,I> p = std::make_pair(begin,end) ;
    typedef typename suffix_container_t::iterator iterator ;
    std::pair<iterator,iterator> r = suffix_c.equal_range(p) ;
    return r.first != r.second ;
}

template <class T>
template <class I, class InI>
bool substring_hash_match<T>::operator()(I begin, I end, InI insert) const
{
    std::pair<I,I> p = std::make_pair(begin,end) ;
    typedef typename suffix_container_t::iterator iterator ;
    std::pair<iterator,iterator> r = suffix_c.equal_range(p) ;
    for(iterator i = r.first; i != r.second; ++i) {
        *insert = i->second ;
        ++insert ;
    }
    return r.first != r.second ;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt 

# endif // SBMT__HASH__SUBSTRING_HASH_MATCH_HPP
