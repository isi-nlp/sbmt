# include <iostream>
# include <iterator>

# include <sbmt/token.hpp>
# include <sbmt/sentence.hpp>

# include <filesystem.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/convenience.hpp>
# include <boost/date_time/posix_time/posix_time.hpp>
# include <boost/program_options.hpp>
# include <boost/graph/adjacency_list.hpp>
# include <boost/graph/properties.hpp>
# include <boost/bind.hpp>

# include <gusc/trie/sentence_lattice.hpp>
# include <word_cluster.hpp>
# include <word_cluster_db.hpp>
# include <lattice_reader.hpp>

# include <sbmt/hash/hash_set.hpp>
# include <map>

using namespace xrsdb;
using namespace sbmt;
using namespace boost::filesystem;
using namespace boost::posix_time;
using namespace boost;
using namespace gusc;

using std::cin;
using std::cout;
using std::cerr;
using std::clog;
using std::flush;
using std::endl;
using std::getline;
using std::string;
using std::ostream_iterator;
using std::make_pair;

typedef graph_traits<graph_t>::edge_descriptor edge_t;
typedef std::multimap<indexed_token,edge_t> work_t;
typedef word_cluster::value_type<graph_t> rules_t;
typedef stlext::hash_set< rules_t, hash<rules_t> > rules_set_t;

////////////////////////////////////////////////////////////////////////////////

time_duration retrieve( word_cluster const& wc
                      , graph_t const& g
                      , edge_t e
                      , rules_set_t& rules
                      )
{

    ptime t1 = microsec_clock::local_time();
    indexed_token tok = g[e];
    size_t id = get_property(g).id;
    size_t f = g[source(e,g)];
    size_t t = g[target(e,g)];
    clog << "retrieving rules from word_cluster(" << tok << ") "
         << "against sentence "<< id
         << ", edge [" << f << "," << t << "] " << flush;
    wc.search( g
             , e
             , inserter(rules,rules.end())
             )
             ;
    ptime t2 = microsec_clock::local_time();
    clog << "[" << t2 - t1 << "]" << endl;
    return t2 - t1;
}

////////////////////////////////////////////////////////////////////////////////

void retrieve_rules( word_cluster_db& db
                   , indexed_token_factory& dict
                   , graph_t const& gg
                   , tuple<time_duration&,time_duration&,time_duration&> times )
{
    size_t id = get_property(gg).id;
    work_t work;
    graph_t g = skip_lattice(gg,dict);
    graph_traits<graph_t>::edge_iterator ei,ee;
    tie(ei,ee) = edges(g);

    for (;ei != ee; ++ei) {
        if (g[*ei].type() != virtual_tag_token)
            work.insert(make_pair(g[*ei],*ei));
    }

    work_t::iterator wi = work.begin(), we = work.end();
    rules_set_t rules;

    cout << "--BEGIN [" << id << "]--\n" << flush;
    indexed_token curr = dict.virtual_tag("0");
    word_cluster wc;
    for (; wi != we; ++wi) {
        if (db.exists(wi->first)) {
            if (curr != wi->first) {
                clog << "write rules " << flush;
                ptime t1 = microsec_clock::local_time();
                copy( rules.begin()
                    , rules.end()
                    , ostream_iterator<rules_t>(cout,"")
                    )
                    ;
                rules.clear();
                ptime t2 = microsec_clock::local_time();
                clog << "[" << t2 - t1 << "]" << endl;
                curr = wi->first;
                clog << "loading word_cluster(" << curr << ") "
                     << "from " << structure_from_token(curr).get<1>() << " ";
                wc = db.get(curr);
                ptime t3 = microsec_clock::local_time();
                clog << "[" << t3 - t2 << "]" << endl;
                times.get<1>() += (t2 - t1);
                times.get<0>() += (t3 - t2);
            }
            times.get<2>() += retrieve(wc,g,wi->second,rules);
        }
    }
    copy(rules.begin(),rules.end(),ostream_iterator<rules_t>(cout,""));
    cout << "--END [" << id << "]--\n" << flush;
}

////////////////////////////////////////////////////////////////////////////////

struct options {
    path dbdir;
    size_t cache_sz;
};

////////////////////////////////////////////////////////////////////////////////

options parse_options(int argc, char** argv)
{
    using namespace std;

    namespace po = boost::program_options;

    po::options_description desc;
    po::variables_map vm;

    options opts;

    desc.add_options()
        ( "dbdir,d"
        , po::value(&opts.dbdir)
        , "xrsdb root directory"
        )
        ( "cache-size,c"
        , po::value(&opts.cache_sz)->default_value(512)
        , "cache size (in MB) to store frequently used word_clusters"
        )
        ( "help,h"
        , "print this menu"
        )
        ;
    po::positional_options_description posdesc;
    posdesc.add("dbdir",1); // first unnamed can be

    po::basic_command_line_parser<char> cmd(argc,argv);
    po::store(cmd.options(desc).positional(posdesc).run(),vm);
    po::notify(vm);

    if ( vm.count("help") or
         not vm.count("dbdir")
       ) {
        cerr << desc << endl;
        exit(1);
    }

    return opts;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    options opts = parse_options(argc,argv);

    path dbdir(opts.dbdir);

    time_duration loadtime, writetime, searchtime;

    header h;
    clog << "loading database dictionary..." << flush;
    load_header(h, dbdir);
    clog << "finished." << endl;

    clog << token_label(h.dict);

    word_cluster_db db(dbdir,opts.cache_sz);

    lattice_reader( cin
                  , bind( &retrieve_rules
                        , ref(db)
                        , ref(h.dict)
                        , _1
                        , tie(loadtime,writetime,searchtime)
                        )
                  , h.dict
                  )
                  ;

    clog << "loading:   " << loadtime << endl;
    clog << "writing:   " << writetime << endl;
    clog << "searching: " << searchtime << endl;
    return 0;
}
