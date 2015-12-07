# include <xrsparse/xrs.hpp>
# include "forest_connection.hpp"
# include <boost/lexical_cast.hpp>
# include <boost/tr1/unordered_map.hpp>
# include <gusc/generator/buchse_product_generator.hpp>

struct less_xtree_score {
    bool operator()( xtree_ptr const& t1, xtree_ptr const& t2 ) const
    {
        return t1->scr < t2->scr;
    }
};

void accum(xtree_ptr const& t,sbmt::feature_vector& f)
{
    f *= t->root.scores();
    BOOST_FOREACH(xtree::children_type::value_type const& c, t->children) {
        accum(c.second,f);
    }
}

sbmt::feature_vector accum(xtree_ptr const& t)
{
    sbmt::feature_vector f;
    accum(t,f);
    return f;
}

std::string 
hyptree(xtree_ptr const& t,sbmt::indexed_syntax_rule::tree_node const& n, xrs_grammar* gram)
{
    using namespace sbmt;
    if (n.indexed()) {
        xtree_ptr ct = t->children[n.index()];
        
        return hyptree(ct,*(ct->root.rule().lhs_root()),gram);
    } else if (n.lexical()) {
        return gram->dict().label(n.get_token());
    } else {
        bool closer = n.children_begin()->lexical();
        std::stringstream sstr;
        sstr << '(' << gram->dict().label(n.get_token());
        BOOST_FOREACH(indexed_syntax_rule::tree_node const& c, n.children()) {
            sstr << ' ' << hyptree(t,c,gram);
        }
        if (closer) sstr << ')';
        else sstr << " )";
        return sstr.str();
    }
}

void hypsent(xtree_ptr const& t, xrs_grammar* gram, std::stringstream& sstr)
{
    bool first = true;
    sbmt::indexed_syntax_rule::lhs_preorder_iterator 
        litr = t->root.rule().lhs_begin(),
        lend = t->root.rule().lhs_end();
    for (; litr != lend; ++litr) {
        if (litr->indexed()) {
            if (not first) sstr << ' ';
            hypsent(t->children[litr->index()],gram,sstr);
            first = false;
        } else if (litr->lexical()) {
            if (not first) sstr << ' ';
            sstr << gram->dict().label(litr->get_token());
            first = false;
        }
    }
}

std::string hypsent(xtree_ptr const& t, xrs_grammar* gram)
{
    std::stringstream sstr;
    hypsent(t,gram,sstr);
    return sstr.str();
}

std::string nbest_features(sbmt::feature_vector const& f, xrs_grammar* gram)
{
    std::stringstream sstr;
    sstr << sbmt::logmath::neglog10_scale;
    bool first = true;
    BOOST_FOREACH(sbmt::feature_vector::const_reference v, f) {
        if (not first) sstr << ' ';
        sstr << gram->feature_names().get_token(v.first) << '=' << v.second;
        first = false;
    }
    return sstr.str();
}

nbest_data nbest_line(xtree_ptr const& t, xrs_grammar* gram)
{
    sbmt::feature_vector feat = accum(t);
    sbmt::score_t scr = sbmt::geom(feat,gram->weights);
    std::string sent = hypsent(t,gram);
    return boost::make_tuple(scr,feat,sent);
}

struct less_xtree_children_score {
    bool operator()(xtree_children const& c1, xtree_children const& c2) const
    {
        sbmt::score_t s1;
        BOOST_FOREACH(xtree_ptr const& t1, c1) { s1 *= t1->scr; }
        sbmt::score_t s2;
        BOOST_FOREACH(xtree_ptr const& t2, c2) { s2 *= t2->scr; }
        return s1 < s2;
    }
};

struct append_xtree_children {
    typedef xtree_children result_type;
    xtree_children operator()(xtree_children c, xtree_ptr const& p) const
    {
        c.push_back(p);
        return c;
    }
};

struct make_xtree_children {
    typedef xtree_children result_type;
    xtree_children operator()(xtree_ptr const& p) const
    {
        return xtree_children(1,p);
    }
};


std::ostream& operator << (std::ostream& out, xtree_ptr const& t)
{
    if (not t->children.empty()) out << '(';
    out << t->root.rule().id();
    //out << sbmt::logmath::neglog10_scale;
    //sbmt::print(out,t->root.scores(),t->gram->feature_names());
    //out << '>';
    BOOST_FOREACH(xtree::children_type::value_type const& c, t->children) {
        out << ' ' << c.second;
    }
    if (not t->children.empty()) out << ')';
    return out;
}


