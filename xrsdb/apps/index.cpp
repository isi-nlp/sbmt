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

////////////////////////////////////////////////////////////////////////////////
namespace sbmt {
std::istream& operator >> (std::istream& in, indexed_token& tok)
{
    boost::uint32_t idx;
    in >> idx;
    tok = indexed_token(idx);
    return in;
}

}

struct options {
    fs::path rootdb;
    bool directional;
    options() : directional(false) {}
};

options parse_options (int argc, char** argv)
{
    using namespace boost::program_options;
    options_description desc;
    options opts;
    desc.add_options()
        ( "db-root,d"
        , value(&opts.rootdb)
        , "database root directory"
        )
        ( "directional,b"
        , bool_switch(&opts.directional)
        )
        ( "help,h"
        , "produce help message"
        )
        ;
    positional_options_description posdesc;
    posdesc.add("db-root",1);
    
    basic_command_line_parser<char> cmd(argc,argv);
    variables_map vm;
    store(cmd.options(desc).positional(posdesc).run(),vm);
    notify(vm);
    
    if (vm.count("help")) {
        cerr << desc << endl;
        exit(1);
    }
    if (opts.rootdb.empty()) {
        cerr << "you must specify the location of the newly built xrsdb" 
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
    
    xrsdb::header h;
    load_header(h,opts.rootdb);
    
    sbmt::indexed_token tok;
    fs::path loc;
    std::string locstr;
    uint64_t offset;
    bool unkrule_found;

    while (cin >> tok >> ws >> locstr >> ws >> offset >> ws >> unkrule_found) {
        if (unkrule_found) h.knownset.insert(tok);
        h.add_offset(tok,offset);
        loc = fs::path(locstr);
        //cerr << tok << '\t' << loc << '\t' << offset << unkrule_found << '\n';
        if (not opts.directional) {
            if (xrsdb::structure_from_token(tok).get<0>() != loc) {
                throw std::runtime_error(
                        "inconsistent path in xrsdb offset file. "
                        "mismatch between db and input"
                      );
            }
        }
    }    
    save_header(h,opts.rootdb);
    return 0;
}
