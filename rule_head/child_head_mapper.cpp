# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <sstream>
# include <boost/tr1/unordered_map.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/foreach.hpp>
# include <boost/program_options.hpp>
# include "constants_hpp.py"
// generate symbol, word(postag), count for distribution of heads from each rule


using namespace std;

struct options {
    bool postag;
    options() 
        : postag(false) {}
};

options parse_options(int argc, char** argv)
{
    using namespace boost::program_options;
    bool help = false;
    options opts;
    options_description desc;
    desc.add_options()
      ( "help,h"
      , bool_switch(&help)->default_value(false)
      , "display this message"
      )
      ( "postag,p"
      , bool_switch(&opts.postag)->default_value(false)
      , "generate for head pos tags instead of head words"
      )
      ;

    try {
        variables_map vm;
        store(parse_command_line(argc,argv,desc), vm);
        notify(vm);
        if (help) {
            cout << desc << '\n';
            exit(0);
        } 
    } catch(...) {
        cout << desc << '\n';
        exit(1);
    }
    return opts;
}

typedef map<unsigned int, lhs_node > lhsmap;
typedef vector<lhs_node > lhsvec;

lhsmap get_lhs_map(rule_data rd)
{
  lhsmap ret;
  BOOST_FOREACH(lhs_node curr, rd.lhs) {
    if (curr.indexed)
      ret[curr.index]=curr;
  }
  return ret;
}



int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    cout << print_rule_data_features(false);

    options opts = parse_options(argc,argv);

    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }
        
        size_t index = get_feature(rd, opts.postag ? VARIABLE_HEAD_TAG_DISTRIBUTION_FEATURE : VARIABLE_HEAD_WORD_DISTRIBUTION_FEATURE );
        if (index == rd.features.size()) continue;
        // tokenize by ||| separator.
        // for each element, print the symbol for that variable and the element contents (tab separated)
        unsigned int curr_rhs = 0;
        string rhslabel;
        lhsmap lhs_map = get_lhs_map(rd);
        for ( ; curr_rhs < rd.rhs.size(); curr_rhs++) {
          if (rd.rhs[curr_rhs].indexed) {
            rhslabel = lhs_map[rd.rhs[curr_rhs].index].label;
            curr_rhs++;
            break;
          }
        }
        std::stringstream sstr(rd.features[index].str_value);
        string k;
        long double v;
        while ( sstr >> k ) {
          // change the label
          if (k == "|||") {
            for ( ; curr_rhs < rd.rhs.size(); curr_rhs++) {
              if (rd.rhs[curr_rhs].indexed) {
                rhslabel = lhs_map[rd.rhs[curr_rhs].index].label;
                curr_rhs++;
                sstr >> k;
                break;
              }
            }
          }
          sstr >> v;
          cout << rhslabel << "\t" << k << "\t" << v << endl;
        }
    }
    return 0;
}
