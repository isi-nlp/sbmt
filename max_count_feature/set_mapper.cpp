# include <xrsparse/xrs.hpp>
# include <string>
# include <sstream>
# include <iostream>

using namespace std;

void throw_feature_not_found(rule_data const& rd, string lbl)
{
    stringstream sstr;
    sstr << "feature " << lbl << " not found in rule '" << rd << "'";
    throw runtime_error(sstr.str());
}


feature find_feature(rule_data const& rd, string const& lbl)
{
    BOOST_FOREACH(feature const& f, rd.features)
    {
        if (f.key == lbl) {
            return f;
        }
    }
    throw_feature_not_found(rd,lbl);
    return rd.features[0]; // will not reach -- warning supression
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        cerr << "usage: set_mapper <feature_name>" << endl;
        return 1;
    }
    string lbl = argv[1];
    
    ios::sync_with_stdio(false);
    
    cout << print_rule_data_features(false);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        feature val;
        try {
            rd = parse_xrs(line);
            val = find_feature(rd,lbl);
        } catch(...) { continue; }

        cout << rd << '\t' << val << '\t';
        size_t rpos = get_feature(rd,"cwt");
        if (rpos != rd.features.size()) cout << rd.features[rpos].str_value;
        else cout << 1;
        cout << '\n';
    }
    return 0;
}