typedef gusc::any_generator<std::vector< boost::shared_ptr<xtree> >,gusc::iterator_tag> xtree_children_generator;
typedef gusc::shared_lazy_sequence<xtree_children_generator> xtree_children_list;
typedef gusc::shared_lazy_sequence<xtree_generator> xtree_list;
typedef std::tr1::unordered_map<void*,xtree_list> ornode_map;

xtree_list
xtrees_from_xforest(sbmt::xforest const& forest, xrs_grammar* gram, ornode_map& omap);
xtree_generator 
xtrees_from_xhyp(sbmt::xhyp const& hyp, xrs_grammar* gram, ornode_map& omap);

xtree_children_generator 
generate_xtree_children(sbmt::xhyp const& hyp, xrs_grammar* gram, ornode_map& omap)
{
    int n = hyp.num_children();
    //std::cerr << "+++"<< n << "+++" << sbmt::print(hyp.rule(),gram->dict()) << "+++\n";
    
    xtree_children_generator ret;
    bool first = true;
    sbmt::indexed_syntax_rule::rhs_iterator
        rhsitr = hyp.rule().rhs_begin(),
        rhsend = hyp.rule().rhs_end();
    size_t x = 0;
    for (; rhsitr != rhsend; ++rhsitr, ++x) {
        if (rhsitr->indexed()) {
            if (first) {
                sbmt::xforest xf0 = hyp.child(x);
                ret = gusc::generator_as_iterator(
                        gusc::generate_transform(
                          gusc::generate_from_range(xtrees_from_xforest(xf0,gram,omap))
                        , make_xtree_children()
                        )
                      );
                first = false;
            } else {
                typedef gusc::shared_lazy_sequence<xtree_children_generator> 
                        xtree_children_list;
                typedef gusc::shared_lazy_sequence<xtree_generator> 
                        xtree_list;
                sbmt::xforest xfx = hyp.child(x);
                ret = gusc::generate_buchse_product(
                        append_xtree_children()
                      , less_xtree_children_score()
                      , xtree_children_list(ret)
                      , xtrees_from_xforest(xfx,gram,omap)
                      );
            }
        }
    }
    return ret; 
}

struct make_xtree {
    sbmt::xhyp hyp;
    xrs_grammar* gram;
    make_xtree(sbmt::xhyp h, xrs_grammar* g)
    : hyp(h)
    , gram(g) {}
    
    typedef xtree_ptr result_type;
    xtree_ptr operator()(xtree_children const& c) const
    {
        return xtree_ptr(new xtree(hyp,c,gram));
    }
};

xtree_generator 
xtrees_from_xhyp(sbmt::xhyp const& hyp, xrs_grammar* gram, ornode_map& omap)
{
    if (hyp.num_children() == 0) {
        return gusc::make_single_value_generator(
                 xtree_ptr(new xtree(hyp,xtree_children(),gram))
               );
    } else {
        return gusc::generator_as_iterator(
                 gusc::generate_transform(
                   generate_xtree_children(hyp,gram,omap)
                 , make_xtree(hyp,gram)
                 )
               );
    }
}

struct make_xtree_generator_from_xhyp {
    make_xtree_generator_from_xhyp(xrs_grammar* gram,ornode_map& omap) 
    : gram(gram)
    , omap(&omap) {}
    xrs_grammar* gram;
    ornode_map* omap;
    typedef xtree_generator result_type;
    xtree_generator operator()(sbmt::xhyp const& hyp) const
    {
        return xtrees_from_xhyp(hyp,gram,*omap);
    }
};

xtree_list 
xtrees_from_xforest(sbmt::xforest const& forest, xrs_grammar* gram, ornode_map& omap)
{
    ornode_map::iterator pos = omap.find(forest.id());
    if (pos != omap.end()) return pos->second;
    else {
        xtree_list lst(
         xtree_generator(
          gusc::generate_union_heap(
            gusc::generator_as_iterator(
              gusc::generate_transform(
                forest.begin()
              , make_xtree_generator_from_xhyp(gram,omap)
              )
            )
          , less_xtree_score()
          )
         )
        );
        omap.insert(std::make_pair(forest.id(),lst));
        return lst;
    }
}

struct toplevel_generator {
    typedef xtree_ptr result_type;
    boost::shared_ptr<ornode_map> omap;
    xtree_generator gen;
    toplevel_generator(sbmt::xforest const& forest, xrs_grammar* gram)
    : omap(new ornode_map())
    , gen(gusc::generate_from_range(xtrees_from_xforest(forest,gram,*omap)))
    {
        
    }
    operator bool() const { return bool(gen); }
    xtree_ptr operator()() { return gen(); }
};

xtree_generator xtrees_from_xforest(sbmt::xforest const& forest, xrs_grammar* gram)
{
    return gusc::generator_as_iterator(toplevel_generator(forest,gram));
}


