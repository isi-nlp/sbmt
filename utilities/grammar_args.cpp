# include "grammar_args.hpp"
# include <boost/bind.hpp>
# include <iostream>
# include <sbmt/grammar/rule_feature_constructors.hpp>

namespace sbmt {

void weights_from_file(grammar_args& ga, std::string file_descript)
{
    using namespace std;
    ga.weights = fat_weight_vector();
    string line;
    graehl::istream_arg filearg(file_descript);
    std::istream & file = *filearg;
    if (not file)
        throw std::runtime_error("weight file '" + file_descript + "' not opened successfully");
    while(getline(file,line)) {
        fat_weight_vector w;
        read(w,line);
        ga.weights += w;
    }
    ga.update_feature_weights();
}

void weights_from_string(grammar_args& ga, std::string weight_str)
{
    using namespace std;
    read(ga.weights,weight_str);
    ga.update_feature_weights();
}

inline void throw_if(bool cond,std::string const& reason)
{ if (cond) throw std::runtime_error(reason); }

inline void throw_unless(bool cond,std::string const& reason)
{ if (not cond) throw std::runtime_error(reason); }

grammar_args::grammar_args()
{
    init();
//unused: //    io::logging_stream& log_out = io::registry_log(ga_domain);
}

void grammar_args::init()
{
    prepared=false;
    set_defaults();
}

void grammar_args::set_defaults()
{
    weight_tag_prior=1.0;
    final_weights_to.set_none();
    grammar_format=gar;
    grammar_archive.set_none();
    verbose=false;
    keep_texts=false;
    keep_align=false;
    prior_floor_prob=1e-7;
    prior_bonus_count=100;
}

void grammar_args::reset()
{
    prior_file.set_none();
}

grammar_in_mem& grammar_args::get_grammar()
{
    throw_unless(prepared,"grammar_args: load a grammar before using it");
    return grammar;
}


typedef graehl::printable_options_description<std::ostream> OD;

OD grammar_args::options()
{
    namespace po = boost::program_options;
    using std::string;
    using boost::bind;
    using boost::ref;
    using namespace graehl;
    OD od("grammar options");
    od.add_options()
        ("grammar-archive,g", defaulted_value(&grammar_archive),
         "archived translation rules grammar")
        ( "grammar-format,f"
        , defaulted_value(&grammar_format),
          "legal values: brf, archive(default), fat-archive, "
          "text-archive, fat-text-archive"
        )
        ("weight-file,w"
        ,po::value<string>()->notifier(bind(&weights_from_file,ref(*this),_1))
        ,"file with feature and lm exponents (weights), "
         "single or multiple lines of: a:-1,b:2.5")
        ("weight-string"
        ,po::value<string>()->notifier(bind(&weights_from_string,ref(*this),_1))
        ,"same format as weights file; completely overrides any --weight-file")
        ("verbose-non-numeric", defaulted_value(&verbose),
         "complain to STDERR about nonnumeric rule attributes")
        ("final-weights-to", defaulted_value(&final_weights_to),
         "print weights finally used here (same format as weight-string/file)")
        ;
    OD prior_opts("Heuristic (English) per-tag prior probability options");
    prior_opts.add_options()
        ("prior-file", defaulted_value(&prior_file),
         "file with alternating <tag> <count> e.g. NP 123478.  virtual tags ignored")
        ("prior-floor-prob", defaulted_value(&prior_floor_prob),
         "minimum probability for missing or low-count tags")
        ("prior-bonus-count", defaulted_value(&prior_bonus_count),
         "give every tag that appears in prior-file this many extra counts (before normalization)")
        ("weight-prior", defaulted_value(&weight_tag_prior),
         "raise prior prob to this power for rule heuristic")
        ("tag-prior-bonus", defaulted_value(&grammar.tag_prior_bonus),
         "constant prior multiplied into (nonvirtual) tag heuristic.  greater than 1 <=>  favor tags more than virtuals")
        ;
   od.add(prior_opts);
   OD feature_opts("(nonnumeric) feature options");
   feature_opts.add_options()
       ("keep-text-features",defaulted_value(&keep_texts),
        "Keep the unused nonnumeric features in memory")
       ("keep-align",defaulted_value(&keep_align),
        "parse the 'align' attribute for word/variable alignments")
       ;
   od.add(feature_opts);
   return od;
}

io::logging_stream& grammar_args::log() const
{ return io::registry_log(ga_domain); }


void grammar_args::update_feature_weights()
{
    grammar.update_weights(weights,pc);
}

bool grammar_args::have_grammar()
{
    bool g=grammar_archive.valid();
    return g;
}

