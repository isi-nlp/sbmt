# if ! defined(GUSC__TRIE__BASIC_TRIE_HPP)
# define       GUSC__TRIE__BASIC_TRIE_HPP

# include <utility>
# include <memory>
# include <vector>
# include <boost/noncopyable.hpp>
# include <boost/scoped_ptr.hpp>
# include <algorithm>
# include <map>
# include <boost/tuple/tuple_comparison.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class V>
struct first_equals {
    V v;
    first_equals(V const& v) : v(v) {}
    template <class T>
    bool operator()(T const& t) const
    {
        V const& vv = (*t).key();
        return vv == v;
    }
};

template <class Ptr, class Val>
struct first_less {
    bool operator()(Ptr const& p, Val const& v) const
    {
        Val const& vv = (*p).key();
        return vv < v;
    }
    bool operator()(Val const& v, Ptr const& p) const
	{
	    Val const& vv = (*p).key();
	    return v < vv;
    }
};

template <class V>
first_equals<V> make_first_equals(V const& v) { return first_equals<V>(v); }

////////////////////////////////////////////////////////////////////////////////

struct map_selector {
    template <class Key, class Value>
    struct apply {
        typedef std::map<Key,Value> type;
    };
};

////////////////////////////////////////////////////////////////////////////////

template < class Key
         , class Value
         , class AssocSelector = map_selector
         >
class basic_trie : boost::noncopyable {
public:
    typedef Key key_type;
    typedef Value value_type;

private:
    struct node_;
//    typedef typename AssocSelector::template apply<Key,node_*>::type
//            assoc_container_type;
    typedef std::vector<node_*> assoc_container_type;

    struct node_ {
    private:
        key_type   k;
        value_type val;
    public:
        key_type const& key() const { return k; }
        value_type const& value() const { return val; }
        value_type& value() { return val; }
        bool valset(value_type const& n) const { return val != n; }
        void setval(value_type const& v) { val = v; }
        node_(value_type const& n) : val(n),parent(0) {}
        node_(key_type const& k, value_type const& v) : k(k), val(v), parent(0) {}
        assoc_container_type children;
        node_* parent;
        ~node_();
    };

public:
    typedef node_* state;

    basic_trie(value_type const& none = value_type());
    ~basic_trie();
    template <class Iterator>
    std::pair<bool,state> insert(Iterator b, Iterator e, value_type const& v);
    
    template <class Iterator>
    std::pair<bool,state> insert(state s, Iterator b, Iterator e, value_type const& v);

    double avg_branch_factor() const { return branch_count / double(internal_node_count); }

    typedef typename assoc_container_type::const_iterator iterator;
    
    state parent(state n) const { return n->parent; }

    state start() const { return root.get(); }
    std::pair<iterator,iterator> transitions(state n) const;
    std::pair<bool,state> transition(state s, key_type const& k) const
    {
        
        iterator pos = lower_bound( s->children.begin()
                                  , s->children.end()
                                  , k
                                  , first_less<node_*,key_type>() );
        if ((pos != s->children.end()) && ((**pos).key() == k )) {
            return std::make_pair(true,*pos);
        } else {
            return std::make_pair(false,root.get());
        }
    }
    value_type const& value(state n) const;
    value_type& value(state n);
    key_type const& key(state n) const;
    value_type nonvalue() const { return none; }
    void swap(basic_trie& other);
private:
    boost::scoped_ptr<node_> root;
    size_t internal_node_count;
    size_t branch_count;
    value_type none;
};

template <class K,class V, class AS>
void basic_trie<K,V,AS>::swap(basic_trie& other)
{
    root.swap(other.root);
    std::swap(internal_node_count, other.internal_node_count);
    std::swap(branch_count, other.branch_count);
    std::swap(none, other.none);
}

////////////////////////////////////////////////////////////////////////////////

template <class K,class V, class AS>
template <class I>
std::pair<bool,typename basic_trie<K,V,AS>::state>
basic_trie<K,V,AS>::insert(state s, I itr, I end, value_type const& v)
{
    typedef typename assoc_container_type::iterator pos_t;
    node_* curr = s;

    for (; itr != end; ++itr) {
        pos_t pos = lower_bound( curr->children.begin()
                               , curr->children.end()
                               , *itr
                               , first_less<node_*,key_type>() );
        if ( (pos != curr->children.end()) && ((**pos).key() == *itr )) {
            curr = *pos;
        } else {
            std::auto_ptr<node_> nd(new node_(*itr,none));
            curr->children.insert(pos,nd.get());
            if (curr->children.size() == 1) ++internal_node_count;
            ++branch_count;
            nd->parent = curr;
            curr = nd.get();
            nd.release();
        }
    }
    if (curr->valset(none)) {
        return std::make_pair(false,curr);
    } else {
        curr->setval(v);
        return std::make_pair(true,curr);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class K,class V, class AS>
template <class I>
std::pair<bool,typename basic_trie<K,V,AS>::state>
basic_trie<K,V,AS>::insert(I itr, I end, value_type const& v)
{
    return insert(root.get(),itr,end,v);
}

////////////////////////////////////////////////////////////////////////////////

template <class K,class V, class AS>
basic_trie<K,V,AS>::node_::~node_()
{
    typename assoc_container_type::iterator itr = children.begin(),
                                            end = children.end();
    for (; itr != end; ++itr) { delete *itr; *itr = NULL; }
}

////////////////////////////////////////////////////////////////////////////////

template <class K,class V, class AS>
basic_trie<K,V,AS>::basic_trie(value_type const& n)
  : root(new node_(n))
  , internal_node_count(0)
  , branch_count(0)
  , none(n)
{}

////////////////////////////////////////////////////////////////////////////////

template <class K,class V, class AS>
basic_trie<K,V,AS>::~basic_trie() { }

////////////////////////////////////////////////////////////////////////////////

template <class K,class V, class AS>
typename basic_trie<K,V,AS>::value_type const&
basic_trie<K,V,AS>::value(state n) const
{
     return n->value();
}

////////////////////////////////////////////////////////////////////////////////

template <class K,class V, class AS>
typename basic_trie<K,V,AS>::value_type&
basic_trie<K,V,AS>::value(state n)
{
     return n->value();
}

////////////////////////////////////////////////////////////////////////////////

template <class K,class V, class AS>
typename basic_trie<K,V,AS>::key_type const&
basic_trie<K,V,AS>::key(state n) const
{
     return n->key();
}

////////////////////////////////////////////////////////////////////////////////
template <class K, class V, class AS>
std::pair< typename basic_trie<K,V,AS>::iterator
         , typename basic_trie<K,V,AS>::iterator >
basic_trie<K,V,AS>::transitions(state s) const
{
    return std::make_pair( iterator(s->children.begin())
                         , iterator(s->children.end()) );
}

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class AS>
void swap(basic_trie<K,V,AS>& b1, basic_trie<K,V,AS>& b2)
{
    b1.swap(b2);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__TRIE__BASIC_TRIE_HPP

