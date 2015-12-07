#include <sbmt/sentence.hpp>
#include <sbmt/token/tokenizer.hpp>
#include <iostream>

namespace sbmt {

fat_sentence native_sentence(std::string const& str)
{
	fat_token_factory tf;
	char_separator<char,fat_token_factory> cs(tf,native_token," \t\n\r","") ;
    tokenizer<> tok(str,cs) ;
    return fat_sentence(tok.begin(),tok.end()) ;
}

fat_sentence foreign_sentence(std::string const& str)
{
    fat_token_factory tf;
	char_separator<char,fat_token_factory> cs(tf,foreign_token," \t\n\r","") ;
    tokenizer<> tok(str,cs) ;
    return fat_sentence(tok.begin(),tok.end()) ;
}

} // namespace sbmt
