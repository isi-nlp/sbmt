# include <boost/tokenizer.hpp>
# include <boost/regex.hpp>
# include <boost/function.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/archive/text_oarchive.hpp>
# include <boost/serialization/map.hpp>
# include <boost/program_options.hpp>
# include <boost/enum.hpp>
# include <filesystem.hpp>
# include <syntax_rule_util.hpp>
# include <db_usage.hpp>

# include <iostream>
# include <map>
# include <set>
# include <string>

using namespace std;
using namespace boost;
namespace ba = boost::archive;
namespace fs = boost::filesystem;
using namespace sbmt;

typedef boost::function<
           vector<indexed_token> 
           (string const&, indexed_token_factory&)
        > sig_f;


////////////////////////////////////////////////////////////////////////////////

struct options {
    fs::path mapfile;
    xrsdb::db_usage usage;
    bool debug;
    options() : debug(false) {}
};

options parse_options (int argc, char** argv)
{
    using namespace boost::program_options;
    options_description desc;
    options opts;
    desc.add_options()
        ( "freqtable,f"
        , value(&opts.mapfile)
        , "dump the frequency table of foreign words seen"
        )
        ( "usage,u"
        , value(&opts.usage)->default_value(xrsdb::db_usage::decode)
        , "")
        ( "help,h"
        , "produce help message"
        )
        ( "debug,D"
        , bool_switch(&opts.debug)
        )
        ;
    positional_options_description posdesc;
    posdesc.add("freqtable",1);
    
    basic_command_line_parser<char> cmd(argc,argv);
    variables_map vm;
    store(cmd.options(desc).positional(posdesc).run(),vm);
    notify(vm);
    
    if (vm.count("help")) {
        cerr << desc << endl;
        exit(1);
    }
    if (opts.mapfile.empty()) {
        cerr << "you must specify an output file for the frequency table" 
             << endl;
        exit(1);
    }
    return opts;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    
    options opts = parse_options(argc,argv);
    
    sig_f func;
    
    if (opts.usage == xrsdb::db_usage::decode) {
        func = xrsdb::from_string;
    } else if (opts.usage == xrsdb::db_usage::force_etree){
        func = xrsdb::forcetree_sig;
    } else if (opts.usage == xrsdb::db_usage::force_stateful_etree) {
        func = xrsdb::stateful_forcetree_sig;
    } else {
        std::stringstream sstr;
        sstr << "unsupported operation: "<< opts.usage;
        throw std::runtime_error(sstr.str());
    }
    
    //map<indexed_token,size_t> histmap;
    xrsdb::header h;
    string line;
    
    //indexed_token_factory dict;
    indexed_token wc = h.wildcard();
    
    while (getline(cin,line)) {
        vector<indexed_token> v = func(line,h.dict);
        if (opts.debug) {
            token_format_saver sav(cerr);
            cerr << token_label(h.dict);
            copy(v.begin(),v.end(),ostream_iterator<indexed_token>(cerr," "));
            cerr << " ||| " << line << endl;
        }
        if (v.size() == 0) continue;
        
        for (vector<indexed_token>::iterator i = v.begin(); i != v.end(); ++i) {
            if (*i != wc) h.add_frequency(*i);
        }
    }
    
    if (not opts.mapfile.empty()) {
        fs::ofstream mapfs(opts.mapfile);
        ba::text_oarchive mapa(mapfs);
        mapa & h;
    }
    
    return 0;
}
