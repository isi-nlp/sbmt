# include <xrs_decoder.hpp>
# include <xrs_decoder_options.hpp>
# include <graehl/shared/command_line.hpp>

template <class EdgeInterface>
std::auto_ptr<xrs_decoder_options>
xrs_decoder<EdgeInterface>::parse_options(int argc, char** argv)
{
    namespace po = boost::program_options;

    std::string info_names_str;
    std::auto_ptr<xrs_decoder_options> opts(new xrs_decoder_options());
    po::options_description desc("general options");
    desc.add_options()
        ( "help,h"
        , "display options"
        )
        ( "instruction-file,i"
        , graehl::defaulted_value(&opts->instructions)
        , "instructions for loading grammar, setting info-options, decoding, etc"
        )
        ( "multi-thread"
        , po::bool_switch(&opts->multi_thread)
        , "dispatch cky span combining across multiple threads"
        )
        ( "exit-on-retry,r"
        , po::bool_switch(&opts->exit_on_retry)
        , "smart enough to recover from decoder crashes? then just say no to "
          "memory corruption"
        )
        ( "num-threads"
        , graehl::defaulted_value(&opts->num_threads)
        , "number of threads to use with multi-threaded decoding. "
          "defaults to number of logical processors detected on local machine."
        )
        ( "merge-heap-decode"
        , graehl::defaulted_value(&opts->merge_heap_decode)
        , "use experimental merge-heap decoding.  some pruning parameters "
          "are incompatible, and non-lexical unary rules are not applied"
        )
        ( "merge-heap-lookahead"
        , graehl::defaulted_value(&opts->merge_heap_lookahead)
        , "control the lookahead size of cubes used to decide whether to "
          "expand the merge-heap of cubes"
        )
        ;
    desc.add(sbmt::io::logfile_registry::instance().options());
    desc.add(opts->filter_arg_options());
    desc.add(opts->output_arg_options());
    desc.add(opts->grammar_arg_options());
    desc.add(edge_interface.options());
    po::variables_map vm;
    try {
        store(parse_command_line(argc,argv,desc), vm);
        notify(vm);
# if 0
        std::stringstream sstr;
        for (int x = 0; x != argc; ++x) sstr << argv[x] << ' ';
        SBMT_INFO_MSG(xrs_info_decoder,"command-line: %s",sstr.str());
# else
        SBMT_INFO_STREAM(xrs_info_decoder,graehl::get_command_line(argc,argv,"command-line: "));
# endif
    } catch(std::exception const& e) {
        std::cerr << "error parsing command-line options: " << e.what() << std::endl;
        exit(1);
    }

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }

    return opts;
}