    // must call before prepare/load
void grammar_args::set_prior( std::istream& prior_stream
                            , score_t prior_floor_prob
                            , double prior_bonus_count )
{
    //std::cerr << "prior set: floor: " << prior_floor_prob << " count: " << prior_bonus_count << std::endl;
    log() << io::debug_msg << "Loading tag prior" << io::endmsg;
    grammar.load_prior(prior_stream,prior_floor_prob,prior_bonus_count,1.0,weight_tag_prior);
}

void grammar_args::validate(bool must_have)
{
    throw_if(must_have&&!have_grammar(),"brf-grammar-file (or grammar-archive) required");
}

void grammar_args::prepare(bool must_have)
{
    assert(!prepared);
    if (!prior_file.is_none())
        set_prior(prior_file.stream(),prior_floor_prob,prior_bonus_count);
    bool have=have_grammar();
    // so folks can use push-grammar without ever calling load,
    // if the feature-weights are on the command-line
    update_feature_weights();
    validate(must_have);
    if (have)
        load_grammar();
    else
        log() << io::warning_msg << "No grammar supplied" << io::endmsg;
    prepared=true;
}

void grammar_args::grammar_summary()
{
    log() << io::info_msg
          << "Grammar has " << grammar.size() << " rules" << io::endmsg;
    throw_if(grammar.size()==0,"Empty grammar");
}

boost::shared_ptr<brf_reader>
create_brf_reader(std::istream& in, archive_type a, bool verbose)
{
    boost::shared_ptr<brf_reader> retval;
    switch (a) {
        case brf:
            retval.reset(new brf_stream_reader(in,verbose)); break;
        case gar:
            retval.reset(new brf_archive_reader(in)); break;
        case fat_gar:
            retval.reset(new brf_fat_to_indexed_archive_reader(in)); break;
        case text_gar:
            retval.reset(new brf_text_archive_reader(in)); break;
        case fat_text_gar:
            retval.reset(new brf_fat_to_indexed_text_archive_reader(in)); break;
    }
    if (not retval) throw std::runtime_error("unsupported archive");
    return retval;
}

void log_grammar(grammar_in_mem const& grammar)
{
    io::logging_stream& log_out = io::registry_log(ga_domain);
    if (io::logging_at_level(log_out,io::lvl_pedantic)) {
        grammar_in_mem::rule_range rr = grammar.all_rules();
        grammar_in_mem::rule_iterator ritr = rr.begin(),
                                      rend = rr.end();
        log_out << io::pedantic_msg;
        for (; ritr != rend; ++ritr)
            continue_log(log_out) << print(*ritr,grammar)
                                  << " heur=" << grammar.rule_score_estimate(*ritr)
                                  << " inside=" << grammar.rule_score(*ritr)
                                  << std::endl;
        continue_log(log_out) << io::endmsg;
    }
}

void grammar_args::load_grammar()
{
    io::logging_stream& log_out = io::registry_log(ga_domain);
    io::log_time_space_report
        report(log_out,io::lvl_terse,"Grammar loaded: ");
    boost::shared_ptr<brf_reader> ar =
            create_brf_reader(grammar_archive.stream(),grammar_format,verbose);
    grammar.load(*ar,weights,pc,keep_texts,keep_align);
    log_grammar(grammar);
}

void grammar_args::load_grammar( std::string const& filename
                               , std::string const& weight_str
                               , archive_type a )
{
    if (weight_str != "") read(weights,weight_str);
    grammar_format = a;
    grammar_archive.set(filename);
    load_grammar();

}

void grammar_args::push_grammar(std::string const& filename, archive_type a)
{
    grammar_format = a;
    io::logging_stream& log_out = io::registry_log(ga_domain);
    io::log_time_space_report report(log_out,io::lvl_terse,"Grammar pushed: ");
    log_out << io::terse_msg << "pushing grammar from '" << filename << "'" << io::endmsg;
    grammar_archive.set(filename); // shouldn't leak (unless gzopen/gzclose leak) - by code inspection 2011-6-28
    boost::shared_ptr<brf_reader> ar =
        create_brf_reader(grammar_archive.stream(),grammar_format,verbose);
    //std::cerr << "pushing now" << std::endl;
    grammar.push(*ar,keep_texts,keep_align);
    log_grammar(grammar);
}

void grammar_args::pop_grammar()
{
    io::logging_stream& log_out = io::registry_log(ga_domain);
    io::log_time_space_report report(log_out,io::lvl_terse,"Grammar popped: ");
    log_out << io::terse_msg << "popping grammar" << io::endmsg;
    grammar.pop();
}

void grammar_args::set_feature_weights(fat_weight_vector const& w)
{
    log() << io::verbose_msg
          << "setting feature weights: " << w << io::endmsg;
    log() << io::pedantic_msg
          << "previous feature weights: " << weights << io::endmsg;
    weights = w;
    update_feature_weights();
}

} // namespace sbmt
