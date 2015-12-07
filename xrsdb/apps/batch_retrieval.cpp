# include <iostream>
# include <iterator>

# include <sbmt/hash/hash_set.hpp>
# include <sbmt/token.hpp>
# include <sbmt/sentence.hpp>
# include <sbmt/grammar/tree_tags.hpp>

# include <boost/program_options.hpp>

# include <filesystem.hpp>
# include <db_usage.hpp>

# include <boost/function_output_iterator.hpp>
# include <boost/foreach.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/convenience.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/filesystem/fstream.hpp>
//# include <boost/timer.hpp>
# include <boost/graph/adjacency_list.hpp>
# include <boost/graph/properties.hpp>
# include <boost/graph/graphviz.hpp>

# include <gusc/trie/sentence_lattice.hpp>
# include <gusc/filesystem/create_directories.hpp>
# include <gusc/filesystem/path_from_integer.hpp>
# include <word_cluster.hpp>
# include <word_cluster_db.hpp>
# include <lattice_reader.hpp>
# include <syntax_rule_util.hpp>

# include <map>
# include <vector>
# include <stack>

# include <boost/bind.hpp>
# include <boost/regex.hpp>
# include <boost/range.hpp>

# include <boost/iostreams/filter/line.hpp>
# include <boost/iostreams/filtering_stream.hpp>
# include <boost/iostreams/device/file.hpp>
# include <boost/iostreams/filter/gzip.hpp>

# include <boost/date_time/posix_time/posix_time.hpp>

using namespace xrsdb;
using namespace sbmt;
using namespace boost::filesystem;
using namespace boost;
using namespace gusc;
using namespace boost::posix_time;

namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace bio = boost::iostreams;

using std::make_pair;
using std::cin;
using std::cout;
using std::cerr;
using std::clog;
using std::flush;
using std::endl;
using std::getline;
using std::string;
using std::ostream_iterator;
using std::ostream;
using std::multimap;
using std::map;
using std::pair;
using std::vector;

typedef graph_traits<graph_t>::edge_iterator eitr_t;
typedef graph_traits<graph_t>::edge_descriptor edge_t;

////////////////////////////////////////////////////////////////////////////////

struct options {
    path dbdir;
    path inputfile;
    path einputfile;
    xrsdb::db_usage usage;
    string outprefix;
    string outsuffix;
    size_t modulo;
    size_t startid;
    bool debug;
    bool treebank_etree;
    options() : debug(false), treebank_etree(false) {}
};

////////////////////////////////////////////////////////////////////////////////

options parse_options(int argc, char** argv)
{
    using namespace std;

    po::options_description desc;
    po::variables_map vm;

    options opts;

    desc.add_options()
        ( "dbdir,d"
        , po::value(&opts.dbdir)
        , "xrsdb root directory"
        )
        ( "input,i"
        , po::value(&opts.inputfile)
        , "sentence queries"
        )
        ( "constraint-input,e"
        , po::value(&opts.einputfile)
        , "etree/estring constraints matching foreign sentences"
        )
        ( "treebank-etree"
        , po::bool_switch(&opts.treebank_etree)
        , "use treebank style etrees.  default is lisp-style etrees"
        )
        ( "start-id,n"
        , po::value(&opts.startid)->default_value(1)
        , "id offset (for force_etree/force_sentence only)"
        )
        ( "modulo,m"
        , po::value(&opts.modulo)->default_value(0)
        , "if non-zero, insert modulo-path directory sequence before filename"
        )
        ( "usage,u"
        , po::value(&opts.usage)->default_value(xrsdb::db_usage::decode)
        )
        ( "output-prefix,p"
        , po::value(&opts.outprefix)->default_value("./")
        , "directory/file prefix for output rule files.  all output files will "
          "have an appended index.  example:\n"
          "    \t-m 2 -p foo/bar. causes foo/bar.0 foo/bar.1 to be created"
        )
        ( "output-suffix,s"
        , po::value(&opts.outsuffix)
        , "suffix to attach to output rule files.  if suffix ends in \".gz\", "
          "the output files will be gzipped"
        )
        ( "help,h"
        , "print this menu"
        )
        ( "debug,D"
        , po::bool_switch(&opts.debug)
        )
        ;

    po::store(po::parse_command_line(argc,argv,desc),vm);
    po::notify(vm);

    if ( vm.count("help") or
         not vm.count("dbdir") or
         not vm.count("input")
       ) {
        cerr << desc << endl;
        exit(1);
    }

    return opts;
}

////////////////////////////////////////////////////////////////////////////////

