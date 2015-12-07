# include <boost/archive/text_iarchive.hpp>
# include <boost/archive/text_oarchive.hpp>
# include <boost/archive/binary_iarchive.hpp>
# include <boost/archive/binary_oarchive.hpp>

# include <boost/filesystem/path.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/serialization/map.hpp>
# include <boost/lambda/lambda.hpp>
# include <boost/lambda/bind.hpp>
# include <boost/program_options.hpp>
# include <vector>
# include <string>
# include <iostream>
# include <filesystem.hpp>
# include <sbmt/token.hpp>

using namespace std;
using namespace xrsdb;
using namespace sbmt;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace ba = boost::archive;

////////////////////////////////////////////////////////////////////////////////

struct options {
    vector<fs::path> inpath;
    fs::path outpath;
};

////////////////////////////////////////////////////////////////////////////////

options parse_options(int argc, char** argv)
{
    options ret;
    
    po::options_description desc;
    desc.add_options()
        ( "help,h"
        , "print this menu"
        )
        ( "input,i"
        , po::value< vector<fs::path> >(&ret.inpath)->composing()
        , "frequency tables to be merged"
        )
        ( "output,o"
        , po::value<fs::path>(&ret.outpath)
        , "filename for merged and indexed frequency table"
        )

        ;
        
    po::positional_options_description posdesc;
    posdesc.add("input",-1);
    
    po::basic_command_line_parser<char> cmd(argc,argv);
    po::variables_map vm;
    po::store(cmd.options(desc).positional(posdesc).run(),vm);
    po::notify(vm);
    
    if (vm.count("help")) {
        cerr << desc << endl;
        exit(1);
    }

    return ret;
}

////////////////////////////////////////////////////////////////////////////////

void add(xrsdb::header& h, fs::path in)
{
    fs::ifstream mapfs(in);
    ba::text_iarchive mapa(mapfs);
    xrsdb::header hh;
    mapa & hh;
    h.add_frequencies(hh);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    using namespace boost::lambda;
    using boost::ref;
    
    std::ios_base::sync_with_stdio(false);
    
    options opts = parse_options(argc,argv);

    xrsdb::header h;
    for_each( opts.inpath.begin()
            , opts.inpath.end()
	    , bind(&add,ref(h),boost::lambda::_1)
            );

    std::ofstream f(opts.outpath.string().c_str());
    ba::text_oarchive ar(f);
    ar & h;
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
