# include <sbmt/edge/any_info.hpp>
# include <sbmt/grammar/rule_feature_constructors.hpp>
# include <gusc/generator/single_value_generator.hpp>

using namespace sbmt;
using namespace boost;

////////////////////////////////////////////////////////////////////////////////

class td_info : public info_base<td_info> {
public:
    bool equal_to(td_info const& other) const
    {
        return true;
    }

    size_t hash_value() const { return 0; }
private:
    unsigned short len;
};

template <class C, class T, class TF>
void print(std::basic_ostream<C,T>&, td_info const&, TF const& tf)
{
    return;
}

////////////////////////////////////////////////////////////////////////////////

class td_info_factory 
: public info_factory_new_component_scores<td_info_factory>
{
public:
    typedef td_info info_type;

    typedef tuple<info_type,score_t,score_t> result_type;

    td_info_factory(weight_vector const& weights, feature_dictionary& dict, size_t t_feature_id)
      : t_weight(get(weights,dict,"tl"))
      , d_weight(get(weights,dict,"ds"))
      , t_id(dict.get_index("tl"))
      , d_id(dict.get_index("ds"))
      , t_feature_id(t_feature_id) {}

    template <class Grammar>
    score_t text_length(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        indexed_lm_string const& lmstr =
            grammar.template rule_property<indexed_lm_string>(rule,t_feature_id);
        return pow(score_t(0.1),lmstr.n_native_tokens());
    }
    
    template <class Grammar>
    std::string hash_string(Grammar const& grammar, info_type const& info) const
    {
        return "";
    }

    template <class Grammar>
    score_t
    rule_heuristic(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        return 1.0;
    }

    template <class Grammar>
    bool
    scoreable_rule(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        return grammar.rule_has_property(rule,t_feature_id);
    }

    typedef gusc::single_value_generator<result_type> result_generator;
    template <class ConstituentIterator>
    result_generator
    create_info( grammar_in_mem const& grammar
               , grammar_in_mem::rule_type rule
               , span_t span
               , iterator_range<ConstituentIterator> const& range )
    {
        score_t inside = 1.0;
        if (grammar.is_complete_rule(rule) and d_weight != 0)
            inside *= pow(score_t(0.1),d_weight);
        if (t_weight != 0)
            inside *= pow(text_length(grammar,rule),t_weight);

        return make_tuple(td_info(),inside,1.0);
    }

    ////////////////////////////////////////////////////////////////////////////

    template <class ConstituentIterator, class ScoreOutputIterator>
    ScoreOutputIterator
    component_scores_old( grammar_in_mem const& grammar
                        , grammar_in_mem::rule_type rule
                        , span_t span
                        , iterator_range<ConstituentIterator> const& range
                        , info_type const& result
                        , ScoreOutputIterator scores_out )
    {
        *scores_out = std::make_pair(t_id,text_length(grammar,rule));
        ++scores_out;
        *scores_out = grammar.is_complete_rule(rule)
                    ? std::make_pair(d_id,score_t(0.1))
                    : std::make_pair(d_id,score_t(1));
        ++scores_out;
        return scores_out;
    }

    ////////////////////////////////////////////////////////////////////////////

private:
    double t_weight;
    double d_weight;
    boost::uint32_t t_id;
    boost::uint32_t d_id;
    size_t t_feature_id;
};

////////////////////////////////////////////////////////////////////////////////

struct td_constructor {
    options_map get_options() const { return options_map("no options"); }

    void init(sbmt::in_memory_dictionary& dict) {}

    // options set via options_map
    bool set_option(std::string,std::string) { return false; }

    template <class Grammar>
    td_info_factory construct( Grammar& grammar
                             , lattice_tree const&
                             , property_map_type pmap ) const
    {
        return td_info_factory(grammar.get_weights(), grammar.feature_names(), pmap["lm_string"]);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct td_init {
    td_init()
    {
        register_info_factory_constructor("td", td_constructor());
        register_rule_property_constructor("td","lm_string",lm_string_constructor());
    }
};

static td_init tinit;

