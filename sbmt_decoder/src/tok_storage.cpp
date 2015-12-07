/*
#include <sbmt/detail/tok_storage.hpp>
#include <boost/functional/hash.hpp>

namespace sbmt { namespace detail {

////////////////////////////////////////////////////////////////////////////////

bool operator==(tok_storage const& t1, tok_storage const& t2)
{
    if (t1.type() == top_token) return t2.type() == top_token;
    else return t1.type() == t2.type() and t1.label() == t2.label();
}

bool operator!=(tok_storage const& t1, tok_storage const& t2)
{
    return !(t1 == t2);
}

std::size_t hash_value(tok_storage const& tok)
{
    std::size_t seed = 0;
    boost::hash<std::string> hasher;
    if (tok.type() != top_token) {
        boost::hash_combine(seed,hasher(tok.label()));
    }
    boost::hash_combine(seed,tok.type());
    return seed;
}

////////////////////////////////////////////////////////////////////////////////

tok_storage::tok_storage(std::string const& l, token_type t)
: lbl(t == top_token ? std::string(top_token_text) : l)
, typ(t) {}

tok_storage::tok_storage(char const* l, token_type t)
: lbl(t == top_token ? top_token_text : l)
, typ(t) {}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::detail
*/
