# include <sbmt/edge/dlm_constructor.hpp>
# include <sbmt/edge/head_history_dlm_info.hpp>
# include <stdexcept>
# include <boost/lambda/lambda.hpp>
# include <boost/preprocessor/iteration/local.hpp>

# define DLM_MIN_ORDER 2
# define DLM_MAX_ORDER 20

namespace sbmt {
    
////////////////////////////////////////////////////////////////////////////////

void dlm_constructor::load_dependency_lm()
{
    //SBMT_LOG_TIME_SPACE(dlm_construct_log,info,"dependency LM loaded: ");
    if (not is_set) {
        if (dependency_lm_file.empty()) 
            throw std::runtime_error("dependency-lm not set");
        if (dlm_order < DLM_MIN_ORDER or dlm_order > DLM_MAX_ORDER)
            throw std::runtime_error("dlm-order out of bounds for this build");
        free_dependency_lms();
        dependency_language_model->create(dependency_lm_file);
    } 
    is_set = true;
}

////////////////////////////////////////////////////////////////////////////////

void dlm_constructor::free_dependency_lms() 
{
    dependency_language_model->clear();
}

////////////////////////////////////////////////////////////////////////////////

options_map dlm_constructor::get_options()
{
    options_map opts( "dependency-lm decoding options"
                    , boost::lambda::var(is_set) = false );
        
    opts.add_option
        ( "dependency-lm"
        , optvar(dependency_lm_file)
        , "the dependency language model file"
        )
        ;
    opts.add_option
        ( "dlm-order"
        , optvar(dlm_order)
        , "the order of dependency lm"
        )
        ;
    opts.add_option
        ( "greedy-dlm"
        , optvar(greedy)
        , "if true, dlm info is applied to edges without affecting recombining"
        )
        ;
                      
    return opts;
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned int N>
any_info_factory
make_dlm_factory(boost::shared_ptr<MultiDLM> d, property_map_type p, bool greedy, indexed_token_factory& tf)
{
    if (greedy)
        return head_history_dlm_info_factory<N-1,LWNgramLM,true>(d,p, tf);
    else 
        return head_history_dlm_info_factory<N-1,LWNgramLM,false>(d,p, tf);
}

////////////////////////////////////////////////////////////////////////////////

any_info_factory 
dlm_constructor::construct( weight_vector const& weights
                          , feature_dictionary& dict
                          , property_map_type const& pmap
                          , indexed_token_factory& tf)
{
   tf.native_word("@UNKNOWN@");
    dependency_language_model->set_weights(weights,dict);
    switch (dlm_order) {
        # define BOOST_PP_LOCAL_LIMITS (DLM_MIN_ORDER, DLM_MAX_ORDER)
        # define BOOST_PP_LOCAL_MACRO(N) \
          case N: return make_dlm_factory<N>(dependency_language_model,pmap,greedy, tf); break;
        # include BOOST_PP_LOCAL_ITERATE()
        default:
            throw std::runtime_error("unsupported dlm-order");
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

