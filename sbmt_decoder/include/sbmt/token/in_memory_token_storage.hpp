#ifndef   SBMT_TOKEN_IN_MEMORY_TOKEN_STORAGE
#define   SBMT_TOKEN_IN_MEMORY_TOKEN_STORAGE

#include <boost/utility.hpp>
//#include <boost/multi_index_container.hpp>
//#include <boost/multi_index/hashed_index.hpp>
#include <boost/functional/hash.hpp>
#include <boost/serialization/vector.hpp>
#include <vector>
#include <string>
#include <boost/cstdint.hpp>
//#define OA_HASHTABLE_PRINT_ERROR 1
#include <sbmt/hash/oa_hashtable.hpp>
//#define OA_HASHTABLE_PRINT_ERROR 0



namespace sbmt {

class in_memory_token_storage
{
public:
    typedef boost::uint32_t index_type;
    typedef std::string token_type;
    ////////////////////////////////////////////////////////////////////////////////

    class key_extractor
    {
    private:
        std::vector<token_type> const* vec_ref;
    public:
        typedef token_type result_type;

        result_type const& operator()(index_type idx) const
        { return (*vec_ref)[idx]; }

        key_extractor(std::vector<token_type> const& ref)
        : vec_ref(&ref) {}

    };

    ////////////////////////////////////////////////////////////////////////////////

    token_type const& get_token(index_type idx) const
    { return tokens[idx]; }

    index_type get_index(token_type const& t);
    index_type get_index(token_type const& t) const;
    index_type operator[](token_type const& t) { return get_index(t); }
    index_type operator[](token_type const& t) const { return get_index(t); }

    bool get_index_unk(token_type const& t,index_type unk) const //TODO: optimize?  nah, not critical
    {
        return has_token(t) ? get_index(t) : unk;
    }

    bool has_token(token_type const& t) const
    {
        return dict_hash.find(t)!=dict_hash.end();
    }

    bool has_index(index_type idx) const
    {
        return idx < size();
    }

    token_type const& operator[](index_type i) const
    {
        return tokens[i];
    }

    /// adds as new index token t, even if it already existed as some other
    /// index.  returns the index for retrieving the added token instance
    index_type push_back(token_type const& t);

    void reset();
    in_memory_token_storage();

    in_memory_token_storage(index_type reserve);

    void swap(in_memory_token_storage& other);

    index_type size() const { return tokens.size(); }

    index_type n_actual_tokens() const
    {
        return size();
    }

    index_type ibegin() const
    { return 0; }

    index_type iend() const
    { return n_actual_tokens(); }

    template <class Archive>
    void serialize(Archive& ar, const unsigned version)
    {
        if (Archive::is_loading::value)
            drop_get_index();
        ar & BOOST_SERIALIZATION_NVP(tokens);
        if (Archive::is_loading::value)
            regenerate_get_index();
    }

    void shrink(index_type new_end);

    void clear()
    { shrink(0); }

private:
    /// promise not to use get_index() until regenerate_get_index()
    void drop_get_index()
    { dict_hash.clear(); }

    /// ensures get_index() works properly after changing tokens vector or drop_get_index
    void generate_get_index()
    {
        for (index_type i=ibegin();i<iend();++i)
            dict_hash.insert(i);
    }

    void regenerate_get_index()
    {
        drop_get_index();
        generate_get_index();
    }

    friend class boost::serialization::access;
    typedef oa_hashtable<index_type, key_extractor> dict_hash_type;

    std::vector<token_type> tokens;
    dict_hash_type          dict_hash;
};

inline
void swap(in_memory_token_storage& s1, in_memory_token_storage& s2)
{ return s1.swap(s2); }

/// the reason for the no-op swap is because oa_hashtable swaps its functors when
/// it swaps its data (thats a good thing), but in our case we dont want the objects
/// to be swapped, since they have data that refers to other objects in the
/// containing class.
inline
void swap( in_memory_token_storage::key_extractor& k1
         , in_memory_token_storage::key_extractor& k2){ }

} // namespace sbmt

#endif // SBMT_TOKEN_IN_MEMORY_TOKEN_STORAGE
