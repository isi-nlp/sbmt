# include <boost/archive/binary_iarchive.hpp>
# include <boost/archive/binary_oarchive.hpp>
# include <boost/archive/text_iarchive.hpp>
# include <boost/archive/text_oarchive.hpp>
# include <boost/archive/xml_iarchive.hpp>
# include <boost/archive/xml_oarchive.hpp>
# include <filesystem.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/program_options.hpp>

# include <iostream>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace ba = boost::archive;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

struct options {
    fs::path rootdb;
    fs::path dictarchive;
};

////////////////////////////////////////////////////////////////////////////////

options parse_options(int argc, char** argv)
{
    options opts;
    po::options_description desc;
    desc.add_options()
        ( "help,h"
        , "print this message"
        )
        ( "db-root,d"
        , po::value(&opts.rootdb)
        , "database root directory"
        )
        ( "freqtable,f"
        , po::value(&opts.dictarchive)
        , "dictionary/frequency table archive, created by merge_tables"
        )
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv,desc),vm);
    po::notify(vm);
    
    if (vm.count("help")) {
        cerr << desc << endl;
        exit(1);
    }
    if (not vm.count("db-root")) {
        cerr << desc << endl;
        cerr << "specify a database root directory" << endl;
        exit(1);
    }
    if (not vm.count("freqtable")) {
        cerr << desc << endl;
        cerr << "specify a frequency table (output from merge_tables)" << endl;
    }
    return opts;
}

////////////////////////////////////////////////////////////////////////////////

void load_text_header(xrsdb::header& h, fs::path const& p)
{
    try {
        std::ifstream hfs(p.string().c_str());
        ba::xml_iarchive ha(hfs);
        ha & boost::serialization::make_nvp("dict",h);
    } catch (std::exception const& e) {
        cerr << "error loading header at " << p << " : " << endl;
        cerr << e.what();
        throw;
        exit(1);
    }
    
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    options opts = parse_options(argc, argv);
    xrsdb::header h;
    clog << "loading header..." << flush;
    load_text_header(h,opts.dictarchive);
    clog << "done" << endl;
    
    clog << "creating skeletal directories..." << flush;
    create_empty_database(h,opts.rootdb);
    clog << "done" << endl;
    
    return 0;
}
