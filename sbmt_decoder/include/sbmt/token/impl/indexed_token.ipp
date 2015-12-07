#include <boost/serialization/string.hpp>
#include <limits>
#include <sbmt/token/fat_token.hpp>
#include <stdexcept>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

inline void indexed_token::swap(indexed_token & other)
{
    std::swap(typ_and_idx,other.typ_and_idx);
}

////////////////////////////////////////////////////////////////////////////////

template <class ArchiveT>
void indexed_token::serialize(ArchiveT& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(typ_and_idx);
}

////////////////////////////////////////////////////////////////////////////////

template <class C, class T>
void indexed_token::print_self(std::basic_ostream<C,T>& os) const
{
    indexed_token_factory const* dict = get_dict(os);
    if (dict) {
        if (type() != top_token) os << dict->label(*this);
        else os << "TOP";
    } else {
        os << typ_and_idx;
    }
}

/*
////////////////////////////////////////////////////////////////////////////////

template <class AR>
void save_storage_to_archive( AR & ar
                            , indexed_token_factory const& tf
                            , token_type_id typ
                            , indexed_token::size_type sz)
{
    ar & BOOST_SERIALIZATION_NVP(sz);
    for (indexed_token::size_type i = 0; i != sz; ++i) {
        std::string s = tf.label(indexed_token(i,typ));
        ar & BOOST_SERIALIZATION_NVP(s);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class AR>
void load_storage_from_archive( AR & ar
                              , indexed_token_factory& tf
                              , token_type_id typ)
{
    indexed_token::size_type sz;
    ar & BOOST_SERIALIZATION_NVP(sz);

    for (indexed_token::size_type i = 0; i != sz; ++i) {
        std::string s;
        ar & BOOST_SERIALIZATION_NVP(s);
        tf.create_token(s,typ);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class Archive>
void indexed_token_factory::save(Archive & ar, const unsigned int version) const
{
    save_storage_to_archive(ar, *this, foreign_token, pimpl->foreign_word_count());
    save_storage_to_archive(ar, *this, native_token, pimpl->native_word_count());
    save_storage_to_archive(ar, *this, tag_token, pimpl->tag_count());
    save_storage_to_archive(ar, *this, virtual_tag_token, pimpl->virtual_tag_count());
}

////////////////////////////////////////////////////////////////////////////////

template <class Archive>
void indexed_token_factory::load(Archive & ar, const unsigned int version)
{
    reset();
    
    load_storage_from_archive(ar, *this, foreign_token);
    load_storage_from_archive(ar, *this, native_token);
    load_storage_from_archive(ar, *this, tag_token);
    load_storage_from_archive(ar, *this, virtual_tag_token);
}
*/

////////////////////////////////////////////////////////////////////////////////

template <class ST>
std::string const& dictionary<ST>::label(indexed_token const& tok) const
{
    size_type idx = tok.index();
    switch (tok.type()) {
    case top_token: return top_string; break;
    case foreign_token: return foreign_storage.get_token(idx); break;
    case native_token: return native_storage.get_token(idx); break;
    case tag_token: return tag_storage.get_token(idx); break;
    case virtual_tag_token: return virtual_tag_storage.get_token(idx); break;
    default:
        throw std::runtime_error("Unknown token type");
    }
}

template <class ST>
std::string const dictionary<ST>::top_string(top_token_text);

////////////////////////////////////////////////////////////////////////////////

template <class ST>
indexed_token 
dictionary<ST>::native_word(std::string const& s)
{
    size_type idx = native_storage.get_index(s);
    if (idx >= indexed_token::max_normal_index) 
        throw_max_tokens_exceeded("too many native tokens");
    return indexed_token(idx,native_token);
} 

template <class ST>
indexed_token 
dictionary<ST>::find_native_word(std::string const& s) const
{
    size_type idx = native_storage.get_index(s);
    if (idx >= indexed_token::max_normal_index) 
        throw_max_tokens_exceeded("too many native tokens");
    return indexed_token(idx,native_token);
}

template <class ST>
bool dictionary<ST>::has_native_word(std::string const& s) const
{
    return native_storage.has_token(s);
}

