# if ! defined(BUILTIN_INFOS__NGRAM_CONSTRUCTOR_HPP)
# define       BUILTIN_INFOS__NGRAM_CONSTRUCTOR_HPP

# include <sbmt/edge/any_info.hpp>
# include <sbmt/edge/ngram_info.hpp>
# include <boost/program_options.hpp>
# include <boost/shared_ptr.hpp>
# include <sbmt/ngram/LWNgramLM.hpp>
# include <sbmt/ngram/dynamic_ngram_lm.hpp>
# include <string>
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/token.hpp>

# define NGRAM_CONSTRUCTOR_MIN_ORDER 1
# ifndef NGRAM_CONSTRUCTOR_MAX_ORDER
# define NGRAM_CONSTRUCTOR_MAX_ORDER 12
# endif

namespace sbmt {

class ngram_constructor {
public:
    options_map get_options();
    
    void init(in_memory_dictionary& dict);

    // options set via options_map
    bool set_option(std::string,std::string);

    template <class Grammar>
    any_type_info_factory<Grammar>
    construct( Grammar& grammar
             , lattice_tree const& lattice
             , property_map_type const& pmap
             ); // actually called
    ngram_constructor(std::string lmstr = "lm_string", std::string ns = "");

    /*
    template <unsigned int Order, class Grammar>
    ngram_info_factory<Order,dynamic_ngram_lm>
    construct( Grammar& grammar
               , property_map_type const& pmap ); // unused in xrs_info_decoder?  maybe mini_decoder uses?
    */
private:
    std::string lmstr;
    std::string ns;
    bool is_prepared;
    unsigned higher_ngram_order;
    bool ngram_shorten;
    unsigned short greedy_order;
    unsigned ngram_order;
    load_lm lm;
    load_lm clm[2]; // clm left/right
    std::string clm_lr_spec; //
        // add to dictionary all the tokens
    boost::shared_ptr< std::vector<indexed_token::size_type> > lmmap;

    ngram_options ngram_opt;
    unsigned ngram_cache_size;
    bool clm_always;
    bool clm_virtual;
    bool clm_constituent;

    bool using_lw_lm() const;
    bool using_dynamic_lm() const;

    template <class Grammar>
    void bind_grammar_and_weights(Grammar&);

    template <class Grammar>
    any_type_info_factory<Grammar>
    construct( lattice_tree const& lattice
             , weight_vector const&
             , feature_dictionary&
             , property_map_type
             , indexed_token_factory& dict ); // iterates over below:

    template <unsigned int Order>
    ngram_info_factory<Order,dynamic_ngram_lm>
    construct(lattice_tree const& lattice, weight_vector const&, feature_dictionary&,property_map_type, indexed_token_factory& dict);

    void prepare(in_memory_dictionary* dict = 0);
    void validate();
    void load(in_memory_dictionary* dict = 0);

    void check_lm_order(unsigned got);

};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
any_type_info_factory<Grammar>
ngram_constructor::construct( Grammar& gram
                            , lattice_tree const& lattice
                            , property_map_type const& pmap )
{
    prepare(&gram.dict());
    bind_grammar_and_weights(gram);
    return construct<Grammar>(lattice,gram.get_weights(),gram.feature_names(),pmap,gram.dict());
}

# include <boost/preprocessor/iteration/local.hpp>

template <class Grammar>
any_type_info_factory<Grammar>
ngram_constructor::construct( lattice_tree const& lattice
                            , weight_vector const& wv
                            , feature_dictionary& fn
                            , property_map_type pmap
                            , indexed_token_factory& dict )
{
    switch (ngram_order) {
        # define BOOST_PP_LOCAL_LIMITS (NGRAM_CONSTRUCTOR_MIN_ORDER, NGRAM_CONSTRUCTOR_MAX_ORDER)
        # define BOOST_PP_LOCAL_MACRO(N) \
            case N: return construct<N>(lattice,wv,fn,pmap,dict); break;
        # include BOOST_PP_LOCAL_ITERATE()
        default:
            throw std::runtime_error("unsupported ngram order");
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
void
ngram_constructor::bind_grammar_and_weights( Grammar& grammar )
{
    lm.lm().set_weights(grammar.get_weights(),grammar.feature_names(),1.0);
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned Order>
ngram_info_factory<Order,dynamic_ngram_lm>
ngram_constructor::construct(lattice_tree const& lattice
                             , weight_vector const& wv
                             , feature_dictionary& fn
                             , property_map_type pmap
                             , indexed_token_factory &dict)
{

    return ngram_info_factory<Order,dynamic_ngram_lm>
            ( wv
            , fn
            , lattice
            , dict
            , lmmap
            , lm.splm
            , pmap[lmstr]
            , ngram_shorten
            , clm[0].splm
            , clm[1].splm
            , clm_virtual
            );

}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     BUILTIN_INFOS__NGRAM_CONSTRUCTOR_HPP
