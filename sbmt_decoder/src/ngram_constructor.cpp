# include <sbmt/edge/ngram_constructor.hpp>
# include <graehl/shared/program_options.hpp>
# include <graehl/shared/maybe_update_bound.hpp>
# include <sbmt/io/log_auto_report.hpp>
# include <sbmt/edge/ngram_info.hpp>

# include <boost/lambda/lambda.hpp>

# define NGRAM_CONSTRUCTOR_MIN_ORDER 1
# ifndef NGRAM_CONSTRUCTOR_MAX_ORDER
# define NGRAM_CONSTRUCTOR_MAX_ORDER 20
# endif
namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME( ngram_cons_domain
                                       , "ngram_constructor"
                                       , root_domain );

bool load_lm::load(unsigned cache_size,unsigned /*req_order*/)
{
    splm.reset();
    SBMT_LOG_TIME_SPACE(ngram_cons_domain,info,"LM loaded: ");
    if (spec == "none") {
        set_null();
        return false;
    }

    SBMT_INFO_MSG( ngram_cons_domain
                 , "loading dynamic language model from spec: %1%"
                 , spec );
    splm=
        ngram_lm_factory::instance().create( spec
                                           , boost::filesystem::initial_path()
                                           , cache_size );

    return (plm=splm.get());
}

////////////////////////////////////////////////////////////////////////////////

