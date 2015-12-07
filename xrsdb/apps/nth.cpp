# include <iostream>
# include <string>
# include <sstream>
# include <boost/program_options.hpp>

namespace po = boost::program_options;

struct options {
    size_t offset;
    size_t modulo;
};

options parse_options(int argc, char** argv)
{
    po::options_description desc;
    options opts;
    bool help = false;
    desc.add_options()
        ( "help,h"
        , po::bool_switch(&help)
        , "display this menu"
        )
        ( "offset,k"
        , po::value(&opts.offset)->default_value(0)
        )
        ( "modulo,m"
        , po::value(&opts.modulo)->default_value(1)
        )
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv,desc),vm);
    po::notify(vm);
    if (help) {
        std::cerr << desc << std::endl;
        exit(1);
    }
    if (opts.offset >= opts.modulo) {
        std::cerr << "offset="<< opts.offset << " can not be as large as "
                  << "modulo="<< opts.modulo << std::endl;
        exit(1);
    }
    return opts;
}

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    options opts = parse_options(argc,argv);
    std::string line;
    size_t x = 0;
    while (getline(std::cin,line)) {
        ++x;
        if (x % opts.modulo == opts.offset) {
            std::cout << line << '\n';
        }
	//	if (x % 10000 == 0) std::cout << std::flush;
    }
    exit(0);
}
