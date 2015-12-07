#ifndef   SBMT_GRAMMAR_RULE_INPUT_HPP
#define   SBMT_GRAMMAR_RULE_INPUT_HPP

#include <sbmt/token.hpp>
#include <string>
#include <map>
#include <iostream>
#include <sbmt/grammar/bad_rule_format.hpp>
#include <sbmt/grammar/property_construct.hpp>
#include <sbmt/grammar/lm_string.hpp>
#include <boost/serialization/version.hpp>
#include <sbmt/printer.hpp>
#include <gusc/varray.hpp>
#include <gusc/const_any.hpp>

namespace ns_RuleReader { class Rule; }

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class T> class rule_topology;

template <class T>
void swap(rule_topology<T>& r1, rule_topology<T>& r2);

////////////////////////////////////////////////////////////////////////////////

template <class TokenT>
class rule_topology
{
public:
    typedef TokenT token_t;
    template <class TokenFactory>
    rule_topology(ns_RuleReader::Rule& r, TokenFactory& tf);
    rule_topology(token_t const& lhs, token_t const& rhs1, token_t const& rhs2);
    rule_topology(token_t const& lhs, token_t const& rhs);
    rule_topology();
    
    token_t lhs() const { return tok[0]; }
    token_t rhs(std::size_t idx) const { return tok[idx + 1]; }
    
    std::size_t rhs_size() const { return unry ? 1 : 2; }

    template <class O,class TF>
    void print(O&o,TF const& tf) const
    {
        o << sbmt::print(lhs(),tf) << " ->";
        for (std::size_t x = 0; x < rhs_size(); ++x)
            o << " " << sbmt::print(rhs(x),tf);
    }

    token_t const* rhs_begin() const
    {
        return &tok[1];
    }
    token_t const* rhs_end() const
    {
        return &tok[rhs_size()];
    }
    
private:
    friend class boost::serialization::access;
    template <class AR>
        void serialize(AR& ar, const unsigned int version)
        {
            ar & unry;
            ar & tok;
        }
    bool    unry;
    token_t tok[3];
    
    friend void swap<>(rule_topology<TokenT>& r1, rule_topology<TokenT>& r2);
    
};

template <class TF> rule_topology<typename TF::token_t>
index(rule_topology<fat_token> const& frt, TF& tf)
{
    typedef rule_topology<typename TF::token_t> irt;
    if (frt.rhs_size() == 1) {
        return irt(index(frt.lhs(),tf),index(frt.rhs(0),tf));
    } else {
        return irt(index(frt.lhs(),tf),index(frt.rhs(0),tf),index(frt.rhs(1),tf));
    }
}

