# include "extra_rules.hpp"
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/fstream.hpp>
# include <iostream>
# include <boost/program_options.hpp>
# include <gusc/filesystem_io.hpp>
# include <xrsparse/xrs.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using namespace std;

struct options {
    fs::path ntfile;
    rule_data::rule_id_type id_start;
    bool add_headmarker;
    string exclude;
    options()
      : id_start(400000000000) // 400B
      , add_headmarker(false)
      , exclude("<foreign-sentence>"){}
};

options parse_options(int argc, char** argv)
{
    options opts;
    po::options_description desc;
    desc.add_options()
        ( "help,h"
        , "print this menu"
        )
        ( "nt-file,i"
        , po::value(&opts.ntfile)
        , "file containing nonterminals to be used as glue rule roots"
        )
        ( "start-id,s"
        , po::value(&opts.id_start)->default_value(opts.id_start)
        , "starting id to attach to generated rules"
        )
        ( "add-headmarker"
        , po::bool_switch(&opts.add_headmarker)
        , "if set, add the head markers to the maroon rules."
        )
        ;

    po::variables_map vm;
    store(parse_command_line(argc,argv,desc),vm);
    notify(vm);

    if (vm.count("help")) {
        cerr << desc << endl;
        exit(1);
    }
    if (!vm.count("nt-file")) {
        cerr << desc << endl;
        cerr << "must provide nonterminal file" << endl;
        exit(1);
    }
    return opts;
}

int main(int argc, char** argv)
{
    options opts = parse_options(argc,argv);
    fs::ifstream ntin(opts.ntfile);
    set<string> ntset;
    copy( istream_iterator<string>(ntin)
        , istream_iterator<string>()
        , inserter(ntset,ntset.begin())
        )
        ;

    set<string> wdset;
    copy( istream_iterator<string>(cin)
        , istream_iterator<string>()
        , inserter(wdset,wdset.begin())
        )
        ;

    maroon_rules(cout,ntset,wdset,opts.id_start, opts.add_headmarker);
}
