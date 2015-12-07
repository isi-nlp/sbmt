# include <search/grammar.hpp>

namespace xrsdb { namespace search {
;

void read_weights(sbmt::weight_vector& wv, std::istream& in, sbmt::feature_dictionary& fdict)
{
    std::string line;
    while (getline(in,line)) {
        sbmt::weight_vector _wv;
        read(_wv,line,fdict);
        wv += _wv;
    }
    //SBMT_VERBOSE_STREAM(decoder_domain, "weights: " <<  print(wv,fdict));
}

fixed_rule const& grammar_facade::get_syntax(rule_type r) const
{
    return r->rule;
}

bool info_property(grammar_facade::rule_type r, size_t id);

bool grammar_facade::rule_has_property(rule_type r, size_t id) const 
{
    return info_property(r,id);
}

sbmt::in_memory_dictionary& grammar_facade::dict() const 
{ 
    return h->dict; 
}

sbmt::feature_dictionary& grammar_facade::feature_names() const 
{ 
    return h->fdict; 
}

//static 
void* grammar_facade::id(rule_type r) { return (void*)(r); }


sbmt::weight_vector const& grammar_facade::get_weights() const 
{ 
    return *w; 
}

sbmt::weight_vector& grammar_facade::get_weights()
{ 
    return *w; 
}

size_t grammar_facade::get_syntax_id(rule_type r) const 
{ 
    return r->rule.id(); 
}

sbmt::indexed_token grammar_facade::rule_lhs(rule_type r) const 
{ 
    return r->rule.lhs_root()->get_token(); 
}

size_t grammar_facade::rule_rhs_size(rule_type r) const 
{
    return r->rule.rhs_size(); 
}

sbmt::indexed_token grammar_facade::rule_rhs(rule_type r, size_t idx) const 
{ 
    return (r->rule.rhs_begin() + idx)->get_token();
}

}} // namespace xrsdb::search