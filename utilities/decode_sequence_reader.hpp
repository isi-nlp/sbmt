#ifndef   UTILITIES__DECODE_SEQUENCE_READER_HPP
#define   UTILITIES__DECODE_SEQUENCE_READER_HPP

#include <iosfwd>
#include <string>
#include <stdexcept>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>


#include <sbmt/search/block_lattice_tree.hpp>
#include <sbmt/grammar/brf_archive_io.hpp>
#include <sbmt/feature/feature_vector.hpp>

////////////////////////////////////////////////////////////////////////////////
/// 
/// \file decode_sequence_reader.hpp
/// \todo consider whether or not this belongs in sbmt
///
////////////////////////////////////////////////////////////////////////////////

struct decode_sequence_grammar;
struct decode_sequence_data;

class decode_sequence_parse_error: public std::runtime_error
{
public:
    decode_sequence_parse_error(std::string const&);
    virtual ~decode_sequence_parse_error() throw(){}
};

void throw_decode_sequence_parse_error(std::string const&);


////////////////////////////////////////////////////////////////////////////////
///
///  reads a decode sequence file, and executes user supplied callbacks for 
///  each command in the file.
///
////////////////////////////////////////////////////////////////////////////////
class decode_sequence_reader
{
public:
    typedef boost::function<void (std::vector<std::string> const&)>
            decode_forest_cb_type;
            
    typedef boost::function<void (sbmt::fat_weight_vector const&)>
            set_weights_cb_type;
            
    typedef boost::function<void (std::string const&, std::string const&, sbmt::archive_type)>
            load_grammar_cb_type;
            
    typedef boost::function<void (std::string const&, sbmt::archive_type)>
            push_grammar_cb_type;
    
    typedef boost::function<void (void)>
            pop_grammar_cb_type;
            
    typedef boost::function<void (std::vector<std::string> const&)> 
            use_info_cb_type;

    typedef boost::function<void (std::string const&,boost::filesystem::path const&)> 
            load_dynamic_ngram_cb_type;
    
    typedef boost::function<void (std::string, std::string, std::string)> 
            set_info_option_cb_type;
    
    typedef boost::function<void (std::string const&)> 
            change_options_cb_type;

    typedef boost::function<void (std::vector<std::string> const&)> 
            multipass_options_cb_type;
    
    typedef boost::function<void ( std::string const&
                                 , std::string const&
                                 , std::size_t)>
            force_decode_cb_type;
    
    typedef boost::function<void ( gusc::lattice_ast const& ltree
                                 , std::size_t )>
            decode_cb_type;
    
    decode_sequence_reader(std::size_t sentid = 1);
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// DecodeCB must have signature
    /// \code
    /// void ( gusc::lattice::ast const& lattice, std::size_t sentid )
    /// \endcode
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class DecodeCB>
    void set_decode_callback(DecodeCB cb)
    { set_decode_cb(cb); }
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// LoadGramCB must have signature
    /// \code
    /// void ( std::string const& filename
    ///      , std::string const& weights
    ///      , sbmt::archive_type fmt )
    /// \endcode
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class LoadGramCB>
    void set_load_grammar_callback(LoadGramCB cb)
    { set_load_grammar_cb(cb); }
    
    template <class PushGramCB>
    void set_push_grammar_callback(PushGramCB cb)
    { set_push_grammar_cb(cb); }
    
    template <class PopGramCB>
    void set_pop_grammar_callback(PopGramCB cb)
    { set_pop_grammar_cb(cb); }
    
    template <class SetWeightsCB>
    void set_weights_callback(SetWeightsCB cb)
    { set_weights_cb(cb); }
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// LoadLMCB must have signature
    /// \code
    /// void ( std::string const& init,boost::filesystem::path const& base_cd)
    /// \endcode
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class LoadLMCB>
    void set_load_dynamic_ngram_callback(LoadLMCB cb)
    { set_load_dynamic_ngram_cb(cb); }

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  must have signature
    /// \code
    /// void ( std::string const& cmdline)
    /// \endcode
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class CB>
    void set_change_options_callback(CB cb)
    { set_change_options_cb(cb); }

    
    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  must have signature
    /// \code
    /// void ( std::vector<std::string> const& cmdline)
    /// \endcode
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class CB>
    void set_multipass_options_callback(CB cb)
    { set_multipass_options_cb(cb); }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// ForceDecodeCB must have signature
    /// \code
    /// void ( std::string const& source
    ///      , std::string const& target
    ///      , std::size_t sentid )
    /// \endcode
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class ForceDecodeCB>
    void set_force_decode_callback(ForceDecodeCB cb)
    { set_force_decode_cb(cb); }
    
    template <class CB>
    void set_use_info_callback(CB cb) { set_use_info_cb(cb); }
    
    template <class CB>
    void set_info_option_callback(CB cb) { set_info_option_cb(cb); }
    
    template <class CB>
    void set_decode_forest_callback(CB cb) { set_decode_forest_cb(cb); }
    
    void read(std::istream& in);
    void read(std::string filename);
    
private:
    void set_decode_forest_cb(decode_forest_cb_type cb);
    void set_load_dynamic_ngram_cb(load_dynamic_ngram_cb_type cb);
    void set_multipass_options_cb(multipass_options_cb_type cb);
    void set_change_options_cb(change_options_cb_type cb);
    void set_load_grammar_cb(load_grammar_cb_type cb);
    void set_push_grammar_cb(push_grammar_cb_type cb);
    void set_pop_grammar_cb(pop_grammar_cb_type cb);
    void set_decode_cb(decode_cb_type cb);
    void set_force_decode_cb(force_decode_cb_type cb);
    void set_use_info_cb(use_info_cb_type cb);
    void set_weights_cb(set_weights_cb_type cb);
    void set_info_option_cb(set_info_option_cb_type cb);
    
    void read_impl(std::istream& in);
    boost::shared_ptr<decode_sequence_data> data;
    friend struct decode_sequence_grammar;
};

////////////////////////////////////////////////////////////////////////////////

#endif // UTILITIES__DECODE_SEQUENCE_READER_HPP

