# if 0
# if ! defined(SBMT__FOREST__XRS_FOREST_HPP)
# define       SBMT__FOREST__XRS_FOREST_HPP

# include <sbmt/hash/hash_map.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/foreach.hpp>
# include <gusc/string/escape_string.hpp>
# include <sbmt/feature/feature_vector.hpp>

namespace sbmt {

template<class Edge, class Gram, class StopCondition>
void collect_tm_scores( feature_vector& scores_accum
                      , Edge const& e
                      , Gram const& gram
                      , StopCondition const& stop )
{
    if (e.has_rule_id()) {
      scores_accum *= gram.rule_scores(gram.rule(e.rule_id()));
    }

    if (e.has_first_child()) {
        if (not stop(e.first_child())) {
            collect_tm_scores(scores_accum,e.first_child(),gram,stop);
        }
        if (e.has_second_child() and not stop(e.second_child())) {
            collect_tm_scores(scores_accum,e.second_child(),gram,stop);
        }
    }
}

template<class Edge, class Gram>
void collect_tm_scores( feature_vector& scores_accum
                      , Edge const& e
                      , Gram const& gram)
{
  collect_tm_scores(scores_accum,e,gram,gusc::always_false());
}

template <class Info, class Gram>
feature_vector
component_scores( edge<Info> const& e
                , Gram& gram
                , concrete_edge_factory<edge<Info>,Gram> const& ef
                )
{
    feature_vector scores;
    collect_tm_scores(scores,e,gram,gusc::always_true());
    feature_vector info_scores;
    ef.component_info_scores(e,gram,info_scores,gusc::always_true());
    scores *= info_scores;
    return scores;
}


struct xrs_forest;
struct xrs_hyperedge;
typedef boost::shared_ptr<xrs_forest> xrs_forest_ptr;
typedef boost::shared_ptr<xrs_hyperedge> xrs_hyperedge_ptr;

struct xrs_hyperedge {
    typedef
        stlext::hash_map<size_t, xrs_forest_ptr>
        children_map;

    indexed_syntax_rule rule;
    feature_vector scores;
    children_map children;

    template <class Children>
    xrs_hyperedge(indexed_syntax_rule const& r, feature_vector const& v, Children const& c)
      : rule(r)
      , scores(v)
    {
        typename boost::range_const_iterator<Children>::type
            itr = boost::begin(c),
            end = boost::end(c);
        indexed_syntax_rule::rhs_iterator
            rhsitr = rule.rhs_begin(),
            rhsend = rule.rhs_end();
        size_t x = 0;
        for (; rhsitr != rhsend; ++rhsitr, ++x) {
            if (rhsitr->indexed()) {
                assert(itr != end);
                children.insert(std::make_pair(x,*itr));
                ++itr;
            }
        }
        assert(itr == end);
    }
};

struct xrs_forest {
    std::string hashid;
    std::vector< xrs_hyperedge_ptr > hyperedges;
    template <class Edge, class Factory, class Grammar>
    xrs_forest(edge_equivalence<Edge> const& eq, Factory const& factory, Grammar const& grammar)
    : hashid(factory.hash_string(grammar,eq.representative())) {}
};

////////////////////////////////////////////////////////////////////////////////

template <class Info>
class forest_xploder {
    typedef boost::tuple<
      feature_vector
    , std::vector<xrs_forest_ptr>
    > xplode_t;

    typedef
        stlext::hash_map<
            edge_equivalence< edge<Info> >
          , xrs_forest_ptr
          , boost::hash< edge_equivalence< edge<Info> > >
        >
        oldnewmap_t;

    oldnewmap_t oldnewmap;

    template <class Grammar, class Factory>
    xrs_forest_ptr
    xplode_forest(edge_equivalence< edge<Info> > const& eq, Grammar& gram, Factory const& ef)
    {
        using boost::shared_ptr;

        #ifndef NDEBUG
        BOOST_FOREACH(edge<Info> const& e, eq)
        {
            assert(e.has_syntax_id(gram));
        }
        #endif // NDEBUG

        typename oldnewmap_t::iterator pos = oldnewmap.find(eq);
        if (pos != oldnewmap.end()) {
            return pos->second;
        }
        xrs_forest_ptr xforest(new xrs_forest(eq,ef,gram));
        BOOST_FOREACH(edge<Info> const& e, eq)
        {
            BOOST_FOREACH(xrs_hyperedge_ptr hyp, xplode_hyps(e,gram,ef))
            {
                xforest->hyperedges.push_back(hyp);
            }
        }
        assert(xforest->hyperedges.size() > 0);
        oldnewmap.insert(std::make_pair(eq,xforest));
        return xforest;
    }

