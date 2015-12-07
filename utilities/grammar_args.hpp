#ifndef SBMT__UTILITIES__GRAMMAR_ARGS_HPP
#define SBMT__UTILITIES__GRAMMAR_ARGS_HPP

#include <graehl/shared/fileargs.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <sbmt/grammar/brf_archive_io.hpp>
#include <sbmt/grammar/brf_file_reader.hpp>
#include "logging.hpp"
#include <sbmt/io/log_auto_report.hpp>
#include <sbmt/feature/feature_vector.hpp>
#include <stdexcept>

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(ga_domain,"grammar-args",app_domain);

namespace sbmt {
    


struct grammar_args
{
    bool prepared;

    grammar_args();

    void init();

    void set_defaults();

    void reset();

    grammar_in_mem grammar;
    grammar_in_mem &get_grammar();

    graehl::istream_arg grammar_archive,
                        prior_file;
    graehl::ostream_arg final_weights_to;
    //std::string weights_string;
    bool verbose;
    double weight_tag_prior;
    score_t prior_floor_prob;
    double prior_bonus_count;
    bool keep_texts,keep_align;
    archive_type grammar_format;

    property_constructors<> pc;

    graehl::printable_options_description<std::ostream>
    options();

    io::logging_stream& log() const;

    void update_feature_weights();

    bool have_grammar();

    // must call before prepare/load
    void set_prior( std::istream& prior_stream
                  , score_t prior_floor_prob=1e-7
                  , double prior_bonus_count=0 );


    void validate(bool must_have=true);
    
    void prepare(bool must_have=true);
    
    void grammar_summary();
            
    void load_grammar();
    
    void load_grammar( std::string const& filename
                     , std::string const& weight_str
                     , archive_type a );

    void pop_grammar();
    void push_grammar(std::string const& filename, archive_type a);
    
    fat_weight_vector weights;

    void ensure_implicit_weights(); 
    
    void set_feature_weights(fat_weight_vector const& w);
};

}


#endif
