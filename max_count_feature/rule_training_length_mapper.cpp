# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <iomanip>
# include <sstream>

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
    ios::sync_with_stdio(false);
    
    cout << print_rule_data_features(false);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        int64_t linenum, linelen;
        
        try {
            rd = parse_xrs(line);
            linenum  = find_feature(rd,"lineRawID").num_value;
            linelen = find_feature(rd,"lineLength").num_value;
            cout << rd << '\t' << setfill('0') << setw(5) << linelen << '\t' << setw(0) << linenum << '\n';
        } catch(...) { continue; }

        
    }
    return 0;
}
