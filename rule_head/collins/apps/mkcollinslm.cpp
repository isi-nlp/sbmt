# include <collins/lm.hpp>
# include <iostream>
# include <boost/program_options.hpp>
# include <string>

int main(int argc, char** argv) 
{
    std::ios::sync_with_stdio(false);
    namespace po = boost::program_options;
    po::options_description desc;
    collins::output_type type(collins::output_type::ruleroottagword);
    std::string filename;
    bool help(false);
    desc.add_options()
          ( "help,h"
          , po::bool_switch(&help)->default_value(false)
          , "display this message"
          )
          ( "filename,f"
          , po::value(&filename)
          )
          ( "pass,p"
          , po::value(&type)->default_value(type)
          , "which model to map"
          );
    po::variables_map vm;
    store(parse_command_line(argc,argv,desc), vm);
    notify(vm);
    if (help) {
        std::cerr << desc << std::endl;
        return 0;
    }
    
    collins::make_section(std::cin,filename,type);
    return 0; 
}


