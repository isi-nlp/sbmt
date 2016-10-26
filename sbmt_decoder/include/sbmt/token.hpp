#ifndef   SBMT_TOKEN_HPP
#define   SBMT_TOKEN_HPP
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include <sbmt/token/token.hpp>
#include <sbmt/token/indexed_token.hpp>
#include <sbmt/token/fat_token.hpp>
#include <sbmt/token/tokenizer.hpp>
#include <sbmt/token/in_memory_token_storage.hpp>
#include <sbmt/token/token_io.hpp>

namespace sbmt {
    typedef dictionary<in_memory_token_storage> 
            in_memory_dictionary;
}

#endif // SBMT_TOKEN_HPP

