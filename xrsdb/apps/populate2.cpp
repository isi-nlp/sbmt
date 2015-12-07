# include <boost/archive/binary_oarchive.hpp>
# include <boost/iostreams/device/file_descriptor.hpp>
# include <boost/iostreams/copy.hpp>
# include <boost/iostreams/filter/zlib.hpp>
# include <boost/iostreams/device/back_inserter.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/date_time/posix_time/posix_time.hpp>
# include <boost/cstdint.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/regex.hpp>
# include <boost/interprocess/file_mapping.hpp>
# include <boost/interprocess/shared_memory_object.hpp>
# include <boost/interprocess/mapped_region.hpp>
# include <boost/interprocess/offset_ptr.hpp>
# include <boost/interprocess/managed_mapped_file.hpp>
# include <boost/interprocess/allocators/allocator.hpp>
# include <boost/interprocess/containers/vector.hpp>
# include <boost/interprocess/mem_algo/simple_seq_fit.hpp>
# include <boost/interprocess/indexes/flat_map_index.hpp>
# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/logging.hpp>
# include <word_cluster.hpp>
# include <filesystem.hpp>
# include <collapsed_signature_iterator.hpp>
# include <syntax_rule_util.hpp>
# include <search/info_registry.hpp>
# include <search/sort.hpp>

# include <iostream>
# include <stdexcept>
# include <iterator>
# include <cstdlib>


namespace sbmt {

std::istream& operator >> (std::istream& in, indexed_token& tok)
{
    boost::uint32_t idx;
    in >> idx;
    tok = indexed_token(idx);
    return in;
}

}

using namespace boost;
using namespace boost::posix_time;
using namespace boost::filesystem;
using namespace boost::serialization;
using namespace boost::archive;
using namespace sbmt;
using namespace xrsdb::search;
using namespace xrsdb;

using std::vector;
using std::string;
using std::swap;
using std::stringstream;
using std::istream_iterator;
using std::ostream_iterator;
using std::exception;
using std::cin;
using std::cerr;
using std::clog;
using std::endl;

typedef 
    tuple< path
         , indexed_token
         , vector< tuple<short,indexed_token> >
         , vector<indexed_token>
         , rule_data 
         >  procline_t;

////////////////////////////////////////////////////////////////////////////////


procline_t procline(string const& s)
{
    // a line is
    // path \t rarest \t sig \t rhs \t rule
    static regex splt("([^\\t]*)\\t([^\\t]*)\\t([^\\t]*)\\t([^\\t]*)\\t(.*)\\s*");
    smatch what;
    regex_match(s,what,splt);

    procline_t
        tpl( lexical_cast<path>(what.str(1))
           , lexical_cast<indexed_token>(what.str(2))
           , vector< tuple<short,indexed_token> >()
           , vector< indexed_token >()
           , parse_xrs(what.str(5))
           );
    
    if (tpl.get<4>().lhs.size() > 255 or tpl.get<4>().rhs.size() > 255) {
        throw std::runtime_error("rule too big in " + s);
    }
    
    stringstream sstr(what.str(3));
    string d;
    while (sstr >> d) {
        if (d[0] == '+') {
            tpl.get<2>().push_back(make_tuple(1,lexical_cast<indexed_token>(d.substr(1))));
        } else {
            tpl.get<2>().push_back(make_tuple(-1,lexical_cast<indexed_token>(d.substr(1))));
        }
    }
    
    stringstream ssstr(what.str(4));
    while (ssstr >> d) {
        tpl.get<3>().push_back(lexical_cast<indexed_token>(d));
    }
    
    //std::cerr << "SIG:";
    //copy(tpl.get<3>().begin(),tpl.get<3>().end(),std::ostream_iterator<indexed_token>(std::cerr," "));
    //std::cerr << '\n';    
    
    return tpl;
}

