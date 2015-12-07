// connects sbmt::implicit_xrs_forest interface to node.h interface.
# ifndef FORESTREADER__FOREST_CONNECTION_HPP
# define FORESTREADER__FOREST_CONNECTION_HPP

# include <string>
# include <sbmt/forest/implicit_xrs_forest.hpp>
# include "node.h"
//# include <boost/tr1/unordered_map.hpp>
# include <map>

struct node_as_xforest;

struct xrs_grammar {
    typedef std::map<int,sbmt::indexed_syntax_rule> rule_map;
    rule_map rules;
    //std::set<int> rules;
    sbmt::indexed_token_factory d;
    sbmt::feature_names_type f;
    sbmt::feature_names_type& feature_names() { return f; }
    sbmt::indexed_token_factory& dict() { return d; }
    sbmt::weight_vector weights;
    //char foo[sizeof(sbmt::indexed_token_factory)];
};

//xrs_grammar* read_xrs_grammar(std::istream& in);
inline xrs_grammar* read_xrs_grammar(std::istream& in)
{
    xrs_grammar* gram(new xrs_grammar());
    std::string line;
    while (std::getline(in,line)) {
        //std::cerr << line << "\n---\n";
        rule_data r = parse_xrs(line);
        sbmt::indexed_syntax_rule rule(r,gram->dict());
        gram->rules.insert(std::make_pair(rule.id(),rule));
    }
    return gram;
}

struct node_as_xforest {
    std::string id_string() const;
    void* id() const;
    sbmt::xhyp_generator children() const;
    sbmt::score_t score() const;
    sbmt::indexed_token root() const;
    node_as_xforest(xrs_grammar* gram, Node* nd) 
    : gram(gram), nd(nd) {}
private:
    node_as_xforest();
    xrs_grammar* gram;
    Node* nd;
};

struct xtree;
typedef boost::shared_ptr<xtree> xtree_ptr;
typedef std::vector<xtree_ptr> xtree_children;
struct xtree {
    sbmt::score_t scr;
    sbmt::xhyp root;
    typedef std::tr1::unordered_map<int,xtree_ptr> children_type;
    children_type children;
    xrs_grammar* gram;
    template <class RNG>
    xtree(sbmt::xhyp const& root, RNG c, xrs_grammar* gram)
    : scr(1.0)
    , root(root)
    , gram(gram)
     {
         scr = sbmt::geom(root.scores(),gram->weights);
         typename boost::range_const_iterator<RNG>::type 
             itr = boost::begin(c),
             end = boost::end(c);
         sbmt::indexed_syntax_rule::rhs_iterator
             rhsitr = root.rule().rhs_begin(),
             rhsend = root.rule().rhs_end();
         size_t x = 0;
         for (; rhsitr != rhsend; ++rhsitr, ++x) {
             if (rhsitr->indexed()) {
                 children.insert(std::make_pair(x,*itr));
                 scr *= (*itr)->scr;
                 ++itr;
             }
         }
     }
};

typedef boost::tuple<sbmt::score_t,sbmt::feature_vector,std::string> nbest_data;
nbest_data nbest_line(xtree_ptr const& t, xrs_grammar* gram);

sbmt::weight_vector weights(xrs_grammar* gram, std::map<std::string,float>& w);

typedef gusc::any_generator<boost::shared_ptr<xtree>,gusc::iterator_tag> xtree_generator;

xtree_generator xtrees_from_xforest(sbmt::xforest const& forest, xrs_grammar* gram);

std::ostream& operator << (std::ostream& out, xtree_ptr const& t);

std::string nbest_features(sbmt::feature_vector const& f, xrs_grammar* gram);

std::string 
hypsent(xtree_ptr const& t, xrs_grammar* gram);

std::string 
hyptree(xtree_ptr const& t, sbmt::indexed_syntax_rule::tree_node const& n, xrs_grammar* gram);

sbmt::feature_vector accum(xtree_ptr const& t);

# endif // FORESTREADER__FOREST_CONNECTION_HPP
