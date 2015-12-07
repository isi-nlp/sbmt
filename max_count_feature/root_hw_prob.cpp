# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <fstream>
# include <sstream>
# include <boost/tr1/unordered_map.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/foreach.hpp>
# include <boost/program_options.hpp>
# include <cmath>
# include "root_hw.hpp"

using namespace std;
typedef tr1::unordered_map<string, std::pair<size_t, size_t> > wordmap_t;

// add count(sym, hw) to count(rule, (sym), hw) so p(rule|sym, hw) can be formed.

void read(char const* root_hw_table, wordmap_t& archive)
{
    ifstream root_hw_tablef(root_hw_table);
    string k;
    string line;
    while (getline(root_hw_tablef, line)) {
        stringstream sstr(line);
        sstr >> k;
        size_t c, n;
        sstr >> c >> n;
    
        archive[k] = std::make_pair(c,n);
    }
}

double logdiv(double d, double n)
{
    if (d <= 0) return -20.0;
    else return std::max(std::log10(d) - n, -20.0);
}

struct options {
  bool postag;
  string inputfile;
  options() 
    : postag(false) {}
};

options parse_options(int argc, char** argv)
{
    using namespace boost::program_options;
    bool help = false;
    options opts;
    options_description desc;
    positional_options_description posopt;
    desc.add_options()
      ( "help,h"
      , bool_switch(&help)->default_value(false)
      , "display this message"
      )
      ( "postag,p"
      , bool_switch(&opts.postag)->default_value(false)
      , "generate probs for head pos tag instead of word"
      )
      ( "input-file", "file of symbol counts" )
      ;

    posopt.add("input-file", -1);
    try {
        variables_map vm;
        store(command_line_parser(argc,argv).options(desc).positional(posopt).run(), vm);
        notify(vm);
        if (help) {
            cout << desc << '\n';
            exit(0);
        } 
        opts.inputfile=vm["input-file"].as< string >();
    } catch(...) {
        cout << desc << '\n';
        exit(1);
    }
    return opts;
}

int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);

    options opts = parse_options(argc,argv);
    // denominator counts
    wordmap_t archive;
    read(opts.inputfile.c_str(), archive);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }


        size_t hwpos = get_feature(rd, opts.postag  ? RULE_HEAD_TAG_DISTRIBUTION_FEATURE  : RULE_HEAD_WORD_DISTRIBUTION_FEATURE );
        if (hwpos == rd.features.size()) continue;
        std::stringstream sstr(rd.features[hwpos].str_value);
        string tag = rd.lhs[0].label;
        cout << rd.id << "\t";
        string k;
        size_t v;
        while ( sstr >> k && sstr >> v ) {
          string tagword = joinkey(tag, k);
          cout << k << " " << archive[tagword].first << " " << archive[tagword].second << " ";
        }
        cout << endl;
    }
    return 0;
}
