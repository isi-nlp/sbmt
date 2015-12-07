# if ! defined(GUSC__MATH__SPARSE_VECTOR_HPP)
# define       GUSC__MATH__SPARSE_VECTOR_HPP

# include <functional>
# include <gusc/functional.hpp>
# include <boost/cstdint.hpp>
# include <map>
# include <boost/tuple/tuple.hpp>
# include <boost/range.hpp>
# include <boost/utility/result_of.hpp>
# include <boost/foreach.hpp>
# include <string>
# include <boost/tokenizer.hpp>
# include <boost/lexical_cast.hpp>
# include <iostream>
# include <boost/iterator/iterator_adaptor.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/functional/hash.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class X> struct second_ {};

template <class A, class B> struct second_<std::pair<A,B> > {
    typedef B type;
};

template <class V1, class V2, class Product>
class sparse_op_reduce_result {
    typedef typename second_<typename boost::range_value<V1 const>::type>::type value1;
    typedef typename second_<typename boost::range_value<V2 const>::type>::type value2;
    typedef typename boost::result_of<Product(value1,value2)>::type product;
public:
    typedef product type;
};

template <class OP>
struct reverse_args {

    template <class X> struct result {};
    template <class X, class Y, class c> struct result<reverse_args<c>(X,Y)> {
        typedef typename boost::result_of<c(Y,X)>::type type;
    };

    template <class X, class Y>
    typename boost::result_of<OP(Y,X)>::type
    operator()(X const& x, Y const& y) const
    {
        return op(y,x);
    }

    OP op;
    explicit reverse_args(OP const& op = OP()) : op(op) {}
};

template <class V1, class V2, class Op, class Reduce>
typename sparse_op_reduce_result<V1,V2,Op>::type
sparse_lookup_op_reduce(V1 const& v1, V2 const& v2, Op const& op, Reduce const& reduce)
{
    assert(v1.size() >= v2.size());

    typedef typename sparse_op_reduce_result<V1,V2,Op>::type return_type;
    typedef typename boost::range_value<V2 const>::type value_type;

    return_type ret = return_type();

    BOOST_FOREACH(value_type const& p, v2) {
        ret = reduce(ret,op(v1[p.first],p.second));
    }

    return ret;
}

template <class V1, class V2, class Product, class Sum>
typename sparse_op_reduce_result<V1,V2,Product>::type
sparse_pairwise_op_reduce(V1 const& v1, V2 const& v2, Product const& op, Sum const& reduce)
{
    typedef typename sparse_op_reduce_result<V1,V2,Product>::type result_type;
    typedef typename second_<typename boost::range_value<V1 const>::type>::type value1;
    typedef typename second_<typename boost::range_value<V2 const>::type>::type value2;

    typename boost::range_iterator<V1 const>::type
        i1 = boost::begin(v1),
        e1 = boost::end(v1);
    typename boost::range_iterator<V2 const>::type
        i2 = boost::begin(v2),
        e2 = boost::end(v2);

    result_type res = result_type();

    while ((i1 != e1) && (i2 != e2)) {
        if ((i1->first) < (i2->first)) {
            ++i1;
        } else if ((i2->first) < (i1->first)) {
            ++i2;
        } else {
            res = reduce(res,op(i1->second,i2->second));
            ++i1;
            ++i2;
        }
    }

    return res;
}

template <class V1, class V2, class Op, class Reduce>
typename sparse_op_reduce_result<V1,V2,Op>::type
sparse_op_reduce(V1 const& v1, V2 const& v2, Op const& op, Reduce const& reduce)
{
    if (v1.size() < v2.size()) {
        return sparse_op_reduce(v2,v1,reverse_args<Op>(op),reduce);
    }
    # if !defined(NDEBUG)
        typename sparse_op_reduce_result<V1,V2,Op>::type r1, r2;
        r1 = sparse_lookup_op_reduce(v1,v2,op,reduce);
        r2 = sparse_pairwise_op_reduce(v1,v2,op,reduce);
        assert(r2 == 0 or r1/r2 >= 0.9);
        assert(r1 == 0 or r2/r1 <= 1.1);
    # endif
    if (v1.size() > 2 * v2.size()) {
        return sparse_lookup_op_reduce(v1,v2,op,reduce);
    } else {
        return sparse_pairwise_op_reduce(v1,v2,op,reduce);
    }
}

