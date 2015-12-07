# include <xrs_decoder.hpp>
# include <xrs_decoder_options.hpp>
# include <decode_sequence_reader.hpp>
# include <grammar_args.hpp>

template <class EdgeInterface>
int xrs_decoder<EdgeInterface>::run(int argc, char** argv)
{
    try {
        using namespace sbmt;
        using boost::bind;
        using boost::ref;
        boost::scoped_ptr<xrs_decoder_options> opts(parse_options(argc,argv));

        std::stringstream sstr;
        std::copy(argv, argv + argc, std::ostream_iterator<std::string>(sstr," "));

        decode_sequence_reader reader;

        //reader.set_use_info_callback(bind(&set_info_names,ref(*opts),_1));

        reader.set_load_grammar_callback(
            bind(&grammar_args::load_grammar, ref(*(opts->gram_args)),_1,_2,_3)
        );

        reader.set_push_grammar_callback(
            bind(&grammar_args::push_grammar, ref(*(opts->gram_args)),_1,_2)
        );

        reader.set_weights_callback(
            bind(&grammar_args::set_feature_weights, ref(*(opts->gram_args)),_1)
        );

        reader.set_pop_grammar_callback(
            bind(&grammar_args::pop_grammar, ref(*(opts->gram_args)))
        );

        reader.set_decode_callback(bind(&decoder_::decode_lattice,this,ref(*opts),_1,_2));

       // reader.set_decode_callback(bind(&decoder_::decode_sentence,this,ref(*opts),_1,_2));

        opts->gram_args->pc = edge_interface.grammar_properties();
        opts->gram_args->prepare(!opts->instructions.valid());

        if (not opts->instructions.name.size() || opts->instructions.name[0] == '-') {
            reader.read(opts->instructions.stream());
        } else {
            reader.read(opts->instructions.name);
    	}
    } catch (std::exception const& error) {
        std::cerr << "\nERROR: caught exception: " << error.what() << ".  exiting.";
        return 1;
    } catch (...) {
        std::cerr << "\nERROR: caught unknown exception.  exiting.";
        return 1;
    }
    return 0;
}
