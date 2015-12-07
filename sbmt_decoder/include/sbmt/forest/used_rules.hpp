#ifndef SBMT__USED_RULES_HPP
#define SBMT__USED_RULES_HPP

//#include <sbmt/hash/oa_hashtable.hpp>
#include <sbmt/forest/dfs_forest.hpp>
#include <map>
#include <sbmt/grammar/syntax_id_type.hpp>
#include <sbmt/logmath.hpp>

namespace sbmt {

/// Which rules were used in anything in the forest (reachable from the top)?
/// Intended for building restricted grammars for further parsing passes.
template <class Gram>
struct used_syntax_rules :  public dfs_forest_visitor_base_ptr<used_syntax_rules<Gram>,dummy_result>
{
    typedef Gram grammar_type;
    grammar_type const& gram;
    used_syntax_rules(grammar_type const& gram) : gram(gram) {}
    
//    typedef oa_hashtable<syntax_id_type,unsigned> rule_uses_type;
    typedef std::map<syntax_id_type,unsigned> rule_uses_type;
    rule_uses_type rule_uses;
    
    template <class Edge>
    void visit_edge_ptr(Edge &edge,dummy_result &r,dummy_result *c1,dummy_result *c2)  {
        syntax_id_type id=edge.syntax_id(gram);
        
        if (id!=NULL_SYNTAX_ID)
            ++rule_uses[id];
    }

    template <class O>
    void print(O &o,char const*count_field="parse-forest-uses",bool print_score=true) const
    {
        for (rule_uses_type::const_iterator i=rule_uses.begin(),e=rule_uses.end();
             i!=e;++i) {            
            gram.print_syntax_rule_by_id(o,i->first,print_score);
            if (count_field)
                o << ' ' << count_field << '=' << score_t(i->second,as_neglog10());
            o << std::endl;
        }
    }

    /*
    template <class Equiv,class O>
    static void print_equiv(Equiv const& top_equiv,O &o,grammar_type const& g,
                            char const*count_field="parse-forest-uses",bool print_score=true)
    {
        visited_forest<used_syntax_rules<grammar_type> >,typename Equiv::edge_type> v(g);
        v(top_equiv);
        v.print(o,count_field,print_score);
    }
    */
};

template <class Equiv,class O,class grammar_type>
void print_used_syntax_rules(Equiv const& top_equiv,O &o,grammar_type const& g,
                             char const*count_field="parse-forest-uses",bool print_score=true)
{
    visited_forest<used_syntax_rules<grammar_type>,typename Equiv::edge_type> v(g);
    v(top_equiv);
    v.print(o,count_field,print_score);
}


}


#endif