template <class V1, class V2, class Op, class Reduce>
typename sparse_op_reduce_result<V1,V2,Op>::type
sparse_op_reduce(V1 const& v1, V2 const& v2, reverse_args<Op> const& op, Reduce const& reduce)
{
    if (v1.size() > 2 * v2.size()) {
        return sparse_lookup_op_reduce(v1,v2,op,reduce);
    } else {
        return sparse_pairwise_op_reduce(v1,v2,op,reduce);
    }
}

template <class V1, class V2, class Op>
void sparse_lookup_op(V1& v1, V2 const& v2, Op const& op)
{
    typedef typename boost::range_value<V2 const>::type value_type;
    BOOST_FOREACH(value_type const& p, v2) {
        v1[p.first] = op(typename V1::data_const_reference(v1[p.first]),p.second);
    }
}

template <class V1, class V2, class Op>
void sparse_pairwise_op(V1& v1, V2 const& v2, Op const& op)
{
    typename boost::range_iterator<V1>::type
        old = boost::begin(v1),
        i1 = boost::begin(v1),
        e1 = boost::end(v1);
    typename boost::range_iterator<V2 const>::type
        i2 = boost::begin(v2),
        e2 = boost::end(v2);

    while ((i1 != e1) && (i2 != e2)) {
        if ((i1->first) < (i2->first)) {
            // v2[i1->first] == 0
            old = i1;
            ++i1;
            old->second = op(typename V1::data_const_reference(old->second),typename V2::data_type());
        } else if ((i2->first) < (i1->first)) {
            // v1[i2->first] == 0
            v1[i2->first] = op(typename V1::data_type(),i2->second);
            ++i2;
        } else {
            old = i1;
            ++i1;
            old->second = op(typename V1::data_const_reference(old->second),i2->second);
            ++i2;
        }
    }
    while (i1 != e1) {
        //v2[i1->first] == 0
        old = i1;
        ++i1;
        old->second = op(typename V1::data_const_reference(old->second),typename V2::data_type());
    } 
    while (i2 != e2) {
         v1[i2->first] = op(typename V1::data_type(),i2->second);
        ++i2;
    }
}

template <class W, class V, class Op>
void sparse_op_inplace(W& w, V const& v, Op const& op, bool force_pairwise = false)
{
    # if !defined(NDEBUG)
        if (not force_pairwise) {
            W w1 = w;
            W w2 = w;
            sparse_lookup_op(w1,v,op);
            sparse_pairwise_op(w2,v,op);
            //std::cerr << "w1: " << w1 << std::endl;
            //std::cerr << "w2: " << w2 << std::endl;

            typename boost::range_iterator<W const>::type
                i1 = boost::begin(const_cast<W const&>(w1)),
                e1 = boost::end(const_cast<W const&>(w1));
            typename boost::range_iterator<W const>::type
                i2 = boost::begin(const_cast<W const&>(w2)),
                e2 = boost::end(const_cast<W const&>(w2));

            while ((i1 != e1) && (i2 != e2)) {
                typename W::data_type i1s = i1->second, i2s = i2->second;
                typename W::key_type i1f = i1->first, i2f = i2->first;
                if (i1f < i2f) {
                    assert(i1s < 0.01);
                    assert(i1s > -0.01);
                    ++i1;
                } else if (i2f < i1f) {
                    assert(i2s < 0.01);
                    assert(i2s > -0.01);
                    ++i2;
                } else {
                    assert(i1s/i2s >= 0.9);
                    assert(i2s/i1s <= 1.1);
                    ++i1;
                    ++i2;
                }
            }
            while (i2 != e2) {
                assert(i2->second < 0.1);
                assert(i2->second > -0.1);
                ++i2;
            }
            while (i1 != e1) {
                assert(i1->second < 0.1);
                assert(i1->second > -0.1);
                ++i1;
            }
        }
    # endif
    if (2 * v.size() < w.size() and not force_pairwise)
        sparse_lookup_op(w,v,op);
    else
        sparse_pairwise_op(w,v,op);
}

template <class W, class V1, class V2, class Op>
void sparse_op_copy(W& w, V1 const& v1, V2 const& v2, Op const& op,bool force_pairwise=false)
{
    w = v1;
    sparse_op_inplace(w,v2,op,force_pairwise);
}

template <class V, class Op, class C>
V& sparse_scalar_op(V& v, Op const& op, C const& c)
{
    typedef typename boost::range_value<V>::type value_type;
    BOOST_FOREACH(value_type p, v) {
        p.second = op(typename V::data_const_reference(p.second),c);
    }
    return v;
}

template <class V, class W, class Op, class C>
V& sparse_scalar_op(V& v, W const& w, Op const& op,C const& c)
{
    v = w;
    sparse_scalar_op(v,op,c);
    return v;
}

