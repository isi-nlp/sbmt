# include <boost/archive/binary_iarchive.hpp>
# include <boost/archive/text_iarchive.hpp>
# include <boost/archive/xml_iarchive.hpp>
# include <sbmt/token.hpp>
# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/search/lattice_reader.hpp>
# include <gusc/trie/basic_trie.hpp>
# include <gusc/trie/fixed_trie.hpp>
# include <gusc/trie/trie_algo.hpp>
# include <gusc/iterator/ostream_iterator.hpp>
# include <boost/tokenizer.hpp>
# include <boost/regex.hpp>
# include <boost/range.hpp>
# include <filesystem.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/program_options.hpp>
# include <boost/function.hpp>
# include <boost/enum.hpp>
# include <fstream>
# include <filesystem.hpp>
# include <collapsed_signature_iterator.hpp>
# include <syntax_rule_util.hpp>
# include <db_usage.hpp>


namespace ba = boost::archive;
using namespace sbmt;
using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

typedef boost::function<
           vector<indexed_token> 
           (string const&, indexed_token_factory&)
        > sig_f;

typedef gusc::basic_trie<indexed_token,int> trie_t;
typedef gusc::fixed_trie<indexed_token,int> fixed_trie_t;

////////////////////////////////////////////////////////////////////////////////
/*
bool match( trie_t const& trie
          , fixed_trie_t const& ftrie
          , vector<indexed_token> const& v )
{
    pair<bool,trie_t::state> p = trie_find(trie,v.begin(),v.end());
    pair<bool,fixed_trie_t::state> pf = trie_find(ftrie,v.begin(),v.end());
    
    if (p.first != pf.first) {
        return false;
    }
    if (p.first and (trie.value(p.second) != ftrie.value(pf.second))) {
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

template <class Iterator>
bool match_all( trie_t const& trie
              , fixed_trie_t const& ftrie
              , Iterator itr, Iterator end )
{
    for (; itr != end; ++itr) if(not match(trie,ftrie,*itr)) return false;
    return true;
}

template <class Iterator>
trie_t create(Iterator itr, Iterator end)
{
    trie_t t(0);
    for (; itr != end; ++itr) t.insert(itr->begin(),itr->end(),1);
    return t;
}
*/
////////////////////////////////////////////////////////////////////////////////

struct options {
    fs::path headerfile;
    xrsdb::db_usage usage;
    bool directional;
    bool subtrie;
    bool nonlex;
    options() : directional(false),subtrie(false),nonlex(false) {}
};

options parse_options (int argc, char** argv)
{
    using namespace boost::program_options;
    options_description desc;
    options opts;
    desc.add_options()
        ( "freqtable,f"
        , value(&opts.headerfile)
        , "frequency table"
        )
        ("usage,u"
        , value(&opts.usage)->default_value(xrsdb::db_usage::decode)
        , ""
        )
        ( "directional,d"
        , bool_switch(&opts.directional)
        , "new style xrsdb stores a single trie for each rarest word, and "
          "keys specify whether they traverse to left or to right of word. "
          "this flag generates the proper keys"
        )
        ( "subtrie,s"
        , bool_switch(&opts.subtrie)
        , "generate subtrie keys of variables"
        )
        ( "nonlex,n"
        , bool_switch(&opts.nonlex)
        , "store nonlex rules"
        )
        ( "help,h"
        , "produce help message"
        )
        ;
    variables_map vm;
    store(parse_command_line(argc,argv,desc),vm);
    notify(vm);
    
    if (vm.count("help")) {
        cerr << desc << endl;
        exit(1);
    }
    if (not vm.count("freqtable")) {
        cerr << desc << endl;
        cerr << "must provide frequency table" << endl;
        exit(1);
    }
    return opts;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    
    sig_f func;
    
    options opts = parse_options(argc,argv);
    
    if (opts.usage == xrsdb::db_usage::decode) {
        func = xrsdb::from_string;
    } else if (opts.usage == xrsdb::db_usage::raw_sig) {
        func = xrsdb::from_sig; // but see below
    } else if (opts.usage == xrsdb::db_usage::force_etree) {
        func = xrsdb::forcetree_sig;
    } else if (opts.usage == xrsdb::db_usage::force_stateful_etree) {
        func = xrsdb::stateful_forcetree_sig;
    } else {
        std::stringstream sstr;
        sstr << "unsupported operation: "<< opts.usage;
        throw std::runtime_error(sstr.str());
    }
    clog << "loading frequency-table from " << opts.headerfile << "..." << flush;
    fs::ifstream headerfs(opts.headerfile);
    ba::xml_iarchive headerar(headerfs);
    
    xrsdb::header h;
    headerar & boost::serialization::make_nvp("dict",h);
    clog << "done" << endl;
    
    istream& sigs = cin;
    ostream& marked = cout;
    string line;
    
    boost::char_separator<char> tabsep("\t");
    std::cerr << "nonlex=" << opts.nonlex << '\n';
    wildcard_array wc(h.dict);
    while (getline(sigs,line)) {
        vector<indexed_token> v;
        string rule;
        if (opts.usage == xrsdb::db_usage::raw_sig) {
            // When reading raw signatures, the signature and rule are in
            // separate tab-delimited fields, so separate them
            boost::tokenizer<boost::char_separator<char> > tabtoker(line,tabsep);
            boost::tokenizer<boost::char_separator<char> >::iterator itr = tabtoker.begin();
            v = func(*itr,h.dict);
            ++itr;
            stringstream sstr;
            copy(itr, tabtoker.end(), gusc::ostream_iterator(sstr,"\t"));
            rule = sstr.str();
        } else {
            v = func(line,h.dict);
            rule = line;
        }

        if (v.size() == 0) continue;
        
        vector<indexed_token>::iterator pos = v.begin();
        for (vector<indexed_token>::iterator i = v.begin(); i != v.end(); ++i) {
            if (i->type() != virtual_tag_token) {
                pos = i;
                break;
            }
        }
        
        if (pos->type() == virtual_tag_token and not opts.nonlex) continue;
        
        for (vector<indexed_token>::iterator i = v.begin(); i != v.end(); ++i) {
            if (i->type() != virtual_tag_token) if(h.freq[*i] <= h.freq[*pos]) pos = i;
        }
        
        indexed_token rarest = *pos;
        if (pos->type() == virtual_tag_token) rarest = wc[0];

        marked << xrsdb::structure_from_token(*pos).get<0>().native() << "\t" << rarest << "\t";
        
        if (opts.directional) {
            if (pos->type() == virtual_tag_token) {
                marked << '+' << *pos << ' ';
            }
            else {
                vector<indexed_token>::iterator i = pos; --i;
                vector<indexed_token>::iterator e = v.begin(); --e;
                for (; i != e; --i) {
                    marked << '-' << *i << ' ';
                }
                i = pos; ++i;
                e = v.end();
                for (; i != e; ++i) {
                    marked << '+' << *i << ' ';
                }
            }
        } else {
            copy(v.begin(), v.end(), ostream_iterator<indexed_token>(marked," "));
        }
        
        if (opts.subtrie) {
            rule_data rd = parse_xrs(rule);
            marked << "\t";
            BOOST_FOREACH(rhs_node rnd, rd.rhs) {
                if (rnd.indexed) {
                    marked << h.dict.tag(label(rd,rnd)) << ' ';
                } else {
                    marked << h.dict.foreign_word(label(rd,rnd)) << ' ';
                }
            }
        }
        marked << "\t" << rule << "\n";
    }
    
    return 0;
}
