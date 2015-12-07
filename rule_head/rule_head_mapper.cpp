# include <xrsparse/xrs.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/bind.hpp>
# include <boost/algorithm/string/trim.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/program_options.hpp>
# include <iostream>
# include <iterator>
# include <sstream>
# include <gusc/iterator/ostream_iterator.hpp>


using namespace std;
using namespace boost;

std::string at_replace(std::string str);



////////////////////////////////////////////////////////////////////////////////

vector<string> rule_head(rule_data const& rd, bool get_tag)
{
    vector<string> pre, retval;
    
    bool vec_found = false;
    bool head_found = false;

    std::string vec_key = get_tag ? "htf" : "hwf";
    std::string head_key= get_tag ? "ht" : "hw";
    
    int v = 0;
    BOOST_FOREACH(rhs_node const& r, rd.rhs) {
        if (r.indexed) ++v;
    }
    
    BOOST_FOREACH(feature const& f, rd.features) {
        if (f.key == vec_key) {
            vec_found = true;
            stringstream sstr(f.str_value);
            copy( istream_iterator<string>(sstr)
                , istream_iterator<string>()
                , back_inserter(pre)
                )
                ;
        } else if (f.key == head_key) {
            head_found = true;
            retval.push_back(at_replace(boost::trim_copy(f.str_value)));
        }
    }
    
    if (not (vec_found and head_found)) {
        throw std::runtime_error(
          "rule_head_mapper requires features hw, hwf, ht, and htf on all rules"
        );
    }
    if (v != pre.size()) {
        throw std::runtime_error(
          "rule_head_mapper requires features |h[wt]f| == |variables(rule.rhs)|"
        );
    }
    if (retval[0] == "") {
        throw std::runtime_error(
          "rule_head_mapper requires non-null hw/ht feature"
        );
    }
    
    BOOST_FOREACH(rhs_node const& r, rd.rhs)
    {
        if (r.indexed) retval.push_back(at_replace(pre.at(r.index)));
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

struct options {
    bool nonterminal;
    options() 
     : nonterminal(false) {}
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
      ( "nonterminal-map,n"
      , bool_switch(&opts.nonterminal)->default_value(false)
      , "generate nonterminal to word mappings"
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
    cout << print_rule_data_features(false);

    options opts = parse_options(argc,argv);
        
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        vector<string> hv, tv;
        try {
            rd = parse_xrs(line);
            if (not opts.nonterminal) {
              hv = rule_head(rd, false);
              tv = rule_head(rd, true);
            }
        } catch(std::exception const& e) { 
            cerr << line << "error={{{" << e.what() << "}}}" << endl;
            continue; 
        }
        if (opts.nonterminal) {
            string wd;
            BOOST_FOREACH(feature const& f, rd.features) {
                if (f.key == "hw") {
                    wd = at_replace(f.str_value);
                }
            }
            if (wd != "") cout << rd.lhs[0].label << '\t' << wd << '\n';
        } else {
            if (not hv.empty()) {
                cout << rd << '\t';
                copy(hv.begin(),hv.end(),gusc::ostream_iterator(cout," "));
                cout << '\t';
                copy(tv.begin(),tv.end(),gusc::ostream_iterator(cout," "));
                cout << '\n';
            }
        }
    }
    return 0;
}