template<class V>
class sparse_vector_element {
public:
    typedef V vector_type;
    typedef typename V::key_type key_type;
    typedef typename V::mapped_type value_type;
    typedef value_type const& const_reference;
    typedef value_type *pointer;

private:
    const_reference get_d() const
    {
        typename V::const_iterator pos = container->find(i);
        if (pos != container->end()) d = pos->second;
        else d = value_type();
        return d;
    }

    void set(value_type const& s) const
    {
        //std::cout << "set(" << s << ")" << std::endl;
        typename V::iterator pos = container->find(i);
        if (s == value_type()) {
            if (pos != container->end()) container->erase(pos);
        } else {
            if (pos == container->end()) container->insert(std::make_pair(i,s));
            else pos->second = s;
        }
    }

public:
    sparse_vector_element() : container(NULL) {}
    sparse_vector_element(vector_type& v, key_type i)
     : container(&v), i(i) {}

    sparse_vector_element(vector_type& v, typename vector_type::iterator p)
     : container(&v), i(p->first) {}

    sparse_vector_element& operator=(sparse_vector_element const& p)
    {
        // Overide the implict copy assignment
        if (not container) { // initialization
            container = p.container;
            d = p.d;
            i = p.i;
        } else { // reference assignment
            p.get_d();
            set(p.d);
            return *this;
        }
    }

    template<class D>
    sparse_vector_element& operator=(D const& dd)
    {
        //std::cout << "ref = " << dd << std::endl;
        set(dd);
        return *this;
    }

    template<class D>
    sparse_vector_element& operator+=(D const& dd)
    {
        //std::cout << "ref += " << dd << std::endl;
        get_d();
        d += dd;
        set(d);
        return *this;
    }

    template<class D>
    sparse_vector_element& operator-=(D const& dd)
    {
        //std::cout << "ref -= " << dd << std::endl;
        get_d();
        d -= dd;
        set(d);
        return *this;
    }

    template<class D>
    sparse_vector_element& operator*=(D const& dd)
    {
        //std::cout << "ref *= " << dd << std::endl;
        get_d();
        d *= dd;
        set (d);
        return *this;
    }

    template<class D>
    sparse_vector_element& operator/=(D const& dd)
    {
        //std::cout << "ref /= " << dd << std::endl;
        get_d();
        d /= dd;
        set(d);
        return *this;
    }

    template<class D>
    bool operator==(D const& dd) const
    {
        return get_d() == dd;
    }

    template<class D>
    bool operator != (D const& dd) const
    {
        return get_d() != dd;
    }

    operator const_reference() const
    {
        return get_d();
    }

    const_reference cref() const { return get_d(); }

    // Conversion to reference - may be invalidated
    value_type& ref() const
    {
        typename V::const_iterator pos = container->find(i);
        if (pos == container->end()){
            container->insert(i, value_type());
            pos = container->find(i);
        }
        return pos->second;
    }

private:
    V* container;
    key_type i;
    mutable value_type d;
};

template <class V>
std::ostream& operator <<(std::ostream& out, sparse_vector_element<V> const& sve)
{
    return out << sve.cref();
}

template <class V>
class sparse_vector_iterator
 : public boost::iterator_adaptor<
     sparse_vector_iterator<V>
   , typename V::iterator
   , std::pair< typename V::key_type const, sparse_vector_element<V> >
   , boost::use_default
   , std::pair< typename V::key_type const, sparse_vector_element<V> >
   > {
    typedef V vector_type;
    typedef typename V::key_type key_type;
    typedef sparse_vector_element<V> mapped_type;
    typedef std::pair<key_type const, mapped_type> result_type;

    result_type dereference() const
    {
        return result_type(this->base()->first, mapped_type(*c,this->base()));
    }
    friend class boost::iterator_core_access;
    V* c;
public:
    sparse_vector_iterator() : c(NULL){}
    sparse_vector_iterator(V& c, typename V::iterator pos)
      : sparse_vector_iterator::iterator_adaptor_(pos), c(&c) {}
};

template <class V>
class sparse_vector_const_iterator
 : public boost::iterator_adaptor<
     sparse_vector_const_iterator<V>
   , typename V::const_iterator
   , std::pair< typename V::key_type const, sparse_vector_element<V> > const
   , boost::use_default
   , std::pair< typename V::key_type const, sparse_vector_element<V> > const
   > {
    typedef V vector_type;
    typedef typename V::key_type key_type;
    typedef sparse_vector_element<V> mapped_type;
    typedef std::pair<key_type const, mapped_type> result_type;

    result_type dereference() const
    {
        return result_type(this->base()->first, mapped_type(*c,this->base()));
    }
    friend class boost::iterator_core_access;
    V* c;
public:
    sparse_vector_const_iterator() : c(NULL){}
    sparse_vector_const_iterator(V& c, typename V::const_iterator pos)
      : sparse_vector_const_iterator::iterator_adaptor_(pos), c(&c) {}
};


