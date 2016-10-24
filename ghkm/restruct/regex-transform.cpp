#include "Transformer.h"
#include <boost/program_options.hpp>

//using namespace boost;
using namespace std;
using namespace boost::program_options;

bool help;
string configFile;
string parsesFile;


int main(int argc, char **argv)
{
    options_description general("General options");
    general.add_options() ("help,h", bool_switch(&help), "show usage/documentation")
        ("config,c",  
	      value<string>(&configFile)->default_value("default.config"),
	      "config file")
        ("parses,p",  
	      value<string>(&parsesFile)->default_value(""),
	      "the parses.") ;

    variables_map vm;
    store(parse_command_line(argc, argv, general), vm);
    notify(vm);

    if(help || argc == 1){
	cerr<<general<<endl;
	exit(0);
    }

  Transformer t(configFile);
  t.process(parsesFile);
  return 0;
}