template <class S>
struct avoid_crosses_f {
    avoid_crosses_f( graph_t const& g, S& s )
      : g(&g)
      , s(&s) {}
    void operator()(word_cluster::value_type<graph_t> const& v)
    {
        typename S::iterator pos = s->find(v);
        if (pos == s->end()) {
            sbmt::span_t span((*g)[v.left],(*g)[v.right]);

            BOOST_FOREACH(sbmt::span_t const& block, get_property(*g).brackets)
            {
                if (crossing(block,span)) return;
            }
            s->insert(v);
        }
    }
private:
    graph_t const* g;
    S* s;
};

////////////////////////////////////////////////////////////////////////////////

template <class S> avoid_crosses_f<S>
avoid_crosses(graph_t const& g, S& s) { return avoid_crosses_f<S>(g,s); }

////////////////////////////////////////////////////////////////////////////////

void attach_etree_to_query( vector<indexed_token> const& etree
                          , graph_t& lat
                          , indexed_token_factory& dict
                          , indexed_token skip
                          , size_t n
                          , bool force_index_tokens )
{
    graph_traits<graph_t>::vertex_descriptor from, to;
    typedef vector<graph_traits<graph_t>::vertex_descriptor> vertex_vec_t;
    vertex_vec_t tagstack;
    vertex_vec_t rhs(boost::begin(vertices(lat)), boost::end(vertices(lat)));
    from = add_vertex(n++,lat);

    vector<indexed_token>::const_iterator ei = etree.begin(), ee = etree.end();
    for (; ei != ee; ++ei) {
        to = add_vertex(n++,lat);
        add_edge(from,to,*ei,lat);
        if (not is_lexical(*ei)) {
            if (dict.label(*ei)[0] == '/') {
                if (force_index_tokens)
                    add_edge(tagstack.back(),from,skip,lat);
                else
                    add_edge(tagstack.back(),to,skip,lat);
                vertex_vec_t::iterator vi = rhs.begin(), ve = rhs.end();
                for (; vi != ve; ++vi) {
                    add_edge(to,*vi,skip,lat);
                }
                tagstack.pop_back();
            } else {
                if (force_index_tokens)
                    tagstack.push_back(to);
                else
                    tagstack.push_back(from);
            }
        }
        from = to;
    }
}

typedef std::vector<sbmt::indexed_token>
        (*tree_tags_f)(std::string const&, sbmt::indexed_token_factory&);

vector<graph_t> force_etree_inputs( fs::path etreefile
                                  , fs::path ffile
                                  , indexed_token_factory& dict
                                  , indexed_token skip
                                  , bool force_index_tokens
                                  , tree_tags_f treetags
                                  , size_t graphid )
{
    vector<graph_t> v;

    fs::ifstream estr(etreefile), fstr(ffile);
    string eline, fline;
    while (getline(estr,eline) and getline(fstr,fline)) {
        indexed_sentence fsent = foreign_sentence(fline,dict);
        graph_t lat;
        skip_lattice_from_sentence( lat
                                  , get(boost::edge_bundle,lat)
                                  , fsent.begin()
                                  , fsent.end()
                                  , wildcard_array(dict) );
        size_t len = boost::size(std::make_pair(fsent.begin(),fsent.end())) + 1;

        vector<indexed_token> etree = treetags(eline,dict);

        attach_etree_to_query(etree,lat,dict,skip,len,force_index_tokens);
        get_property(lat).id = graphid;
        v.push_back(lat);
        ++graphid;
    }

    return v;
}

////////////////////////////////////////////////////////////////////////////////

vector<graph_t> inputs( fs::path inputfile
                      , indexed_token_factory& dict
                      , indexed_token skip )
{
    boost::shared_ptr<std::istream> pin;
    std::istream* ppin;
    if (inputfile == "-") ppin = &std::cin;
    else {
        pin.reset(new fs::ifstream(inputfile));
        ppin = pin.get();
    }
    string line;
    vector<graph_t> v;

    lattice_reader( *ppin
                  , bind( &vector<graph_t>::push_back
                        , ref(v)
                        , bind(&skip_lattice,_1,boost::ref(dict))
                        )
                  , dict
                  )
                  ;

    return v;
}

vector<graph_t> raw_inputs( fs::path inputfile
                          , indexed_token_factory& dict
                          )
{
    boost::shared_ptr<std::istream> pin;
    std::istream* ppin;
    if (inputfile == "-") ppin = &std::cin;
    else {
        pin.reset(new fs::ifstream(inputfile));
        ppin = pin.get();
    }
    string line;
    vector<graph_t> v;

    lattice_reader( *ppin
                  , bind( &vector<graph_t>::push_back
                        , ref(v)
                        , _1
                        )
                  , dict
                  )
                  ;

    return v;
}

////////////////////////////////////////////////////////////////////////////////

typedef multimap<indexed_token,pair<size_t,edge_t> >
        work_map_t;

