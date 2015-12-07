# include <xrsparse/xrs.hpp>
# include <iostream>
# include <string>
# include <cstdlib>
# include <boost/program_options.hpp>
# include <graehl/shared/fileargs.hpp>
# include <graehl/shared/fileargs.cpp>
# include <graehl/shared/program_options.hpp>

struct options {
    graehl::ostream_arg output;
    graehl::istream_arg input;
    options() : output("-"), input("-") {}
};

std::auto_ptr<options> parse_options(int argc, char** argv)
{
    using namespace boost::program_options;
    options_description desc;
    std::auto_ptr<options> opts(new options());
    desc.add_options()
        ( "input,i"
        , value(&opts->input)
        , "xrs rule input"
        )
        ( "output,o"
        , value(&opts->output)
        , "mapped rule output"
        )
        ( "help,h"
        , "produce help message"
        )
        ;
    positional_options_description posdesc;
    posdesc.add("input",1);
    posdesc.add("output",2);
    
    variables_map vm;
    basic_command_line_parser<char> cmd(argc,argv);
    store(cmd.options(desc).positional(posdesc).run(),vm);
    notify(vm);
    
    if (vm.count("help")) {
        std::cerr << desc << std::endl;
        exit(0);
    }

    return opts;
}

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    std::auto_ptr<options> opts = parse_options(argc,argv);
    std::string line;
    while (getline(*(opts->input),line)) {
        rule_data r = parse_xrs(line);
        *(opts->output) << r.rhs.size() << ' ' << line << '\n';
    }
    return 0;
}