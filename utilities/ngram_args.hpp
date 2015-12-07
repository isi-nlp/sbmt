#ifndef SBMT__UTILITIES__NGRAM_ARGS_HPP
#define SBMT__UTILITIES__NGRAM_ARGS_HPP

#include <sbmt/io/log_auto_report.hpp>
#include <graehl/shared/fileargs.hpp>
#include <sbmt/ngram/LWNgramLM.hpp>
#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <boost/filesystem/operations.hpp>
#include <sbmt/forest/derivation.hpp>
#include <sbmt/dependency_lm/DLM.hpp>
#include <boost/tokenizer.hpp>
//#include <graehl/shared/is_null.hpp>

#include "output_args.hpp"
//FIXME: use ngram_args instead of decoder_app logging output?

#include "logging.hpp"
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(na_domain,"ngram-args",app_domain);

namespace sbmt {




struct ngram_args
{
    static const char* dynamic_lm_ngram_doc()
    {
        return
            "ONLY use --ngram-order and --ngram-shorten with this: lm=lw[@,c][file.lw] or "
            "multi[@][lm1=lw[o][3gram.lw],lm2=lw[c][2gram.lw]]. "
            "@->lm-at-numclass=1\n"
            "c->openclass-lm=0\n"
            "(c or u)->unknown-word-prob*=`[lmname]-unk' feature weight\n"
            "o->openclass-lm\nu->openclass-lm plus unk feature.\n"
            "--weight-lm-ngram replaced by `[lmname]' feature weight.";
    }

    bool prepared;

    bool ngram_shorten;

    unsigned short greedy_order;

    unsigned ngram_order;
    unsigned dlm_order;

    std::string lm_file;
    std::string dyn_lm_spec;
    std::string dependency_lm_file;
    std::string dlm_type;

    typedef boost::shared_ptr<LWNgramLM> LWLM;
    LWLM language_model;
    ngram_ptr dynamic_language_model;
    boost::shared_ptr<MultiDLM> dependency_language_model;

    ngram_options ngram_opt;

    unsigned ngram_cache_size;

    ngram_args()
    {
        init();
    }

    void init()
    {
        prepared=false;
        set_defaults();
    }

    void set_defaults()
    {
        ngram_opt.set_defaults();
        ngram_opt.openclass_lm=false;
        ngram_opt.check_unks=true;

        ngram_order=3;
        dlm_order = 3;
        greedy_order=0;

        ngram_shorten=false;
        reset();
        ngram_cache_size=24000;
    }

    void reset()
    {
        free_lms();
        lm_file.clear();
        dyn_lm_spec.clear();
    }

    bool using_lm() const
    { return ngram_order > 0 && have_lm(); }

    bool using_ngram_lm() const
    { return ngram_order > 0 && have_ngram_lm(); }

    bool have_lm() const
    { return using_lw_lm() || using_dynamic_lm() || using_dependency_lm(); }

    bool have_ngram_lm() const
    { return using_lw_lm() || using_dynamic_lm() ; }

    bool using_lw_lm_arg() const
    { return !lm_file.empty(); }

    bool using_dynamic_lm_arg() const
    { return !dyn_lm_spec.empty(); }

    bool using_dependency_lm_arg() const
    {
        return !dependency_lm_file.empty();
    }

    bool using_lm_arg() const
    { return using_dynamic_lm_arg() or using_lw_lm_arg(); }

    bool using_lw_lm() const
    { return language_model; }

    bool using_dynamic_lm() const
    { return dynamic_language_model; }

    extra_english_output_t extra_output(bool more_details=false) const
    {
        if (using_dynamic_lm())
            return make_extra_english_lm(dynamic_language_model.get(),more_details);
        else if (using_lw_lm())
            return make_extra_english_lm(language_model.get(),more_details);
        return null_extra_english_output();
    }

    bool using_dependency_lm() const
    { return !dependency_lm_file.empty(); }

    void set_grammar(grammar_in_mem &grammar)
    {
        if (using_dynamic_lm())
            set_dynamic_grammar(grammar);
        else if (using_lw_lm())
            set_lw_grammar(grammar);
        else if(using_dependency_lm())
            set_dependency_lm_grammar(grammar);
    }

