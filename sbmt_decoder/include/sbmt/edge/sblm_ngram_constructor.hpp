# if ! defined(BUILTIN_INFOS__SBLM_NGRAM_CONSTRUCTOR_HPP)
# define       BUILTIN_INFOS__SBLM_NGRAM_CONSTRUCTOR_HPP

# include <sbmt/edge/any_info.hpp>
# include <sbmt/edge/sblm_bigram.hpp>
# include <boost/program_options.hpp>
# include <boost/shared_ptr.hpp>
# include <string>
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/token.hpp>
# include <sbmt/edge/ngram_constructor.hpp>

namespace sbmt {


class sblm_ngram_constructor {
public:
    options_map get_options();

    // options set via options_map
    bool set_option(std::string,std::string) { return false; }

    template <class Grammar>
    any_info_factory
    construct( Grammar& grammar
             , lattice_tree const& lattice
               , property_map_type const& pmap
        ); // actually called
    ngram_constructor();

    /*
    template <unsigned int Order, class Grammar>
    ngram_info_factory<Order,dynamic_ngram_lm>
    construct( Grammar& grammar
               , property_map_type const& pmap ); // unused in xrs_info_decoder?  maybe mini_decoder uses?
    */
private:
    bool is_prepared;
    unsigned higher_ngram_order;
    bool ngram_shorten;
  //unsigned short greedy_order;
    unsigned ngram_order;
    load_lm lm;

    ngram_options ngram_opt;
    unsigned ngram_cache_size;

    bool using_lw_lm() const;
    bool using_dynamic_lm() const;

    template <class Grammar>
    void bind_grammar_and_weights(Grammar&);

    any_info_factory
    construct(lattice_tree const& lattice,weight_vector const&, feature_dictionary&,property_map_type,indexed_token_factory& dict); // iterates over below:

    template <unsigned int Order>
    sblm_ngram_info_factory<Order>
    construct(lattice_tree const& lattice, weight_vector const&, feature_dictionary&,property_map_type, indexed_token_factory& dict);

    void prepare();
    void validate();
    void load();

    void check_lm_order(unsigned got);

};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
any_info_factory
sblm_ngram_constructor::construct(
                             Grammar& gram
                            , lattice_tree const& lattice
                            , property_map_type const& pmap)
{
    prepare();
    bind_grammar_and_weights(gram);
    return construct(lattice,gram.get_weights(),gram.feature_names(),pmap,gram.dict());
}


////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
void
sblm_ngram_constructor::bind_grammar_and_weights( Grammar& grammar )
{
    lm.lm().set_weights(grammar.get_weights(),grammar.feature_names(),1.0);
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned Order>
sblm_ngram_info_factory<Order>
sblm_ngram_constructor::construct(lattice_tree const& lattice
                             , weight_vector const& wv
                             , feature_dictionary& fn
                             , property_map_type pmap
                             , indexed_token_factory &dict)
{

    return sblm_ngram_info_factory<Order>
        (wv
         ,fn
         ,lattice
         ,dict
         , lm.splm
         , pmap["lm_string"]
         , ngram_shorten
         , clm[0].splm
         , clm[1].splm
         , clm_virtual
            );

}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     BUILTIN_INFOS__SBLM_NGRAM_CONSTRUCTOR_HPP
