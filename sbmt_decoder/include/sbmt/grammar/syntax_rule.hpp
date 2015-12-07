#ifndef   SBMT_GRAMMAR_SYNTAX_RULE_HPP
#define   SBMT_GRAMMAR_SYNTAX_RULE_HPP

#include <boost/noncopyable.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
//TODO: use single shared_ptr to struct of array, or pass refs
#include <boost/shared_array.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/cstdint.hpp>
#include <sbmt/grammar/alignment.hpp>

#include <sbmt/grammar/bad_rule_format.hpp>
#include <sbmt/token.hpp>
#include <string>
#include <sbmt/grammar/syntax_id_type.hpp>
#include <sbmt/grammar/lm_string.hpp>
#include <xrsparse/xrs.hpp>
#include <algorithm>

namespace ns_RuleReader { class Rule; class RuleNode; }

//#include <sbmt/grammar/syntax_id_type.hpp>
#include <boost/cstdint.hpp>

namespace sbmt {

struct create_shared {
private:
    template <class Alloc> 
    struct deleter {
        explicit deleter(Alloc& alloc):palloc(&alloc) {}
        Alloc* palloc;
        void operator()(typename Alloc::pointer p)
        {
            palloc->destroy(p);
            palloc->deallocate(p,1);
        }
    };
public:
    template <class X> struct result;
    template <class F, class Alloc>
    struct result<F(Alloc)> {
        typedef boost::shared_ptr<typename Alloc::value_type> type;
    };
    template <class Alloc>
    boost::shared_ptr<typename Alloc::value_type> operator()(Alloc& alloc) const
    {
        return boost::shared_ptr<typename Alloc::value_type>(alloc.allocate(1),deleter<Alloc>(alloc));
    }
};

template <
  class T
, class Alloc = std::allocator<T>
, class CSA = create_shared
, class Offset = boost::uint8_t
> 
class syntax_rule;

template <class TF> syntax_rule<indexed_token>
index(syntax_rule<fat_token> const& rule, TF& tf);

template <class TF> syntax_rule<fat_token>
fatten(syntax_rule<indexed_token> const& rule, TF& tf);

// from version 0 to 1 of syntax_rule, the size of rule_offset_t goes from
// 8bit to 16bit
// but then i changed it back before checking in, so now
template <class AR, class Offset>
void load_offset(AR& ar, Offset& offset, unsigned int version)
{
    ar >> offset;
}

////////////////////////////////////////////////////////////////////////////////
///
///  a lightweight-as-possible representation of a syntax rule.
///
///  syntax rules have two parts to them: the left-hand-side native syntax tree,
///  and the right-hand-side foreign rule.
///
///  the right hand side can be thought of as a list or array of rhs_node
///  objects, where a rule_node contains a token (see token/token.hpp), and also
///  a pointer (index) to its corresponding place in the lhs tree if the token
///  is a non-terminal.
///  the left hand side can be thought of as a structured tree tree_node
///  objects, where a tree_node contains a token, and
///  also a pointer (index) to the corresponding place in the rhs array if the
///  token is a non-terminal leaf of the tree.
///
///  the representation directly corresponds to the syntax tree textual
///  representation, e.g.:
///
///  S(NP-C(x0:NPB) VP(MD("will") VP-C(VB("be") x1:VP-C)) .(".")) ->
///     x0 "A" "B" x1 "." ### id=9
///
///  and in fact, syntax_rule can be created directly from such a string.
///
///  the actual implementation of the class is very compact, and the user is
///  encouraged to focus on the interface and not the implementation.
///
///  syntax_rule objects can use token objects that are indexed by a dictionary
///  (indexed_token) or tokens that keep track of their own string labels
///  (fat_token).  this is the T template parameter.  to keep the code identical,
///  construction and printing both require a dictionary interface, even though
///  a dictionary type for fat_token is just a dummy class.  see printer.hpp .
///
///  acess to the nodes of the rhs or lhs are provided through iterators.
///  for instance, to access all nodes on the rhs, rhs_begin() and rhs_end()
///  provide rhs_iterator objects.  to traverse through the lhs syntax tree you
///  can either start at the root node and then use children_begin() and
///  children_end() to recursively descend using lhs_children_iterator objects,
///  or you can call lhs_begin() and lhs_end() to get an lhs_preorder_iterator
///  which will move through the tree in a top-down fashion.
///  index() functions return iterators into the lhs and rhs
///  corresponding to identies between lhs
///  and rhs nodes, which allow for algorithms that handle conversions between
///  native and foreign tree structures.  all iterators are valid standard
///  c++ forward iterators.
///
////////////////////////////////////////////////////////////////////////////////
//typedef boost::uint8_t rule_offset_t;

template <class T, class Alloc, class CSA, class OFFSET>
class syntax_rule : Alloc::template rebind<T>::other
{
public:
    typedef OFFSET rule_offset_t;
    typedef T token_t;
    typedef T token_type;
    class tree_node;
    class rule_node;
private:
    typedef typename Alloc::template rebind<T>::other 
            allocator_type;
    typedef typename Alloc::template rebind<tree_node>::other 
            tree_node_allocator_type;
    typedef typename Alloc::template rebind<rule_node>::other 
            rule_node_allocator_type;
    typedef typename rule_node_allocator_type::const_pointer 
            rule_node_const_pointer;
    typedef typename tree_node_allocator_type::const_pointer 
            tree_node_const_pointer;
public:
    typedef rule_node_const_pointer rhs_iterator;
    typedef tree_node_const_pointer lhs_preorder_iterator;
    class   lhs_children_iterator;

