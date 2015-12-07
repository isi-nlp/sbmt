# include <sbmt/edge/any_info.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <sbmt/logging.hpp>
# include <graehl/shared/hash_functions.hpp>
# include <sbmt/feature/array.hpp>

using namespace sbmt;

# define NOISE_FEATNAME noise
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(NOISE_FEATNAME,#NOISE_FEATNAME,sbmt::root_domain);
char const* noise_featname=#NOISE_FEATNAME;

typedef boost::uint64_t seedt;

struct noise_factory : info_factory_new_component_scores<noise_factory> {
  //typedef seedt info_type; // we want greedy? will that handicap mira?
  typedef void info_type;
  typedef tuple<info_type,score_t,score_t> result_type;
  unsigned N;
  seedt seed;
  typedef boost::shared_ptr<arrayfeats> namer;
  template <class G>
  noise_factory(unsigned N,seedt seed,G const& g) : N(N),seed(seed),namersp(new namer_t(noise_featname,g.feature_names()),N) {
    namersp->set_weights(g.get_weights());
  }
//TODO: compute_info
};

struct noise_factory_constructor {
  unsigned N;
  seedt seed;
  noise_factory_constructor() : N(1),seed(graehl::golden_ratio_fraction_64) {  }
  options_map get_options()
  {
    options_map opts("noise info options");
    opts.add_option( "noise-dimensions"
                     , optvar(N)
                     , "pseudo-random (reproducible) unbiased features noise1 ... noiseN to test MIRA overfitting vs. regularization"
      );
    opts.add_option( "noise-seed"
                     , optvar(seed)
                     , "pseudo-random seed used at leaves"
      )
      ;
    return opts;
  }
  template<class Grammar>
  any_info_factory construct(Grammar& grammar,
                             const lattice_tree& lattice,
                             property_map_type const& pmap )
  {
    return noise_factory(grammar);
  }

};

struct noise_init{
  noise_init()
  {
    register_info_factory_constructor(noise_featname, noise_factory_constructor());
  }
  static noise_init init;
};

noise_init noise_init::init;
