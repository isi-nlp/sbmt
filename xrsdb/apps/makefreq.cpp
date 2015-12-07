# include <boost/tokenizer.hpp>
# include <boost/regex.hpp>
# include <boost/function.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/archive/text_oarchive.hpp>
# include <boost/archive/xml_oarchive.hpp>
# include <boost/serialization/map.hpp>
# include <boost/program_options.hpp>
# include <boost/enum.hpp>
# include <filesystem.hpp>
# include <syntax_rule_util.hpp>
# include <db_usage.hpp>
# include <sbmt_decoder/include/sbmt/search/lattice_reader.hpp>

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

struct options {
    fs::path mapfile;
    xrsdb::db_usage usage;
    bool debug;
    bool typed;
    options() : debug(false), typed(false) {}
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
        ( "help,h"
        , "produce help message"
        )
        ( "debug,D"
        , bool_switch(&opts.debug)
        )
        ( "typed,t"
        , bool_switch(&opts.typed)
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
    
    //map<indexed_token,size_t> histmap;
    xrsdb::header h;
    string line;
    
    //indexed_token_factory dict;
    wildcard_array wc(h.dict);
    
    while (getline(cin,line)) {
      boost::char_separator<char> sep(" \t");
      boost::tokenizer<boost::char_separator<char> > toker(line,sep);
      boost::tokenizer<boost::char_separator<char> >::iterator itr = toker.begin();
      if (not opts.typed) {
          indexed_token t = h.dict.foreign_word(*itr);
          ++itr;
          size_t c = boost::lexical_cast<size_t>(*itr);
          h.add_frequency(t, c);
      } else {
          std::string type = *itr;
          ++itr;
          if (type == "tag") h.dict.tag(*itr);
          else if (type == "target") h.dict.native_word(*itr);
          else if (type == "feat") h.fdict.get_index(*itr);
          else if (type == "source") {
              indexed_token t = h.dict.foreign_word(*itr);
              ++itr;
              size_t c = boost::lexical_cast<size_t>(*itr);
              h.add_frequency(t, c);
          }
          else if (type == "virt") {
              int idx = boost::lexical_cast<int>(*itr);
              size_t c = std::numeric_limits<size_t>::max();
              indexed_token t = wc[idx];
              h.add_frequency(t,c);
          }
      }
    }
    
    h.dict.native_word("<s>");
    h.dict.native_word("</s>");
    
    if (not opts.mapfile.empty()) {
        fs::ofstream mapfs(opts.mapfile);
        ba::xml_oarchive mapa(mapfs);
        mapa &  boost::serialization::make_nvp("dict",h);
    }
    
    return 0;
}
