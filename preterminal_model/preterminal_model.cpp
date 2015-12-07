# include <sbmt/edge/any_info.hpp>
# include <sbmt/edge/ngram_info.hpp>
# include <sbmt/ngram/dynamic_ngram_lm.hpp>
# include <sbmt/grammar/rule_feature_constructors.hpp>
# include <boost/lambda/lambda.hpp>
# include <boost/preprocessor.hpp>
# include <boost/shared_ptr.hpp>
# include <sbmt/search/block_lattice_tree.hpp>

using namespace sbmt;
using namespace boost;
using namespace std;

# define PT_NGRAM_MIN 1
# define PT_NGRAM_MAX 20

////////////////////////////////////////////////////////////////////////////////
///
///  preterminal-ngram info type does not need its own info-type and
///  info-factory.  instead, it just re-uses ngram_info_factory, but supplies
///  it with access to the rule feature pt_lm_string, instead of lm_string.
///  pt_lm_string records the preterminals of a rule instead of the english words
///
////////////////////////////////////////////////////////////////////////////////
class preterminal_ngram_constructor {
public:
    preterminal_ngram_constructor()
      : constructor_ready(false)
      , ngram_shorten(false)
      , ngram_cache_size(24000) {}

    ////////////////////////////////////////////////////////////////////////////

    any_info_factory construct( grammar_in_mem& grammar
                              , lattice_tree const& lattice
                              , property_map_type pmap )
    {
        prepare(grammar);
        unsigned int ngram_order = language_model->max_order();
        switch (ngram_order) {
            // this block just makes the right order ngram for each order ranging
            // from PT_NGRAM_MIN through PT_NGRAM_MAX
            # define BOOST_PP_LOCAL_LIMITS (PT_NGRAM_MIN, PT_NGRAM_MAX)
            # define BOOST_PP_LOCAL_MACRO(N) \
                case N: return ngram_info_factory<N,dynamic_ngram_lm>(grammar.get_weights(),grammar.feature_names(),lattice,grammar.dict(),language_model,pmap["pt_lm_string"],ngram_shorten);break;
            # include BOOST_PP_LOCAL_ITERATE()
            default:
                throw std::runtime_error("unsupported pre-terminal ngram order");
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    void prepare( grammar_in_mem& grammar )
    {
        if (not constructor_ready) {
            language_model =
                ngram_lm_factory::instance().create( ngram_spec
                                                   , boost::filesystem::initial_path()
                                                   , ngram_cache_size );
            language_model->set_weights(grammar.get_weights(),grammar.feature_names(),0.0);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    void init(sbmt::in_memory_dictionary& dict) {}
    options_map get_options()
    {
        // meaning of `lambda::var(constructor_ready) = false`
        // whenever an option is set, constructor_ready will be set to false
        // so we can re-read the options before setting up ngram_info_factory
        options_map opts( "pre-terminal language model options"
                        , lambda::var(constructor_ready) = false );
        opts.add_option
          ( "preterminal-ngram-spec"
          , optvar(ngram_spec)
          , "same as 'dynamic-lm-ngram' option for language model"
          )
          ;
        opts.add_option
          ( "preterminal-ngram-cache-size"
          , optvar(ngram_cache_size)
          , "same as 'ngram-cache' option for language model"
          )
          ;
        opts.add_option
          ( "preterminal-ngram-shorten"
          , optvar(ngram_shorten)
          , "same as 'ngram-shorten' option for language model"
          )
          ;
        return opts;
    }

    // options set via options_map
    bool set_option(std::string,std::string) { return false; }

    ////////////////////////////////////////////////////////////////////////////

private:
    bool constructor_ready;
    string ngram_spec;
    bool ngram_shorten;
    size_t ngram_cache_size;
    ngram_ptr language_model;
};

////////////////////////////////////////////////////////////////////////////////


struct preterminal_init {
	preterminal_init()
	{
    	register_info_factory_constructor( "preterminal-ngram"
    	                                 , preterminal_ngram_constructor()
    	                                 );

    	register_rule_property_constructor( "preterminal-ngram"
    	                                  , "pt_lm_string"
    	                                  , lm_string_constructor()
    	                                  );
	}
};

static preterminal_init pinit;