    template <class Grammar, class Factory>
    std::vector< xrs_hyperedge_ptr >
    xplode_hyps(edge<Info> const& e, Grammar& gram, Factory const& ef)
    {
        using std::vector;
        using boost::shared_ptr;
        using boost::get;

        indexed_syntax_rule rule = gram.get_syntax(gram.rule(e.rule_id()));
        feature_vector scores = component_scores(e,gram,ef);

        vector< xrs_hyperedge_ptr > hyps;
        if (e.child_count() == 0) {
            xrs_hyperedge_ptr
                hyp(new xrs_hyperedge(rule,scores,vector< xrs_forest_ptr >()));
            hyps.push_back(hyp);
        } else {
            BOOST_FOREACH(xplode_t const& x, xplode(e,gram,ef))
            {
                xrs_hyperedge_ptr
                    hyp(new xrs_hyperedge(rule, scores * get<0>(x), get<1>(x)));
                hyps.push_back(hyp);
            }
        }
        return hyps;
    }

    template <class Grammar, class Factory>
    std::vector<xplode_t>
    xplode(edge<Info> const& e, Grammar& gram, Factory const& ef)
    {
        using std::vector;
        using boost::shared_ptr;
        using boost::get;

        vector< vector<xplode_t> > prechildren;
        if (e.has_first_child()) {
            prechildren.push_back(xplode_ornode(e.first_children(),gram,ef));
            if (e.has_second_child()) {
                prechildren.push_back(xplode_ornode(e.second_children(),gram,ef));
            }
        }
        vector<xplode_t> gc(1);

        BOOST_FOREACH(vector<xplode_t> const& pc, prechildren)
        {
            vector<xplode_t> gnc;
            BOOST_FOREACH(xplode_t const& x, gc)
            {
                BOOST_FOREACH(xplode_t const& y, pc)
                {
                    vector< xrs_forest_ptr > children = get<1>(x);
                    children.insert(children.end(),get<1>(y).begin(),get<1>(y).end());
                    gnc.push_back(
                        xplode_t(
                            get<0>(x) * get<0>(y)
                          , children
                        )
                    );
                }
            }
            gc.swap(gnc);
        }
        return gc;
    }

    template <class Grammar, class Factory>
    std::vector<xplode_t>
    xplode_ornode(edge_equivalence< edge<Info> > const& eq, Grammar& gram, Factory const& ef)
    {
        using std::vector;
        using boost::shared_ptr;
        using boost::get;

        vector<xplode_t> retval;

        if (not eq.representative().has_syntax_id(gram)) {
            BOOST_FOREACH(edge<Info> const& e, eq) {
                BOOST_FOREACH(xplode_t const& x, xplode_andnode(e,gram,ef)) {
                    retval.push_back(x);
                }
            }
            assert(retval.size() > 0);
        } else {
            vector< xrs_forest_ptr > f(1,xplode_forest(eq,gram,ef));
            retval.push_back(xplode_t(feature_vector(),f));
        }

        return retval;
    };

    template <class Grammar, class Factory>
    std::vector<xplode_t>
    xplode_andnode(edge<Info> const& e, Grammar& gram, Factory const& ef)
    {
        using std::vector;
        using boost::shared_ptr;
        using boost::get;

        vector<xplode_t> retval;
        assert(not e.has_syntax_id(gram));
        feature_vector feats = component_scores(e,gram,ef);

        if (e.child_count() == 0) {
            retval.push_back(xplode_t(feats,vector< xrs_forest_ptr >()));
        } else {
            retval = xplode(e,gram,ef);
            BOOST_FOREACH(xplode_t& x, retval) {
                get<0>(x) = feats * get<0>(x);
            }
            assert(retval.size() > 0);
        }

        return retval;
    }

public:
    template <class Grammar, class Factory>
    xrs_forest_ptr
    operator()(edge_equivalence< edge<Info> > const& eq, Grammar& gram, Factory const& ef)
    {
        return xplode_forest(eq,gram,ef);
    }

    template <class Factory>
    forest_xploder(Factory const& ef) {}
};

////////////////////////////////////////////////////////////////////////////////

template <class Info, class Gram, class Factory>
xrs_forest_ptr
xplode_forest(edge_equivalence< edge<Info> > const& eq, Gram& gram, Factory const& ef)
{
    forest_xploder<Info> x(ef);
    return x(eq,gram,ef);
}

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
class xrs_forest_printer {
public:
    struct hyperedge_printer_interface {
        virtual bool supply_parens(xrs_hyperedge_ptr hyp) const = 0;
        virtual void print_hyperedge(std::ostream&, xrs_hyperedge_ptr, Grammar&) = 0;
        virtual ~hyperedge_printer_interface(){}
    };