template <class ST>
typename dictionary<ST>::range
dictionary<ST>::native_words(size_type x) const
{
    return range( iterator(indexed_token(x,native_token))
                , iterator(indexed_token(native_storage.size(),native_token)) );
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
indexed_token 
dictionary<ST>::foreign_word(std::string const& s)
{
    size_type idx = foreign_storage.get_index(s);
    if (idx >= indexed_token::max_normal_index) 
        throw_max_tokens_exceeded("too many foreign tokens");
    return indexed_token(idx,foreign_token);
}

template <class ST>
indexed_token 
dictionary<ST>::find_foreign_word(std::string const& s) const
{
    size_type idx = foreign_storage.get_index(s);
    if (idx >= indexed_token::max_normal_index) 
        throw_max_tokens_exceeded("too many foreign tokens");
    return indexed_token(idx,foreign_token);
}

template <class ST>
typename dictionary<ST>::range
dictionary<ST>::foreign_words(size_type x) const
{
    return range( iterator(indexed_token(x,foreign_token))
                , iterator(indexed_token(foreign_storage.size(),foreign_token)) );
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
indexed_token 
dictionary<ST>::tag(std::string const& s)
{
    size_type idx = tag_storage.get_index(s);
    if (idx >= indexed_token::max_normal_index) 
        throw_max_tokens_exceeded("too many tag tokens");
    return indexed_token(idx,tag_token);
}


template <class ST>
indexed_token 
dictionary<ST>::find_tag(std::string const& s) const
{
    size_type idx = tag_storage.get_index(s);
    if (idx >= indexed_token::max_normal_index) 
        throw_max_tokens_exceeded("too many tag tokens");
    return indexed_token(idx,tag_token);
}

template <class ST>
typename dictionary<ST>::range
dictionary<ST>::tags(size_type x) const
{
    return range( iterator(indexed_token(x,tag_token))
                , iterator(indexed_token(tag_storage.size(),tag_token)) );
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
indexed_token 
dictionary<ST>::virtual_tag(std::string const& s)
{
    size_type idx = virtual_tag_storage.get_index(s);
    if (idx >= indexed_token::max_normal_index) 
        throw_max_tokens_exceeded("too many virtual tag tokens");
    return indexed_token(idx,virtual_tag_token);
}

template <class ST>
indexed_token 
dictionary<ST>::find_virtual_tag(std::string const& s) const
{
    size_type idx = virtual_tag_storage.get_index(s);
    if (idx >= indexed_token::max_normal_index) 
        throw_max_tokens_exceeded("too many virtual tag tokens");
    return indexed_token(idx,virtual_tag_token);
}

template <class ST>
typename dictionary<ST>::range
dictionary<ST>::virtual_tags() const
{
    return range( iterator(indexed_token(0,virtual_tag_token))
                , iterator(indexed_token(virtual_tag_storage.size(),virtual_tag_token)) );
}

template <class ST>
void dictionary<ST>::reset_storage()
{
    foreign_storage.reset();
    native_storage.reset();
    tag_storage.reset();
    virtual_tag_storage.reset();
    init_special_tokens();
    reset_signal();
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
void dictionary<ST>::shrink( indexed_token const& f
                           , indexed_token const& n
                           , indexed_token const& t
                           , indexed_token const& v )
{
    foreign_storage.shrink(f.index());
    native_storage.shrink(n.index());
    tag_storage.shrink(t.index());
    virtual_tag_storage.shrink(v.index());
    reset_signal();
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
indexed_token 
dictionary<ST>::toplevel_tag() const
{
    return indexed_token(as_top());
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
indexed_token
dictionary<ST>::create_token(std::string const& s, token_type_id t)
{
    switch(t){
    case top_token:         return this->toplevel_tag();  break;
    case foreign_token:     return this->foreign_word(s); break;
    case native_token:      return this->native_word(s);  break;
    case tag_token:         return this->tag(s);          break;
    case virtual_tag_token: return this->virtual_tag(s);  break;
    default:
        throw std::runtime_error("Unknown token type");
    }
}

template <class ST>
indexed_token
dictionary<ST>::find_token(std::string const& s, token_type_id t) const
{
    switch(t){
    case top_token:         return this->toplevel_tag();  break;
    case foreign_token:     return this->find_foreign_word(s); break;
    case native_token:      return this->find_native_word(s);  break;
    case tag_token:         return this->find_tag(s);          break;
    case virtual_tag_token: return this->find_virtual_tag(s);  break;
    default:
        throw std::runtime_error("Unknown token type");
    }
}


template <class ST>
void dictionary<ST>::init_special_tokens()
{
    fat_token ft;
    
    ft = sbmt::foreign_start_sentence();
    foreign_start_sentence_tok = create_token(ft.label(),ft.type());
    
    ft = sbmt::foreign_end_sentence();
    foreign_end_sentence_tok = create_token(ft.label(),ft.type());
    
    ft = sbmt::foreign_unknown_word();
    foreign_unknown_word_tok = create_token(ft.label(),ft.type());
    
    ft = sbmt::native_unknown_word();
    native_unknown_word_tok = create_token(ft.label(),ft.type());
    
    ft = sbmt::native_epsilon();
    native_epsilon_tok = create_token(ft.label(),ft.type());

    ft = sbmt::native_separator();
    native_separator_tok = create_token(ft.label(),ft.type());
}

template <class ST>
dictionary<ST>::dictionary()
{
    init_special_tokens();
}

} // namespace sbmt