double value(rule_data const& r, string const& key)
{
    double v = 0.;
    ptrdiff_t p = get_feature(r,key);
    if (p < int(r.features.size()) and p >= 0 and r.features[p].number) v = r.features[p].num_value;
    //else std::cerr << "no feature "<< key << "(p=" << p <<")\n";
    return v;
}
double wvalue(rule_data const& r)
{
    return value(r,"gt_prob") + 20*value(r,"glue-rule");
}

bool lower_rule_cost(rule_data const& r1, rule_data const& r2)
{
    return wvalue(r1) < wvalue(r2);
}

/*
boost::interprocess::offset_ptr<signature_trie>
make_sig_entry( rule_application_allocator& alloc
              , vector<rule_data>& rules 
              , )
*/

boost::tuple<uint64_t,uint64_t>
save_word_trie( std::ofstream& os
              , external_buffer_type& wordtrie_buffer
              , trie_construct_t& dbc )
{
    char_allocator alloc(wordtrie_buffer.get_segment_manager());
    uint64_t pt = os.tellp();
    wordtrie_buffer.construct<word_trie>("root")(dbc,alloc);
    wordtrie_buffer.get_segment_manager()->shrink_to_fit();

    uint64_t sz = wordtrie_buffer.get_size();
    boost::iostreams::write(os,(char*)(&sz),sizeof(uint64_t));
    boost::iostreams::write(os,(char const*)wordtrie_buffer.get_address(),sz);
    return boost::make_tuple(pt,sz);
}

////////////////////////////////////////////////////////////////////////////////

struct grammar_options {
    bool debug;
    path dbdir;
    path wfile;
    path pfile;
    string infos;
    double prior_floor_prob;
    double prior_bonus_count;
    double weight_tag_prior;
    sbmt::tag_prior priormap;
    header h;
    sbmt::weight_vector weights;
    gusc::shared_varray<any_xinfo_factory> factories;
    boost::shared_ptr<grammar_facade> gram;
    grammar_options()
    : prior_floor_prob(1e-7)
    , prior_bonus_count(100)
    , weight_tag_prior(1.0) {}
};

boost::program_options::options_description 
write_grammar_options(grammar_options& opts)
{
    using namespace boost::program_options;
    options_description desc;
    desc.add_options()
        ( "dbdir,d"
        , value(&opts.dbdir)
        , "grammar database"
        )
        ( "prior-file"
        , value(&opts.pfile)
        , "file with alternating <tag> <count> e.g. NP 123478.  virtual tags ignored"
        )
        ( "prior-floor-prob"
        , value(&opts.prior_floor_prob)->default_value(opts.prior_floor_prob)
        , "minimum probability for missing or low-count tags"
        )
        ( "prior-bonus-count"
        , value(&opts.prior_bonus_count)->default_value(opts.prior_bonus_count)
        , "give every tag that appears in prior-file this many extra counts (before normalization)"
        )
        ( "weight-prior"
        , value(&opts.weight_tag_prior)->default_value(opts.weight_tag_prior)
        , "raise prior prob to this power for rule heuristic"
        )
        ( "weight-file,w"
        , value(&opts.wfile)
        )
        ( "use-info,u"
        , value(&opts.infos)
        , "info types to use"
        )
        ;
    return desc;
}

grammar_options& parse_grammar_options(int argc, char** argv, grammar_options& opts)
{
    using namespace boost::program_options;
    options_description desc = write_grammar_options(opts);
    desc.add(sbmt::io::logfile_registry::instance().options());
    desc.add(get_info_options());
    positional_options_description posdesc;
    posdesc.add("dbdir",1);
    basic_command_line_parser<char> cmd(argc,argv);
    variables_map vm;
    store(cmd.options(desc).positional(posdesc).run(),vm);
    notify(vm);
    
    if (not vm.count("dbdir")) {
        cerr << desc << endl;
        cerr << "must provide xrs rule db" << endl;
        exit(1);
    }
    
    load_header(opts.h,opts.dbdir);
    
    if (exists(opts.pfile)) {
        std::ifstream ifs(opts.pfile.native().c_str());
        opts.priormap.set(ifs,opts.h.dict,opts.prior_floor_prob,opts.prior_bonus_count);
        opts.priormap.raise_pow(opts.weight_tag_prior);
    }
    
    if (exists(opts.wfile)) {
        std::ifstream wfs(opts.wfile.c_str());
        read_weights(opts.weights,wfs,opts.h.fdict);
    }
    
    make_fullmap(opts.h.dict);
    init_info_factories(opts.h.dict);
    std::set<sbmt::span_t> dummys;
    std::map<std::string,std::string> dummyf;
    sbmt::lattice_tree ltree(0,dummys,dummyf); //dummy
    opts.gram.reset(new grammar_facade(&opts.h,&opts.weights));
    opts.factories = get_info_factories(opts.infos,*opts.gram,ltree,get_property_map());
    return opts;
}

