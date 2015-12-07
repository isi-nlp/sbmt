# include <cstdio>
# include <cstring>
namespace std {
template <class N>
ptrdiff_t distance(gusc::fixed_trie_iterator<N> begin, gusc::fixed_trie_iterator<N> end)
{
    return end - begin;
}
}

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class Trie,class Size> void
count(Trie const& trie, typename Trie::state s, Size& internal, Size& total)
{
    ++total;
    typename Trie::iterator itr, end;
    boost::tie(itr,end) = trie.transitions(s);
    if (itr != end) ++internal;
    for (;itr != end; ++itr) count(trie,*itr,internal,total);
}

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class A, class C>
template <class Archive>
void fixed_trie<K,V,A,C>::save(Archive& ar, const unsigned int version) const
{
    ar & sz;
    ar & none;
    ar & boost::serialization::make_binary_object(trie_.get(),sz);
}

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class A, class C>
template <class Archive>
void fixed_trie<K,V,A,C>::load(Archive& ar, const unsigned int version)
{
    uint_type s;
    V n;
    ar & s;
    ar & n;
    pointer_type t;
    if (s > 0) { t = C()(get_allocator(),s); }
    ar & boost::serialization::make_binary_object(t.get(),s);
    trie_ = t;
    sz = s;
    none = n;
}

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class A, class C>
template <class Trie, class Transform>
typename fixed_trie<K,V,A,C>::uint_type
fixed_trie<K,V,A,C>::build( Trie const& trie
                      , typename Trie::state const& s
                      , Transform f
                      , node* curr
                      , char* buf
                      , uint_type offset
                      )
{

    assert(curr->key == trie.key(s));
    assert(curr->value == trie.value(s));
    typename Trie::iterator begin, end;

    boost::tie(begin,end) = trie.transitions(s);
    if (begin != end) {
        uint_type& child_count = *(uint_type*)(buf + offset);
        offset += sizeof(uint_type); // the num_children record

        child_count = 0;
        curr->child_offset = offset;
        typename Trie::iterator itr = begin;
        for (; itr != end; ++itr) {
            new (buf + offset) node(trie,*itr,f);

            ++child_count;
            offset += sizeof(node);
        }
        node* new_itr = child_begin(buf,curr);
        node* new_end = child_end(buf,curr);

        assert(curr->key == trie.key(s));
        assert(curr->value == trie.value(s));

        itr = begin;

        assert(ptrdiff_t(child_count) == std::distance(itr,end));
        assert(std::distance(itr,end) == std::distance(new_itr,new_end));

        for (; itr != end and new_itr != new_end; ++itr, ++new_itr ) {
            offset = build(trie, *itr, f, new_itr, buf, offset);
            assert(new_itr->key == trie.key(*itr));
            assert(new_itr->value == f(trie.value(*itr)));
        }
    }
    return offset;
}

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class A, class C>
template <class Trie, class Transform> void
fixed_trie<K,V,A,C>::init(Trie const& trie, typename Trie::state s, Transform f)
{
    none = trie.nonvalue();
    typedef typename Trie::state trie_state;
    // first, need to count internal nodes, total nodes:
    uint_type internal(0),total(0);
    count(trie,s,internal,total);

    sz = sizeof(node) * total + sizeof(uint_type) * internal;
    pointer_type trie__(C()(get_allocator(),sz));
    //boost::shared_array<char> trie__(new char[sz]);
    std::memset(trie__.get(),11,sz);

    node* x = new(trie__.get()) node(trie,s,f);
    uint_type offset = sizeof(node);

    offset = build(trie,s,f,x,trie__.get(),offset);
    assert(offset == sz);
    trie_ = trie__;

    assert((char*)x == trie_.get());
    assert(f(trie.value(trie.start())) == value(start()));
}
/*
template <class K, class V, class A, class C>
fixed_trie<K,V,A,C>::fixed_trie( fixed_trie<K,V,A,C> const& other
                               , A const& a )
: allocator_type(a)
, trie_(0)
, sz(other.sz)
{
    
}
*/

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class A, class C>
typename fixed_trie<K,V,A,C>::node*
fixed_trie<K,V,A,C>::child_begin(char* buf, state s) const
{
    if (s->child_offset == 0) return NULL;
    else return (node*)(buf + s->child_offset);
}

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class A, class C>
typename fixed_trie<K,V,A,C>::node*
fixed_trie<K,V,A,C>::child_end(char* buf, state s) const
{
    if (s->child_offset == 0) return NULL;
    else {
        uint_type* num_c = (uint_type*)(buf + (s->child_offset - sizeof(uint_type)));
        return ((node*)(buf + s->child_offset)) + *num_c;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class A, class C>
std::pair<typename fixed_trie<K,V,A,C>::iterator,typename fixed_trie<K,V,A,C>::iterator>
fixed_trie<K,V,A,C>::transitions(state s) const
{
    return std::make_pair( iterator(child_begin(trie_.get(),s))
                         , iterator(child_end(trie_.get(),s))
                         );
}

template <class S, class K>
struct fixed_trie_key_less {
    bool operator()(S const& s, K k) const { return s.key < k; }
    bool operator()(K k, S const& s) const { return k < s.key; }
};

template <class K, class V, class A, class C>
std::pair<bool,typename fixed_trie<K,V,A,C>::state> 
fixed_trie<K,V,A,C>::transition(state s, key_type const& k) const
{
    
    state pos = std::lower_bound( child_begin(trie_.get(),s)
                                   , child_end(trie_.get(),s)
                                   , k
                                   , fixed_trie_key_less<node,key_type>() );
    if ((pos != child_end(trie_.get(),s)) && (pos->key == k )) {
        return std::make_pair(true,pos);
    } else {
        return std::make_pair(false,start());
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class N>
fixed_trie_iterator<N>::fixed_trie_iterator() {}

////////////////////////////////////////////////////////////////////////////////

template <class N>
fixed_trie_iterator<N>::fixed_trie_iterator(N n)
  : fixed_trie_iterator<N>::iterator_adaptor_(n) {}

////////////////////////////////////////////////////////////////////////////////

template <class N>
N fixed_trie_iterator<N>::dereference() const
{
    return fixed_trie_iterator<N>::iterator_adaptor_::base_reference();
}

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class A, class C>
template <class Trie, class Transform>
fixed_trie<K,V,A,C>::node::node( Trie const& trie
                           , typename Trie::state const& s
                           , Transform f)
  : key(trie.key(s))
  , value(f(trie.value(s)))
  , child_offset(0) {}

////////////////////////////////////////////////////////////////////////////////

template <class K, class V, class A, class C>
fixed_trie<K,V,A,C>::~fixed_trie()
{}

template <class K, class V, class A, class C>
fixed_trie<K,V,A,C>::fixed_trie() : sz(0) {}

template <class K, class V, class A, class C>
bool fixed_trie<K,V,A,C>::operator==(fixed_trie const& other) const
{
    std::cerr << "this:";
    dump(std::cerr);
    std::cerr << "\nother:";
    other.dump(std::cerr);
    std::cerr << "\n";
    return (sz == other.sz) and (memcmp(trie_.get(),other.trie_.get(),sz) == 0);
}

template <class K, class V, class A, class C>
void fixed_trie<K,V,A,C>::dump(std::ostream& out) const
{
    out <<  sz << ":";
    for (uint_type x = 0; x != sz; ++x) out << '<' << std::hex << int(trie_[x]) << std::dec << '>';
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc
