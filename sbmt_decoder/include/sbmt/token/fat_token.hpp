#ifndef   SBMT_TOKEN_FAT_TOKEN_HPP
#define   SBMT_TOKEN_FAT_TOKEN_HPP

#include <sbmt/token/token.hpp>

#include <boost/serialization/string.hpp>

namespace sbmt {
    
////////////////////////////////////////////////////////////////////////////////

class fat_token : public token<fat_token>
{
public:
    token_type_id  type() const;
    std::string const& label() const;
    
    fat_token(std::string const& lbl, token_type_id typ);
    fat_token(char const* lbl, token_type_id typ);
    fat_token();
    
    template <class ArchiveT>
    void serialize(ArchiveT& ar, const unsigned int version)
    {
        ar & typ;
        ar & lbl;
    }
    
private:
    void swap(fat_token& other);
    std::size_t hash_value() const;
    bool equal(fat_token const& other) const;
    bool less(fat_token const& other) const;
    template <class C, class T>
        void print_self(std::basic_ostream<C,T>& os) const;
    friend class token_access;
    char typ;
    std::string lbl;
};

template <class C, class T>
void fat_token::print_self(std::basic_ostream<C,T>& os) const
{
    os << lbl;
}

////////////////////////////////////////////////////////////////////////////////

fat_token foreign_word(char const* word);
fat_token foreign_word(std::string const& word);

fat_token native_word(char const* word);
fat_token native_word(std::string const& word);

fat_token tag(char const* word);
fat_token tag(std::string const& word);

fat_token virtual_tag(char const* word);
fat_token virtual_tag(std::string const& word);

fat_token toplevel_tag();

fat_token foreign_start_sentence();
fat_token foreign_end_sentence();

fat_token foreign_unknown_word();
fat_token native_unknown_word();

fat_token native_epsilon();

fat_token native_separator(); // for internal use only

////////////////////////////////////////////////////////////////////////////////

class fat_token_factory : public token_factory<fat_token_factory, fat_token>
{
public:
    template <class Archive> void serialize(Archive& ar, unsigned int) {}
    typedef fat_token token_type;
    fat_token toplevel_tag() const
    { return sbmt::toplevel_tag(); }
    
    fat_token foreign_word(std::string const& s) const
    { return sbmt::foreign_word(s); }
    
    fat_token native_word(std::string const& s) const
    { return sbmt::native_word(s); }
    
    fat_token tag(std::string const& s) const
    { return sbmt::tag(s); }
    
    fat_token virtual_tag(std::string const& s) const
    { return sbmt::virtual_tag(s); }

    fat_token create_token(std::string const& s, token_type_id typ)
    { return fat_token(s,typ); }
    
    std::string label(fat_token const& t) const { return t.label(); }
};

namespace {
// note: will be warned as unused in some apps.  but it is used in others e.g. binal.cc
static fat_token_factory fat_tf;
void kill_fat_token_factory_warning(fat_token_factory&) {}
void kill_fat_token_factory_warning() { return kill_fat_token_factory_warning(fat_tf); }
}

}

#endif // SBMT_TOKEN_FAT_TOKEN_HPP