    ////////////////////////////////////////////////////////////////////////////

    class tree_node {
    public:
        /// true if this node is a non terminal leaf.  ie it corresponds to a
        /// non terminal on the rhs of the rule.
        bool indexed() const;

        /// true if this node is a terminal leaf.  ie it corresponds to a native
        /// word.
        bool lexical() const { return t.type() == native_token; }

        /// if this is a non-terminal leaf (an indexed node), then
        /// index returns the position of this node on the rhs of the rule.
        unsigned index() const;

        /// returns the grammatical symbol at this node, whether it be
        /// a terminal or non-terminal.
        token_t get_token() const
        { return t; }

        bool is_leaf() const
        {
          //return indexed() || lexical();
          return !first_child;
        }

        /// used to iterate through the children of this node. (only if !is_leaf())
        lhs_children_iterator children_begin() const;
        lhs_children_iterator children_end() const;
        std::pair<lhs_children_iterator,lhs_children_iterator> children() const
        {
            return std::make_pair(children_begin(),children_end());
        }

        /// calls v(unsigned i,tree_node const& leaf_i) for 0<=i<[# leaves], returning number of leaves
        template <class leaf_visitor>
        unsigned visit_leaves(leaf_visitor &v,unsigned p=0) const
        {
            // p is the global offset of first leaf in this subtree
            if (is_leaf()) {
                v(p,*this);
                return p+1;
            }
            for (lhs_children_iterator i=children_begin(),e=children_end();i!=e;++i)
                p=i->visit_leaves(v,p);
            return p;
        }
        struct noop_leaf_visit
        {
            void operator ()(unsigned i,tree_node const &t) const {}
        };
        struct native_token_counter
        {
            unsigned n;
            native_token_counter() : n(0) {}
            void operator ()(unsigned i,tree_node const &t) {
                if (t.get_token().type()==native_token)
                    ++n;
            }
        };

        
        unsigned n_leaves() const
        {
            noop_leaf_visit n;
            return visit_leaves(n);
        }

        unsigned n_native_tokens() const
        {
            native_token_counter n;
            visit_leaves(n);
            return n.n;
        }


