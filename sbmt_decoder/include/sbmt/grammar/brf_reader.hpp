# ifndef   SBMT_GRAMMAR_BRF_READER_HPP
# define   SBMT_GRAMMAR_BRF_READER_HPP

# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/grammar/rule_input.hpp>
# include <sbmt/grammar/text_features_byid.hpp>
# include <sbmt/feature/feature_vector.hpp>
# include <RuleReader/Rule.h>
# include <string>
# include <sbmt/logmath.hpp>
# include <boost/function.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

typedef feature_vector score_map_type;

typedef text_feature_vector_byid texts_map_type;

template <class Token>
void null_syntax_cb( syntax_rule<Token> const&
                   , texts_map_type const& ) {}

template <class Token>
void null_binary_cb( binary_rule<Token> const&
                   , score_map_type const&
                   , texts_map_type const&
                   , std::vector<big_syntax_id_type> const& ) {}


////////////////////////////////////////////////////////////////////////////////
///
///  represents the persistent storage of a grammar, and provides an interface
///  for loading that grammar into memory. 
///  different ways of storing a grammar should have different brf_reader 
///  implementations.
///  for instance, brf text files created by itg-binarizer can be loaded into
///  grammar_in_mem by using brf_file_reader, and archive files created by
///  archive_grammar can be loaded using brf_archive_reader.  
///
///  archive_grammar actually works by using brf_file_reader to read in a brf
///  text file and serialize the input using boost::serialization.  so there is
///  no requirement that brf_reader be used to initialize a grammar.  they can
///  be used to process a collection of rules any way deemed fit.
///
///  to implement a brf_reader, you need to know how its expected to be used.
///  a user of a brf_reader is expecting to receive a collection of syntax and
///  binarized rules.  the syntax rules will come with feature vectors, and 
///  the binarized rules will come with a vector that identifies all the syntax
///  rules this binarized rule can build.  note, this mirrors the contents of
///  a text brf file.
///
///  the way the user receives these rules and data is by passing functions that
///  take the above data.  the user will initialize your brf_reader with a 
///  SyntaxHandler function that looks like:
///  \code
///  void f(indexed_syntax_rule const& rule, map<string,score_t> const& features)
///  \endcode
///
///  and a BinaryHandler function that looks like:
///  \code
///  void f(indexed_binary_rule const& rule, vector<int> const& syntax_ids)
///  \endcode
///  
///  you dont have to worry about what they do, you just call them.
///  
///  next, the user will call read().  this is where you will read the storage
///  of the rules, and feed them to the callbacks.  for instance, in
///  brf_file_reader, lines in the brf file are read sequentially, parsed to
///  determine if they are syntax rules or binarized, and fed accordingly.
///
///  as another example, to implement a database reader, the reader might first
///  be initialized with the sentences to be decoded.  then, on read() a db 
///  of syntax rules is somehow queried to find all applicable rules for the 
///  sentences, the rules are binarized (we should really bring the important
///  itg-binarizer code into the fold), and passed to the appropriate callbacks.
///
////////////////////////////////////////////////////////////////////////////////
template <class TF = indexed_token_factory>
class brf_reader_tmpl
{
public:
    typedef typename TF::token_t token_t;
    brf_reader_tmpl()
    : syntax_rule_cb(null_syntax_cb<token_t>)
    , binary_rule_cb(null_binary_cb<token_t>)
    , ptf(0)
    , pdict(0) {}
    
    template<class SyntaxHandler, class BinaryHandler>
    brf_reader_tmpl( SyntaxHandler sf
                   , BinaryHandler bf
                   , TF& tf
                   , in_memory_token_storage& dict )
    : syntax_rule_cb(sf)
    , binary_rule_cb(bf)
    , ptf(&tf)
    , pdict(&dict) {}

    virtual void read() = 0;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// SyntaxHandler is a functor of signature 
    /// void (indexed_syntax_rule const& , map<string, score_t> const& )
    ///
    /// BinaryHandler is a functor of signature
    /// void (binary_rule<TokenType> const&, vector<int> syntax_ids const&)
    /// 
    ////////////////////////////////////////////////////////////////////////////
    template<class SyntaxHandler, class BrfHandler>
    void set_handlers( SyntaxHandler sf
                     , BrfHandler    bf
                     , TF & tf_
                     , in_memory_token_storage& dict )
    {
        
        syntax_rule_cb = sf;
        binary_rule_cb = bf;
        ptf = &tf_;
        pdict = &dict;

    }
    
    virtual ~brf_reader_tmpl(){}

protected:
    
    typedef boost::function <void ( syntax_rule<token_t> const&
                                  , texts_map_type const&) > 
            syntax_rule_callback_type;

    typedef boost::function< void ( binary_rule<token_t> const&
                                  , score_map_type const&
                                  , texts_map_type const&
                                  , std::vector<big_syntax_id_type> const& ) > 
            binary_rule_callback_type;
    
    syntax_rule_callback_type syntax_rule_cb;
    binary_rule_callback_type binary_rule_cb;
    TF* ptf;
    in_memory_token_storage* pdict;
    in_memory_token_storage& dict() { return *pdict; }
    TF& tf() {return *ptf;}
};

typedef brf_reader_tmpl<> brf_reader;
typedef brf_reader_tmpl<fat_token_factory> brf_fat_reader;

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT_GRAMMAR_BRF_READER_HPP
