# ifndef XRSDB__SEARCH__GRAMMAR_HPP
# define XRSDB__SEARCH__GRAMMAR_HPP

# include <filesystem.hpp>
# include <sbmt/token.hpp>

namespace xrsdb { namespace search {

template <class Type> 
struct rule_property_op {};

struct grammar_facade {
    typedef rule_application const* rule_type;
    typedef fixed_rule syntax_rule_type;
    typedef fixed_feature_vector feature_vector_type;
    
    fixed_rule const& get_syntax(rule_type r) const;
    
    bool rule_has_property(rule_type r, size_t id) const;
    
    template <class T> 
    typename rule_property_op<T>::result_type 
    rule_property(rule_type r, size_t id, T* pt = 0) const
    {
        return rule_property_op<T>()(r,id,this);
    }
    
    sbmt::in_memory_dictionary& dict() const;
    
    sbmt::feature_dictionary& feature_names() const;
    
    static void* id(rule_type r);
    
    template <class S>
    rule_type insert_terminal_rule( sbmt::indexed_token
                                  , S const& svec
                                  )
    {
        return 0;
    }
    
    template <class S, class T>
    rule_type insert_terminal_rule( sbmt::indexed_token
                                  , S const& svec
                                  , T const& text_feats
                                  )
    {
        return 0;
    }
    
    sbmt::weight_vector const& get_weights() const;
    
    sbmt::weight_vector& get_weights();
    
    size_t get_syntax_id(rule_type r) const;

    sbmt::indexed_token rule_lhs(rule_type r) const;
    
    size_t rule_rhs_size(rule_type r) const;
    
    sbmt::indexed_token rule_rhs(rule_type r, size_t idx) const;
    
    header* h;
    sbmt::weight_vector* w;
    grammar_facade(header* h, sbmt::weight_vector* w) : h(h),w(w) {}
};

void read_weights(sbmt::weight_vector& wv, std::istream& in, sbmt::feature_dictionary& fdict);

template <class C, class S>
std::basic_ostream<C,S>& print(std::basic_ostream<C,S>& os, grammar_facade::rule_type r, grammar_facade const& g)
{
    print(os,r->rule,g.dict());
    return os;
}

}} // namespace xrsdb::search

# endif // XRSDB__SEARCH__GRAMMAR_HPP