        template <class Archive>
        void load(Archive& ar, unsigned int version)
        {
            load_offset(ar, next_sibling, version);
            load_offset(ar, first_child, version);
            load_offset(ar, idx, version);
            ar & t;
        }

        template <class Archive>
        void save(Archive& ar)
        {
            ar & next_sibling;
            ar & first_child;
            ar & idx;
            ar & t;
        }

        bool operator==(tree_node const&) const;
        bool operator!=(tree_node const&) const;

        tree_node();
        // implementation note:
        // next_sibling represents the offset of the next child
        // from the address of this.
        // similarly for first_child
        rule_offset_t  next_sibling;
        rule_offset_t  first_child;
        /// refers to position of rule_node in rhs (which refers back to us)
        rule_offset_t idx;
        token_t       t;
        friend class  lhs_children_iterator;
        friend class  syntax_rule;
    };

    ////////////////////////////////////////////////////////////////////////////

    class rule_node {
    public:
        /// returns true if this node is a non-terminal, ie it corresponds to
        /// a non-terminal leaf on lhs-tree of this rule
        bool indexed() const;

        /// returns true if this node is representing a foreign word.
        bool lexical() const { return t.type() == foreign_token; }

        /// returns the grammatical symbol that this node represents, whether
        /// it is a foreign word or a non-terminal.
        token_t const& get_token() const
        { return t; }

        template <class Archive>
        void load(Archive& ar, unsigned int version)
        {
            load_offset(ar, lhs_pos, version);
            ar & t;
        }

        template <class Archive>
        void save(Archive& ar)
        {
            ar & lhs_pos;
            ar & t;
        }

        bool operator==(rule_node const&) const;
        bool operator!=(rule_node const&) const;

        rule_node();
        rule_offset_t lhs_pos;
        token_t       t; // note: token type = tag -> variable (lhs_pos defined).  otherwise (lexical), lhs_pos undefined
        friend class syntax_rule;
    };

    ////////////////////////////////////////////////////////////////////////////

    class lhs_children_iterator
    : public boost::iterator_facade<
        lhs_children_iterator
      , tree_node const
      , boost::forward_traversal_tag
      > {
    public:
        lhs_children_iterator();
        lhs_children_iterator(tree_node_const_pointer curr);
        lhs_preorder_iterator current() const {return curr; }
    private:
        bool equal(lhs_children_iterator const& other) const;
        void increment();
        tree_node const& dereference() const;
        tree_node_const_pointer curr;
        friend class boost::iterator_core_access;
        friend class tree_node;
    };

    ////////////////////////////////////////////////////////////////////////////

    explicit syntax_rule(Alloc const& alloc = Alloc())
     : allocator_type(alloc) {}

    template <class Range, class TokenFactory>
    syntax_rule(Range const& rule_str, TokenFactory &tf, Alloc const& alloc = Alloc());

    template <class TokenFactory>
    syntax_rule(ns_RuleReader::Rule& r, TokenFactory &tf, Alloc const& alloc = Alloc());

    template <class TokenFactory>
    syntax_rule(rule_data const& rd, TokenFactory& tf, Alloc const& alloc = Alloc()) 
     : allocator_type(alloc)
    { init(rd,tf); }

    template <class ArchiveT>
    void save(ArchiveT & ar, const unsigned int version) const;

    template <class ArchiveT>
    void load(ArchiveT & ar, const unsigned int version);

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    /// align variables, and fully biconnect the source/target lexemes unless vars_only
    alignment default_alignment(bool lexical_align=true) const;

    lhs_preorder_iterator lhs_root() const;
    token_t lhs_root_token() const
    { return lhs_root()->get_token(); }

    lhs_preorder_iterator lhs_begin() const;
    lhs_preorder_iterator lhs_end() const;
    std::pair<lhs_preorder_iterator,lhs_preorder_iterator> lhs() const
    {
        return std::make_pair(lhs_begin(),lhs_end());
    }
    unsigned           lhs_size() const { return lhs_tree->size(); }
    unsigned           lhs_yield_size() const { return lhs_root()->n_leaves(); }
    unsigned           n_leaves() const { return lhs_root()->n_leaves(); }
    unsigned n_native_tokens() const { return lhs_root()->n_native_tokens(); }