std::string node_as_xforest::id_string() const
{
    return boost::lexical_cast<std::string>(id());
}

void* node_as_xforest::id() const
{
    return nd;
}

sbmt::weight_vector weights(xrs_grammar* gram, std::map<std::string,float>& w)
{
    std::stringstream sstr;
    Node::feature_vector_map::iterator 
        itr = w.begin(), 
        end = w.end();
    if (itr != end) {
        sstr << itr->first << ':' << itr->second;
        ++itr;
    }
    for (; itr != end; ++itr) {
        sstr << ',' << itr->first << ':' << itr->second;
    }
    sbmt::weight_vector wv;
    sbmt::read(wv,sstr.str(),gram->feature_names());
    return wv;
}

sbmt::feature_vector feats(xrs_grammar* gram, Node* nd)
{
    std::stringstream sstr;
    Node::feature_vector_map::iterator 
        itr = nd->feature_vector.begin(), 
        end = nd->feature_vector.end();
    if (itr != end) {
        sstr << itr->first << ':' << sbmt::pow(sbmt::score_t(0.1),itr->second);
        ++itr;
    }
    for (; itr != end; ++itr) {
        sstr << ',' << itr->first << ':' << sbmt::pow(sbmt::score_t(0.1),itr->second);
    }
    sbmt::feature_vector fv;
    sbmt::read(fv,sstr.str(),gram->feature_names());
    return fv;
}

struct andnode_xhyp_generator {
    typedef sbmt::xhyp result_type;
    sbmt::xhyp operator()()
    {
        if (more) {
            more = false;
            sbmt::feature_vector fv = feats(gram,nd);
            sbmt::score_t scr = sbmt::pow(sbmt::score_t(0.1),nd->best_cost);
            std::vector<sbmt::xforest> children;
            BOOST_FOREACH(Node* cnd, nd->children) {
                children.push_back(sbmt::xforest(node_as_xforest(gram,cnd)));
            }
            return sbmt::xhyp(scr,gram->rules[nd->id],fv,children);
        }
        throw std::logic_error("accessing past end of andnode_xhyp_generator");
    }
    operator bool() const { return more; }
    andnode_xhyp_generator(xrs_grammar* gram, Node* nd) 
    : gram(gram)
    , nd(nd)
    , more(true)
    {
        assert(not nd->is_or_node);
    }
private:
    xrs_grammar* gram;
    Node* nd;
    bool more;
};

struct ornode_xhyp_generator {
    typedef sbmt::xhyp result_type;
    sbmt::xhyp operator()()
    {
        if (itr == nd->children.end()) throw std::runtime_error("ornode out of bounds");
        Node* cnd = *itr;
        if (cnd->is_or_node) throw std::runtime_error("ornode child not andnode");
        ++itr;
        //std::cerr << "{AND id=" << cnd->id << "}\n";
        sbmt::score_t score = sbmt::pow(sbmt::score_t(0.1),cnd->best_cost);
        std::vector<sbmt::xforest> children;
        BOOST_FOREACH(Node* ccnd, cnd->children) {
            children.push_back(sbmt::xforest(node_as_xforest(gram,ccnd)));
        }
        sbmt::feature_vector fv = feats(gram,cnd);
        //std::cerr << "{/AND id=" << cnd->id << "}\n";
        return sbmt::xhyp(score,gram->rules[cnd->id],fv,children);
    }
    operator bool() const { return itr != nd->children.end(); }
    ornode_xhyp_generator(xrs_grammar* gram, Node* nd) 
    : gram(gram)
    , nd(nd)
    , itr(nd->children.begin()) {}
private:
    xrs_grammar* gram;
    Node* nd;
    Node::children_collection::iterator itr;
};

sbmt::xhyp_generator node_as_xforest::children() const
{
    if (nd->is_or_node) {
        //std::cerr << "[OR ref="<<nd->is_forest_reference<<" unk="<<nd->is_unknown_node<<"]\n";
        return gusc::generator_as_iterator(ornode_xhyp_generator(gram,nd));
    } else {
        //std::cerr << "[AND-AS-OR id="<<nd->id<<"ref="<<nd->is_forest_reference<<" unk="<<nd->is_unknown_node<<"]\n";
        return gusc::generator_as_iterator(andnode_xhyp_generator(gram,nd));
    }
}

sbmt::score_t node_as_xforest::score() const
{
    return sbmt::pow(sbmt::score_t(0.1),nd->best_cost);
}

sbmt::indexed_token node_as_xforest::root() const
{
    if (not nd->is_or_node) return gram->rules[nd->id].lhs_root()->get_token();
    if (nd->children.empty()) return gram->dict().tag("NONE");
    return gram->rules[nd->children.front()->id].lhs_root()->get_token();
}
