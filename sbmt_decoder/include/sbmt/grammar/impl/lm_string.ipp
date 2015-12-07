#include <boost/regex.hpp>
#include <boost/variant/get.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/vector.hpp>
#include <graehl/shared/word_spacer.hpp>

#include <sbmt/hash/swap.hpp>

namespace sbmt {

extern boost::regex index_string_regex;
extern boost::regex lexical_string_regex;
extern boost::regex nonterminal_string_regex;

template <class TF>
struct lm_toker {
    lm_toker(TF& tf) : tf(&tf) {}
    
    typedef typename TF::token_type token_type;
    typedef lm_token<token_type> result_type;
    result_type operator()(std::string const& str) const
    {
        boost::smatch s;
        if (boost::regex_match(str, s, index_string_regex)) {
            return lm_token<token_type>(boost::lexical_cast<unsigned int>(s.str(1)));
        } 
        else if (boost::regex_match(str, s, lexical_string_regex)) {
            return lm_token<token_type>(tf->native_word(s.str(1)));
        } else if (boost::regex_match(str, s, nonterminal_string_regex)) {
            if (s.str(1) != "TOP") {
                return lm_token<token_type>(tf->tag(s.str(1)));
            } else {
                return lm_token<token_type>(tf->toplevel_tag());
            }
        } else {
            // <D> or </D> for dep lm.
            return lm_token<token_type>(tf->tag(str));
        }
    }
private:
    TF* tf;
};

////////////////////////////////////////////////////////////////////////////////

template <class TT, class A>
template <class TFT>
lm_string<TT,A>::lm_string(std::string const& str, TFT& tfactory, A const& alloc)
: lm_vec(alloc)
{
    boost::char_delimiters_separator<char> sep(false,""," \t");
    boost::tokenizer<> toker(str,sep);
    boost::tokenizer<>::iterator itr = toker.begin();
    boost::tokenizer<>::iterator end = toker.end();
    
    lm_toker<TFT> lmtokf(tfactory);
    lm_vec_type( boost::make_transform_iterator(itr,lmtokf)
               , boost::make_transform_iterator(end,lmtokf)
               , alloc ).swap(lm_vec);
}

////////////////////////////////////////////////////////////////////////////////

template <class TT, class A, class TFT>
void print(std::ostream& out, lm_string<TT,A> const& lms, TFT const& tfactory)
{
    out << "{{{";
    graehl::word_spacer sep;
    for (typename lm_string<TT,A>::const_iterator i=lms.begin(),e=lms.end();
         i!=e;++i)
        out << sep << print(*i,tfactory);
    out << "}}}";
}

////////////////////////////////////////////////////////////////////////////////

template <class TT>
lm_token<TT>::lm_token(unsigned int idx)
: var(idx) {}

template <class TT>
template <class C>
lm_token<TT>::lm_token(lm_token<C> const& c)
{
    if (c.is_indexed()) var = c.get_index();
    else var = c.get_token();
}

////////////////////////////////////////////////////////////////////////////////

//template <class TT>
//template <class TFT>
//lm_token<TT>::lm_token(std::string const& str, TFT& tfactory)
//: var(tfactory.native_word(str)) {}
template <class TT>
lm_token<TT>::lm_token(TT const& var)
: var(var) {}

////////////////////////////////////////////////////////////////////////////////

template <class TT>
typename lm_token<TT>::token_type const& lm_token<TT>::get_token() const
{
    return *boost::get<token_type>(&var);
}

template <class TT>
typename lm_token<TT>::token_type& lm_token<TT>::get_token()
{
    return *boost::get<token_type>(&var);
}

////////////////////////////////////////////////////////////////////////////////

template <class TT>
unsigned int lm_token<TT>::get_index() const
{
    return *boost::get<unsigned int>(&var);
}

////////////////////////////////////////////////////////////////////////////////

template <class TT>
bool lm_token<TT>::is_token() const { return var.which() == 0; }

template <class TT>
bool lm_token<TT>::is_index() const { return var.which() == 1; }

////////////////////////////////////////////////////////////////////////////////

template <class TT>
template <class AR>
void lm_token<TT>::serialize(AR & ar, const unsigned int version)
{
    ar & var;
}

////////////////////////////////////////////////////////////////////////////////

template <class TT, class A>
void swap(lm_string<TT,A>& m1, lm_string<TT,A>& m2)
{
    m1.swap(m2);
}

////////////////////////////////////////////////////////////////////////////////

template <class TT>
void swap(lm_token<TT>& m1, lm_token<TT>& m2)
{
    resolve_swap(m1.val, m2.val);
}

////////////////////////////////////////////////////////////////////////////////

template <class TT, class TFT>
void print(std::ostream& out, lm_token<TT> const& lmt, TFT const& tfactory)
{
    if (lmt.is_token()) {
        if (is_lexical(lmt.get_token())) {
            out << '"' << print(lmt.get_token(),tfactory) << '"';
        } else {
            out << '(' << print(lmt.get_token(),tfactory) << ')';
        }
    } else out << "x" << lmt.get_index();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

namespace boost { namespace serialization {
template<class TokenT, class A>
struct version< sbmt::lm_string<TokenT, A> >
{
    typedef mpl::int_<1> type;
    typedef mpl::integral_c_tag tag;
    BOOST_STATIC_CONSTANT(int, value = version::type::value);
};
} }

