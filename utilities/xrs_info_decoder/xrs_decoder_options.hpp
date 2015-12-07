# ifndef   XRS_INFO_DECODER_OPTIONS
# define   XRS_INFO_DECODER_OPTIONS
# include <boost/shared_ptr.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <sbmt/edge/edge_equivalence.hpp>
# include <decoder_filters.hpp>

namespace sbmt {
    class output_args;
    class grammar_args;
    class filt_args;
}
struct xrs_decoder_options {
    boost::shared_ptr<filter_args>        filt_args;
    boost::shared_ptr<sbmt::output_args>  out_args;
    boost::shared_ptr<sbmt::grammar_args> gram_args;
    
    graehl::istream_arg instructions;
    bool multi_thread;
    size_t num_threads;
    bool merge_heap_decode;
    size_t merge_heap_lookahead;
    bool exit_on_retry;

    xrs_decoder_options();
    sbmt::grammar_in_mem& grammar();
    boost::shared_ptr<sbmt::cky_generator> create_cky_generator();
    size_t max_equivalents();
    graehl::size_t_bytes reserve_nbest();
    size_t maximum_retries();
    graehl::printable_options_description<std::ostream>
    filter_arg_options();
    graehl::printable_options_description<std::ostream>
    grammar_arg_options();
    boost::program_options::options_description
    output_arg_options();
    
};

template <class ET>
std::string
dummy_output_results( xrs_decoder_options& opts
                    , std::size_t sentid
                    , sbmt::grammar_in_mem& gram
                    , sbmt::concrete_edge_factory<ET,sbmt::grammar_in_mem>& efact
                    , std::string const& msg
                    , unsigned pass=0
                    );

template <class ET>
std::string output_results( xrs_decoder_options& opts
                          , sbmt::edge_equivalence<ET> const& top_equiv
                          , sbmt::grammar_in_mem& gram
                          , sbmt::concrete_edge_factory<ET,sbmt::grammar_in_mem>& efact
                          , unsigned sentid
                          , unsigned pass=0
                          );


# endif // XRS_INFO_DECODER_OPTIONS
