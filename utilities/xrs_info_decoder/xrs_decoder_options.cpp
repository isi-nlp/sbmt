# include <xrs_decoder_options.hpp>
# include <output_args.hpp>
# include <grammar_args.hpp>
# include <numproc.hpp>

sbmt::grammar_in_mem& xrs_decoder_options::grammar()
{
    return gram_args->grammar;
}

boost::shared_ptr<sbmt::cky_generator> xrs_decoder_options::create_cky_generator()
{
    return filt_args->create_cky_generator();
}

size_t xrs_decoder_options::max_equivalents()
{
    return filt_args->max_equivalents;
}

size_t xrs_decoder_options::maximum_retries()
{
    return filt_args->maximum_retries();
}

graehl::size_t_bytes xrs_decoder_options::reserve_nbest()
{
    return out_args->reserve_nbest;
}

graehl::printable_options_description<std::ostream>
xrs_decoder_options::filter_arg_options()
{
    return filt_args->options();
}

graehl::printable_options_description<std::ostream>
xrs_decoder_options::grammar_arg_options()
{
    return gram_args->options();
}

boost::program_options::options_description
xrs_decoder_options::output_arg_options()
{
    return out_args->options();
}

xrs_decoder_options::xrs_decoder_options()
  : filt_args(new filter_args())
  , out_args(new sbmt::output_args())
  , gram_args(new sbmt::grammar_args())
  , instructions("-")
  , multi_thread(false)
  , num_threads(numproc_online())
  , merge_heap_decode(false)
  , merge_heap_lookahead(1) 
  , exit_on_retry(false)
{}