#ifndef   SBMT_GRAMMAR_BRF_FILE_READER_HPP
#define   SBMT_GRAMMAR_BRF_FILE_READER_HPP

#include <sbmt/grammar/syntax_rule.hpp>
#include <sbmt/grammar/rule_input.hpp>
#include <sbmt/grammar/brf_reader.hpp>
#include <sbmt/logmath.hpp>

#include <RuleReader/Rule.h>

#include <string>

#include <boost/function.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
/// brf_stream_reader interprets a binarized file, calling user supplied 
/// callbacks to pass binary and syntax rules to the user.
/// class grammar_in_mem has a good example of it in use in its 
/// grammar_in_mem::load() function.
///
/// \note with the new decoder, we have added some brf information that allows 
/// us to never have to keep track of a separate xrs rule file.  right now, 
/// only the version of itg-binarizer that is in subversion supports this brf
/// format.
///
////////////////////////////////////////////////////////////////////////////////
template <class TF = indexed_token_factory>
class brf_stream_reader_tmpl : public brf_reader_tmpl<TF>
{
public:
    brf_stream_reader_tmpl(std::istream& input,bool verbose=true)
        : verbose(verbose),input(input) {}
    
    template<class SyntaxHandler, class BinaryHandler>
    brf_stream_reader_tmpl( std::istream& in
                          , SyntaxHandler sf
                          , BinaryHandler bf
                          , TF& tf
                          , in_memory_token_storage& dict )
    : brf_reader_tmpl<TF>(sf,bf,tf,dict)
    , input(in) {}

    virtual void read();

private:
    bool verbose;
    std::istream& input;
};

typedef brf_stream_reader_tmpl<> brf_stream_reader;
typedef brf_stream_reader_tmpl<fat_token_factory> brf_fat_stream_reader;

} // namespace sbmt

#endif // SBMT_GRAMMAR_BRF_READER_HPP