    void set_dynamic_grammar(grammar_in_mem &grammar)
    {
        dynamic_language_model->set_grammar(grammar);
        dynamic_language_model->sync_vocab_and_grammar();  //FIXME: necessary? or does signal handle
    }

    void set_lw_grammar(grammar_in_mem &grammar)
    {
        language_model->set_grammar(grammar);
        language_model->sync_vocab_and_grammar(); //necessary.  this is the last thing to happen (after grammar loaded)
    }

    void set_dependency_lm_grammar(grammar_in_mem &grammar)
    {
        dependency_language_model->set_grammar(grammar);
        dependency_language_model->sync_vocab_and_grammar(); //necessary.  this is the last thing to happen (after grammar loaded)
    }

    void check_lm_order(unsigned rep_order)
    {
        io::logging_stream& log = io::registry_log(na_domain);
        log << io::info_msg
            << "loaded language model reports order="<<rep_order << io::endmsg;
        if (rep_order < ngram_order) {
            log << io::warning_msg
                << "LM was lower ("<<rep_order<<") order than requested "
                << "--ngram-order ("<<ngram_order<<"); "
                << "using lower order ngram_info since scores will be the same"
                << io::endmsg;
            ngram_order=rep_order;
        } else if (rep_order > ngram_order)
            log << io::warning_msg
                << "LM has higher ("<<rep_order<<") order than requested "
                << "--ngram-order ("<<ngram_order<<"); "
                << "some words will be scored up to the higher order, "
                << "as if you chose --higher-ngram-order=" << rep_order
                << io::endmsg;
    }

  void maybe_prepare(grammar_in_mem &g)
  {
    if (prepared) return;
    prepare(g);
  }

    void prepare(grammar_in_mem &g)
    {
      prepare();
      set_grammar(g);
    }

    void set_weights(weight_vector const& wv, feature_dictionary& dict)
    {
        double w=1;
        set_weights(wv,dict,w);
    }

    void set_weights(weight_vector const& wv, feature_dictionary& dict,double &default_weight)
    {
        if (using_lw_lm())
            language_model->set_weights(wv,dict,default_weight);
        else if (using_dynamic_lm())
            dynamic_language_model->set_weights(wv,dict,default_weight);
        else if (using_dependency_lm())
            dependency_language_model->set_weights(wv,dict);
    }

    void prepare()
    {
        assert(!prepared);
        load_lm();
        prepared=true;
    }

    void free_lms()
    {
        language_model.reset();
        dynamic_language_model.reset();
    }
    void free_dependency_lms() {
        if (dependency_language_model) dependency_language_model->clear();

    }

    static inline void throw_if(bool cond,std::string const& reason)
    { if (cond) throw std::runtime_error(reason); }

    static inline void throw_unless(bool cond,std::string const& reason)
    { throw_if(!cond,reason); }

    void validate()
    {
        using namespace graehl;
        if (using_dynamic_lm())
            throw_if(using_lw_lm(),"--dynamic-lm-ngram may not be used with --lm-ngram");

        if (using_lw_lm())
            throw_unless(boost::filesystem::exists(lm_file),lm_file+" (lm file) not found");

        if (ngram_order<1)
            ngram_order=1;
        if (not using_lm_arg()) {
            ngram_order=0;
        }

        if (using_dependency_lm())
            throw_if(dlm_type != "bilexical" && dlm_type != "trigram","--dlm-type must be bilexical or trigram");

        throw_if( ngram_order > MINI_MAX_NGRAM_ORDER
                , "--ngram-order greater than " BOOST_PP_STRINGIZE(MINI_MAX_NGRAM_ORDER) " unsupported" );


    }

    void load_lm(boost::filesystem::path const& relative_base=boost::filesystem::initial_path())
    {
        if (using_lw_lm_arg() || using_dynamic_lm_arg()) {
            if (using_lw_lm_arg())
                load_lw_lm(lm_file);
            else
                load_dynamic_lm(dyn_lm_spec,relative_base);
        } if(using_dependency_lm_arg()){
            load_dependency_lm(dependency_lm_file);
        }
    }

