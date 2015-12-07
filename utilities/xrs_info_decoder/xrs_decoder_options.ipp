# include <xrs_decoder_options.hpp>
# include <output_args.hpp>

template <class ET>
std::string
dummy_output_results( xrs_decoder_options& opts
                    , std::size_t sentid
                    , sbmt::grammar_in_mem& gram
                    , sbmt::concrete_edge_factory<ET,sbmt::grammar_in_mem>& efact
                    , std::string const& msg
                    , unsigned pass
                    )
{
    std::string onebest = opts.out_args->dummy_output_results(sentid,gram,efact,msg,pass);
    //graehl says: this is already done whenever nbest_output differs from output. why do you want repeats? this actually causes xrs_decoder_wrapper problems!
    //if (opts.out_args->nbest_output) opts.out_args->out() << onebest << std::endl;
    return onebest;
}

template <class ET>
std::string output_results( xrs_decoder_options& opts
                          , sbmt::edge_equivalence<ET> const& top_equiv
                          , sbmt::grammar_in_mem& gram
                          , sbmt::concrete_edge_factory<ET,sbmt::grammar_in_mem>& efact
                          , unsigned sentid
                          , unsigned pass
                          )
{
    std::string onebest = opts.out_args->output_results(top_equiv,gram,efact,sentid,pass);
    //if (opts.out_args->nbest_output) opts.out_args->out() << onebest << std::endl;
    return onebest;
}
