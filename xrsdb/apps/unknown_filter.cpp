# include <filesystem.hpp>
# include <word_cluster.hpp>
# include <word_cluster_db.hpp>
# include <lattice_reader.hpp>

# include <boost/program_options.hpp>
# include <boost/filesystem/convenience.hpp>
# include <boost/filesystem/operations.hpp>

# include <iostream>
# include <vector>

# include <sbmt/token.hpp>
# include <sbmt/sentence.hpp>

using namespace xrsdb;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

struct options {
    fs::path dbdir;
};

////////////////////////////////////////////////////////////////////////////////

options parse_options(int argc, char** argv)
{
    options opts;
    po::options_description desc;
    po::variables_map vm;
    
    desc.add_options()
        ( "dbdir,d"
        , po::value(&opts.dbdir)
        , "xrsdb root directory"
        )
        ( "help,h"
        , "generate this message"
        );
    
    po::positional_options_description posdesc;
    posdesc.add("dbdir",-1);
    
    po::basic_command_line_parser<char> cmd(argc,argv);
    po::store(cmd.options(desc).positional(posdesc).run(),vm);
    po::notify(vm);
    
    if (vm.count("help")) {
        std::cerr << "xrsdb_unknown_filter <dbdir>\n";
        std::cerr << "reads words from stdin and ouputs to stdio words\n";
        std::cerr << "considered unknown to <dbdir>\n";
        std::cerr << desc << std::endl;
        exit(1);
    }

    return opts;
}

////////////////////////////////////////////////////////////////////////////////

bool unknown(fs::path const& dbdir, std::string const& word, header& h)
{
    using boost::graph_traits;
    using sbmt::graph_t;
    
    sbmt::indexed_token w = h.dict.foreign_word(word);
    if (not h.knownset.empty()) return h.knownset.find(w) == h.knownset.end();
    if (not cluster_exists(dbdir,h,w)) return true;
    
    word_cluster wc = load_word_cluster(dbdir,h,w);
    
    graph_t lat;
    graph_traits<graph_t>::vertex_descriptor 
        v1 = add_vertex(lat), v2 = add_vertex(lat);
    graph_traits<graph_t>::edge_descriptor 
        e = add_edge(v1,v2,w,lat).first;
    
    std::vector< word_cluster::value_type<graph_t> > sentrules;
    wc.search( lat
             , e
             , back_inserter(sentrules)
             )
             ;
    return sentrules.empty();
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    using namespace std;
    using namespace sbmt;
    
    std::ios_base::sync_with_stdio(false);
    
    options opts = parse_options(argc, argv);
    header h;
    load_header(h,opts.dbdir);
    map<string,bool> seen;
    string somewords;
    while (getline(cin,somewords)) {
        stringstream somewordstrm(somewords);
        string word;
	bool first = true;
        while (somewordstrm >> word) {
            map<string,bool>::iterator pos = seen.find(word);
            if (pos == seen.end()) {
                pos = seen.insert(make_pair(word,unknown(opts.dbdir,word,h))).first;
            }
            if (pos->second) {
	        if (not first) cout << ' ';
                cout << word;
                first = false;
            }
        }
        cout << endl;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
