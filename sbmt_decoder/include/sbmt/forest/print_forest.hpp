#ifndef PRINT_FOREST_HPP
#define PRINT_FOREST_HPP

#include <ostream>
#include <sbmt/forest/dfs_forest.hpp>
#include <sbmt/forest/outside_score.hpp>
#include <sbmt/edge/edge_equivalence.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/indent_level.hpp>
#include <sbmt/printer.hpp>
#include <sbmt/forest/forest_em_output.hpp>

namespace sbmt {

struct print_forest_options 
{
    bool outside_score;
    bool meaningful_names;
    bool indent;
    bool oneperline;
    bool lmstring;
    bool syntax_features;
    bool whole_syntax_rule;
    bool edge_worse_by;
    bool emacs_outline_mode;
    void set_default() 
    {
        emacs_outline_mode=false;
        edge_worse_by=false;
        outside_score=false;
        meaningful_names=true;
        indent=true;
        oneperline=true;
        lmstring=false;
        syntax_features=true;
        whole_syntax_rule=false;
    }
    print_forest_options() 
    {
        set_default();
    }
};
    

template <class Edge,class Gram=grammar_in_memory,class O=std::ostream>
struct print_forest_visitor :
        public dfs_forest_visitor_base_ptr_local_state<print_forest_visitor<Edge,Gram,O>,edge_equivalence_impl<Edge> *,score_t> // pointer to equiv used as non-meaningful item name, score_t holds equiv_worse_by
{
    typedef Edge edge_type;
    typedef edge_equivalence<Edge>  edge_equiv_type;
    typedef edge_equivalence_impl<Edge>
    //typename edge_equiv_type::impl_type
    edge_equiv_impl;
    typedef edge_equiv_impl *edge_equiv_ptr;

    score_t best_score;
    bool first_equiv;
    
    typedef Gram grammar_type;
    typedef typename grammar_type::binary_rule_type binary_rule_type;
    typedef typename grammar_type::rule_type rule_type;
 private:
    O &o;
    grammar_type& gram;
    print_forest_options opt;
    typedef graehl::word_spacer_c<' '> Spacer;
    Spacer equiv_sep,edge_sep;
    graehl::indent_level depth;      
    bool lastwasblank;
    bool firstedge;
    concrete_edge_factory<Edge,Gram> ef;
    
 public:
    print_forest_visitor(O &o, grammar_type& gram, concrete_edge_factory<Edge,Gram> const& ef, print_forest_options const& opt=print_forest_options()) 
    : o(o)
    , gram(gram)
    , opt(opt)
    , ef(ef)
    { lastwasblank=false; firstedge=true; first_equiv=true; prep_indent(); }

    void prep_indent()
    {
        if (opt.emacs_outline_mode)
            depth.reset('*'," ");
        else
            depth.reset();
    }
    
    /// everything else is for dfs_forest:
    void start_equiv(edge_equiv_type const&equiv,edge_equiv_ptr &lhs_item,score_t &equiv_worse_by) {
        lhs_item=equiv.get_raw();
//        INF9("print_forest","Equiv at " << &equiv << prototype.getRootSpan());
//        INF9IN;
        if (!opt.indent) 
            o << equiv_sep;
        newline();
        o << "(";
        print_equiv_name(lhs_item);
        edge_type const& e=equiv.representative();
        if (first_equiv) {
            first_equiv=false;
            best_score=e.inside_score();
        }
        o << " inside=" << e.inside_score();
        if (opt.outside_score) {
            o << " outside="<< e.score();
            equiv_worse_by=e.score()*e.inside_score()/best_score;
            if (equiv_worse_by.nearly_equal(as_one()))
                equiv_worse_by.set(as_one());
            o << " global_worse_by="<<equiv_worse_by;
        }
        //<< " OR&" << &equiv << ':';
        ++depth;
        edge_sep.reset();
        firstedge=true;
        lastwasblank=false;
    }
    void finish_equiv(edge_equiv_type const&equiv,edge_equiv_ptr &lhs_item,score_t const&equiv_worse_by) {
//        INF9OUT;
//        INF9("print_forest","Equiv at " << &equiv << prototype.getRootSpan()  << "(size=" << equiv.size() << ") has " << lhs_item << " trees.");
        --depth;
        newline();
        if (!opt.indent) o << ' ';
        o << '/';
        print_equiv_name(lhs_item);
        o << ')';
        lastwasblank=false;
        newline();
        //              lastwasblank=true;
    }
    void visit_edge_ptr(edge_type const&edge,edge_equiv_ptr &lhs_item,edge_equiv_ptr *first_ant,edge_equiv_ptr *second_ant,score_t const&equiv_worse_by) {
        if (opt.oneperline) {
            newline();
        } else {
            if (firstedge) {
                firstedge=false;
                newline();
            }
            o << edge_sep;
        }              
        o << "( ";
        print_equiv_name(lhs_item);
        if (first_ant) {
            o << " <- ";
            print_equiv_name(*first_ant);
        }
        if (second_ant) {
            o << ' ';
            print_equiv_name(*second_ant);
        }              
        o << " | ";
        print_edge_features(edge,lhs_item,equiv_worse_by);      
        o << " )";
        lastwasblank=false;
    }
 private:    
    void print_edge_features(edge_type const& edge, edge_equiv_ptr& lhs_item, score_t const& equiv_worse_by) const {
        print_component_scores(o,edge,gram,ef,gusc::always_true());
        
        o << " | ";
        o << " p=" << edge.delta_inside_score();
        if (edge.has_rule_id()) {
            o << " p_tm=" << gram.rule_score(gram.rule(edge.rule_id()));
        }
        
        syntax_id_type sid=edge.syntax_id(gram);
        
        if (opt.edge_worse_by) {
            score_t inside_worse=edge.inside_score()/lhs_item->representative().inside_score();
            o << " local_worse_by="<<inside_worse;
            if (opt.outside_score)
                o << " global_worse_by="<<inside_worse*equiv_worse_by;
        }
        
        if (sid!=NULL_SYNTAX_ID) {
            indexed_syntax_rule const& srule=gram.get_syntax(sid);
            if (opt.whole_syntax_rule) {
                o << " | " << print(srule, gram.dict());
            } else {
                o << " | id=" << sid;
            }
        }
        
        //        else o << " p_rule="<<rule.score; // virtual -> heuristic only

    }
    void newline()  {
        if (!lastwasblank)// || opt.oneperline)
            if (opt.indent)
                depth.newline(o);
        lastwasblank=true;
    }
    
    void print_equiv_name(edge_equiv_type const& eq)
    {
        print_equiv_name(eq.get_raw());
    }
    void print_equiv_name(edge_equiv_ptr peq) 
    {
        if (opt.meaningful_names)
            o << ef.hash_string(gram,peq->representative());
        else
            o << peq;
    }
};

template <class O,class Edge,class Gram>
inline void print_forest(O &o,edge_equivalence<Edge> const & top,Gram& gram, concrete_edge_factory<Edge,Gram> const& ef, print_forest_options const& opt,outside_score &outside)
{
    if (opt.outside_score)
        outside.compute_equiv(top);
    typedef print_forest_visitor<Edge,Gram,O> PF;
    PF pf(o,gram,ef,opt);
    dfs_forest<PF,Edge> dfs(pf);
    dfs.compute(top);
    o << std::endl; // newline at end to help people dealing with several decodes' forests in one file (even though they can just look for the /TOP bracket or balanced parens, or indents ...
}

template <class O,class Edge,class Gram>
inline void print_forest(O &o,edge_equivalence<Edge> const & top,Gram& gram, concrete_edge_factory<Edge,Gram> const& ef, print_forest_options const& opt)
{
    outside_score out;
    print_forest(o,top,gram,ef,opt,out);
}

}

#endif
