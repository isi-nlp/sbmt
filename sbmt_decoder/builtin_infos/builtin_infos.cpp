# include <graehl/shared/intrusive_refcount.hpp>
# include <sbmt/edge/null_info.hpp>
# include <sbmt/edge/dlm_constructor.hpp>
# include <sbmt/edge/ngram_constructor.hpp>
//# include <sbmt/edge/sblm_ngram_constructor.hpp>
# include <sbmt/grammar/rule_feature_constructors.hpp>
# include <sbmt/edge/any_info.hpp>
# include <iostream>

struct builtin_init {
    builtin_init()
    : init(0)
    {
        using namespace sbmt;
        register_info_factory_constructor("null", null_factory_constructor<null_info>());

//        register_info_factory_constructor("sblm_ngram", sblm_ngram_constructor());
//        register_rule_property_constructor("sblm","sblm_string",sblm_string_constructor());

        register_info_factory_constructor("dlm", dlm_constructor());
        register_rule_property_constructor("dlm","dep_lm_string",lm_string_constructor());

        register_info_factory_constructor("ngram",ngram_constructor());
        register_rule_property_constructor("ngram","lm_string",lm_string_constructor());

        init = 1;
    }
    int init;
};

builtin_init binit;









