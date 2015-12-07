# include <boost/archive/text_iarchive.hpp>
# include <boost/program_options.hpp>
# include <boost/regex.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/filesystem/convenience.hpp>

# include <filesystem.hpp>
# include <string>
# include <vector>

namespace fs = boost::filesystem;
namespace ar = boost::archive;
namespace po = boost::program_options;

namespace sbmt {
    
std::istream& operator >> (std::istream& in, indexed_token& tok)
{
    uint32_t idx;
    in >> idx;
    tok = indexed_token(idx);
    return in;
}

}

size_t hashline(std::string const& line)
{
    using namespace boost;
    static regex splt("\\s*(\\S+)\\s+\\|\\|\\|\\s+(.*)");
    smatch what;
    regex_match(line,what,splt);
    fs::path p = xrsdb::path_from_token(boost::lexical_cast<sbmt::indexed_token>(what.str(1))).branch_path();    
    return boost::lexical_cast<size_t>(p.leaf());
}

struct options {
    fs::path dictpath;
    size_t modulo;
    std::string outprefix;
};

options parse_options(int argc, char** argv)
{
    using namespace std;
    
    po::options_description desc;
    po::variables_map vm;
    
    options opts;
    
    desc.add_options()
        ( "freqtable,f"
        , po::value(&opts.dictpath)
        , "merged frequency table / dictionary created by xrsdb_mergetables"
        )
        ( "modulo,m"
        , po::value(&opts.modulo)->default_value(1)
        , "number of files to split input"
        )
        ( "prefix,p"
        , po::value(&opts.outprefix)->default_value("./")
        , "directory/file prefix for output files.  all output files will have "
          "an appended index.  example:\n"
          "    \t-m 2 -p foo/bar. causes foo/bar.0 foo/bar.1 to be created"
        )
        ( "help,h"
        , "print this menu"
        )
        ;
        
    po::store(po::parse_command_line(argc,argv,desc),vm);
    po::notify(vm);
    
    if (vm.count("help")) {
        cerr << desc << endl;
        exit(1);
    }
    if (not vm.count("freqtable")) {
        cerr << desc << endl;
        cerr << "must supply a frequency table" << endl;
        exit(1);
    }
    
    return opts;
}

int main(int argc, char** argv)
{
    using namespace std;
    using namespace boost;
    
    ios_base::sync_with_stdio(false);
    
    options opts = parse_options(argc,argv);
    
    fs::ifstream ifs(opts.dictpath);
    ar::text_iarchive iar(ifs);
    xrsdb::header h;
    iar >> h;
    
    vector< shared_ptr<fs::ofstream> > outfs;
    
    for (size_t x = 0; x != opts.modulo; ++x) {
        fs::path p = opts.outprefix + lexical_cast<string>(x);
        create_directories(p.branch_path());
        shared_ptr<fs::ofstream> f(new fs::ofstream(p));
        outfs.push_back(f);
    }
    
    string line;
    while (getline(cin,line)) {
        size_t id = hashline(line) % opts.modulo;
        *(outfs[id]) << line << '\n';
    }
}
