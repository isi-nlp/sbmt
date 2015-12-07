#ifndef   SBMT_TOKEN_INDEXED_TOKEN_HPP
#define   SBMT_TOKEN_INDEXED_TOKEN_HPP

#include <sbmt/token/token.hpp>
#include <sbmt/token/fat_token.hpp>
#include <sbmt/token/in_memory_token_storage.hpp>
#include <boost/range.hpp>

#include <string>
#include <iostream>
#include <iomanip>

#include <boost/signals.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/cstdint.hpp>
#include <boost/io/ios_state.hpp>
#include <graehl/shared/is_null.hpp>

namespace sbmt
{

////////////////////////////////////////////////////////////////////////////////

class max_tokens_exceeded : public std::exception
{
public:
    max_tokens_exceeded(char const* const msg)
    : msg(msg) {}
    virtual char const* what() const throw() { return msg; }
    virtual ~max_tokens_exceeded() throw() {}
private:
    char const* const msg;
};

void throw_max_tokens_exceeded(char const* const msg);

template <class StorageP> class dictionary;

  struct as_top {};

////////////////////////////////////////////////////////////////////////////////
///
/// a light-weight token (implemented as a single size_t).
/// has fast equality, hashing and type-checking (lexeme or tag? virtual? top?)
/// requires a dictionary to create one correctly.  the string information that
/// created the token can only be found in the dictionary, and depending on
/// the dictionaries storage strategy, recovery of the string label can be
/// slow.
///
////////////////////////////////////////////////////////////////////////////////
class indexed_token : public token<indexed_token>
{
public:
    typedef indexed_token self_type;
    typedef boost::uint32_t size_type;
    BOOST_STATIC_CONSTANT(size_type,type_stride = ((size_type)1)<<30);
    BOOST_STATIC_CONSTANT(size_type,top_index = type_stride-1);
    BOOST_STATIC_CONSTANT(size_type,null_index=type_stride-2);
    BOOST_STATIC_CONSTANT(size_type,max_normal_index=type_stride-3);
    /// index ranges from 1...dictionary::count_type()
    size_type index() const
    { return typ_and_idx % indexed_token::type_stride; }

    bool is_top() const
    {
        return typ_and_idx == top_index;
    }

    bool is_null() const
    {
        return typ_and_idx==null_index;
    }
    void set_null()
    {
        typ_and_idx=null_index;
    }
    MEMBER_IS_SET_NULL
    token_type_id  type() const
    {
      if (is_top()) return top_token; //this is *why* index ranges from 1...
      return token_type_id(typ_and_idx / indexed_token::type_stride);
    }

    indexed_token() : typ_and_idx(null_index) {}

    explicit indexed_token(as_top tag) : typ_and_idx(top_index) {}

    indexed_token(size_type index, token_type_id typ)
    {
//        if (typ==top_token) typ_and_idx=top_index; else
        {
            assert(typ!=top_token && index != top_index && index <= max_normal_index);
            typ_and_idx=typ*type_stride + index;
        }

    }

    explicit indexed_token(size_type typidx) : typ_and_idx(typidx) {}

    friend inline std::size_t hash_value(self_type const& s) { return s.hash_value(); }

    template <class ArchiveT>
    void serialize(ArchiveT& ar, const unsigned int version);
private:
    size_type typ_and_idx;
    bool equal(indexed_token const& other) const
    { return typ_and_idx == other.typ_and_idx; }
    bool less(indexed_token const& other) const
    { return typ_and_idx < other.typ_and_idx; }
    size_type hash_value() const
    { return typ_and_idx; }
    void swap(indexed_token & other);
    template <class C, class T>
        void print_self(std::basic_ostream<C,T>& os) const;
    friend class token_access;
    template <class X> friend class dictionary;
};

////////////////////////////////////////////////////////////////////////////////
///
///  dictionary essentially stores all the string names for indexed_token
///  objects, and is what you use to make indexed_tokens.
///
///  dictionaries could store the strings a variety of ways, encapsulated by
///  StorageP, which by default stores the strings in memory.
///
///  StorageP must provide the interface:
///   std::size_t index(std::string const& token);
///   std::string token(std::size_t idx);
///
///   each call to index with the same token must return the same index,
///   each call to index with a different token must return a different index.
///   the first call to index must return 1.
///   the first call to index with a new token must return the smallest positive
///    integer not yet seen.
///
////////////////////////////////////////////////////////////////////////////////
template <class StorageP = in_memory_token_storage>
class dictionary : boost::noncopyable
{
public:
    typedef indexed_token token_t;
    typedef indexed_token token_type;
    typedef token_t::size_type size_type;

    class iterator
    : public boost::iterator_facade<
                 iterator
               , token_t const
               , boost::forward_traversal_tag
             >
    {
        bool equal(iterator const& other) const
        {
            return current == other.current;
        }

        token_t const& dereference() const
        {
            return current;
        }

        void increment()
        {
            current = token_t(current.index()+1,current.type());
        }

        iterator(token_t begin) : current(begin) {}

        token_t   current;

        friend class boost::iterator_core_access;
        friend class dictionary<StorageP>;
    };

    typedef boost::iterator_range<iterator> range;

    token_t foreign_word(std::string const& word);
    token_t find_foreign_word(std::string const& word) const;
    token_t foreign_word(std::string const& word) const { return find_foreign_word(word); }
    range   foreign_words(size_type x = 0) const;

