# if ! defined(SBMT__SEARCH__BLOCK_LATTICE_TREE_HPP)
# define       SBMT__SEARCH__BLOCK_LATTICE_TREE_HPP

# include <sbmt/span.hpp>
# include <stdexcept>
# include <list>
# include <map>
# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/logmath.hpp>
# include <iosfwd>
# include <boost/shared_ptr.hpp>
# include <sbmt/grammar/grammar.hpp>
# include <gusc/lattice/ast.hpp>

namespace sbmt {


////////////////////////////////////////////////////////////////////////////////

struct lattice_edge {
    span_t span;
    indexed_token source;
    grammar_rule_id rule_id;
    grammar_rule_id syntax_rule_id;
    std::map<std::string,std::string> features;
    template <class Grammar>
    lattice_edge( Grammar& gram
                , span_t span
                , std::string const& source
                , std::map<std::string,score_t> const& score_vec
                , std::map<std::string,std::string> const& text_vec
                , std::string const& target
                , std::map<std::string,score_t> const& target_score_vec
                , std::map<std::string,std::string> const& target_text_vec );

    lattice_edge();
};

////////////////////////////////////////////////////////////////////////////////

class lattice_tree {
public:
    class node;
    typedef std::list<node>::const_iterator children_iterator;
    struct exception : std::logic_error
    {
        exception(std::string const& msg) : std::logic_error(msg) {}
        virtual ~exception() throw() {}
    };
private:
    struct node_impl {
        virtual sbmt::span_t span() const = 0;
        virtual bool is_internal() const = 0;
        virtual node_impl* clone() const = 0;

        virtual children_iterator children_begin() const
        { return throw_<children_iterator>("children_begin"); }

        virtual children_iterator children_end() const
        { return throw_<children_iterator>("children_end"); }
        

        virtual lattice_edge const& lat_edge() const
        { return throw_<lattice_edge const&>("lat_edge"); }

        virtual ~node_impl() {}
    private:
        template <class RT>
        RT throw_(std::string const& str) const
        {
            throw exception("function call " +str+ " unsupported by type");
        }
    };
public:
    template <class V>
    void visit_edges(V &v) const
    {
        visit_edges(root(),v);
    }

    template <class V>
    void visit_edges(node const& n,V &v) const
    {
        if (n.is_internal())
            for (children_iterator i=n.children_begin(),e=n.children_end();i!=e;++i)
                visit_edges(*i,v);
        else
            v(n.lat_edge());
    }

    class node {
        boost::shared_ptr<node_impl const> impl;
    public:
        bool is_internal() const { return impl->is_internal(); }

        sbmt::span_t span() const { return impl->span(); }
        node(lattice_edge const& e);
        node();
        template <class ItrT>
            node(ItrT begin, ItrT end);

        children_iterator children_begin() const
        { return impl->children_begin(); }
        children_iterator children_end() const
        { return impl->children_end(); }
        std::pair<children_iterator,children_iterator>
        children() const { return std::make_pair(impl->children_begin(),impl->children_end()); }
        lattice_edge const& lat_edge() const { return impl->lat_edge(); }
    };

    lattice_tree( size_t id
                , std::set<span_t> const& r
                , std::map<std::string,std::string> const& lf );

    lattice_tree( node const& n
                , size_t id
                , std::set<span_t> const& r
                , std::map<std::string,std::string> const& lf );

    size_t id;
    node root() const { return rt; }

    std::set<span_t> const& span_restrictions() const { return sr; }
    bool has_span_restrictions() const { return not sr.empty(); }
    std::map<std::string,std::string> const& feature_map() const { return features; }
private:

    struct internal_node : node_impl {
        span_t spn;
        virtual span_t span() const;
        virtual bool is_internal() const;
        virtual node_impl* clone() const;
        template <class ItrT>
            internal_node(ItrT itr, ItrT end);
        internal_node();
        std::list<node> children;
        virtual children_iterator children_begin() const;
        virtual children_iterator children_end() const;
        virtual ~internal_node();
    };

    struct leaf_node : node_impl {
        lattice_edge e;
        leaf_node(lattice_edge const& e);
        virtual node_impl* clone() const;
        virtual bool is_internal() const;
        virtual sbmt::span_t span() const;
        virtual lattice_edge const& lat_edge() const;
        virtual ~leaf_node();
    };

    node rt;
    std::set<span_t> sr;
    std::map<std::string,std::string> features;
    friend class node;
};

////////////////////////////////////////////////////////////////////////////////

template <class IT>
lattice_tree::node::node(IT itr, IT end)
: impl(new lattice_tree::internal_node(itr,end)) {}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
lattice_tree::internal_node::internal_node(IT itr, IT end)
{
    sbmt::span_index_t min(0), max(0);
    if (itr != end) {
        min = itr->span().left();
        max = itr->span().right();
    }
    for (; itr != end; ++itr) {
        min = std::min(min,itr->span().left());
        max = std::max(max,itr->span().right());
        children.push_back(*itr);
    }
    spn = sbmt::span_t(min,max);
}

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
lattice_tree::node convert(Grammar& grammar,gusc::lattice_ast::line const& line);


////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
lattice_tree convert(Grammar& grammar, gusc::lattice_ast const& ast);

} // namespace sbmt

# endif //     SBMT__SEARCH__BLOCK_LATTICE_TREE_HPP
