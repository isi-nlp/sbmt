# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <sstream>
# include <boost/tr1/unordered_map.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/foreach.hpp>
# include <boost/program_options.hpp>
# include "root_hw.hpp"

// count root, headword as pair. probably should be 
// replaced by mr-friendlier version

using namespace std;
typedef tr1::unordered_map<string, std::pair<size_t,size_t> > wordmap_t;

void print(wordmap_t& mp)
{
    string k;
    std::pair<size_t, size_t> v;
    BOOST_FOREACH(boost::tie(k,v), mp) {
      cout << k << '\t' << v.first << ' ' << v.second << endl;
    }
    mp.clear();
}

void sum( wordmap_t& archive
        , wordmap_t const& hws )
{
  BOOST_FOREACH(const wordmap_t::value_type& pair, hws) {
    wordmap_t::iterator pos = archive.find(pair.first);
    if (pos == archive.end()) {
      archive.insert(std::make_pair(pair.first,pair.second));
    } else {
      pos->second.first += pair.second.first;
      pos->second.second += pair.second.second;
    }
  }
}

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
      , "generate counts for head pos tag instead of word"
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


int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    wordmap_t archive;
    cout << print_rule_data_features(false);

    options opts = parse_options(argc,argv);

    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }

        size_t index = get_feature(rd, opts.postag ? RULE_HEAD_TAG_DISTRIBUTION_FEATURE  : RULE_HEAD_WORD_DISTRIBUTION_FEATURE );
        if (index == rd.features.size()) continue;
        wordmap_t hws;
        std::stringstream sstr(rd.features[index].str_value);
        string tag = rd.lhs[0].label;
        string k;
        size_t v;
        while ( sstr >> k && sstr >> v ) {
          string tagword = joinkey(tag, k);
          hws.insert(std::make_pair(tagword, std::make_pair(1,v)));
        }
        sum(archive, hws);
    }
    print(archive);
    return 0;
}
