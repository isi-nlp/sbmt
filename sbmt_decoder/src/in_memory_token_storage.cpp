#include <cassert>
#include <stdexcept>
#include <sbmt/token/in_memory_token_storage.hpp>
#include <sbmt/token/token.hpp>

#ifndef SBMT_DICTIONARY_INITIAL_SIZE
#define SBMT_DICTIONARY_INITIAL_SIZE 10000
#define SBMT_DICTIONARY_LOAD_FACTOR 0.75
#endif

using namespace std;

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

in_memory_token_storage::in_memory_token_storage()
    : dict_hash( SBMT_DICTIONARY_INITIAL_SIZE
               , SBMT_DICTIONARY_LOAD_FACTOR
               , key_extractor(tokens) )
{}

in_memory_token_storage::
in_memory_token_storage(in_memory_token_storage::index_type reserve)
    : dict_hash( reserve
               , SBMT_DICTIONARY_LOAD_FACTOR
               , key_extractor(tokens) )
{}

void in_memory_token_storage::swap(in_memory_token_storage& other)
{
    dict_hash.swap(other.dict_hash);
    tokens.swap(other.tokens);
}


////////////////////////////////////////////////////////////////////////////////

in_memory_token_storage::index_type
in_memory_token_storage::
push_back(token_type const& t)
{       
    assert(tokens.size() == dict_hash.size());
    index_type newi=tokens.size();
    std::string tt(t.c_str());
    tokens.push_back(tt);
    // must happen after because key extractor references tokens[newi]
    dict_hash.insert(newi); 
    assert(dict_hash.size() == tokens.size());
    return newi;
}

////////////////////////////////////////////////////////////////////////////////

void in_memory_token_storage::shrink(index_type new_end)
{
    assert(tokens.size() == dict_hash.size());
    assert(new_end <= tokens.size());
    for (index_type i = new_end; i < tokens.size(); ++i) {
        dict_hash.erase(tokens[i]);
    }
    tokens.resize(new_end);
    assert(tokens.size() == dict_hash.size());
}

////////////////////////////////////////////////////////////////////////////////

in_memory_token_storage::index_type
in_memory_token_storage::
get_index(std::string const& tok)
{
    assert(tokens.size() == dict_hash.size());
    dict_hash_type::iterator pos = dict_hash.find(tok);
    //FIXME: (here and elsewhere).  use insert and check if inserted or not - 
    //       faster than find then maybe insert
    //tok must be in tokens array to call insert.  but instead, the return of 
    //pos can be used as a hint for the future insertion call...
    return pos == dict_hash.end() ? push_back(tok) : *pos;
}


in_memory_token_storage::index_type
in_memory_token_storage::
get_index(std::string const& tok) const
{
    assert(tokens.size() == dict_hash.size());
    dict_hash_type::iterator pos = dict_hash.find(tok);
    //FIXME: (here and elsewhere).  use insert and check if inserted or not - 
    //       faster than find then maybe insert
    //tok must be in tokens array to call insert.  but instead, the return of 
    //pos can be used as a hint for the future insertion call...
    if (pos == dict_hash.end()) 
        throw std::runtime_error(
            "attempted to retrieve non-existent index from const token_storage "
            "for token '" + tok + "'" 
        );
    return *pos;
}

////////////////////////////////////////////////////////////////////////////////

void in_memory_token_storage::reset()
{
    dict_hash.clear();
    tokens.clear();
}

////////////////////////////////////////////////////////////////////////////////
    
} // namespace sbmt


