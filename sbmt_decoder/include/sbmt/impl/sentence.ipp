# include <boost/version.hpp>
# if BOOST_VERSION < 103400
#   include <boost/functional/hash/deque.hpp>
# else
#   include <boost/functional/hash.hpp>
# endif
# include <sbmt/token/tokenizer.hpp>

namespace sbmt {

template <class T>
sentence<T>::sentence()
    : typ(foreign_token),any_type(true)
{
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
sentence<T>::sentence(token_type_id typ)
    : typ(typ) , any_type(false)
{
    if (typ != foreign_token && typ != native_token) {
        throw_mismatched_sentence();
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class T>    
void sentence<T>::append(sentence<T> const& other)
{
    check_include(other); // special value of typ if other is anytype    
    s.insert(s.end(),other.s.begin(),other.s.end());
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
void sentence<T>::prepend(sentence<T> const& other)
{
    check_include(other);
    s.insert(s.begin(),other.s.begin(),other.s.end());
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
void sentence<T>::push_front(T t)
{
    check_type(t.type());
    s.push_front(t);
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
void sentence<T>::push_back(T t)
{
    check_type(t.type());
    s.push_back(t);
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
T const& sentence<T>::at(std::size_t idx) const 
{ 
    return s.at(idx);
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
void sentence<T>::replace(std::size_t idx, T t)
{
    check_type(t.type());
    s.at(idx) = t;
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
sentence<T> join(sentence<T> const& s1, sentence<T> const& s2)
{
    s1.check_include(s2);
    sentence<T> ret(s1);
    ret.append(s2);
    return ret;
}

template <class T>
std::size_t sentence<T>::hash_value() const
{
	boost::hash< std::deque<T> > hasher;
	return hasher(s);
}

////////////////////////////////////////////////////////////////////////////////

template <class Ch, class Tr, class T>
std::basic_ostream<Ch,Tr>& 
operator << (std::basic_ostream<Ch,Tr>& os, sentence<T> const& s)
{
    typename sentence<T>::const_iterator itr = s.begin();
    typename sentence<T>::const_iterator end = s.end();
    if (itr != end) os << itr->label();
    ++itr;
    for (;itr != end; ++itr) os << " " << itr->label();
    return os;
}

////////////////////////////////////////////////////////////////////////////////

template <class S>
indexed_sentence native_sentence(std::string const& str, dictionary<S>& dict)
{
	typedef char_separator<char,dictionary<S> > sep_t;
	sep_t cs(dict,native_token," \t\n\r","") ;
    tokenizer<sep_t> tok(str,cs) ;
    return tok.begin() == tok.end() ? 
           indexed_sentence(native_token) : 
           indexed_sentence(tok.begin(),tok.end()) ;
}

template <class S>
indexed_sentence foreign_sentence(std::string const& str, dictionary<S>& dict)
{
	typedef char_separator<char,dictionary<S> > sep_t;
	sep_t cs(dict,foreign_token," \t\n\r","") ;
    tokenizer<sep_t> tok(str,cs) ;
    return tok.begin() == tok.end() ? 
           indexed_sentence(foreign_token) : 
           indexed_sentence(tok.begin(),tok.end()) ;
}

} // namespace sbmt
