#include <RuleReader/Rule.h>
#include <RuleReader/RuleNode.h>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/version.hpp>

namespace sbmt {

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::tree_node_array
syntax_rule<T,A,C,O>::new_lhs_tree(size_t sz)
{
    C c;
    raw_tree_node_array_allocator tnaalloc(get_allocator());
    tree_node_array 
        ta = c(tnaalloc);
    /* avoid allocator construct until on a compiler with rvalue references */
    new (ta.get()) raw_tree_node_array(sz,tree_node(),tree_node_allocator_type(get_allocator()));
    return ta;
}

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::rule_node_array
syntax_rule<T,A,C,O>::new_rhs_rule(size_t sz)
{
    C c;
    raw_rule_node_array_allocator rnaalloc(get_allocator());
    rule_node_array 
        ra = c(rnaalloc);
    /* avoid allocator construct until on a compiler with rvalue references */
    new (ra.get()) raw_rule_node_array(sz,rule_node(),rule_node_allocator_type(get_allocator()));
    return ra;
}

////////////////////////////////////////////////////////////////////////////////
///
/// tree_node methods
///
////////////////////////////////////////////////////////////////////////////////
template <class T, class A, class C, class O>
syntax_rule<T,A,C,O>::tree_node::tree_node()
    : next_sibling(0)
    ,  first_child(0)
    , idx(0){}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
bool syntax_rule<T,A,C,O>::tree_node::indexed() const
{
    //leaf tree_nodes that are not lexemes are indexed.

    ///FIXME: how are we going to represent rules that use native tokens as
    ///their condition?
    return is_native_tag(t) and first_child == 0;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
unsigned syntax_rule<T,A,C,O>::tree_node::index() const
{
    return idx;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::lhs_children_iterator
syntax_rule<T,A,C,O>::tree_node::children_begin() const
{
    return typename
    syntax_rule<T,A,C,O>::lhs_children_iterator(first_child
                                          ? this + first_child
                                          : NULL );
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::lhs_children_iterator
syntax_rule<T,A,C,O>::tree_node::children_end() const
{
    return typename syntax_rule<T,A,C,O>::lhs_children_iterator(NULL);
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
bool syntax_rule<T,A,C,O>::tree_node::operator == (tree_node const& o) const
{
    return t == o.t
       and next_sibling == o.next_sibling
       and first_child == o.first_child
       and (not is_native_tag(t) or idx == o.idx);
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
bool syntax_rule<T,A,C,O>::tree_node::operator != (tree_node const& o) const
{
    return !this->operator==(o);
}

////////////////////////////////////////////////////////////////////////////////
///
/// rule_node methods
///
////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
syntax_rule<T,A,C,O>::rule_node::rule_node() {}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
bool syntax_rule<T,A,C,O>::rule_node::indexed() const
{
    return is_native_tag(t);
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
bool syntax_rule<T,A,C,O>::rule_node::operator == (rule_node const& o) const
{
    return t == o.t and (not is_native_tag(t) or lhs_pos == o.lhs_pos);

}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
bool syntax_rule<T,A,C,O>::rule_node::operator != (rule_node const& o) const
{
    return !this->operator==(o);
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
unsigned syntax_rule<T,A,C,O>::index(rule_node const& node) const
{
    return (lhs_tree->begin() + node.lhs_pos)->index();
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::lhs_preorder_iterator
syntax_rule<T,A,C,O>::lhs_position(rule_node const& node) const
{
    return lhs_tree->begin() + node.lhs_pos;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
syntax_rule<T,A,C,O>::lhs_children_iterator::lhs_children_iterator()
: curr(NULL) {}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
syntax_rule<T,A,C,O>::lhs_children_iterator::
lhs_children_iterator(tree_node_const_pointer curr)
: curr(curr) {}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
bool syntax_rule<T,A,C,O>::lhs_children_iterator::
equal(typename syntax_rule<T,A,C,O>::lhs_children_iterator const& other) const
{
    return curr == other.curr;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
void syntax_rule<T,A,C,O>::lhs_children_iterator::increment()
{
    curr = curr->next_sibling ? curr + curr->next_sibling : NULL;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::tree_node const&
syntax_rule<T,A,C,O>::lhs_children_iterator::dereference() const
{
    return *curr;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::tree_node_const_pointer
syntax_rule<T,A,C,O>::lhs_root() const
{
    return lhs_tree->begin();
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::lhs_preorder_iterator
syntax_rule<T,A,C,O>::lhs_begin() const
{
    return lhs_tree->begin();
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::lhs_preorder_iterator
syntax_rule<T,A,C,O>::lhs_end() const
{
    return lhs_tree->end();
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::rhs_iterator
syntax_rule<T,A,C,O>::rhs_begin() const
{
    return rhs_rule->begin();
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
typename syntax_rule<T,A,C,O>::rhs_iterator syntax_rule<T,A,C,O>::rhs_end() const
{
    return rhs_rule->end();
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
template <class TokenFactory>
syntax_rule<T,A,C,O>::syntax_rule(ns_RuleReader::Rule& r, TokenFactory &tf, A const& a)
 : allocator_type(a)
{
    init(r,tf);
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
template <class Range, class TokenFactory>
syntax_rule<T,A,C,O>::syntax_rule(Range const& rule_str, TokenFactory& tf, A const& a)
 : allocator_type(a)
{
    rule_data rd = parse_xrs(rule_str);
    init(rd,tf);
}

////////////////////////////////////////////////////////////////////////////////
///
/// todo:  constructors should take an allocator as argument to make
/// mempooling easier.  note, not required to template whole class on allocator,
/// since
///  -# no memory is allocated outside of constructor
///  -# shared_ptr can take a custom destruction functor that can
///     encapsulate allocator, and assure correct destruction.
///
////////////////////////////////////////////////////////////////////////////////
template <class T, class A, class C, class O>
template <class TokenFactory>
void syntax_rule<T,A,C,O>::init(ns_RuleReader::Rule& r, TokenFactory& tf)
{
    if (r.is_binarized_rule())
        throw_bad_rule_format("syntax_rule requires an xrs rule as input");
    if (r.numNodes() >= std::numeric_limits<rule_offset_t>::max() or
        r.getRHSLexicalItems()->size() >= std::numeric_limits<rule_offset_t>::max()) {
        throw_bad_rule_format(
            "rule size exceeds syntax_rule storage capacity. rule: "
          + r.entireString()
        );
    }
    
    m_id = boost::lexical_cast<syntax_id_type>(r.getAttributeValue("id"));
    lhs_tree = new_lhs_tree(r.numNodes());
    rhs_rule = new_rhs_rule(r.getRHSLexicalItems()->size());

    size_t idx = preorder_init(r.getLHSRoot(), 0, tf);
    assert(idx == lhs_tree->size());

    for (idx = 0; idx != rhs_rule->size(); ++idx) {
        if(r.getRHSLexicalItems()->at(idx) != "") {
            rhs_rule->at(idx).t = tf.foreign_word(r.getRHSLexicalItems()->at(idx));
        }
    }
}

template <class TokenFactory>
typename TokenFactory::token_t
token_from_lhs_node(lhs_node const& lnode, TokenFactory& tf)
{
    if (lnode.indexed or lnode.children) {
        if (lnode.label == "TOP") {
            return tf.toplevel_tag();
        } else {
            return tf.tag(lnode.label);
        }
    } else {
        return tf.native_word(lnode.label);
    }
}


template <class T, class A, class C, class O>
template <class TokenFactory>
void syntax_rule<T,A,C,O>::init(rule_data const& rd, TokenFactory& tf)
{
    if ( std::max(rd.lhs.size(),rd.rhs.size()) >=
         std::numeric_limits<rule_offset_t>::max() ) {
         std::stringstream sstr;
         sstr << "rule size exceeds syntax_rule storage capacity. rule: "
              << rd;
         throw_bad_rule_format(sstr.str());
    }

    m_id = rd.id;
    lhs_tree = new_lhs_tree(rd.lhs.size());
    rhs_rule = new_rhs_rule(rd.rhs.size());
    
    typename raw_tree_node_array::iterator 
        tree_pos = lhs_tree->begin(),
        tree_end = lhs_tree->end();
    lhs_node const* lhs_pos = &(rd.lhs[0]);

    for (; tree_pos != tree_end; ++tree_pos, ++lhs_pos) {
        tree_pos->t = token_from_lhs_node(*lhs_pos,tf);

        if (lhs_pos->indexed) {
            tree_pos->idx = rhs_position(rd,lhs_pos->index);
            rhs_rule->at(tree_pos->idx).lhs_pos = tree_pos - lhs_tree->begin();
            rhs_rule->at(tree_pos->idx).t = tree_pos->t;
        }

        tree_pos->next_sibling = lhs_pos->next == 0 ? 0 : (lhs_tree->begin() + lhs_pos->next) - tree_pos;
        tree_pos->first_child = lhs_pos->children ? 1 : 0;
    }

    typename raw_rule_node_array::iterator 
        rule_pos = rhs_rule->begin(),
        rule_end = rhs_rule->end();
    rhs_node const* rhs_pos = &(rd.rhs[0]);
    for (; rule_pos != rule_end; ++rule_pos, ++rhs_pos) {
        if (not rhs_pos->indexed) {
            rule_pos->t = tf.foreign_word(rhs_pos->label);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

namespace syntax_rule_impl {
struct lhs_aligner
{
    alignment a; // remember: rhs->lhs
    typedef std::vector<unsigned> L;
    L lhs_lex;
    template <class T, class A, class C, class O>
    lhs_aligner(syntax_rule<T,A,C,O> const& rule,bool lexical_align=true)
    {
        a.set_empty(rule.rhs_size(),rule.lhs_root()->n_leaves());
        rule.lhs_root()->visit_leaves(*this); // adds variable alignments
        unsigned rhs_i=0;

        if (!lexical_align)
            return;

        // fully connected word alignments
        for (typename syntax_rule<T,A,C,O>::rhs_iterator i=rule.rhs_begin(),e=rule.rhs_end();
             i!=e;++i,++rhs_i)
            if (i->lexical())
                for (L::iterator li=lhs_lex.begin(),le=lhs_lex.end();li!=le;++li)
                    a.add(rhs_i,*li);

    }

    // visit a leaf
    template <class tree_node>
    void operator()(unsigned lhs_i, tree_node const& t)
    {
        if (t.lexical())
            lhs_lex.push_back(lhs_i);
        else {
            assert(t.indexed());
            a.add(t.index(),lhs_i);
        }
    }

};


}


template <class T, class A, class C, class O>
alignment
syntax_rule<T,A,C,O>::default_alignment(bool lexical_align) const
{
    syntax_rule_impl::lhs_aligner la(*this,lexical_align);
    return la.a;
}


////////////////////////////////////////////////////////////////////////////////

template <class TokenFactory>
typename TokenFactory::token_t
token_from_xrs_rulenode(ns_RuleReader::RuleNode* r,TokenFactory& tf)
{
    if (r->isLexical()) return tf.native_word(r->getString(false,false));
    else if (r->isPointer()) return tf.tag(r->get_pos());
    else if (r->getString(false,false) == std::string("TOP")) {
        return tf.toplevel_tag();
    }
    else return tf.tag(r->getString(false,false));

}



////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
template <class TokenFactory>
typename syntax_rule<T,A,C,O>::rule_offset_t
syntax_rule<T,A,C,O>::preorder_init( ns_RuleReader::RuleNode* r
                                 , rule_offset_t idx
                                 , TokenFactory& tf )
{
    using ns_RuleReader::RuleNode;
    using namespace std;

    lhs_tree->at(idx).t = token_from_xrs_rulenode(r,tf);

    rule_offset_t idx_next = idx + 1;
    if (r->isLeaf()) {
        assert(is_native_tag(lhs_tree->at(idx).t) == r->isPointer());
        if (is_native_tag(lhs_tree->at(idx).t)) {
            lhs_tree->at(idx).idx = r->getRHSIndex();
            rhs_rule->at(r->getRHSIndex()).lhs_pos = idx;
            rhs_rule->at(r->getRHSIndex()).t = lhs_tree->at(idx).t;
        }
        return idx_next;
    }

    vector<RuleNode*>::iterator itr = r->getChildren()->begin();
    vector<RuleNode*>::iterator end = r->getChildren()->end();

    // x->first_child is the offset between x and the first
    // child of x.
    lhs_tree->at(idx).first_child = (lhs_tree->begin() + idx_next) -
                                    (lhs_tree->begin() + idx);

    for (; itr != end; ++itr) {
        rule_offset_t idx_new = preorder_init(*itr,idx_next, tf);
        vector<RuleNode*>::iterator peek = itr;
        ++peek;
        if (peek != end)
            // x->next_sibling is the offset between x and
            // the next sibling of x
            lhs_tree->at(idx_next).next_sibling = (lhs_tree->begin() + idx_new) -
                                                  (lhs_tree->begin() + idx_next);
        idx_next = idx_new;
    }
    return idx_next;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
template <class ArchiveT>
void syntax_rule<T,A,C,O>::save(ArchiveT & ar, const unsigned int version) const
{
    rule_offset_t ls = lhs_tree->size(), rs = rhs_rule->size();
    ar & ls;
    ar & rs;

    for (unsigned idx = 0; idx != lhs_tree->size(); ++idx)
        //FIXME: aren't you supposed to dispatch through ar again?
        //ANSWER: there's no such rule. serialization should be helped along
        //for efficiencies sake, sometimes.... but since i removed the
        //pointers from lhs_tree, its moot.  but not for rhs_rule.
        //
        //also, sometimes you dont want the public to serialize the parts of
        //your object, just the total sum.
        lhs_tree->at(idx).save(ar);

    for (unsigned idx = 0; idx != rhs_rule->size(); ++idx)
        rhs_rule->at(idx).save(ar);

    ar & m_id;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
template <class ArchiveT>
void syntax_rule<T,A,C,O>::load(ArchiveT & ar, const unsigned int version)
{
    rule_offset_t new_lhs_size, new_rhs_size;

    load_offset(ar, new_lhs_size, version);
    load_offset(ar, new_rhs_size, version);

    tree_node_array new_lhs = new_lhs_tree(new_lhs_size);
    rule_node_array new_rhs = new_rhs_rule(new_rhs_size);

    for (rule_offset_t idx = 0; idx != new_lhs_size; ++idx)
        new_lhs->at(idx).load(ar,version);
    for (rule_offset_t idx = 0; idx != new_rhs_size; ++idx)
        new_rhs->at(idx).load(ar,version);

    if (version == 2) {
        indexed_lm_string dep_lm_string;
        ar & dep_lm_string;
    }
    // 64 bit ids now
    if (version < 4) {
        boost::int32_t id_;
        ar & id_;
        m_id = id_;
    } else {
        ar & m_id;
    }

    std::swap(new_lhs,lhs_tree);
    std::swap(new_rhs,rhs_rule);
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
std::map<int,int> map_ids_rhs(syntax_rule<T,A,C,O> const& rule)
{
    std::map<int,int> id_map;
    typename syntax_rule<T,A,C,O>::lhs_preorder_iterator i = rule.lhs_begin(),
                                                   e = rule.lhs_end();
    int id_count = 0;
    for (; i != e; ++i) if (i->indexed()) id_map[i->index()] = id_count++;

    return id_map;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
std::map<int,int> map_ids_lhs(syntax_rule<T,A,C,O> const& rule)
{
    std::map<int,int> id_map;
    typename syntax_rule<T,A,C,O>::rhs_iterator i = rule.rhs_begin(),
                                          e = rule.rhs_end();
    int id_count = 0;
    for (; i != e; ++i) if (i->indexed()) id_map[rule.index(*i)] = id_count++;

    return id_map;
}

////////////////////////////////////////////////////////////////////////////////

template <class CC, class CT, class T, class A, class C, class O>
void print_node( std::basic_ostream<CC,CT>& os
               , typename syntax_rule<T,A,C,O>::tree_node const& n
               , std::map<int,int> const& id_map )
{
    if (n.lexical()) {
        os << "\"" << n.get_token() << "\"";
    }
    else if (n.indexed()) {
        os << "x" << id_map.find(n.index())->second
           << ":" << n.get_token();
    }
    else os << n.get_token();
}

////////////////////////////////////////////////////////////////////////////////

template <class CC, class CT, class T, class A, class C, class O>
void print_node( std::basic_ostream<CC,CT>& os
               , typename syntax_rule<T,A,C,O>::rule_node const& n
               , syntax_rule<T,A,C,O> const& rule
               , std::map<int,int> const& id_map )
{
    if (n.lexical()) {
        os << "\"" << n.get_token() << "\"";
    }
    else if (n.indexed()) {
        os << "x" << id_map.find(rule.index(n))->second;
    }
    else os << n.get_token();
}

////////////////////////////////////////////////////////////////////////////////

template <class CC, class CT, class T, class A, class C, class O>
void print_recurse( std::basic_ostream<CC,CT>& os
                  , typename syntax_rule<T,A,C,O>::tree_node const& n
                  , std::map<int,int> const& id_map );


////////////////////////////////////////////////////////////////////////////////

template <class CC, class CT, class T, class A, class C, class O>
void print_recurse( std::basic_ostream<CC,CT>& os
                  , typename syntax_rule<T,A,C,O>::tree_node const& n
                  , std::map<int,int> const& id_map )
{
    print_node<CC,CT,T,A,C,O>(os,n,id_map);

    typename sbmt::syntax_rule<T,A,C,O>::lhs_children_iterator
        itr = n.children_begin();
    typename sbmt::syntax_rule<T,A,C,O>::lhs_children_iterator
        end = n.children_end();

    if (itr != end) {
        os << "(";
        print_recurse<CC,CT,T,A,C,O>(os, *itr, id_map);
        ++itr;
        for (; itr != end; ++itr) {
            os << " ";
            print_recurse<CC,CT,T,A,C,O>(os, *itr, id_map);
        }
        os << ")";
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class CC, class CT, class T, class A, class C, class O>
void print_lhs( std::basic_ostream<CC,CT>& os
              , sbmt::syntax_rule<T,A,C,O> const& r )
{
    std::map<int,int> id_map = map_ids_rhs(r);
    print_recurse<CC,CT,T,A,C,O>(os,*r.lhs_root(),id_map);
}

template <class CC, class CT, class T, class A, class C, class O, class TF>
void print_lhs( std::basic_ostream<CC,CT>& os
              , sbmt::syntax_rule<T,A,C,O> const& r
              , TF& tf )
{
    token_format_saver saver(os);
    os << token_label(tf);
    print_lhs(os,r);
}


////////////////////////////////////////////////////////////////////////////////

template <class CC, class CT, class T, class A, class C, class O>
void print_rhs( std::basic_ostream<CC,CT>& os
              , std::map<int,int> const& id_map
              , syntax_rule<T,A,C,O> const& r
              )
{
    typename sbmt::syntax_rule<T,A,C,O>::rhs_iterator itr = r.rhs_begin();
    typename sbmt::syntax_rule<T,A,C,O>::rhs_iterator end = r.rhs_end();
    bool beg = true;
    for (; itr != end; ++itr) {
        if (beg == false)  os << ' '; 
        else beg = false;
        print_node(os,*itr,r,id_map);
    }
}

template <class CC, class CT, class T, class A, class C, class O, class TF>
void print_rhs( std::basic_ostream<CC,CT>& os
              , std::map<int,int> const& id_map
              , syntax_rule<T,A,C,O> const& r
              , TF& tf)
{
    token_format_saver saver(os);
    os << token_label(tf);
    print_rhs(os,id_map,r);
}

////////////////////////////////////////////////////////////////////////////////

template <class CC, class CT, class T, class A, class C, class O>
void print_rhs( std::basic_ostream<CC,CT>& os
              , syntax_rule<T,A,C,O> const& r )
{
    std::map<int,int> id_map = map_ids_rhs(r);
    print_rhs(os,id_map,r);
}

template <class CC, class CT, class T, class A, class C, class O, class TF>
void print_rhs( std::basic_ostream<CC,CT>& os
              , syntax_rule<T,A,C,O> const& r
              , TF& tf )
{
    token_format_saver saver(os);
    os << token_label(tf);
    print_rhs(os,r);
}

////////////////////////////////////////////////////////////////////////////////

template <class CC, class CT, class T, class A, class C, class O>
std::basic_ostream<CC,CT>& operator<<( std::basic_ostream<CC,CT>& os
                                     , syntax_rule<T,A,C,O> const& r )
{
    bool p_lhs = syntax_rule_iomanip::print(os,syntax_rule_iomanip::lhs),
         p_rhs = syntax_rule_iomanip::print(os,syntax_rule_iomanip::rhs),
         p_id  = syntax_rule_iomanip::print(os,syntax_rule_iomanip::id);
    std::map<int,int> id_map = map_ids_rhs(r);
    
    if (p_lhs) print_recurse<CC,CT,T,A,C,O>(os,*r.lhs_root(),id_map);
    if (p_lhs and p_rhs) os << " -> ";
    if (p_rhs) print_rhs(os,id_map,r);
    if ((p_lhs or p_rhs) and p_id) os << " ### id=";
    if (p_id) os << r.id();
    return os;
}

////////////////////////////////////////////////////////////////////////////////

template <class CC, class CT, class T, class A, class C, class O, class TF>
void print( std::basic_ostream<CC,CT>& os
          , sbmt::syntax_rule<T,A,C,O> const& r
          , TF const& tf )
{
    token_format_saver saver(os);
    os << token_label(tf) << r;
}

} // namespace sbmt


////////////////////////////////////////////////////////////////////////////////
//
//  this is the same thing you get from specifying BOOST_CLASS_VERSION,
//  except it works with templated types, whereas BOOST_CLASS_VERSION only
//  works with concrete types.
//  version 3: eliminate deplm string as part of syntax rule
//  version 4: id field increased to 64 bit.
//  at version 4.
//
////////////////////////////////////////////////////////////////////////////////
namespace boost { namespace serialization {
template<class TokenT, class A, class C>
struct version< sbmt::syntax_rule<TokenT,A,C> >
{
    typedef mpl::int_<4> type;
    typedef mpl::integral_c_tag tag;
    BOOST_STATIC_CONSTANT(int, value = version::type::value);
};
} }
