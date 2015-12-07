#include "sblm_info.hpp"
#include <sbmt/grammar/rule_feature_constructors.hpp>
#include <sbmt/grammar/alignment.hpp>
using namespace sbmt;
using namespace sblm;

debug sblm::debug::dbg;
bool sblm::greedy;

class sblm_constructor {
public:

  sblm_constructor() : ngram_cache_size(24000), is_prepared(false), bigrams(false), pcbigrams(false), greedy(false) {
  }

  options_map get_options() {
    options_map opts("sblm options");
    opts.add_option(SBLM_FEATNAME "-ngram",
                    optvar(lm.spec),
                    "ngram LM spec e.g. big[@][pcfg.5gram.biglm] with PCFG child-sequence (parent = final backoff) events."); //TODO: require open-class? nah.
    opts.add_option(SBLM_FEATNAME "-bigram-indicators",
                    optvar(bigrams),
                    "feature named " SBLM_BIFEATPRE "[NP-C][NP-C] counting that sibling bigram; <s> </s> events are also emitted. parent is not mentioned. note: escaped ^ -> ^^, [ -> ^{, ] -> ^}, , -> ^/, and : -> ^: in feature names only");
    opts.add_option(SBLM_FEATNAME "-pc-bigram-indicators",
                    optvar(pcbigrams),
                    "feature named " SBLM_PCFEATPRE "[S][NP-C] counting that [parent][child] bigram; note: escaped ^ -> ^^, [ -> ^{, ] -> ^}, , -> ^/, and : -> ^: in feature names only");
    opts.add_option(SBLM_FEATNAME "-greedy",
                    optvar(greedy),
                    "fully greedy info - consider all states equivalent, so only best scoring one is used - similar to greedy deplm");
    tbc.add_options(opts);
    return opts;
  }

  bool set_option(std::string key, std::string value)
  {
    return false;
  }

  void prepare() {
    if (!is_prepared) {
      if (!lm.load(ngram_cache_size,0))
        throw std::runtime_error("SBLM ngram model "+lm.spec+" failed to load");
      sbmt::io::logging_stream& log = sbmt::io::registry_log(sblm_domain);
      log << sbmt::io::info_msg
          << "loaded SBLM ngram model "<<lm.spec<<" reports order="<<lm.lm().max_order()<<sbmt::io::endmsg;
      log << sbmt::io::info_msg
          << "greedy="<<greedy<<" bigram-indicators="<<bigrams<<" categories="<<tbc<<sbmt::io::endmsg;

    }
    is_prepared = true;
  }

  template<class Grammar>
  any_info_factory construct(Grammar& grammar,
                             const lattice_tree& lattice,
                             property_map_type const& pmap )
  {
    //pmap["sblm_string"] -> rule prop id
    prepare();
    lm.lm().set_weights(grammar.get_weights(),grammar.feature_names(),1.0); // does nothing really, unless you're crazy enough to use a multi lm
    lm.lm().name="sblm";
    lm.lm().set_weight_1(0); // 0 cost for unks. also, ignore any weight to the main LM name (we apply "sblm" weight ourselves)
    return sblm_info_factory(grammar,lattice,pmap,lm,tbc,bigrams,pcbigrams,greedy);
  }
  void init(sbmt::in_memory_dictionary& dict) {}
private:
  unsigned ngram_cache_size;
  load_lm lm;
  bool is_prepared;
  bool bigrams,pcbigrams;
  bool greedy;
  treebank_categories tbc;
  //unsigned ngram_order;
  //ngram_options ngram_opt;
};

struct sblm_init{
  sblm_init()
  {
    register_info_factory_constructor(sblm_featname, sblm_constructor());
  }
  static sblm_init init;
};

sblm_init sblm_init::init;

void dump(sblm_string const& s) {
  s.print(cerr);
}

void dump(sblm_string const& s,lmid i) {
  s.print(cerr,i);
}