// todo: make it a legit vector ? using reference-proxy code ?
template <class K, class V >
class sparse_vector {
    typedef std::map<K,V> PairMap;
public:
    typedef typename PairMap::key_type key_type;
    typedef typename PairMap::mapped_type data_type;
    
    typedef typename PairMap::value_type value_type;

    typedef sparse_vector_element<PairMap> data_reference;
    typedef V const& data_const_reference;

    void insert(value_type const& v)
    {
        smap.insert(v);
    }

    void insert(key_type const& k,data_type const& v)
    {
        smap.insert(value_type(k,v));
    }

    typedef std::size_t size_type;

    typedef typename PairMap::const_iterator const_iterator;
    //typedef typename PairMap::iterator iterator;
    typedef sparse_vector_iterator<PairMap> iterator;

    typedef typename const_iterator::reference const_reference;
    typedef typename iterator::reference reference;
    
    void insert( iterator ignore,value_type const& v) { insert(v);}

    sparse_vector(sparse_vector const& other) : smap(other.smap) {}

    template <class Range>
    explicit sparse_vector(Range const& r)
     : smap(boost::begin(r), boost::end(r)) {}

    sparse_vector& operator=(sparse_vector const& other)
    {
        sparse_vector sv(other);
        swap(sv);
        return *this;
    }

    sparse_vector() {}

    data_reference operator[](key_type k)
    {
        return data_reference(smap,k);
    }

    data_const_reference operator[](key_type k) const
    {
        typename PairMap::const_iterator pos = smap.find(k);
        if (pos == smap.end()) return default_;
        else return pos->second;
    }

    const_iterator find(key_type k) const { return smap.find(k); }
    const_iterator begin() const { return smap.begin(); }
    const_iterator end() const { return smap.end(); }

    iterator find(key_type k) { return iterator(smap,smap.find(k)); }
    iterator begin() { return iterator(smap,smap.begin()); }
    iterator end() { return iterator(smap,smap.end()); }

    bool operator==(sparse_vector const& other) const
    {
        return smap == other.smap; 
    }

    bool operator!=(sparse_vector const& other) const
    {
        return not this->operator==(other);
    }

    size_type size() const { return smap.size(); }

    size_type hash_self() const 
    { 
        boost::hash<PairMap> hasher;
        return hasher(smap); 
    }

    void clear() { smap.clear(); }

    bool empty() const { return smap.empty(); }

    void swap(sparse_vector& other) { smap.swap(other.smap); }

    static const data_type default_;
private:
    PairMap smap;
};

template <class K, class V>
typename sparse_vector<K,V>::data_type const
sparse_vector<K,V>::default_ = typename sparse_vector<K,V>::data_type();

template <class K, class V>
void swap(sparse_vector<K,V>& s1, sparse_vector<K,V>& s2)
{
    s1.swap(s2);
}

// string based setters/getters
template <class K, class V, class Dict>
typename sparse_vector<K,V>::data_const_reference
get(sparse_vector<K,V> const& v, Dict& dict, std::string const& name)
{
    return v[dict.get_index(name)];
}

template <class K, class V, class Dict>
typename sparse_vector<K,V>::data_const_reference
get(sparse_vector<K,V> const& v, Dict const& dict, std::string const& name)
{
    if (dict.has_token(name)) return v[dict.get_index(name)];
    else return sparse_vector<K,V>::default_;
}

template <class K, class V, class Dict>
typename sparse_vector<K,V>::data_reference
get(sparse_vector<K,V>& v, Dict& dict, std::string const& name)
{
    return v[dict.get_index(name)];
}

template <class K, class V>
size_t hash_value(sparse_vector<K,V> const& v) { return v.hash_self(); }

template <class S, class T, class K, class V>
std::basic_ostream<S,T>&
operator<<(std::basic_ostream<S,T>& os, sparse_vector<K,V> const& v)
{
    typename sparse_vector<K,V>::const_iterator
        itr = boost::begin(v),
        end = boost::end(v);
    if (itr != end) {
        // todo: iomanips to control ':' and ','
        os << itr->first << ':' << itr->second;
        ++itr;
    }
    for (; itr != end; ++itr) {
        os << ',' << itr->first << ':' << itr->second;
    }
    return os;
}