    /// if this is an indexed node, then lhs_position returns an iterator to
    /// the corresponding node in the lhs-tree.
    lhs_preorder_iterator lhs_position(rule_node const&) const;
    unsigned           index(rule_node const&) const;

    rhs_iterator rhs_begin() const;
    rhs_iterator rhs_end() const;
    std::pair<rhs_iterator,rhs_iterator> rhs() const
    {
        return std::make_pair(rhs_begin(),rhs_end());
    }
    unsigned  rhs_size() const { return rhs_rule->size(); }

    big_syntax_id_type id() const { return m_id; }
    void set_id(big_syntax_id_type idx) { m_id = idx; }

    allocator_type& get_allocator() { return *this; }
    
    syntax_rule& operator=(syntax_rule const& o)
    {
        syntax_rule r(o);
        r.swap(*this);
        return *this;
    }
    
    void swap(syntax_rule& o)
    {
        std::swap(lhs_tree,o.lhs_tree);
        std::swap(rhs_rule,o.rhs_rule);
        std::swap(m_id,o.m_id);
    }
    
private:
    template <class TokenFactory>
    void init(ns_RuleReader::Rule& r, TokenFactory& tf);

    template <class TokenFactory>
    void init(rule_data const& rd, TokenFactory& tf);

    template <class TokenFactory>
    rule_offset_t preorder_init( ns_RuleReader::RuleNode* r
                                 , rule_offset_t curr
                                 , TokenFactory& tf );

    typedef gusc::varray<tree_node,tree_node_allocator_type> raw_tree_node_array;
    typedef gusc::varray<rule_node,rule_node_allocator_type> raw_rule_node_array;
    typedef typename Alloc::template rebind<raw_rule_node_array>::other raw_rule_node_array_allocator;
    typedef typename Alloc::template rebind<raw_tree_node_array>::other raw_tree_node_array_allocator;
    typedef typename boost::result_of<CSA(raw_tree_node_array_allocator)>::type tree_node_array;
    typedef typename boost::result_of<CSA(raw_rule_node_array_allocator)>::type rule_node_array;

    tree_node_array lhs_tree;
    rule_node_array rhs_rule;
    big_syntax_id_type m_id;
    
    tree_node_array new_lhs_tree(size_t sz);
    rule_node_array new_rhs_rule(size_t sz);
    
    
    template <class TF> friend syntax_rule<indexed_token>
    index(syntax_rule<fat_token> const& rule, TF& tf);

