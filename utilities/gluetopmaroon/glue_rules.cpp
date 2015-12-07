# include "extra_rules.hpp"
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/fstream.hpp>
# include <iostream>
# include <boost/program_options.hpp>
# include <boost/iostreams/filtering_stream.hpp>
# include <boost/iostreams/device/file.hpp>
# include <gusc/filesystem_io.hpp>
# include <xrsparse/xrs.hpp>
namespace po = boost::program_options;
namespace io = boost::iostreams;
namespace fs = boost::filesystem;
using namespace std;

struct options {
    std::string ntfile;
    string foreign_start;
    string foreign_end;
    rule_data::rule_id_type id_start;
    bool noalign;
    bool add_headmarker;
    bool mira_features;
    options()
      : ntfile("-")
      , foreign_start("<foreign-sentence>")
      , id_start(300000000000) // thats 300B
      , noalign(false)
      , add_headmarker(false)
      , mira_features(false)
       {}
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
        ( "foreign-start-token,f"
        , po::value(&opts.foreign_start)->default_value(opts.foreign_start)
        , "introductory token"
        )
        ( "foreign-end-token,e"
        , po::value(&opts.foreign_end)->default_value(opts.foreign_end)
        , "optional ending token"
        )
        ( "start-id,s"
        , po::value(&opts.id_start)->default_value(opts.id_start)
        , "starting id to attach to generated rules"
        )
        ( "no-align"
        , po::bool_switch(&opts.noalign)
        , "disable alignment generation"
        )
        ( "add-headmarker"
        , po::bool_switch(&opts.add_headmarker)
        , "add head marker to glue rules."
        )
        ( "mira-features"
        , po::bool_switch(&opts.mira_features)
        , "generate distinct glue indicator features for each non-terminal"
        )
        ;

    po::variables_map vm;
    store(parse_command_line(argc,argv,desc),vm);
    notify(vm);

    if (vm.count("help")) {
        cerr << desc << endl;
        exit(1);
    }
    return opts;
}

int main(int argc, char** argv)
{
    options opts = parse_options(argc,argv);
    io::filtering_istream ntstream;
    if (opts.ntfile == "-") ntstream.push(boost::ref(std::cin));
    else ntstream.push(io::file_source(opts.ntfile));
    set<string> ntset;
    copy( istream_iterator<string>(ntstream)
        , istream_iterator<string>()
        , inserter(ntset,ntset.begin())
        )
        ;

    glue_rules(cout,ntset,opts.id_start,!opts.noalign,opts.foreign_start,
               opts.add_headmarker,opts.mira_features,opts.foreign_end);
}
