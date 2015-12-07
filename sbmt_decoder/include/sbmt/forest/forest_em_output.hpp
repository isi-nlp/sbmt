#ifndef SBMT_FOREST__FOREST_EM_OUTPUT_HPP
#define SBMT_FOREST__FOREST_EM_OUTPUT_HPP

#include <sbmt/forest/implicit_xrs_forest.hpp>
//#include <sbmt/forest/dfs_forest.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <boost/foreach.hpp>
#include <boost/enum.hpp>
#include <map>

namespace sbmt {

template <class O, class Edge, class Gram, class StopCondition>
void print_component_scores( O& o
                           , Edge const& e
                           , Gram& gram
                           , concrete_edge_factory<Edge,Gram> const& ef
                           , StopCondition const& stop
                           , char nv_sep = '='
                           , char score_sep = ' '
                           )
{
    feature_vector tm_scores,info_scores;
    collect_tm_scores(tm_scores,e,gram,stop);
    ef.component_info_scores(e,gram,info_scores,stop);

    feature_vector all_scores = tm_scores * info_scores;
    unsigned nbad=check_nonfinite_costs(all_scores,gram.feature_names(),true,"print_component_scores: ");
    if (nbad)
      SBMT_ERROR_STREAM(forest_domain,nbad<<" non-finite costs for edge "<<e);

    bool first = true;

    BOOST_FOREACH(feature_vector::value_type c, const_cast<feature_vector const&>(all_scores))
    {
        if (first) {
            first = false;
        } else {
            o << score_sep;
        }
        o << gram.feature_names().get_token(c.first) << nv_sep << c.second;
    }
}
#if 0
typedef unsigned backref_t;

struct forest_em_or_node
{
    BOOST_STATIC_CONSTANT(backref_t,FRESH=(backref_t)-1);
    // note: top_equiv would always be FRESH (unused by any other edge)
    BOOST_STATIC_CONSTANT(backref_t,ONCE=0); // used by exactly one edge; next time it's used it gets a backref id
    syntax_id_type leaf; // NULL_SYNTAX_ID if this is not a leaf
    backref_t bref;
    bool trivial; // subforest contains no non-singleton equiv, and no syntax-id having edge

    bool printed; // set to true for bref nodes once you gave the definition as #bref(...), so you next time just print #bref

    forest_em_or_node()
        : leaf(NULL_SYNTAX_ID)
        , bref(FRESH)
        , printed(false)
    {}
    void bump_bref(backref_t &nextb)
    {
        if (bref==FRESH)
            bref=ONCE;
        else if (bref==ONCE)
            bref=++nextb;
    }

    // the only syntax id in the subforest is a singleton.  so no need to share.
    bool is_syntax_leaf() const
    {
        return leaf!=NULL_SYNTAX_ID;
    }

    // ask only for non-syntax-leaf
    bool needs_bref() const
    {
        return bref!=ONCE && bref!=FRESH; // NOTE: top_equiv will always be fresh, so check that too
    }