template <class O,class Tok,class TF>
void print(O&o,rule_topology<Tok> const& rtop,TF const& tf) 
{
    rtop.print(o,tf);
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
bool operator==(rule_topology<T> const& r1, rule_topology<T> const& r2);

template <class T>
bool operator!=(rule_topology<T> const& r1, rule_topology<T> const& r2)
{ return !(r1 == r2); }

template <class T>
std::size_t hash_value(rule_topology<T> const& rtop);

template <class CharT, class TraitsT, class T>
std::basic_ostream<CharT,TraitsT>& 
operator << ( std::basic_ostream<CharT,TraitsT>& os
            , rule_topology<T> const& rtop);

////////////////////////////////////////////////////////////////////////////////

template <class TokenT> class binary_rule;

template <class T>
void swap(binary_rule<T>& r1, binary_rule<T>& r2);

template <class TF>
binary_rule<typename TF::token_t>
index(binary_rule<fat_token> const& fr, TF& tf);

////////////////////////////////////////////////////////////////////////////////

template <class TokenT>
class binary_rule
{
public:
    typedef TokenT token_t;
    typedef rule_topology<token_t> topology_t;
    
    template <class Dict>
    binary_rule( std::string const& str
               , Dict& dict
               , property_constructors<Dict> const& pc );
    
    template <class Dict>
    binary_rule( ns_RuleReader::Rule& r
               , Dict& dict
               , property_constructors<Dict> const& pc );
               
    binary_rule( rule_topology<TokenT> const& r
               , std::vector<gusc::const_any> const& props )
      : properties(props.begin(),props.end()) 
      , rule_top(r) {}
    
    explicit binary_rule(rule_topology<TokenT> const& r) : rule_top(r) {}
    
    binary_rule() {}
    
    binary_rule& operator=(binary_rule const& other);
    
    bool operator==(binary_rule<TokenT> const& other) const;
    
    token_t lhs() const { return rule_top.lhs(); }
    token_t rhs(std::size_t x) const { return rule_top.rhs(x); }
    std::size_t rhs_size() const { return rule_top.rhs_size(); }
    
    topology_t const& topology() const { return rule_top; }

    template <class Type> 
    Type const& get_property(size_t idx) const 
    { 
        return *gusc::any_cast<Type>(&properties[idx]);
    }
    
    bool has_property(size_t idx) const
    {
        return not properties[idx].empty();
    }
    
    size_t num_properties() const { return properties.size(); }

    template <class O,class TF>
    void print(O&o,TF const& tf,bool lmstring=true) const
    {
        topology().print(o,tf);
    }
    
    gusc::varray<gusc::const_any> properties;
private:
    topology_t rule_top;
    
    friend class boost::serialization::access;
    template <class AR>
    void serialize(AR & ar, const unsigned int version)
    {
        // if you change this (or any serialize) method, auto-increment
        // the boost::serialization::version struct below
        ar & rule_top;
        gusc::varray<gusc::const_any>().swap(properties);
        if (version < 3) {
            properties = gusc::varray<gusc::const_any>(version == 2 ? 2 : 1);
            bool is_lm_scoreable(false); // init unnecessary.  make compiler happy
            lm_string<token_t> lmstr;
            ar & is_lm_scoreable;
            ar & lmstr;
            properties[0] = lmstr;
            if (version == 2) {
                bool is_dep_lm_scoreable(false); // init unnecessary.  make compiler happy
                lm_string<token_t> dep_lm_str;
                ar & is_dep_lm_scoreable;
                ar & dep_lm_str;
                if (is_dep_lm_scoreable) properties[1] = dep_lm_str;
            }
            if (version < 2) {
                std::size_t id=0; // for archive compatability.  was never init/used
                ar & id;
            }
        }
    }

    template <class TokFactory>
    void init( ns_RuleReader::Rule& r
             , TokFactory& tf
             , property_constructors<TokFactory> const& pmap );
    
    template<class TF> friend binary_rule<typename TF::token_t>
    index(binary_rule<fat_token> const& fr, TF& tf);

    friend void swap<>(binary_rule<TokenT>& r1, binary_rule<TokenT>& r2);
};

template<class TF> binary_rule<typename TF::token_t>
index(binary_rule<fat_token> const& fr, TF& tf)
{
    return binary_rule<typename TF::token_t>(index(fr.topology(),tf));
}

template <class O,class Tok,class TF>
void print(O&o,binary_rule<Tok> const& r,TF const& tf) 
{
    r.print(o,tf);
}


////////////////////////////////////////////////////////////////////////////////

template <class T>
bool operator!=(binary_rule<T> const& r1, binary_rule<T> const& r2)
{ return !(r1 == r2); }

template <class CharT, class TraitsT, class T>
std::basic_ostream<CharT,TraitsT>& 
operator << (std::basic_ostream<CharT,TraitsT>& os, binary_rule<T> const& ri);

////////////////////////////////////////////////////////////////////////////////

typedef binary_rule<indexed_token> indexed_binary_rule;
typedef binary_rule<fat_token>     fat_binary_rule;

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

namespace boost { 
namespace serialization {
template<class Token>
struct version< sbmt::binary_rule<Token> >
{
    BOOST_STATIC_CONSTANT(int, value = 3);
};
} // namespace serialization
} // namespace boost

#include <sbmt/grammar/impl/rule_input.ipp>

#endif // SBMT_GRAMMAR_RULE_INPUT_HPP
