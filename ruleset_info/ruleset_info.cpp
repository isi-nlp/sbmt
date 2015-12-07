# include <sbmt/edge/any_info.hpp>
# include <sbmt/search/lazy/rules.hpp>
# include <boost/tuple/tuple.hpp>
# include <set>

using namespace sbmt;
using namespace sbmt::lazy;

typedef std::map< grammar_in_mem::rule_type, std::set<int> > ruleshare_map;

void ruleshare_map_entry( grammar_in_mem const& gram
                        , ruleshare_map& rsm
                        , lhs_map const& lmap
                        , int id
                        , grammar_in_mem::rule_type r
                        )
{
    ruleshare_map::iterator pos = rsm.find(r);
    if (pos == rsm.end()) {
        pos = rsm.insert(std::make_pair(r,std::set<int>())).first;
    }
    pos->second.insert(id);
    for (size_t x = 0; x != gram.rule_rhs_size(r); ++x) {
        if (is_virtual_tag(gram.rule_rhs(r,x))) {
            ruleshare_map_entry(gram,rsm,lmap,id,lmap.find(gram.rule_rhs(r,x))->second);
        }
    }
}

ruleshare_map make_ruleshare_map(grammar_in_mem const& gram)
{
    ruleshare_map rsm;
    lhs_map lmap = make_lhs_map(gram);
    BOOST_FOREACH(grammar_in_mem::rule_type r, gram.all_rules()) {
        if (gram.is_complete_rule(r)) {
            ruleshare_map_entry(gram,rsm,lmap,gram.get_syntax(r).id(),r);
        }
    }
    return rsm;
}

struct indicator_generator {
    typedef boost::tuple<bool,score_t,score_t> result_type;
    indicator_generator(bool x) : more_(x) {}
    operator bool() const { return more_; }
    result_type operator()() 
    { 
        bool ret = more_; 
        more_ = !more_; 
        score_t scr = ret ? 1.0 : 0.0;
        return boost::make_tuple(ret,scr,scr); 
    }
private:
    bool more_;
};

struct ruleshare_info_factory
: sbmt::info_factory_new_component_scores<ruleshare_info_factory> 
{
    typedef bool info_type;
    typedef boost::tuple<info_type,score_t,score_t> result_type;
    typedef indicator_generator result_generator;
    
    typedef grammar_in_mem Grammar;
    
    std::string hash_string(Grammar const& grammar, info_type const& info) const
    {
        return "";
    }

    score_t
    rule_heuristic(Grammar const& grammar, Grammar::rule_type rule) const
    {
        return intersect(rule) ? 1.0 : 0.0;
    }
    
    bool
    scoreable_rule(Grammar const& grammar, Grammar::rule_type rule) const
    {
        return true;
    }
    
    template <class Constituents>
    result_generator
    create_info( Grammar const& grammar
               , Grammar::rule_type rule
               , span_t span
               , Constituents const& range )
    {
        return result_generator(intersect(rule));
    }
    
    template <class Constituents, class ScoreOutputIterator>
    ScoreOutputIterator
    component_scores_old( Grammar const& grammar
                        , Grammar::rule_type rule
                        , span_t span
                        , Constituents const& range
                        , info_type const& result
                        , ScoreOutputIterator scores_out )
    {
        return scores_out;
    }
    
    ruleshare_info_factory(Grammar const& grammar, std::set<int> const& constraints)
    : rsm(new ruleshare_map(make_ruleshare_map(grammar)))
    , constraints(constraints) {}
    
private:
    bool intersect(std::set<int> const& s1, std::set<int> const& s2) const
    {
        if (s1.size() > s2.size()) return intersect(s2,s1);
        BOOST_FOREACH(int x, s1) {
            if (s2.find(x) != s2.end()) return true;
        }
        return false;
    }
    
    bool intersect(Grammar::rule_type rule) const
    {
        ruleshare_map::iterator pos = rsm->find(rule);
        if (pos == rsm->end()) return true;
        else return intersect(constraints,pos->second);
    }
    
    boost::shared_ptr<ruleshare_map> rsm;
    std::set<int> constraints;
};

struct ruleshare_construct {
    std::set<int> constraints;
    void init(sbmt::in_memory_dictionary& dict) {}
    bool set_option(std::string key,std::string value)
    {
        if(key == "ruleset-constraints") {
            std::stringstream sstr(value);
            constraints.clear();
            std::copy( std::istream_iterator<int>(sstr)
                     , std::istream_iterator<int>()
                     , std::inserter(constraints,constraints.end())
                     )
                     ;
            return true;
        } else {
            return false;
        }
    }
    
    options_map get_options() const { return options_map("no options"); }
    
    ruleshare_info_factory construct( grammar_in_mem& grammar
                                    , lattice_tree const&
                                    , property_map_type pmap ) const
    {
        return ruleshare_info_factory(grammar,constraints);
    }
};

namespace {
struct init {
    init()
    {
        register_info_factory_constructor("ruleset", ruleshare_construct());
    }
};
}

static init initit;