// todo: support basic_string<>
namespace detail {
template <class K, class V, class OP>
void read(std::string const& str, sparse_vector<K,V>& v, OP const& op,char const* map_sep=",",char const* pair_sep=":")
{
    v.clear();
    using namespace boost;
    typedef boost::tokenizer< boost::char_separator<char> > toker;

    boost::char_separator<char> sep_map(map_sep);
    boost::char_separator<char> sep_pair(pair_sep);
    toker tokens(str,sep_map);
    for (toker::iterator itr = tokens.begin(); itr != tokens.end(); ++itr) {
        toker token_pair(*itr,sep_pair);
        toker::iterator jitr1 = token_pair.begin();
        if (jitr1 != token_pair.end()) {
            toker::iterator jitr2 = jitr1; ++jitr2;
            if (jitr2 != token_pair.end()) {
                v[op(*jitr1)]=boost::lexical_cast<V>(*jitr2);
                continue;
            }
        }
        throw std::runtime_error(
                 std::string("malformed feature weight string: ") + *itr
              );
    }
}

template <class K>
struct lexical_cast_ {
    K operator()(std::string const& str) const 
    {
        return boost::lexical_cast<K>(str);
    }
};

} // detail

template <class T, class V>
void read(sparse_vector<T,V>& v, std::string const& str)
{
    detail::read(str,v,detail::lexical_cast_<T>());
}

template <class S, class T, class K, class V>
std::basic_istream<S,T>&
operator >> (std::basic_istream<S,T>& is, sparse_vector<K,V>& v)
{
    std::string str;
    is >> str;
    read(v,str);
    return is;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
sparse_vector< T, F >&
operator*=( sparse_vector< T, F >& v1
          , sparse_vector< T, F > const& v2
          )
{
    sparse_op_inplace(v1,v2, gusc::times(),true);
    return v1;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
sparse_vector< T, F >
operator*( sparse_vector< T, F > const& v1
         , sparse_vector< T, F > const& v2
         )
{
    sparse_vector< T, F > w;
    sparse_op_copy(w,v1,v2,gusc::times(),true);
    return w;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
sparse_vector<T,F>&
operator+=(sparse_vector<T,F>& v1, sparse_vector<T,F> const& v2)
{
    sparse_op_inplace(v1, v2, gusc::plus());
    return v1;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
sparse_vector<T,F>
operator+(sparse_vector<T,F> const& v1, sparse_vector<T,F> const& v2)
{
    sparse_vector<T,F> w;
    sparse_op_copy(w, v1, v2, gusc::plus());
    return w;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
sparse_vector<T,F>
operator-(sparse_vector<T,F> const& v)
{
    sparse_vector<T,F> w;
    sparse_scalar_op(w,v,gusc::times(),F(-1));
    return w;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
sparse_vector<T,F>&
operator-=(sparse_vector<T,F>& v1, sparse_vector<T,F> const& v2)
{
    sparse_op_inplace(v1, v2, gusc::minus());
    return v1;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
sparse_vector<T,F>
operator-(sparse_vector<T,F> const& v1, sparse_vector<T,F> const& v2)
{
    sparse_vector<T,F> w;
    sparse_op_copy(w, v1, v2, gusc::minus());
    return w;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
sparse_vector<T,F>&
operator*=(sparse_vector<T,F>& v, F c)
{
    return sparse_scalar_op(v,gusc::times(),c);
}

template <class T, class F>
sparse_vector<T,F>
operator*(F c, sparse_vector<T,F> const& v)
{
    sparse_vector<T,F> w;
    sparse_scalar_op(w,v,gusc::times(),c);
    return w;
}

template <class T, class F>
sparse_vector<T,F>
operator*(sparse_vector<T,F> const& v, F c)
{
    sparse_vector<T,F> w;
    sparse_scalar_op(w,v,gusc::times(),c);
    return w;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
sparse_vector<T,F>&
operator/=(sparse_vector<T,F>& v, F c)
{
    return sparse_scalar_op(v,gusc::divide(),c);
}

template <class T,class F>
sparse_vector<T,F>
operator/(sparse_vector<T,F> const& v, F c)
{
    sparse_vector<T,F> w;
    sparse_scalar_op(w,v,gusc::divide(),c);
    return w;
}

////////////////////////////////////////////////////////////////////////////////

template <class T, class F>
F dot(sparse_vector<T,F> const& v, sparse_vector<T,F> const& w)
{
    return sparse_op_reduce(w, v, gusc::times(), gusc::plus());
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__MATH__FEATURE_VECTOR_HPP

