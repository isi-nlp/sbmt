#ifndef   SBMT_TOKEN_TOKENIZER_HPP
#define   SBMT_TOKEN_TOKENIZER_HPP

#include <sbmt/token/fat_token.hpp>
#include <boost/tokenizer.hpp>
#include <string>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
/// uses boost::char_separator to return tokens instead of strings.
/// no need to use this class directly, just pass it to tokenizer.
///
////////////////////////////////////////////////////////////////////////////////
template <class Ch, class TF> 
class char_separator
{
public:
    typedef typename TF::token_t token_t;
    char_separator( 
          TF& tf
        , token_type_id t
        , Ch const* dropped_delims
        , Ch const* kept_delims = ""
        , boost::empty_token_policy empty_tokens = boost::drop_empty_tokens
    );
    
    explicit char_separator(TF& tf, token_type_id t = native_token);

    template <class InputItr>
    bool operator()(InputItr& next, InputItr end, typename TF::token_t& tok);
    
    void reset() { char_sep.reset(); }
private:
    boost::char_separator<Ch> char_sep;
    token_type_id                typ;
    TF* tf;
};


////////////////////////////////////////////////////////////////////////////////
///
/// a thin layer around boost::tokenizer, to allow flexible extraction of 
/// sbmt::fat_token sequences from strings and streams and such.
///
/// such a shame to have to wrap this.  i could inherrit, but half of the
/// functionality is in the constructors, which are not inherrited anyway.
/// and this way the interface is in one location for review.
///
/// really, I want to do:
/// 
/// template< class TokFunc  = char_separator
///         , class InputItr = std::string::const_iterator >
/// typedef boost::tokenizer<TokFunc, InputItr, token> tokenizer;
///
/// but thats not allowed in c++
///
////////////////////////////////////////////////////////////////////////////////


template < class TokFunc  = char_separator<char,fat_token_factory>
         , class InputItr = std::string::const_iterator >
class tokenizer
{
	typedef typename TokFunc::token_t token_t;
    typedef boost::tokenizer<TokFunc,InputItr,token_t> toker_t;
public:
    typedef typename toker_t::iterator       iterator;
    typedef typename toker_t::const_iterator const_iterator;

    tokenizer(InputItr begin, InputItr end, TokFunc const& f = TokFunc());
    template <class ContainerT> 
        tokenizer(ContainerT const& c, TokFunc const& f = TokFunc());
    
    void assign(InputItr begin, InputItr end);
    void assign(InputItr begin, InputItr end, TokFunc const& f);
    
    template <class ContainerT>
        void assign(ContainerT const& c);
    
    template <class ContainerT>
        void assign(ContainerT const& c, TokFunc const& f);
    
    iterator begin();
    iterator end();
    
    const_iterator begin() const;
    const_iterator end() const;
    
private:
    toker_t toker;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/token/impl/tokenizer.ipp>


#endif // SBMT_TOKEN_TOKENIZER_HPP