static void throw_if(bool cond,std::string const& reason)
{
    if (cond) throw std::runtime_error(reason);
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

ngram_constructor::ngram_constructor(std::string lmstr, std::string ns)
  : lmstr(lmstr)
  , ns(ns)
  , is_prepared(false)
  , higher_ngram_order(0)
  , ngram_shorten(false)
  , greedy_order(0)
  , ngram_order(0)
  , ngram_cache_size(24000)
  , clm_virtual(false)
  , clm_constituent(false)
{
    ngram_opt.set_defaults(); //FIXME: these options are no longer used for anything at all (all config is in lm spec)
    ngram_opt.openclass_lm=false;
    ngram_opt.check_unks=true;
}

////////////////////////////////////////////////////////////////////////////////

bool ngram_constructor::set_option(std::string s,std::string w) 
{ 
    prepare();
    return lm.lm().set_option(s,w);
}

options_map ngram_constructor::get_options()
{
    options_map opts("ngram options");
    using graehl::defaulted_value;

    opts.add_option
        ( ns + "dynamic-lm-ngram"
        , optvar(lm.spec)
        , dynamic_lm_ngram_doc()
        )
        ;
    opts.add_option
        ( ns + "ngram-order"
        , optvar(ngram_order)
        , "in search, separate and score sequences of at least this many words "
          "(2 is bigram, 3 is trigram, etc.).  "
          "if unset, then use order of loaded language model"
        )
        ;
    opts.add_option
        ( ns + "ngram-shorten"
        , optvar(ngram_shorten)
        , "in search, shorten left and right words when equivalent "
          "(non-noisy) LM score would result.  this takes more work per item "
          "but merges more equivalent ones"
        )
        ;
    opts.add_option
        ( ns + "higher-ngram-order"
        , optvar(higher_ngram_order)
        , "WARNING: for correct behavior, train an LM with the specific order "
          "you want (and don't limit it with this option).  Allow loading and "
          "use of higher order LM than ngram-order, scoring with the higher "
          "order when possible"
        )
        ;
    opts.add_option
        ( ns + "greedy-ngram-order"
        , optvar(greedy_order)
        , "0 implies disabled.  otherwise, score with full ngram context but "
          "use reduced order for equality comparison.  final nbests will be "
          "out of order if enabled, and totalcost will disagree with "
          "component costs; component costs to be trusted"
        )
        ;
    opts.add_option
        ( ns + "ngram-cache-size"
        , optvar(ngram_cache_size)
        , "0 implies disabled.  otherwise, for dynamic ngram only, cache this "
          "many entries in case of slow underlying implementation e.g. type "
          "big or multi (for now just the prob, i.e. run with ngram-shorten=0)"
            )
        ;
    opts.add_option
        (ns + "clm-left"
         , optvar(clm[0].spec)
         , "cross-lingual lm: lm specification (as in --dynamic-lm-ngram)"
         "p(f0|e2,e1) for derivation f0 (e1 e2) f3.  feature name is 'clm-left'."
         "digit replacement, ngram order, etc. should be same as in main lm."
         "--dynamic-lm-ngram is required and should include at least the bilingual"
         "training for the clm, or at least all the include all the english "
         " words and if --ngram-shorten, all the same bilingual training data (shortening is done w.r.t the main lm only)."
            )
        ;
    opts.add_option
        (ns + "clm-right"
         , optvar(clm[1].spec)
         , "as with --clm-left, but for right events: p(f3|e1,e2) for derivation "
         "f0 (e1 e2) f3."
            )
        ;
    opts.add_option
        (ns + "clm-lr"
         , optvar(clm_lr_spec)
         , "for all b,A: if --clm-lr=A[b], use 'A[b.left]' and 'A[b.right]' as --clm-left and --clm-right resp."
         "note: this should thus only be used for a SINGLE lm e.g. lw[@][base.3.srilm].  WARNING: regular english ngram lm must then be multi or lw lm, not biglm (TODO: support adding tokens to biglm so clm E vocab is known in ngram info state)"
            );

    opts.add_option
        ( ns + "clm-virtual"
          , optvar(clm_virtual)
          , "score ANY edge that has an lmstring, not just xrs rules"
            );

    opts.add_option
        ( ns + "clm-constituent"
          , optvar(clm_constituent)
          , "TODO: score ANY edge that covers a whole english constituent (all xrs rules, some virtual rules)"
            )
        ;
    return opts;
}

////////////////////////////////////////////////////////////////////////////////

void ngram_constructor::prepare(in_memory_dictionary* dict)
{
    if (not is_prepared) {
        validate();
        load(dict);
    }
    is_prepared = true;
    if (lm.spec != "none" and dict) {
        std::cerr << "initializing ngram-grammar token map " << lmmap->size() << "-";
        BOOST_FOREACH(indexed_token tok, dict->native_words(lmmap->size())) {
            graehl::at_expand(*lmmap,tok.index()) = lm.lm().id(dict->label(tok));
        }
        std::cerr << lmmap->size() << '\n';
    }
 }

void ngram_constructor::init(in_memory_dictionary& dict)
{
    prepare(&dict);
}

void ngram_constructor::validate()
{
    if (lm.spec != "none") {
      if (ngram_order<1) {
        ngram_order=lm.lm().max_order();
      }
      if (higher_ngram_order < ngram_order)
        higher_ngram_order=ngram_order;

      if (ngram_cache_size) //FIXME: we don't cache backoffs which are needed for shortening, so shortening is turned off even though it expands search space per forest size slightly
        ngram_shorten=false;
    }
    if (!clm_lr_spec.empty()) {
        unsigned l=clm_lr_spec.length()-1;
        throw_if(clm_lr_spec[l]!=']',"--clm-lr lm spec should end in ]");
        std::string t=clm_lr_spec.substr(0,l);
        clm[0].spec=t+".left]";
        clm[1].spec=t+".right]";
    }

    throw_if( ngram_order > NGRAM_CONSTRUCTOR_MAX_ORDER ||
              higher_ngram_order > NGRAM_CONSTRUCTOR_MAX_ORDER
            , "--ngram-order greater than "
              BOOST_PP_STRINGIZE(NGRAM_CONSTRUCTOR_MAX_ORDER)
              " unsupported"
            )
            ;
}

////////////////////////////////////////////////////////////////////////////////

void ngram_constructor::load(in_memory_dictionary* dict)
{
    
    if (lm.load(ngram_cache_size,ngram_order)) {
        unsigned maxo=lm.lm().max_order();
        for (unsigned i=0;i<2;++i) {
            if(clm[i].load(ngram_cache_size,ngram_order)) {
                graehl::maybe_increase_max(maxo,(unsigned)clm[i].lm().max_order());
            }
        }
        check_lm_order(maxo);
        lmmap.reset(new std::vector<sbmt::indexed_token::size_type>());
        if (dict) {
            std::cerr << "initializing ngram-grammar token map 0-"; 
            BOOST_FOREACH(indexed_token tok, dict->native_words()) {
                graehl::at_expand(*lmmap,tok.index()) = lm.lm().id(dict->label(tok));
            }
            std::cerr << lmmap->size() << '\n';
        }
    }
}


////////////////////////////////////////////////////////////////////////////////

void ngram_constructor::check_lm_order(unsigned rep_order)
{
    io::logging_stream& log = io::registry_log(ngram_cons_domain);
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

////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
