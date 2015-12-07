# if ! defined(BUILTIN_INFOS__DLM_INFO_CONSTRUCTOR_HPP)
# define       BUILTIN_INFOS__DLM_INFO_CONSTRUCTOR_HPP

# include <sbmt/edge/any_info.hpp>
# include <sbmt/ngram/LWNgramLM.hpp>
# include <sbmt/dependency_lm/DLM.hpp>
# include <stdexcept>
# include <sbmt/search/block_lattice_tree.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

class dlm_constructor
{
public:
    dlm_constructor()
      : is_set(false)
      , dependency_language_model(new MultiDLM())
      , dlm_order(0)
      , greedy(false) {}

    options_map get_options();

    // options set via options_map
    bool set_option(std::string,std::string) { return false; }
    void init(in_memory_dictionary& dict) {}
    template <class Grammar>
    any_info_factory
    construct( Grammar& gram
             , lattice_tree const& lat
             , property_map_type const& pmap );

private:
    bool is_set;
    boost::shared_ptr<MultiDLM> dependency_language_model;
    std::string dependency_lm_file;
    unsigned int dlm_order;
    bool greedy;
    void load_dependency_lm();
    void free_dependency_lms();
    bool using_dependency_lm() const;
    any_info_factory
    construct(weight_vector const&, feature_dictionary&, property_map_type const&, indexed_token_factory&);
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
any_info_factory
dlm_constructor::construct( Grammar& gram
                          , lattice_tree const& lat
                          , property_map_type const& pmap )
{
    load_dependency_lm();
    return construct(gram.get_weights(),gram.feature_names(),pmap,gram.dict());
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     BUILTIN_INFOS__DLM_INFO_CONSTRUCTOR_HPP
