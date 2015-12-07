#ifndef SBMT__FOREST__UNIQUE_STRING_FILTER_HPP
#define SBMT__FOREST__UNIQUE_STRING_FILTER_HPP

#include <functional>
#include <boost/cstdint.hpp>
#include <graehl/shared/hash_functions.hpp>
#include <graehl/shared/hashed_value.hpp>
#include <sbmt/forest/derivation.hpp>
#include <sbmt/hash/oa_hashtable.hpp>

// much slower on hit (duplicate detected or collision) if enabled (1).  faster, but some (1 in 2^64) false duplicate answers if 0.
#define  SBMT_EXACT_KBEST_YIELD_FILTER 0

namespace sbmt {

/// e.g. Reader = derivation_reader<Edge,Grammar>
template <class Edge,class Reader>
class unique_string_filter
{
 public:
    enum {trivial=0};

    typedef Edge edge_type;
    typedef edge_equivalence<Edge> derivation_type;
    typedef Reader reader_type;
 private:
    typedef unique_string_filter<Edge,Reader> filter_type;
    typedef typename reader_type::english_string_list yield_type;
 public:


 private:
    typedef graehl::hash64_t hash_type;
    struct hash_yield : public std::binary_function<derivation_type,derivation_type,hash_type>
    {
        reader_type reader;
        hash_type operator()(derivation_type const& a) const
        {
            yield_type e(a,reader);
            return graehl::hash_pod_vector(e);
        }
        hash_yield(reader_type const& r) : reader(r) {}
    };
    struct equal_yield : public std::binary_function<derivation_type,derivation_type,bool>
    {
        reader_type reader;
        bool operator()(derivation_type const& a,derivation_type const& b) const
        {
            yield_type e(a,reader),f(b,reader);
            return e==f; //FIXME: yield and compare inline (two stacks, no explicit full strings)
        }
        equal_yield(reader_type const& r) : reader(r) {}
    };
    hash_yield hasher;
    reader_type const& reader() const
    {
        return hasher.reader;
    }

    typedef graehl::hashed_value<derivation_type,hash_yield> key_type;
    typedef std::pair<key_type,unsigned> entry_type;

#if SBMT_EXACT_KBEST_YIELD_FILTER
    typedef first_key_extractor<key_type> key_f;
    typedef boost::hash<key_type> hash_f;
    typedef typename key_type::template equal_to<equal_yield> equal_f;
    typedef oa_hashtable<entry_type,key_f,hash_f,equal_f > lossless_table_type;
    typedef lossless_table_type table_type;
#else
    struct first_key_just_hash
    {
        typedef hash_type result_type;
        template <class Pair>
        hash_type operator()(Pair const& pair) const
        { return pair.first.hash_val(); }
    };
    struct already_hashed // stdext::identity<hash_type> ?
    {
        typedef std::size_t result_type;
        result_type operator()(hash_type h) const
        {
            return h;
        }
    };
    typedef oa_hashtable<entry_type,first_key_just_hash,already_hashed > lossy_table_type; // note: doesn't do exact equal comparison -> false positives (filtered out) for non-1stbest for a node.
    typedef lossy_table_type table_type;
#endif
    table_type table;
    unsigned stop_at;
 public:

    template <class O>
    void print(O &o,derivation_type const& d) const
    {
        key_type k(d,hasher);
        entry_type entry(k,0);
        yield_type yield(d,reader());
        o << "hash(";
        reader().print_tokens(o,yield);
        o << ")="<<k.hash_val();
        typename table_type::iterator i=table.find(table.extract_key()(entry));

        o << " count=";
        if (i==table.end())
            o << "???";
        else
            o << i->second;
    }

    /// checks for (probably) new yield and returns true if new
    bool permit(derivation_type const& d)
    {
        typename table_type::iterator i=table.insert(entry_type(key_type(d,hasher),0)).first;
        unsigned &c=const_cast<entry_type&>(*i).second;
        if (c >= stop_at)
            return false;
// note: could eventually overflow in some cases, so we don't keep incrementing after reaching limit
        ++c;
        return true;
    }

    BOOST_STATIC_CONSTANT(std::size_t,init_table_size=8);

    /// note: max_per=0 -> unlimited

    struct factory
    {
        Reader reader;
        unsigned max_per;
        factory() {}
        factory(Reader const& reader,unsigned max_per)
            : reader(reader),max_per(max_per) {}
        typedef unique_string_filter<Edge,Reader> filter_type;
        factory const& filter_init() const
        {
            return *this;
        }

    };


    explicit unique_string_filter(factory const& init)
        : hasher(init.reader)
        ,table(init_table_size
#if SBMT_EXACT_KBEST_YIELD_FILTER
               , key_f()
               , hash_f()
               , equal_f(init.reader) // not just equal_yield(g): short circuit if cached hashes differ //               , key_type::make_equal_to(r)
#endif
            )
        , stop_at(init.max_per)
    {}
};

template <class O, class Edge, class Reader>
O&   print( O & o
          , typename unique_string_filter<Edge,Reader>::derivation_type const& d
          , unique_string_filter<Edge,Reader> const& uf )
{
    uf.print(o,d);
    return o;
}


template <class Edge,class Grammar>
typename unique_string_filter<Edge,derivation_reader<Edge,Grammar> >::factory
filter_by_unique_string(
    derivation_reader<Edge,Grammar> const& reader
    , unsigned max_per=1
    )
{
    return typename unique_string_filter<Edge,derivation_reader<Edge,Grammar> >::factory
        (reader,max_per);
}


}//sbmt


#endif