    token_t native_word(std::string const& word);
    token_t find_native_word(std::string const& word) const;
    token_t native_word(std::string const& word) const { return find_native_word(word); }
    bool    has_native_word(std::string const& word) const;
    range   native_words(size_type x = 0) const;

    token_t tag(std::string const& word);
    token_t find_tag(std::string const& word) const;
    token_t tag(std::string const& word) const {return find_tag(word); }
    range   tags(size_type x = 0) const;

    token_t virtual_tag(std::string const& word);
    token_t find_virtual_tag(std::string const& word) const;
    token_t virtual_tag(std::string const& word) const { return find_virtual_tag(word); }
    range virtual_tags() const;

    token_t toplevel_tag() const;

    token_t foreign_start_sentence() const
    { return foreign_start_sentence_tok; }

    token_t foreign_end_sentence() const
    { return foreign_end_sentence_tok; }

    token_t foreign_unknown_word() const
    { return foreign_unknown_word_tok; }

    token_t native_unknown_word() const
    { return native_unknown_word_tok; }

    token_t native_epsilon() const
    { return native_epsilon_tok; }

    token_t native_separator() const
    { return native_separator_tok; }

    token_t create_token(std::string const& word, token_type_id t);
    token_t find_token(std::string const& word, token_type_id t) const;

    void reset_storage();
    void reset()
    { reset_storage(); }

    std::string const& label(token_t const& t) const;

    size_type foreign_word_count() const { return foreign_storage.size(); }
    size_type native_word_count() const { return native_storage.size(); }
    size_type tag_count() const { return tag_storage.size(); }
    size_type virtual_tag_count() const { return virtual_tag_storage.size(); }

    template <class Archive>
    void serialize(Archive & ar, const unsigned version)
    {
        ar & BOOST_SERIALIZATION_NVP(foreign_storage);
        ar & BOOST_SERIALIZATION_NVP(native_storage); 
        ar & BOOST_SERIALIZATION_NVP(tag_storage); 
        ar & BOOST_SERIALIZATION_NVP(virtual_tag_storage);
    }

    void shrink( token_t const& foreign_tok
               , token_t const& native_tok
               , token_t const& tag_tok
               , token_t const& virtual_tok );

    /*
    template <class ArchiveT>
    void save( ArchiveT & ar
             , const unsigned int version ) const;

    template <class ArchiveT>
    void load( ArchiveT & ar
             , const unsigned int version );

    BOOST_SERIALIZATION_SPLIT_MEMBER()
    */
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// purpose of attach_reset_action is to be notified whenever someone
    /// resets the storage, as that will cause all tokens to be invalid.
    /// any code making use of caches of token_t may need to use this.
    ///
    /// note: if you ever need to remove the action you have attached, tie the
    /// action to a scoped_connection:
    /// boost::signals::scoped_connection sc = factory.attach_reset_action(f);
    /// now, when sc goes out of scope, your action goes away too.  this
    /// can help deletion order issues.  see boost::signals for more info.
    ///
    /// the type of your action func is expected to be callable as func(),
    /// with no return value.
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class F>
    boost::signals::connection attach_reset_action(F func)
    {
        return reset_signal.connect(func);
    }

    dictionary();

    void swap(dictionary<StorageP> &o)
    {
        foreign_storage.swap(o.foreign_storage);
        native_storage.swap(o.native_storage);
        tag_storage.swap(o.tag_storage);
        virtual_tag_storage.swap(o.virtual_tag_storage);

        //FIXME: signals can't be copied/swapped
        //michael -- not a bug:  swap should actually trigger the signal.
    }

    StorageP& tag_token_factory() { return native_storage; }
    StorageP const& tag_token_factory() const { return native_storage; }

private:
    friend class boost::serialization::access;

    static std::string const top_string;

    void init_special_tokens();

    token_t foreign_start_sentence_tok;
    token_t foreign_end_sentence_tok;
    token_t foreign_unknown_word_tok;
    token_t native_unknown_word_tok;
    token_t native_epsilon_tok;
    token_t native_separator_tok;

    StorageP foreign_storage;
    StorageP native_storage;
    StorageP tag_storage;
    StorageP virtual_tag_storage;
    boost::signal<void ()> reset_signal;
};

typedef dictionary<in_memory_token_storage> indexed_token_factory;

////////////////////////////////////////////////////////////////////////////////

indexed_token_factory const* get_dict(std::ios_base&);

class token_label;
std::ostream& operator << (std::ostream&, token_label const&);

////////////////////////////////////////////////////////////////////////////////
///
/// ties a dictionary to an iostream for using standard iostream functions with
/// indexed_tokens
///
////////////////////////////////////////////////////////////////////////////////
class token_label {
public:
    explicit token_label(fat_token_factory const&) {}
    explicit token_label(indexed_token_factory const& dict);
    explicit token_label(indexed_token_factory const* dict);
    struct saver : public boost::io::ios_pword_saver {
        saver(std::ios_base&);
    };
private:
    indexed_token_factory const* dict;
    static int dict_index();

    friend indexed_token_factory const* get_dict(std::ios_base&);
    friend std::ostream& operator << (std::ostream&, token_label const&);
};

typedef token_label::saver token_format_saver;

/// equivalent to token_label(NULL)
std::ostream& token_raw(std::ostream&);

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/token/impl/indexed_token.ipp>

#endif // SBMT_INDEXED_TOKEN_HPP

