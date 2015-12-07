#include <boost/utility.hpp>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <sbmt/token/fat_token.hpp>
#include <sbmt/token/indexed_token.hpp>

namespace sbmt {

using namespace std;

////////////////////////////////////////////////////////////////////////////////

fat_token foreign_word(char const* word)
{
    return fat_token(word,foreign_token);
}

////////////////////////////////////////////////////////////////////////////////

fat_token foreign_word(std::string const& word)
{
    return fat_token(word,foreign_token);
}

////////////////////////////////////////////////////////////////////////////////

fat_token native_word(char const* word)
{
    return fat_token(word,native_token);
}

////////////////////////////////////////////////////////////////////////////////

fat_token native_word(std::string const& word)
{
    return fat_token(word,native_token);
}

////////////////////////////////////////////////////////////////////////////////

fat_token tag(char const* word)
{
    return fat_token(word,tag_token);
}

////////////////////////////////////////////////////////////////////////////////

fat_token tag(std::string const& word)
{
    return fat_token(word,tag_token);
}

////////////////////////////////////////////////////////////////////////////////

fat_token virtual_tag(char const* word)
{
    return fat_token(word,virtual_tag_token);
}

////////////////////////////////////////////////////////////////////////////////

fat_token virtual_tag(std::string const& word)
{
    return fat_token(word,virtual_tag_token);
}

////////////////////////////////////////////////////////////////////////////////

fat_token toplevel_tag()
{
    return fat_token(top_token_text,top_token);
}

fat_token foreign_start_sentence()
{
    return fat_token("<foreign-sentence>",foreign_token);
}

fat_token foreign_end_sentence()
{
    return fat_token("</foreign-sentence>",foreign_token);
}

fat_token foreign_unknown_word()
{
    return fat_token("<unknown-word/>",foreign_token);
}

fat_token native_unknown_word()
{
    return fat_token("<unknown-word/>",native_token);
}

fat_token native_epsilon()
{
    return fat_token("<epsilon/>",native_token);
}

fat_token native_separator()
{
    return fat_token("<separator/>",native_token);
}

////////////////////////////////////////////////////////////////////////////////

fat_token::fat_token()
: typ(top_token)
, lbl(top_token_text)
{}

////////////////////////////////////////////////////////////////////////////////

string const& fat_token::label() const
{
    return lbl;
}

////////////////////////////////////////////////////////////////////////////////

token_type_id fat_token::type() const
{
    return token_type_id(typ);
}

////////////////////////////////////////////////////////////////////////////////

void fat_token::swap(fat_token& other)
{
    std::swap(typ, other.typ);
    std::swap(lbl, other.lbl);
}

////////////////////////////////////////////////////////////////////////////////

std::size_t fat_token::hash_value() const
{
    boost::hash<std::string> hasher;
    std::size_t retval(hasher(lbl));
    boost::hash_combine(retval,std::size_t(typ));
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

bool fat_token::equal(fat_token const& other) const
{
    return typ == other.typ and (typ == top_token or lbl == other.lbl);
}

bool fat_token::less(fat_token const& other) const
{
    return (typ < other.typ) or (typ == other.typ and lbl < other.lbl);
}

////////////////////////////////////////////////////////////////////////////////

fat_token::fat_token(std::string const& str, token_type_id typ)
: typ(typ)
, lbl(typ == top_token ? std::string(top_token_text) : str)
{}

////////////////////////////////////////////////////////////////////////////////

fat_token::fat_token(char const* str, token_type_id typ)
: typ(typ)
, lbl(typ == top_token ? top_token_text : str)
{}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

void throw_max_tokens_exceeded(char const* const msg)
{
    max_tokens_exceeded e(msg);
    throw e;
}

////////////////////////////////////////////////////////////////////////////////
  
} // namespace sbmt
