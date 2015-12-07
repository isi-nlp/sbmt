#include <RuleReader/Rule.h>
#include <boost/functional/hash.hpp>
#include <sbmt/hash/swap.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class CharT, class TraitsT, class T>
std::basic_ostream<CharT,TraitsT>& 
operator<<(std::basic_ostream<CharT,TraitsT>& os, rule_topology<T> const& rtop)
{
    os << rtop.lhs() << " -> " << rtop.rhs(0);
    for (std::size_t x = 1; x != rtop.rhs_size(); ++x) {
        os << " " << rtop.rhs(x);
    }
    return os;
}

////////////////////////////////////////////////////////////////////////////////

template <class CharT, class TraitsT, class T>
std::basic_ostream<CharT,TraitsT>& 
operator << (std::basic_ostream<CharT,TraitsT>& os, binary_rule<T> const& ri)
{
    os << ri.topology() << " ### ";
    
    return os;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

template <class T>
void throw_if_terminal(token<T> const& t) 
{
    if (!is_nonterminal(t)) { 
        bad_rule_format e("token on lhs is a terminal token");
        throw e;
    }
}

template <class T>
void throw_if_native(T* itr, T* end)
{
    for(; itr != end; ++itr) if (itr->type() == native_token) {
        bad_rule_format e("token on rhs is a native word");
        throw e;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
rule_topology<T>::rule_topology()
: unry(false)
{
    tok[0] = token_t();
    tok[1] = token_t();
    tok[2] = token_t();
}

template <class T>
rule_topology<T>::rule_topology(typename rule_topology<T>::token_t const& lhs
                               ,typename rule_topology<T>::token_t const& rhs)
: unry(true)
{
    tok[0] = lhs;
    tok[1] = rhs;
    throw_if_native(tok, tok + 2);
}

template <class T>
rule_topology<T>::rule_topology(typename rule_topology<T>::token_t const& lhs
                               ,typename rule_topology<T>::token_t const& rhs1
                               ,typename rule_topology<T>::token_t const& rhs2)
: unry(false)
{
    tok[0] = lhs;
    tok[1] = rhs1;
    tok[2] = rhs2;
    throw_if_native(tok + 0, tok + 3);
}

template <class T>
void swap(rule_topology<T>& r1, rule_topology<T>& r2)
{
    using std::resolve_swap;
    resolve_swap(r1.tok[0], r2.tok[0]);
    resolve_swap(r1.tok[1], r2.tok[1]);
    resolve_swap(r1.tok[2], r2.tok[2]);
    resolve_swap(r1.unry, r2.unry);
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
bool operator==(rule_topology<T> const& r1, rule_topology<T> const& r2)
{
    if (r1.lhs() != r2.lhs()) return false;
    if (r1.rhs_size() != r2.rhs_size()) return false;
    for (std::size_t x = 0; x != r1.rhs_size(); ++x) {
        if (r1.rhs(x) != r2.rhs(x)) return false;
    }
    return true;
}

template <class T>
std::size_t hash_value(rule_topology<T> const& rtop)
{
    boost::hash<T> tok_hasher;
    size_t retval = tok_hasher(rtop.lhs());
    for (size_t x = 0; x != rtop.rhs_size(); ++x) {
        boost::hash_combine(retval,tok_hasher(rtop.rhs(x)));
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class TokenFactory>
typename TokenFactory::token_t 
virtual_or_tag(std::string const& str, TokenFactory& tf) {
    if (str.size() > 2 and str[0] == 'V' and (str[1] == '[' or str[1] == '<'))
        return tf.virtual_tag(str);
    else if (str == "TOP") return tf.toplevel_tag();
    else return tf.tag(str);
}

template <class T>
template <class TokenFactory>
rule_topology<T>::rule_topology( ns_RuleReader::Rule& r, TokenFactory& tf )
{
    token_t ls, rs1, rs2;
    
    if (r.is_virtual_label()) ls = tf.virtual_tag(r.get_label());
    else if (r.get_label() == std::string("TOP")) {
        ls = tf.toplevel_tag();
    } else ls = tf.tag(r.get_label());
    
    bool unary = r.getRHSConstituents()->size() == 1;
    
    if (r.getRHSConstituents()->at(0) != "") 
        rs1 = tf.virtual_tag(r.getRHSConstituents()->at(0));
    else if (r.getRHSLexicalItems()->at(0) != "")
        rs1 = tf.foreign_word(r.getRHSLexicalItems()->at(0));
    else if (r.getRHSStates()->at(0) != "")
        rs1 = virtual_or_tag(r.pos_lookup(r.getRHSStates()->at(0)),tf);
    else throw_bad_rule_format("no rhs data in first position");
    
    if (!unary) {
        if (r.getRHSConstituents()->at(1) != "") 
            rs2 = tf.virtual_tag(r.getRHSConstituents()->at(1));
        else if (r.getRHSLexicalItems()->at(1) != "")
            rs2 = tf.foreign_word(r.getRHSLexicalItems()->at(1));
        else if (r.getRHSStates()->at(1) != "")
            rs2 = virtual_or_tag(r.pos_lookup(r.getRHSStates()->at(1)),tf);
        else throw_bad_rule_format("no rhs data in second position");  
    }
    
    unry = unary;
    tok[0] = ls;
    tok[1] = rs1;
    tok[2] = unary ? token_t() : rs2;
}

struct attmap_to_string_pair {
    typedef std::pair<std::string,std::string> result_type;
    template <class Pair>
    result_type operator()(Pair const& p) const
    {
        return result_type(p.first,p.second.value);
    }
};

template <class Range, class F>
boost::iterator_range<
    boost::transform_iterator<
        F
      , typename boost::range_iterator<Range>::type
    >
>
transform_range(Range const& r, F const& f)
{
    return boost::make_iterator_range(
               boost::make_transform_iterator(boost::begin(r),f)
             , boost::make_transform_iterator(boost::end(r),f)
           );
}

template <class T>
template <class TokenFactory>
void binary_rule<T>::init( ns_RuleReader::Rule& r
                         , TokenFactory& tf
                         , property_constructors<TokenFactory> const& pc )
{
    if (!r.is_binarized_rule()) throw_bad_rule_format("not a binarized rule");
    
    rule_top = rule_topology<T>(r,tf);
    
    ns_RuleReader::Rule::attribute_map& attmap = *r.getAttributes();
    properties = pc.construct_properties(tf, transform_range(boost::make_iterator_range(attmap),attmap_to_string_pair()));
}

////////////////////////////////////////////////////////////////////////////////

template<class T>
void swap(binary_rule<T>& r1, binary_rule<T>& r2)
{
    std::resolve_swap(r1.rule_top, r2.rule_top);
    std::resolve_swap(r1.properties, r2.properties);
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class TokFactory>
binary_rule<T>::binary_rule( std::string const& str
                           , TokFactory& tf
                           , property_constructors<TokFactory> const& pc )
{
    ns_RuleReader::Rule r(str);
    init<TokFactory>(r,tf,pc);
}

template <class T>
template <class TokFactory>
binary_rule<T>::binary_rule( ns_RuleReader::Rule& r
                           , TokFactory& tf
                           , property_constructors<TokFactory> const& pc )
{
    init<TokFactory>(r,tf,pc);
}

////////////////////////////////////////////////////////////////////////////////

template<class T>
binary_rule<T>& binary_rule<T>::operator=(binary_rule<T> const& other)
{
    binary_rule<T> r(other);
    swap(r,*this);
    return *this;
}

template<class T>
bool binary_rule<T>::operator==(binary_rule<T> const& other) const
{
    return rule_top == other.rule_top;
}


} // namespace sbmt
