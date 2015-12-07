# include "binalgo.hpp"

struct options {
    std::set<std::string> pointer_features;
    graehl::istream_arg input;
    graehl::ostream_arg output;
    graehl::istream_arg table_input;
    graehl::ostream_arg table_output;
    bool nocompose;
    options() 
    : input("-")
    , output("-")
    , table_input("-0")
    , table_output("-0")
    , nocompose(false) {}
};

std::auto_ptr<options> parse_options(int argc, char** argv)
{
    std::string ptrfeats;
    using namespace boost::program_options;
    options_description desc;
    std::auto_ptr<options> opts(new options());
    desc.add_options()
        ( "input,i"
        , graehl::defaulted_value(&opts->input)
        , "xrs rule input"
        )
        ( "output,o"
        , graehl::defaulted_value(&opts->output)
        , "binarized rule output"
        )
        ( "vocab-table-input,v"
        , graehl::defaulted_value(&opts->table_input)
        , "nonterminal vocab input from previous run for memoization"
        )
        ( "vocab-table-output,t"
        , graehl::defaulted_value(&opts->table_output)
        , "nonterminal vocab output to next run for memoization"
        )
        ( "help,h"
        , "produce help message"
        )
        ( "pointer-features"
        , graehl::defaulted_value(&ptrfeats)
        , "comma separated list of features.  "  
          "features are a list of values intended to match " 
          "up with the rhs indices of corresponding rules, "
          "and they will be binarized accordingly"
        )
        ( "--nocompose,n"
        , bool_switch(&opts->nocompose)
        , "do not follow minimal rules"
        )
        ;
    positional_options_description posdesc;
    posdesc.add("input",1);
    posdesc.add("output",2);
    
    variables_map vm;
    basic_command_line_parser<char> cmd(argc,argv);
    store(cmd.options(desc).positional(posdesc).run(),vm);
    notify(vm);
    
    typedef boost::split_iterator<std::string::iterator> split_iterator;
    for ( split_iterator itr(ptrfeats,boost::first_finder(",",boost::is_iequal()))
        ; itr != split_iterator()
        ; ++itr ) {
        opts->pointer_features.insert(boost::copy_range<std::string>(*itr));
    }
    
    if (vm.count("help")) {
        std::cerr << desc << std::endl;
        exit(0);
    }

    return opts;
}

int main(int argc, char** argv) {
    std::ios_base::sync_with_stdio(false);
    std::auto_ptr<options> opts = parse_options(argc,argv);
    virtmap virts;
    std::string line;
    
    std::ostream& out = *(opts->output);
    std::istream& in = *(opts->input);
    
    if (opts->table_input and opts->table_input.get()) {
        virts = memotable_in(*(opts->table_input));
        std::cerr << "memo table loaded: " << virts.size() << " entries\n";
    }

    out << "BRF version 2" << std::endl;
    while (getline(in,line)) {
        bool b;
        subder_ptr sb;
        aligned_rule ar;
        boost::tie(b,sb,ar) = binarize_robust(line,opts->nocompose,opts->pointer_features,virts);
        if (b) {
            assert(xrs_lm_string_from_bin(sb,ar) == xrs_lm_string(ar) or print_lm_str_cmp(sb,ar));
            generate_bin_rules(out,sb,ar,virts);
        }
    }
    
    BOOST_FOREACH(virtmap::value_type const& vr, virts) {
        if (vr.second != "") {
            out << "V: "<< vr.first << " -> " << vr.second << '\n';
        }
    }
    if (opts->table_output and opts->table_output.get()) memotable_out(*(opts->table_output),virts);
    return 0;
}
