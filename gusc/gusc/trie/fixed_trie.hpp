# if ! defined(GUSC__TRIE__FIXED_TRIE_HPP)
# define       GUSC__TRIE__FIXED_TRIE_HPP

# include <boost/operators.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/utility/result_of.hpp>
# include <utility>
# include <boost/cstdint.hpp>
# include <boost/iterator/iterator_adaptor.hpp>
# include <boost/shared_array.hpp>
# include <boost/serialization/binary_object.hpp>
# include <gusc/functional.hpp>
# include <iostream>
# include <gusc/lifetime.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
///
///  a non-editable trie that maintains all of its data in a single array.
///  can only be constructed from other tries.
///
////////////////////////////////////////////////////////////////////////////////

template <class NodePointer>
struct fixed_trie_iterator 
: public boost::iterator_adaptor<
    fixed_trie_iterator<NodePointer>
  , NodePointer
  , NodePointer
  , boost::random_access_traversal_tag
  , NodePointer
  >
{
    NodePointer dereference() const;// {return iterator_adaptor_::base_reference(); }
    friend class boost::iterator_core_access;
public:
    fixed_trie_iterator(NodePointer n);
    fixed_trie_iterator();
};

template <
  class Key
, class Value
, class Allocator = std::allocator<char>
, class CreatePointer = gusc::create_shared
>
class fixed_trie 
: public boost::equality_comparable< fixed_trie<Key,Value> >
, Allocator::template rebind<char>::other {
    struct node;
    typedef boost::uint32_t uint_type;
    typedef typename Allocator::template rebind<char>::other allocator_type;
    typedef typename Allocator::template rebind<node>::other node_allocator_type;
    allocator_type& get_allocator() { return *this; }
    typedef typename boost::result_of<CreatePointer(allocator_type)>::type pointer_type;
    typedef typename node_allocator_type::pointer node_pointer;
    typedef typename node_allocator_type::const_pointer node_const_pointer;
public:
    typedef Key key_type;
    typedef Value value_type;
    typedef node_const_pointer state;
    typedef fixed_trie_iterator<state> iterator;
private:
    struct node {
        template <class Trie, class Transform>
        node(Trie const& trie, typename Trie::state const& s, Transform f);
        key_type key;
        value_type value;
        uint_type child_offset;
    };

    template <class Trie, class Transform> uint_type
    build( Trie const& trie
         , typename Trie::state const& s
         , Transform f
         , node* curr
         , char* buf
         , uint_type offset
         );

    template <class Trie, class Transform>
    void init(Trie const& trie, typename Trie::state s, Transform f);

    pointer_type trie_;
    uint_type sz;
    value_type none;
    node* child_begin(char* buf, state s) const;
    node* child_end(char* buf, state s) const;

    friend class boost::serialization::access;

    template <class Archive>
    void save(Archive& ar, const unsigned int version) const;

    template <class Archive>
    void load(Archive& ar, const unsigned int version);

    BOOST_SERIALIZATION_SPLIT_MEMBER()
public:
    ~fixed_trie();

    fixed_trie();

    uint_type memory_size() const { return sz + sizeof(*this); }

    state start() const { return (node const*)(trie_.get()); }

    bool operator == (fixed_trie const& other) const;
    void dump(std::ostream&) const;

    std::pair<iterator,iterator> transitions(state s) const;
    
    std::pair<bool,state> transition(state s, key_type const& k) const;

    template <class Trie, class Transform>
    fixed_trie( Trie const& t
              , Transform f
              , typename Trie::state s
              , Allocator const& a = Allocator()
              )
      : allocator_type(a)
      , trie_(0)
      , sz(0) { init(t,s,f); }

    template <class Trie>
    fixed_trie( Trie const& t
              , typename Trie::state s
              , Allocator const& a = Allocator()
              )
      : allocator_type(a)
      , trie_(0)
      , sz(0) { init(t,s,identity()); }

    template <class Trie> 
    fixed_trie(Trie const& t, Allocator const& a = Allocator())
      : allocator_type(a)
      , sz(0) { init(t,t.start(),identity()); }
      
    value_type const& value(state s) const { return s->value; }
    void set_value(state s, value_type const& v) { const_cast<node*>(s)->value = v; }

    key_type const& key(state s) const { return s->key; }

    value_type const& nonvalue() const { return none; }
};

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# include "impl/fixed_trie.ipp"

# endif //     GUSC__TRIE__FIXED_TRIE_HPP