int main(int argc, char** argv)
{
    size_t sz = 7*268435456; //512MB
    //1073741824; //1GB
    //size_t sz = 2*1073741824; //2GB  
    size_t wdsz = sz;
    bool unkrule_found = false;
    std::ios_base::sync_with_stdio(false);
    grammar_options opts;
    parse_grammar_options(argc,argv,opts);
    
    ofstream ofs;
    string line;

    indexed_token wildcard = opts.h.wildcard();
    indexed_token rarest;
    path groupid;
    vector< tuple<short,indexed_token> > keys;
    vector<indexed_token> sig;
    rule_data rule;
    vector<rule_data> rules;
    info_rule_sorter sorter(opts.gram.get(),opts.factories,&opts.weights,&opts.priormap);
    const char* tmpstr = getenv("TMPDIR");
    if (not tmpstr) tmpstr = "/tmp";
    path tmp(tmpstr);
    path tmpfile;
    time_duration reading, writing, converting;
    //mapped_file mfile;
    boost::shared_array<char> subtrie_array, wordtrie_array;
    external_buffer_type subtrie_buffer;
    boost::shared_ptr<external_buffer_type> wordtrie_buffer;
    subtrie_construct_t sdbc(rule_application_array(0,0));
    trie_construct_t dbc(compressed_signature_trie(0,0));
    size_t numlines=0;
    while (getline(cin,line)) {
        try {
            tie(groupid,rarest,keys,sig,rule) = procline(line);
            tmpfile = tmp / lexical_cast<std::string>(rarest);
            boost::filesystem::remove(tmpfile);
            cerr << "create tmpfile " << tmpfile << '\n';
            //subtrie_array.reset();
            if (not subtrie_array) subtrie_array.reset(new char[sz]);
            subtrie_buffer = external_buffer_type(ip::create_only,subtrie_array.get(),sz);
            //std::cerr << "new subtrie_buffer:: " << subtrie_buffer.get_size() << '\n';
            
            if (not wordtrie_array) wordtrie_array.reset(new char[sz]);
            wordtrie_buffer.reset(new external_buffer_type(ip::create_only,wordtrie_array.get(),sz));
            //std::cerr << "new wordtrie_buffer:: " << wordtrie_buffer->get_size() << '\n';
            ofs.open(tmpfile.native().c_str());
            //mfile = mapped_file(ip::create_only,tmpfile.native().c_str(),sz);
            
            rules.push_back(rule);
            ++numlines;
        }  catch (std::exception const& e) {
            cerr << "could not process line: " << line << ".  "
                 << "msg: " << e.what() << "\n" << endl;
            throw e;
        }

        ptime start_read = microsec_clock::local_time();
        ptime start_write;
        ptime start_convert;

        while(getline(cin,line)) {
            try {
                path gid;
                indexed_token r;
                vector< tuple<short,indexed_token> > k;
                vector<indexed_token> s;
                
                tie(gid,r,k,s,rule) = procline(line);
                bool uf = (k.size() == 0) and (r.type() == sbmt::foreign_token);
                
                if (s != sig or k != keys or r != rarest) {
                    //std::cerr << "sig: ";
                    //std::copy(sig.begin(),sig.end(),std::ostream_iterator<indexed_token>(std::cerr," "));
                    //std::cerr << '\n';
                    rule_application_array raa = make_entry(subtrie_buffer,rules,opts.h,opts.weights);
                    // sort raa
                    rule_application_array_adapter raaa(raa);
                    sorter(raaa.begin(),raaa.end());
                    sdbc.insert(sig.begin(),sig.end(),raa);
                    rules.clear();
                    sig = s;
                }
                if (k != keys or r != rarest) {
                    dbc.insert(keys.begin(),keys.end(),make_sig_entry(subtrie_buffer,*wordtrie_buffer,sdbc));
                    subtrie_construct_t(rule_application_array(0,0)).swap(sdbc);
                    //subtrie_array.reset();
                    if (not subtrie_array) subtrie_array.reset(new char[sz]);
                    external_buffer_type(ip::create_only,subtrie_array.get(),sz).swap(subtrie_buffer);
                    //std::cerr << "new subtrie_buffer:: " << subtrie_buffer.get_size( )<< '\n';
                    keys = k;
                }
                if (r != rarest) {
                    reading += start_convert - start_read;
                    start_write = microsec_clock::local_time();
                    uint64_t offset, ssz;
                    boost::tie(offset,ssz) = save_word_trie(ofs,*wordtrie_buffer,dbc);
                    converting += start_write - start_convert;
                    std::cout << rarest  << '\t' 
                              << groupid << '\t' 
                              << offset  << '\t' 
                              << unkrule_found << std::endl; 
                    unkrule_found = false;
                    trie_construct_t(compressed_signature_trie(0,0)).swap(dbc);
                    wdsz = wordtrie_buffer->get_size();
                    
                    if (not wordtrie_array) wordtrie_array.reset(new char[sz]);
                    wordtrie_buffer.reset(new external_buffer_type(ip::create_only,wordtrie_array.get(),sz));
                
                    wdsz = wordtrie_buffer->get_size();
                    //std::cerr << "new wordtrie_buffer:: " << wdsz << '\n';
                    start_read = microsec_clock::local_time();
                    writing += start_read - start_write;
                    rarest = r;
                }
                if (gid != groupid) {
                    ofs.close();
                    cerr << "copy " << tmpfile << " to " << (opts.dbdir / groupid) << '\n';
                    copy_file(tmpfile.native().c_str(),(opts.dbdir / groupid).native().c_str(),copy_option::overwrite_if_exists);
                    remove(tmpfile);
                    tmpfile = tmp / lexical_cast<std::string>(rarest);
                    remove(tmpfile);
                    cerr << "create " << tmpfile << '\n';
                    ofs.open(tmpfile.native().c_str());
                    groupid = gid;
                }
                rules.push_back(rule);
                ++numlines;
                unkrule_found = unkrule_found or uf;
            } catch (std::exception const& e) {
                cerr << "could not process line: " << line << ".  \n"
                     << "msg: " << e.what() << "\n" << endl;
                throw e;
            }
        }
        rule_application_array raa = make_entry(subtrie_buffer,rules,opts.h,opts.weights);
        // sort raa
        rule_application_array_adapter raaa(raa);
        sorter(raaa.begin(),raaa.end());
        sdbc.insert(sig.begin(),sig.end(),raa);
        dbc.insert(keys.begin(),keys.end(),make_sig_entry(subtrie_buffer,*wordtrie_buffer,sdbc));
        uint64_t offset, ssz;
        boost::tie(offset,ssz) = save_word_trie(ofs,*wordtrie_buffer,dbc);
        ofs.close();
        std::cout << rarest << '\t' 
                  << groupid << '\t' 
                  << offset << '\t'
                  << unkrule_found << std::endl;
        copy_file(tmpfile, opts.dbdir / groupid,copy_option::overwrite_if_exists);
        remove(tmpfile);
    }

    clog << "populated database with "<< numlines << " lines" << endl;
    clog << "time spent writing:   " << writing << endl;
    clog << "           reading:   " << reading << endl;
    clog << "           converting:" << converting << endl;

    return 0;
}