    void load_lw_lm(std::string const filename) // NOTE: gotcha!  if you pass by const ref, reset clears filename
    {
        SBMT_LOG_TIME_SPACE(na_domain,info,"LM loaded: ");
        reset();
        lm_file=filename;
        io::logging_stream& log = io::registry_log(na_domain);
        SBMT_INFO_MSG_TO( log
                          , "loading language model: %1%"
                          , lm_file
            );
        language_model.reset(new LWNgramLM(ngram_opt));
        language_model->read(lm_file.c_str());
        check_lm_order(language_model->max_order());
    }

    void load_dependency_lm(std::string const filename){
        SBMT_LOG_TIME_SPACE(na_domain,info,"dependency LM loaded: ");
        free_dependency_lms();
        dependency_lm_file=filename;
        dependency_language_model.reset(new MultiDLM());
        dependency_language_model->create(dependency_lm_file);
        // check_lm_order(dependency_language_model->max_order());
    }

    void load_dynamic_lm( std::string spec // NOTE: gotcha!  if you pass by const ref, reset clears filename
                          , boost::filesystem::path const& relative_base=boost::filesystem::initial_path())
    {
        SBMT_LOG_TIME_SPACE(na_domain,info,"LM loaded: ");
        reset();
        if (spec == "none")
            return;
        dyn_lm_spec=spec;
        io::logging_stream& log = io::registry_log(na_domain);
        SBMT_INFO_MSG_TO( log
                          , "loading dynamic language model from spec: %1%"
                          , spec
            );
        dynamic_language_model=ngram_lm_factory::instance().create(spec,relative_base,ngram_cache_size);
        check_lm_order(dynamic_language_model->max_order());
    }


    template <class OD>
    void add_options(OD &od)
    {
        using namespace graehl;
        od.add_options()

            ("dynamic-lm-ngram", defaulted_value(&dyn_lm_spec), dynamic_lm_ngram_doc())
            ("dependency-lm", defaulted_value(&dependency_lm_file),
             "the dependency LM - if empty, then no LM will be used)")
            ("dlm-type", defaulted_value(&dlm_type),
             "the type of dependency LM: bilexical | trigram")
            ("ngram-order", defaulted_value(&ngram_order),
             "in search, separate and score sequences of at least this many words (2 is bigram, 3 is trigram, etc.)")
            ("dlm-order", defaulted_value(&dlm_order),
              "the order of dependency lm.")
            ("ngram-shorten", defaulted_value(&ngram_shorten),
             "in search, shorten left and right words when equivalent (non-noisy) LM score would result.  this takes more work per item but merges more equivalent ones")
            ("lm-ngram", defaulted_value(&lm_file),
             "file with Language Weaver format ngram language model - if empty, then no LM will be used)")
            ("open-class-lm", defaulted_value(&ngram_opt.openclass_lm),
             "use unigram p(<unk>) in your LM (which must be trained accordingly).  disables --unknown-word-penalty")
            ("add-lm-unk", defaulted_value(&ngram_opt.check_unks),
             "even for open-class-lm=1, use lmname-unk feature as (extra) prob for lm-unknown words")
            ("unknown-word-prob", defaulted_value(&ngram_opt.unknown_word_prob),
             "lm probability assessed for each unknown word (when not using --open-class-lm)")
            ("lm-at-numclass", defaulted_value(&ngram_opt.lm_at_numclass),
             "replace all digits (0-9) with '@' - 12.3 becomes @@.@")
            ( "greedy-ngram-order"
            , defaulted_value(&greedy_order)
            , "0 implies disabled.  otherwise, score with full ngram context but use reduced order for "
              "equality comparison.  final nbests will be out of order if enabled, and "
              "totalcost will disagree with component costs; component costs to be trusted" )
            ( "ngram-cache-size"
              , defaulted_value(&ngram_cache_size)
              , "0 implies disabled.  otherwise, for dynamic ngram only, cache this many entries in case of slow underlying implementation e.g. type big or multi (for now just the prob, i.e. run with ngram-shorten=0)")
            ;

    }

    template <class L>
    void check_lms_destructing(L &sp)
    {
        if (sp.use_count()>1) {
            SBMT_WARNING_MSG(
                na_domain
                , "language model held by %1% shared_ptr ... may not be destroyed."
                , sp.use_count()
                );
            sp.reset(); //FIXME: this is redundant, right?
        }
    }

    ~ngram_args()
    {
        check_lms_destructing(language_model);
        check_lms_destructing(dynamic_language_model);
    }

};

}


#endif
