# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <set>
# include <map>
# include <iterator>
# include <boost/lexical_cast.hpp>
# include <boost/program_options.hpp>
# include <sbmt/grammar/alignment.hpp>

using namespace std;


int get_count(rule_data const& rd)
{
    BOOST_FOREACH(feature const& f, rd.features) {
        if (f.key == "count") {
            //cerr << rd << "\n";
            return boost::lexical_cast<int>(f.str_value);
        }
    }
    throw std::runtime_error("no count");
}

sbmt::alignment get_align(rule_data const& rd)
{
    BOOST_FOREACH(feature const& f, rd.features) {
        if (f.key == "align") {
            return sbmt::alignment(f.str_value);
        }
    }
    throw std::runtime_error("no align");
}

struct options {
    set<string> words;
};

options parse_options(int argc, char** argv)
{
    using namespace boost::program_options;
    string wordfilename;
    bool help = false;
    options opts;
    options_description desc;
    desc.add_options()
      ( "help,h"
      , bool_switch(&help)->default_value(false)
      , "display this message"
      )
      ( "wordfile,w"
      , value<string>(&wordfilename)
      , "apply indicator feature if a word in wordfile is unaligned on rule"
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
        if (wordfilename != "") {
            ifstream ifs(wordfilename.c_str());
            copy( istream_iterator<string>(ifs)
                , istream_iterator<string>()
                , inserter(opts.words,opts.words.end())
                )
                ;
        }
    } catch(...) {
        cout << desc << '\n';
        exit(1);
    }
    return opts;
}

string feature_name(string const& str)
{
    string retval;
    BOOST_FOREACH(char c, str) {
        retval += boost::lexical_cast<string>(int(c)) + "-";
    }
    return retval;
}

int main(int argc, char** argv)
{
    options opts = parse_options(argc, argv);
    string line;
    rule_data rd;
    sbmt::alignment align;
    int count;
    map<string,int> unaligns;
    while (getline(cin,line)) {
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }
        unaligns.clear();
        count = get_count(rd);
        align = get_align(rd);
        assert(align.n_src() == rd.rhs.size());
        
        int unalign = 0;
        for (size_t z = 0; z != align.sa.size(); ++z) { 
            if (align.sa[z].empty()) {
                ++unalign;
                if (opts.words.find(rd.rhs[z].label) != opts.words.end()) {
                    unaligns[rd.rhs[z].label]++;
                }
            }
        }
        string fname;
        if (count < 2) fname = "count1-unalign";
        else fname = "count-unalign";
        
        cout << rd.id << '\t' << fname << '=' << unalign;
        for (map<string,int>::iterator itr = unaligns.begin(); itr != unaligns.end(); ++itr) {
            cout << " count-unalign-" << feature_name(itr->first) << '=' << itr->second;
        }
        cout << '\n';
    }
    return 0;
}