    struct forest_printer_interface {
        virtual void print_forest(std::ostream&, xrs_forest_ptr, Grammar&) = 0;
        virtual ~forest_printer_interface(){}
    };
    boost::shared_ptr<hyperedge_printer_interface> h;
    boost::shared_ptr<forest_printer_interface> f;
    typedef stlext::hash_map<xrs_forest_ptr,bool,boost::hash<xrs_forest_ptr> > occmap_t;
    typedef stlext::hash_map<xrs_forest_ptr,size_t, boost::hash<xrs_forest_ptr> > idmap_t;

    void maps(xrs_forest_ptr forest,occmap_t& occmap)
    {
        occmap_t::iterator pos = occmap.find(forest);
        if (pos != occmap.end()) {
            pos->second = true;
        } else {
            occmap.insert(std::make_pair(forest,false));
            BOOST_FOREACH(xrs_hyperedge_ptr& hyp, forest->hyperedges) {
                typedef std::pair<size_t const,xrs_forest_ptr > c_t;
                BOOST_FOREACH(c_t fp, hyp->children) {
                    maps(fp.second,occmap);
                }
            }
        }
    }

    occmap_t occmap;
    idmap_t idmap;

    void print_features(std::ostream& os, feature_vector const& m, feature_dictionary& dict)
    {
        os << logmath::neglog10_scale;
        os << '<';
        os << print(m,dict);
        os << '>';
    }

    void print_forest(std::ostream& os, xrs_forest_ptr forest, Grammar& gram)
    {
        f->print_forest(os,forest,gram);
    }

    void print_hyperedge(std::ostream& os, xrs_hyperedge_ptr hyp, Grammar& gram)
    {
        h->print_hyperedge(os,hyp,gram);
    }

    bool supply_parens(xrs_hyperedge_ptr hyp) const
    {
        return h->supply_parens(hyp);
    }

