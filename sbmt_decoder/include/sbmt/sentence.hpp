#ifndef   SBMT_TOKEN_SENTENCE_HPP
#define   SBMT_TOKEN_SENTENCE_HPP

#include <deque>
#include <exception>
#include <sbmt/token.hpp>
#include <iosfwd>

#include <boost/iterator/transform_iterator.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

struct mismatched_sentence : public std::exception
{
    virtual char const* what() const throw() 
    { return "mismatched foreign/native sentence or token"; }
    
    virtual ~mismatched_sentence() throw() {}
};

inline void throw_mismatched_sentence()
{
    mismatched_sentence e;
    throw e;
}

template <class T> class sentence;

template <class T> sentence<T> join(sentence<T> const&, sentence<T> const&);

////////////////////////////////////////////////////////////////////////////////
///
/// you dont get much out of a sentence other than an assurance that all of the
/// tokens inside are native or foreign words.  and a print function.
/// all of the convenience of turning strings into containers of tokens is 
/// provided by the tokenizer class.  this pretty much frees you to use whatever
/// container of tokens you want.
///
/// actually, as the need arises, it might be nice to add string-like algorithms
/// to sentence, to make it similar to a string class.  but time-is-a-wasting.
///
////////////////////////////////////////////////////////////////////////////////
template <class T>
class sentence
{
public:
    typedef T token_type;
    typedef typename std::deque<token_type>::const_iterator const_iterator;
    typedef const_iterator iterator; // no modifying behind our back!
    
    sentence(token_type_id typ); // tokens may only be of given type
    sentence(); // tokens may be of mixed type
    sentence(sentence const& o) : typ(o.typ), any_type(o.any_type),s(o.s)
    {}
    
    template <class Itr> sentence(Itr begin, Itr end);
    
    token_type_id type() const { return typ; }
    bool allows_type(token_type_id t) const 
    {
        return any_type || typ == t;
    }

    void check_type(token_type_id t) const
    {
        if (!allows_type(t))
            throw_mismatched_sentence();
    }

    bool can_include(sentence const& other) const 
    {
        if (any_type)
            return true;
        if (!other.any_type)
            return typ==other.typ;
        return can_include(other.begin(),other.end());
    }

    void check_include(sentence const& other) const 
    {
        if (!can_include(other))
            throw_mismatched_sentence();
    }
    
    template <class Itr>
    bool can_include(Itr begin,Itr end) const
    {
        if (any_type)
            return true;
        for (Itr itr = begin; itr != end; ++itr)
            if (!allows_type(itr->type()))
                return false;
        return true;
    }
    
    void append(sentence const& other);
    void prepend(sentence const& other);
    void operator+=(sentence const& other) { append(other); }
    
    void push_front(token_type t);
    void push_back(token_type t);
    void operator+=(token_type t) { push_back(t); }
    
    token_type const& at(std::size_t idx) const;
    void  replace(std::size_t idx, token_type t);
    
    void clear() { s.clear(); }
    
    std::size_t size() const { return s.size(); }
    std::size_t length() const { return size(); }
    bool empty() const { return s.empty(); }
    
    const_iterator begin() const { return s.begin(); }
    const_iterator end()   const { return s.end();   }
    
    bool operator == (sentence const& other) const { return s == other.s; }
    bool operator != (sentence const& other) const { return s != other.s; }
    
    std::size_t hash_value() const;

private:
    friend sentence<T> join<T>(sentence<T> const&, sentence<T> const&);
    token_type_id typ;
    bool any_type; // true if mixed type tokens are allowed    
    std::deque<token_type> s;
    
};

////////////////////////////////////////////////////////////////////////////////

template <class T>
sentence<T> operator+(sentence<T> const& s1, sentence<T> const& s2) 
{ return join(s1,s2); }

template <class T>
std::size_t hash_value(sentence<T> const& s)
{
    return s.hash_value();
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Itr>
sentence<T>::sentence(Itr begin,Itr end)
: typ(begin->type())
, any_type(false)
, s(begin, end)    
{
    if (!can_include(begin,end))
        throw_mismatched_sentence();
}

////////////////////////////////////////////////////////////////////////////////
///
/// temporary to make tests work.  just prints token strings seperated by white 
/// space
/// if more sophisticated printing is needed, consider allowing manipulators
/// that control separator.
///
////////////////////////////////////////////////////////////////////////////////
template <class Ch,class Tr, class T>
std::basic_ostream<Ch,Tr>& 
operator << (std::basic_ostream<Ch,Tr>& os, sentence<T> const& s);

template <class CH, class TR, class T, class TF>
void print(std::basic_ostream<CH,TR>& os, sentence<T> const& s, TF& tf)
{
    typename sentence<T>::iterator itr = s.begin(), end = s.end() ;
    if (itr != end) {
        print(os,*itr,tf) ;
        ++itr ;
    }
    for (; itr != end; ++itr) {
        os << " " ;
        print(os,*itr,tf) ;
    }
}

typedef sentence<fat_token> fat_sentence;
typedef sentence<indexed_token> indexed_sentence;

template <class TF>
fat_sentence fatten(indexed_sentence const& s, TF& tf)
{
    typedef fatten_op<fat_token,indexed_token,TF> op_type;
    op_type func(tf);
    boost::transform_iterator< 
        op_type
      , indexed_sentence::iterator 
    > begin(s.begin(),func), end(s.end(), func);
    
    return fat_sentence(begin,end);
}

template <class TF>
indexed_sentence index(fat_sentence const& s, TF& tf)
{
    typedef index_op<indexed_token,fat_token, TF> op_type;
    op_type func(tf);
    boost::transform_iterator< 
        op_type
      , fat_sentence::iterator 
    > begin(s.begin(),func), end(s.end(), func);
    
    return indexed_sentence(begin,end);    
}


////////////////////////////////////////////////////////////////////////////////
///
/// simple native and foreign sentence generation functions.  they use 
/// sbmt::tokenizer, and more compicated generators can be created by changing
/// the separating function for a tokenizer
///
////////////////////////////////////////////////////////////////////////////////
//\@{
fat_sentence native_sentence(std::string const& str);
fat_sentence foreign_sentence(std::string const& str);

template <class S>
indexed_sentence native_sentence(std::string const& str, dictionary<S>& dict);
template <class S>
indexed_sentence foreign_sentence(std::string const& str, dictionary<S>& dict);
//\@}

} // namespace sbmt

#include <sbmt/impl/sentence.ipp>

#endif // SBMT_TOKEN_SENTENCE_HPP 