    bool bref_already_printed()
    {
        if (!printed) {
            printed=true;
            return false;
        }
        return true;
    }

};



template <class Gram>
struct forest_em_output
    : public dfs_forest_visitor_base_ptr<forest_em_output<Gram>,forest_em_or_node>
{
    backref_t nextb;
    typedef forest_em_or_node Node;

    typedef Gram grammar_type;
    grammar_type &gram;
    forest_em_output(grammar_type &gram) : nextb(0),gram(gram) {}

    /*
    typedef Node result_type;
    typedef dummy_temp temp_type;
    template <class Edge>
    void visit_edge(Edge const& edge,Node &r,dummy_temp &t)
    { self().visit_edge_ptr(edge,r,(Node *)0,(Node *)0,t); }
    template <class Edge>
    void visit_edge(Edge const& edge,Node &r,Node &c1,dummy_temp &t)
    { self().visit_edge_ptr(edge,r,&c1,(Node *)0,t); }
    template <class Edge>
    void visit_edge(Edge const& edge,Node &r,Node &c1,Node &c2,dummy_temp &t)
    { self().visit_edge_ptr(edge,r,&c1,&c2,t); }
    */

    template <class Equiv>
    void start_equiv(Equiv const&equiv,Node &r,dummy_temp &d)
    {
        r.trivial=(equiv.size()==1); // later we may find that the edge has a nontrivial antecedent, though
    }
    template <class Equiv>
    void finish_equiv(Equiv const&equiv,Node &r,dummy_temp &d)
    {}

    void use(Node &p,Node &c)
    {
        if (!c.trivial)
            p.trivial=false;
        c.bump_bref(nextb);
    }


    template <class Edge>
    void visit_edge_ptr(Edge const& edge,Node &r,Node *c1,Node *c2)
    {
        if (c1) {
            use(r,*c1);
            if (c2)
                use(r,*c2);
        }
        if (r.trivial) {
            if ((r.leaf=edge.syntax_id(gram)) != NULL_SYNTAX_ID)
                r.trivial=false;
        }
    }

    template <class O,class Edge>
    struct printer
    {
        typedef visited_forest<forest_em_output,Edge> V;
        O &o;
        grammar_type &gram;
        concrete_edge_factory<Edge,Gram> ef;
        V &v;
        syntax_id_type cons_id;
        bool keep_binary;
        bool print_score_vec;

        printer(O &o,grammar_type &gram,concrete_edge_factory<Edge,Gram> const& ef,V &v,syntax_id_type cons_id,bool keep_binary,bool print_score_vec)
            : o(o),gram(gram),ef(ef),v(v),cons_id(cons_id),keep_binary(keep_binary),print_score_vec(print_score_vec) {}

        typedef edge_equivalence<Edge> Equiv;
        typedef graehl::word_spacer &Sp;

        syntax_id_type out_id(Edge const& e)
        {
            syntax_id_type id=e.syntax_id(gram);
            return (id==NULL_SYNTAX_ID ? cons_id : id);
        }

        void print_scores(Edge const& e)
        {
            if (print_score_vec) {
                boost::function<bool(Edge const&)> stop_condition;
                if (keep_binary) {
                    stop_condition = gusc::always_true();
                } else {
                    stop_condition = stop_on_xrs();
                }
                o << '<';
                print_component_scores(o,e,gram,ef,stop_condition,':',',');
                o << '>';
            }
        }

        /* print a list, vs. just print:
           In some contexts, it's more concise (for EM training) to skip binary (non-syntax-rule) nodes.  Namely, when there is no second use of the subforest, and no OR.  One could even flatten entire subtrees (so the arities are wrong for the rules), with the same effect in forest-em.  So, instead of printing a tree, print a list.  Obviously, immediately inside an OR node, you do NOT print a list of all the rules involved in each alternative, but singly root each alternative.  Note: it's not 100% clear that this code works.  I'll add a runtime control disabling the list-ifying entirely and test more.
        */
        // print_list: ([space] subforest)*
        void print_list(Equiv const& eq)
        {
            Node const &n=v[eq];
            if (n.trivial) return;
            if (n.is_syntax_leaf()) {
                o << ' ' << n.leaf;
                print_scores(eq.representative());
                return;
            }
            bool singleton=(eq.size()==1);
            if (singleton && !n.needs_bref())
                print_list_edge(eq.representative());
            else {
                o << ' ';
                print(eq);
            }

        }

        void print_list_edge(Edge const& e)
        {
            syntax_id_type id=out_id(e);
            if (id!=cons_id) {
                o << ' ';
                print_edge(e);
                return;
            }
            if (e.has_first_child()) {
                print_list(e.first_children());
                if (e.has_second_child())
                    print_list(e.second_children());
            }
        }

        void print_children(Equiv const &eq)
        {
            if (keep_binary) {
                o << ' ';
                print(eq);
            } else
                print_list(eq);
        }

        void print_edge(Edge const& e)
        {
            syntax_id_type id=out_id(e);
            if (e.has_first_child()) {
                o << '('<<id;
                print_scores(e);
                print_children(e.first_children());
                if (e.has_second_child())
                    print_children(e.second_children());
                o << ')';
            } else {
                o << id;
                print_scores(e);
            }
        }


        void print_children_rec(Equiv const& eq)
        {
            print(eq);
        }


        void print_or(Equiv const& eq)
        {
            // non-singleton
            o << "(OR";

            typedef typename Equiv::edge_range ER;
            ER edges=eq.edges();
            for (typename ER::iterator i=edges.begin(),e=edges.end();i!=e;++i) {
                o << ' ';
                print_edge(*i);
            }

            o << ')';
        }

        void print(Equiv const& eq)
        {
            Node &n=v[eq];
            if (n.is_syntax_leaf() and not keep_binary) {
                o << n.leaf;
                print_scores(eq.representative());
                return;
            }
            bool singleton=(eq.size()==1);
            if (n.needs_bref()) {
                o << '#'<<n.bref;
                if (n.bref_already_printed())
                    return;
            }
            if (singleton)
                print_edge(eq.representative());
            else
                print_or(eq);
        }

    };


};
#endif

BOOST_ENUM_VALUES(
    xrs_forest_andnode_type
  , const char*
  , (id)("id")
    (target_string)("target-string")
    (target_tree)("target-tree")
);

BOOST_ENUM_VALUES(
    xrs_forest_ornode_type
  , const char*
  , (traditional)("traditional")
    (mergeable)("mergeable")
);

template <class O,class Gram>
void print_forest_em( O &o,
                      Gram &gram,
                      xforest xf,
                      xrs_forest_andnode_type andtype,
                      xrs_forest_ornode_type ortype,
                      size_t ornode_limit )
{
    if (ornode_limit == 0) ornode_limit = std::numeric_limits<size_t>::max();
    xrs_forest_printer<Gram> printer;

    if (andtype == xrs_forest_andnode_type::target_string) {
        printer.h.reset(new xrs_target_string_hyperedge_printer<Gram>(&printer));
    } else if (andtype == xrs_forest_andnode_type::id) {
        printer.h.reset(new xrs_id_hyperedge_printer<Gram>(&printer));
    } else if (andtype == xrs_forest_andnode_type::target_tree) {
        printer.h.reset(new xrs_target_tree_hyperedge_printer<Gram>(&printer));
    } else {
        throw std::runtime_error("unsupported xrs-forest-andnode-type passed to print_forest_em");
    }

    if (ortype == xrs_forest_ornode_type::traditional) {
        printer.f.reset(new xrs_traditional_forest_printer<Gram>(&printer,ornode_limit));
    } else if (ortype == xrs_forest_ornode_type::mergeable) {
        printer.f.reset(new xrs_mergeable_forest_printer<Gram>(&printer,ornode_limit));
    } else {
        throw std::runtime_error("unsupported xrs-forest-ornode-type passed to print_forest_em");
    }

    printer(o,xf,gram);
}

template <class O,class Edge,class Gram>
void print_forest_em( O &o,
                      Gram &gram,
                      concrete_edge_factory<Edge,Gram> const& ef,
                      edge_equivalence<Edge> const& top,
                      xrs_forest_andnode_type andtype,
                      xrs_forest_ornode_type ortype,
                      size_t ornode_limit )
{
    print_forest_em(o,gram,xforest(make_equiv_as_xforest(top,gram,ef)),andtype,ortype,ornode_limit);
}

}


#endif
