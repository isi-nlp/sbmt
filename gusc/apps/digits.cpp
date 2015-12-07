# include <boost/program_options.hpp>
# include <vector>
# include <gusc/mod_sequence.hpp>
# include <iostream>
# include <iomanip>
# include <cmath>

using namespace gusc;
using namespace boost;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

size_t integral_pow(size_t x, size_t y) {
    if (y == 0) {
        return 1;
    } else {
        size_t p = integral_pow(x, y/2);
        return (y % 2) ? p * p * x : p * p;
    }
}

size_t width(size_t x, size_t base = 10)
{
    if (x < base) return 1;
    else return 1 + width(x/base,base);
}

////////////////////////////////////////////////////////////////////////////////

struct options {
    size_t modulo;
    size_t digits;
    size_t arg;
    std::string sep;
    bool equal_width;
    
    options()
    : modulo(10)
    , digits(0)
    , arg(204)
    , sep("/")
    , equal_width(false) {}
};

////////////////////////////////////////////////////////////////////////////////

options parse_options(int argc, char** argv)
{
    using namespace boost::program_options;
    
    options_description flags, unnamed, all;
    options opts;

    flags.add_options()
        ( "separator,s"
        , value(&opts.sep)->default_value("/")
        , "use string to separate numbers"
        )
        ( "equal-width,w"
        , bool_switch(&opts.equal_width)
        , "equalize width by padding digits with leading zeroes"
        )
        ( "help,h"
        , "display this help and exit"
        )
        ;
    string arg;
    unnamed.add_options()
        ( "ARG"
        , value(&arg)->default_value("0")
        )
        ( "MODULO"
        , value(&opts.modulo)->default_value(10)
        )
        ;

    all.add(flags).add(unnamed);
    
    positional_options_description posdesc;
    posdesc.add("ARG",1);
    posdesc.add("MODULO",1);
    
    basic_command_line_parser<char> cmd(argc,argv);
    variables_map vm;
    parsed_options p = cmd.options(all).positional(posdesc).run();
    
    store(p,vm); 
    notify(vm);

    if (vm.count("help")) {
        cerr << "Usage: digits [OPTION]... ARG" << 
        endl << "   or: digits [OPTION]... ARG MOD" << 
        endl << "Print digit sequence of ARG modulo MOD" <<
        endl;
        cerr << flags << endl;
        exit(0);
    }
    
    opts.digits = arg.size();
    opts.arg = lexical_cast<size_t>(arg);
    
    return opts;
}

int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    
    options opts = parse_options(argc,argv);
    
    size_t max = integral_pow(10,opts.digits) - 1;
    vector<size_t> digits;
    mod_sequence(opts.arg, opts.modulo, back_inserter(digits), max);
    
    vector<size_t>::iterator i = digits.begin(), e = digits.end();
    
    if (i != e) {
        if (opts.equal_width) {
            cout << setw(width(opts.modulo - 1)) 
                 << setfill('0');
        }
        cout << *i;
        ++i;
    }
    for (; i != e; ++i) {
        cout << opts.sep;
        if (opts.equal_width) {
            cout << setw(width(opts.modulo - 1)) 
                 << setfill('0');
        }
        cout << *i;
    }
    cout << '\n';
    return 0;
}