work_map_t work(vector<graph_t> const& v, indexed_token skip)
{
    work_map_t m;
    typedef vector<graph_t> vec_t;
    size_t x = 0;
    for (; x != v.size(); ++x) {
        eitr_t ei, ee;
        tie(ei,ee) = edges(v[x]);
        for (; ei != ee; ++ei) {
            indexed_token wd = v[x][*ei]; //get(edge_name_t(),v[x],*ei);
            if (wd != skip) {
                m.insert(make_pair(wd,make_pair(x,*ei)));
            }
        }
    }
    return m;
}

////////////////////////////////////////////////////////////////////////////////

work_map_t::const_iterator
advance( work_map_t::const_iterator i
       , work_map_t::const_iterator e
       , fs::path const& dbdir 
       , header const& h )
{
    while (true) {
        if (i == e) return i;
        indexed_token tok = i->first;
        if (cluster_exists(dbdir,h,tok)) return i;
        else {
            while (i != e and i->first == tok) ++i;
        }
    }
    return i;
}

fs::path
output_path( string prefix
           , string suffix
           , size_t x
           , size_t modulo = 0
           , size_t maximum = 0 )
{
    string f;
    if (not modulo) f = prefix + lexical_cast<string>(x) + suffix;
    else f = prefix + gusc::path_from_id(x,modulo,maximum).string() + suffix;
    fs::path pth = f;
    return pth;
}

struct hadoop_filter : bio::line_filter {
    std::string prefix;
    hadoop_filter(int lineno) : prefix(boost::lexical_cast<std::string>(lineno) + "\t") {}
    std::string do_filter(std::string const& line)
    {
        return prefix + line;
    }
};


vector< shared_ptr<ostream> >
outputs(string prefix, string suffix, vector<graph_t> const& g, size_t modulo=0)
{
    typedef bio::filtering_stream<bio::output> stream_t;
    vector< shared_ptr<ostream> > v;
    regex gz(".+\\.gz$");
    vector<graph_t>::const_iterator gi = g.begin(), ge = g.end();
    size_t m = 0;
    for (; gi != ge; ++gi) m = std::max(m,get_property(*gi).id);
    gi = g.begin();
    vector<fs::path> paths;
    for (;gi != ge; ++gi) {
        size_t x = get_property(*gi).id;
        paths.push_back(output_path(prefix,suffix,x,modulo,m));
    }
    
    if (prefix + suffix == "hadoop") {
        vector<graph_t>::const_iterator gi = g.begin(), ge = g.end();
        for (; gi != ge; ++gi) {
            shared_ptr<stream_t> pfs(new stream_t());
            pfs->push(hadoop_filter(get_property(*gi).id),0);
            pfs->push(std::cout,0);
            v.push_back(pfs);
        }
        return v;
    }

    gusc::create_directories(paths.begin(), paths.end());

    vector<fs::path>::iterator pi = paths.begin(), pe = paths.end();
    for (; pi != pe; ++pi) {
        fs::path pth = *pi;
        shared_ptr<stream_t> pfs(new stream_t());
        if (regex_match(pth.filename().native(),gz)) {
            pfs->push(bio::gzip_compressor());
        }
        bio::file_sink sink(pth.string());
        if (not sink.is_open()) {
            std::string msg = "could not open file " +
                              pth.string() +
                              " for writing";
            throw std::ios_base::failure(msg);
        }
        pfs->push(sink);
        v.push_back(pfs);
    }
    return v;
}

template std::vector<sbmt::indexed_token>
sbmt::lisp_tree_tags<sbmt::indexed_token_factory>
(std::string const&, sbmt::indexed_token_factory&);

