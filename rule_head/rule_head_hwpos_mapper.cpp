# include <xrsparse/xrs.hpp>
# include <boost/lexical_cast.hpp>
# include <iostream>
# include <iterator>
# include <gusc/iterator/ostream_iterator.hpp>


using namespace std;
using namespace boost;

std::string at_replace(std::string str);
typedef std::vector<feature>::iterator feature_pos_t;

feature_pos_t get_feature(rule_data& rd, string const& name)
{
    feature_pos_t pos = rd.features.begin();
    for (; pos != rd.features.end(); ++pos) {
        if (pos->key == name) return pos;
    }
    return pos;
}

int main(int argc, char** argv) 
{
    ios::sync_with_stdio(false);
    cout << print_rule_data_features(false);

    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(std::exception const& e) { 
            cerr << line << " error={{{" << e.what() << "}}}" << endl;
            continue; 
        }

        feature_pos_t hwpos = get_feature(rd,"hwpos");
        if (hwpos == rd.features.end()) {
            throw runtime_error("hwpos is required");
        }       
        cout << rd << '\t' << *hwpos << '\n';
    }
    return 0;
}