    void operator()(std::ostream& os, xrs_forest_ptr forest, Grammar& grammar)
    {
        idmap.clear();
        occmap.clear();
        maps(forest,occmap);
        print_forest(os,forest,grammar);
        idmap.clear();
        occmap.clear();
    }
};

template <class Grammar>
struct xrs_traditional_forest_printer
  : xrs_forest_printer<Grammar>::forest_printer_interface {
    xrs_forest_printer<Grammar>* printer;

    xrs_traditional_forest_printer(xrs_forest_printer<Grammar>* printer)
    : printer(printer) {}

    void print_forest(std::ostream& os, xrs_forest_ptr forest, Grammar& gram)
    {
        assert(forest->hyperedges.size() > 0);
        bool parens = false;
        if (printer->occmap[forest]) {
            typename xrs_forest_printer<Grammar>::idmap_t::iterator
                pos = printer->idmap.find(forest);
            if (pos != printer->idmap.end()) {
                os << '#' << pos->second;
                return;
            } else {
                size_t id = printer->idmap.size() + 1;
                printer->idmap.insert(std::make_pair(forest,id));
                os << '#' << id;
                parens = true;
            }
        }
        if (forest->hyperedges.size() > 1) {
            os << "(OR";
            BOOST_FOREACH(xrs_hyperedge_ptr hyp, forest->hyperedges) {
                os << ' ';
                printer->print_hyperedge(os,hyp,gram);
            }
            os << " )";
        } else {
            parens = parens && !printer->supply_parens(forest->hyperedges[0]);
            if (parens) os << '(';
            printer->print_hyperedge(os,forest->hyperedges[0],gram);
            if (parens) os << " )";
        }
    }
};

template <class Grammar>
struct xrs_mergeable_forest_printer
  : xrs_forest_printer<Grammar>::forest_printer_interface {
    xrs_forest_printer<Grammar>* printer;
    xrs_mergeable_forest_printer(xrs_forest_printer<Grammar>* printer)
    : printer(printer) {}
    virtual void print_forest(std::ostream& os, xrs_forest_ptr forest, Grammar& gram)
    {
        assert(forest->hyperedges.size() > 0);
        typename xrs_forest_printer<Grammar>::idmap_t::iterator
            pos = printer->idmap.find(forest);
        if (pos != printer->idmap.end()) {
            os << '#' << pos->second;
        } else {
            os << "(OR #" << forest->hashid;
            if (printer->occmap[forest]) {
                size_t id = printer->idmap.size() + 1;
                printer->idmap.insert(std::make_pair(forest,id));
                os << " #" << id;
            }

            BOOST_FOREACH(xrs_hyperedge_ptr hyp, forest->hyperedges) {
                os << ' ';
                printer->print_hyperedge(os,hyp,gram);
            }
            os << " )";
        }
    }
};



template <class Grammar>
struct xrs_id_hyperedge_printer
  : xrs_forest_printer<Grammar>::hyperedge_printer_interface {
    xrs_forest_printer<Grammar>* printer;

    xrs_id_hyperedge_printer(xrs_forest_printer<Grammar>* printer)
      : printer(printer) {}

    virtual bool supply_parens(xrs_hyperedge_ptr hyp) const
    {
        return hyp->children.size() != 0;
    }

    virtual void print_hyperedge(std::ostream& os, xrs_hyperedge_ptr hyp, Grammar& gram)
    {
        std::stringstream sstr;
        sstr << hyp->rule.id();
        printer->print_features(sstr,hyp->scores,gram.feature_names());
        if (!hyp->children.empty()) {
            os << '(' << sstr.str();
            indexed_syntax_rule::rhs_iterator
                ritr = hyp->rule.rhs_begin(),
                rend = hyp->rule.rhs_end();
            size_t x = 0;
            for (; ritr != rend; ++ritr, ++x) {
                if (ritr->indexed()) {
                    os << ' ';
                    printer->print_forest(os,hyp->children[x],gram);
                }
            }
            os << " )";
        } else {
            os << sstr.str();
        }
    }
};

template <class Grammar>
struct xrs_target_tree_hyperedge_printer
  : xrs_forest_printer<Grammar>::hyperedge_printer_interface {
    xrs_forest_printer<Grammar>* printer;

    xrs_target_tree_hyperedge_printer(xrs_forest_printer<Grammar>* printer)
      : printer(printer) {}

    bool supply_parens(xrs_hyperedge_ptr hyp) const { return true; }

    void print_internal(std::ostream& os, indexed_syntax_rule::tree_node const& nd, xrs_hyperedge_ptr hyp, Grammar& gram)
    {
        if (nd.indexed()) {
            printer->print_forest(os,hyp->children[nd.index()],gram);
        } else if (nd.lexical()) {
            os << '"' << gusc::escape_c(gram.dict().label(nd.get_token())) << '"';
        } else {
            os << '(' << print(nd.get_token(),gram.dict());
            indexed_syntax_rule::lhs_children_iterator
                itr = nd.children_begin(),
                end = nd.children_end();
            for (; itr != end; ++itr) {
                os << ' ';
                print_internal(os,*itr,hyp,gram);
            }
            os << " )";
        }
    }

    void print_hyperedge(std::ostream& os, xrs_hyperedge_ptr hyp, Grammar& gram)
    {
        std::stringstream sstr;
        printer->print_features(sstr,hyp->scores,gram.feature_names());
        os << '(' << print(hyp->rule.lhs_root()->get_token(),gram.dict())
           << '~' << hyp->rule.id() << sstr.str();
        indexed_syntax_rule::lhs_children_iterator
            litr = hyp->rule.lhs_root()->children_begin(),
            lend = hyp->rule.lhs_root()->children_end();
        for (; litr != lend; ++litr) {
            os << ' ';
            print_internal(os,*litr,hyp,gram);
        }
        os << " )";
    }
};

template <class Grammar>
struct xrs_target_string_hyperedge_printer
  : xrs_forest_printer<Grammar>::hyperedge_printer_interface {
    xrs_forest_printer<Grammar>* printer;

    xrs_target_string_hyperedge_printer(xrs_forest_printer<Grammar>* printer)
      : printer(printer) {}

    bool supply_parens(xrs_hyperedge_ptr hyp) const { return true; }

    void print_hyperedge(std::ostream& os, xrs_hyperedge_ptr hyp, Grammar& gram)
    {
        std::stringstream sstr;
        printer->print_features(sstr,hyp->scores,gram.feature_names());
        os << '(' << hyp->rule.id() << sstr.str();
        indexed_syntax_rule::lhs_preorder_iterator
            litr = hyp->rule.lhs_begin(),
            lend = hyp->rule.lhs_end();

        for (; litr != lend; ++litr) {
            if (litr->indexed()) {
                os << ' ';
                printer->print_forest(os,hyp->children[litr->index()],gram);
            } else if(litr->lexical()) {
                os << ' ' << '"' << gusc::escape_c(gram.dict().label(litr->get_token())) << '"';
            }
        }
        os << " )";
    }
};

template <class Gram>
void print_target_string_forest(std::ostream& os, xrs_forest_ptr ptr, Gram& gram)
{
    xrs_forest_printer<Gram> printer;
    printer.h.reset(new xrs_target_string_hyperedge_printer<Gram>(&printer));
    printer.f.reset(new xrs_traditional_forest_printer<Gram>(&printer));
    printer(os,ptr,gram);
}

template <class Gram>
void print_id_forest(std::ostream& os, xrs_forest_ptr ptr, Gram& gram)
{
    xrs_forest_printer<Gram> printer;
    printer.h.reset(new xrs_id_hyperedge_printer<Gram>(&printer));
    printer.f.reset(new xrs_traditional_forest_printer<Gram>(&printer));
    printer(os,ptr,gram);
}

} // namespace sbmt

# endif //     SBMT__FOREST__XRS_FOREST_HPP
# endif // 0