    template <class TF> friend syntax_rule<fat_token>
    fatten(syntax_rule<indexed_token> const& rule, TF& tf);
};

typedef syntax_rule<indexed_token> indexed_syntax_rule;
typedef syntax_rule<fat_token> fat_syntax_rule;
typedef syntax_rule<fat_token,std::allocator<fat_token>,create_shared,boost::uint16_t>     
        fatter_syntax_rule;
 
////////////////////////////////////////////////////////////////////////////////

template <class TF> indexed_syntax_rule::rule_node
index(fat_syntax_rule::rule_node const& rn, TF& tf)
{
    indexed_syntax_rule::rule_node retval;
    retval.lhs_pos = rn.lhs_pos;
    retval.t = index(rn.t,tf);
    return retval;
}

template <class TF> fat_syntax_rule::rule_node
fatten(indexed_syntax_rule::rule_node const& rn, TF& tf)
{
    fat_syntax_rule::rule_node retval;
    retval.lhs_pos = rn.lhs_pos;
    retval.t = fatten(rn.t,tf);
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class TF> indexed_syntax_rule::tree_node
index(fat_syntax_rule::tree_node const& rn, TF& tf)
{
    indexed_syntax_rule::tree_node retval;
    retval.next_sibling = rn.next_sibling;
    retval.first_child = rn.first_child;
    retval.idx = rn.idx;
    retval.t = index(rn.t,tf);
    return retval;
}

template <class TF> fat_syntax_rule::tree_node
fatten(indexed_syntax_rule::tree_node const& rn, TF& tf)
{
    fat_syntax_rule::tree_node retval;
    retval.next_sibling = rn.next_sibling;
    retval.first_child = rn.first_child;
    retval.idx = rn.idx;
    retval.t = fatten(rn.t,tf);
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class TF>
indexed_syntax_rule index(fat_syntax_rule const& rule, TF& tf)
{
    indexed_syntax_rule retval;
    retval.m_id = rule.m_id;
    retval.lhs_tree = retval.new_lhs_tree(rule.lhs_size());
    retval.rhs_rule = retval.new_rhs_rule(rule.rhs_size());

    typedef index_op< indexed_syntax_rule::tree_node
                    , fat_syntax_rule::tree_node
                    , TF> tree_op_t;

    typedef index_op< indexed_syntax_rule::rule_node
                    , fat_syntax_rule::rule_node
                    , TF > rule_op_t;
    tree_op_t tree_op(tf);
    rule_op_t rule_op(tf);

    boost::transform_iterator<
        tree_op_t, fat_syntax_rule::tree_node_const_pointer
    > tree_begin(rule.lhs_tree->begin(), tree_op),
      tree_end(rule.lhs_tree->end(), tree_op);

    boost::transform_iterator<
        rule_op_t, fat_syntax_rule::rule_node_const_pointer
    > rule_begin(rule.rhs_rule->begin(), rule_op),
      rule_end(rule.rhs_rule->end(), rule_op);

    std::copy(tree_begin,tree_end,retval.lhs_tree->begin());
    std::copy(rule_begin,rule_end,retval.rhs_rule->begin());

    return retval;
}

template <class TF>
fat_syntax_rule fatten(indexed_syntax_rule const& rule, TF& tf)
{
    fat_syntax_rule retval;
    retval.m_id = rule.m_id;
    retval.lhs_tree = retval.new_lhs_tree(rule.lhs_size());
    retval.rhs_rule = retval.new_rhs_rule(rule.rhs_size());

    typedef fatten_op< fat_syntax_rule::tree_node
                     , indexed_syntax_rule::tree_node
                     , TF > tree_op_t;

    typedef fatten_op< fat_syntax_rule::rule_node
                     , indexed_syntax_rule::rule_node
                     , TF > rule_op_t;
    tree_op_t tree_op(tf);
    rule_op_t rule_op(tf);

    boost::transform_iterator<
        tree_op_t, indexed_syntax_rule::tree_node const*
    > tree_begin(rule.lhs_tree->begin(), tree_op),
      tree_end(rule.lhs_tree->end(), tree_op);

    boost::transform_iterator<
        rule_op_t, indexed_syntax_rule::rule_node const*
    > rule_begin(rule.rhs_rule->begin(), rule_op),
      rule_end(rule.rhs_rule->end(), rule_op);

    std::copy(tree_begin,tree_end,retval.lhs_tree->begin());
    std::copy(rule_begin,rule_end,retval.rhs_rule->begin());

    return retval;
}

///
/// post: outvec[i]=j where rhs[i] is the jth variable. outvec[i]=-1 if it's not a var
template <class OV, class T, class A, class C, class O>
void rhs_to_vars_only_index_vec(syntax_rule<T,A,C,O> const& r, OV& outvec) {
  outvec.resize(r.rhs_size());
  rhs_to_vars_only_index_output(r,outvec.begin());
}

template <class OUT, class T, class A, class C, class O>
void rhs_to_vars_only_index_output(syntax_rule<T,A,C,O> const& r, OUT out) {
  typename syntax_rule<T,A,C,O>::rhs_iterator i=r.rhs_begin(),e=r.rhs_end();
  typename syntax_rule<T,A,C,O>::rule_offset_t n=0;
  typedef typename syntax_rule<T,A,C,O>::rule_offset_t rule_offset_t;
  for(;i!=e;++i)
    *out++ = i->indexed() ? n++ : (rule_offset_t)-1;
}


template <class T, class A, class C, class O>
std::vector<typename syntax_rule<T,A,C,O>::rule_offset_t> 
rhs_to_vars_only_index(syntax_rule<T,A,C,O> const& r) {
  std::vector<typename syntax_rule<T,A,C,O>::rule_offset_t> ret;
  rhs_to_vars_only_index_vec(r,ret);
  return ret;
}


////////////////////////////////////////////////////////////////////////////////

template <class T, class A, class C, class O>
bool operator == (syntax_rule<T,A,C,O> const& r1, syntax_rule<T,A,C,O> const& r2)
{
    return r1.lhs_size() == r2.lhs_size()
       and r1.rhs_size() == r2.rhs_size()
       and std::equal(r1.lhs_begin(),r1.lhs_end(),r2.lhs_begin())
       and std::equal(r1.rhs_begin(),r1.rhs_end(),r2.rhs_begin());
}

template <class T, class A, class C, class O>
bool operator != (syntax_rule<T,A,C,O> const& r1, syntax_rule<T,A,C,O> const& r2)
{
    return !(r1 == r2);
}

////////////////////////////////////////////////////////////////////////////////
///
///  our library-standard way of printing objects which potentially contain
///  indexed token data.  see printer.hpp
///
////////////////////////////////////////////////////////////////////////////////
template<class CC, class CT ,class T, class A, class C, class O, class TF>
void print( std::basic_ostream<CC,CT>&o, syntax_rule<T,A,C,O> const& rule, TF const& tf);

template <class CC, class CT, class T, class A, class C, class O>
std::basic_ostream<CC,CT>&
operator << (std::basic_ostream<CC,CT>&, syntax_rule<T,A,C,O> const& rule);

/*
template <class T, class TF>
void print( std::ostream&
          , typename syntax_rule<T>::tree_node const&
          , token_factory<TF,T> const& );

template <class T, class TF>
void print( std::ostream&
          , typename syntax_rule<T>::rule_node const&
          , token_factory<TF,T> const& );
*/

// copied from rule_data_iomanip. to do: use same object for both
struct syntax_rule_iomanip {
public:
  enum which { lhs=0, rhs=1, id=2 };

  syntax_rule_iomanip(which w, bool p) : w(w), p(p) {}
  static int get_index(which w)
  {
      static int indices[3] = { std::ios_base::xalloc()
                              , std::ios_base::xalloc()
                              , std::ios_base::xalloc() };
      return indices[w];
  }
  static bool print(std::ios_base& ios, which w) { return !ios.iword(get_index(w)); }
  void set_ios(std::ios_base& ios)
  {
      ios.iword(get_index(w)) = !p;
  }
private:
  which w;
  bool p;
};

inline syntax_rule_iomanip print_rule_lhs(bool yesno)
{ return syntax_rule_iomanip(syntax_rule_iomanip::lhs,yesno); }

inline syntax_rule_iomanip print_rule_rhs(bool yesno)
{ return syntax_rule_iomanip(syntax_rule_iomanip::rhs,yesno); }

inline syntax_rule_iomanip print_rule_id(bool yesno)
{ return syntax_rule_iomanip(syntax_rule_iomanip::id,yesno); }

inline std::ostream& operator << (std::ostream& os, syntax_rule_iomanip manip)
{
    manip.set_ios(os);
    return os;
}

} // namespace sbmt



#include <sbmt/grammar/impl/syntax_rule.ipp>

#endif // SBMT_GRAMMAR_SYNTAX_RULE_HPP