template std::vector<sbmt::indexed_token>
sbmt::tree_tags<sbmt::indexed_token_factory>
(std::string const&, sbmt::indexed_token_factory&);

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    time_duration dump_t,load_t,search_t;
    stlext::hash_set< word_cluster::value_type<graph_t>
                    , hash<word_cluster::value_type<graph_t> >
                    > sentrules;
    options opts = parse_options(argc,argv);

    bool debug = opts.debug;

    path dbdir(opts.dbdir);

    header h;
    if (debug) clog << "loading database dictionary..." << flush;
    load_header(h, dbdir);
    if (debug) clog << "finished." << endl;

    indexed_token skip = h.wildcard();

    tree_tags_f treetags;
    if (opts.treebank_etree)
        treetags = &sbmt::tree_tags<sbmt::indexed_token_factory>;
    else
        treetags = &sbmt::lisp_tree_tags<sbmt::indexed_token_factory>;

    boost::function<vector<graph_t> (void)> get_graphs;
    if (opts.usage == xrsdb::db_usage::decode) {
        get_graphs = boost::bind( inputs
                                , opts.inputfile
                                , boost::ref(h.dict)
                                , skip );
    } else if (opts.usage == xrsdb::db_usage::raw_sig) {
        get_graphs = boost::bind( raw_inputs
                                , opts.inputfile
                                , boost::ref(h.dict) 
                                );
    } else if (opts.usage == xrsdb::db_usage::force_etree) {
        get_graphs = boost::bind( force_etree_inputs
                                , opts.einputfile
                                , opts.inputfile
                                , boost::ref(h.dict)
                                , skip
                                , true
                                , treetags
                                , opts.startid );
    } else if (opts.usage == xrsdb::db_usage::force_stateful_etree) {
        get_graphs = boost::bind( force_etree_inputs
                                , opts.einputfile
                                , opts.inputfile
                                , boost::ref(h.dict)
                                , skip
                                , false
                                , treetags
                                , opts.startid );
    } else {
        std::stringstream sstr;
        sstr << "unsupported operation: "<< opts.usage;
        throw std::runtime_error(sstr.str());
    }

    vector<graph_t> graphvec = get_graphs();

    if (debug) clog << token_label(h.dict);
    vector<graph_t>::iterator vi = graphvec.begin(), ve = graphvec.end();

    if (debug) for (;vi != ve; ++vi) {
        write_graphviz( clog
                      , *vi
                      , default_writer()
                      , make_label_writer(get(edge_bundle,*vi))
                      , default_writer()
                      , get(vertex_bundle,*vi)
                      )
                      ;
    }


    const work_map_t workmap = work(graphvec,skip);
    vector< shared_ptr<ostream> >
        outstreams = outputs(opts.outprefix,opts.outsuffix,graphvec,opts.modulo);

    if (debug) clog << "begin work on "<< graphvec.size() << " input lattices" << endl;

    work_map_t::const_iterator mitr = workmap.begin(), mend = workmap.end();
    mitr = advance(mitr,mend,dbdir,h);
    word_cluster wc;
    indexed_token curr = skip;
    size_t sentidx = graphvec.size();
    for (; mitr != mend; mitr = advance(++mitr,mend,dbdir,h) ) {
        if (sentidx != mitr->second.first or curr != mitr->first) {
            if (sentidx < graphvec.size()) {
                ptime bt = microsec_clock::local_time();
                if (debug) clog << "write rules " << flush;
                ostream& out = *(outstreams[sentidx]);
                copy( sentrules.begin()
                    , sentrules.end()
                    , ostream_iterator<word_cluster::value_type<graph_t> >(out,"")
                    )
                    ;
                sentrules.clear();
                ptime et = microsec_clock::local_time();
                if (debug) clog << "[" << et - bt << "]" << endl;
                dump_t += (et - bt);
            }
        }
        if (curr != mitr->first) {
            curr = mitr->first;
            ptime bt = microsec_clock::local_time();
            if (debug) {
                clog << "loading word_cluster(" << print(curr,h.dict)
                     << ") from "<< dbdir / structure_from_token(curr).get<0>() << " "
                     << flush;
            }
            wc = load_word_cluster(dbdir,h,curr);
            ptime et = microsec_clock::local_time();
            if (debug) { clog << "[" << et - bt <<"]" << endl; }
            load_t += (et - bt);
        }
        sentidx = mitr->second.first;
        edge_t e = mitr->second.second;


        graph_t& g = graphvec[sentidx];
        size_t id = get_property(g).id;
        if (debug) {
            clog << "retrieving rules from word_cluster("
                 << print(curr,h.dict)<<") "
                 << "against sentence " << id << " " << flush;
        }

        { // scope for controlling lifetime of bt.
        ptime bt = microsec_clock::local_time();
        wc.search( g
		         , e
                 , make_function_output_iterator(avoid_crosses(g,sentrules))
                 );
        ptime et = microsec_clock::local_time();
        if (debug) { clog << "[" << et - bt << "]" << endl; }
        search_t += (et - bt);
        } // end scope
    }
    if (not sentrules.empty()) {
        ptime bt = microsec_clock::local_time();
        if (debug) clog << "write rules " << flush;
        ostream& out = *(outstreams[sentidx]);
        copy( sentrules.begin()
            , sentrules.end()
            , ostream_iterator<word_cluster::value_type<graph_t> >(out,"")
            )
            ;
        sentrules.clear();
        ptime et = microsec_clock::local_time();
        if (debug) { clog << "[" << et - bt << "]" << endl; }
        dump_t += (et - bt);
    }

    clog << "time loading xrsdb word-clusters:   " << load_t << "\n";
    clog << "time writing rule matches to file:   " << dump_t << "\n";
    clog << "time searching word-clusters for matches: " << search_t << "\n";

    return 0;
}
