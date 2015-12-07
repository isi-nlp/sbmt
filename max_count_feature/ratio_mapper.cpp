# include <xrsparse/xrs.hpp>
# include <string>
# include <sstream>
# include <iostream>
# include <boost/tr1/unordered_map.hpp>

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

typedef
    tr1::unordered_map<string,long double>
    count_map;

typedef 
    tr1::unordered_map<string, count_map>
    combiner_map;

void print(ostream& out, combiner_map const& mp, string const& lbl)
{
    combiner_map::const_iterator itr = mp.begin(), end = mp.end();
    for (; itr != end; ++itr) {
        count_map::const_iterator citr = itr->second.begin(), cend = itr->second.end();
        out << itr->first << '\t' << lbl << '\t';
        bool first = true;
        for (; citr != cend; ++citr) {
            if (not first) out << ' ';
            first = false;
            out << citr->first  << '=' << citr->second;
        }
        out << '\n';
    }
}

int main(int argc, char** argv)
{
    combiner_map mp;

    if (argc < 2) {
        cerr << "usage: ratio_feature_mapper <feature_name>" << endl;
        return 1;
    }
    string lbl = argv[1];
    string cwt = "cwt";
    if (argc > 2) cwt = argv[2];

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
        stringstream sstr;
        sstr << print_rule_data_features(false) << rd;
        size_t rpos = get_feature(rd,cwt);
        if (rpos != rd.features.size()) mp[sstr.str()][val.str_value] += rd.features[rpos].num_value;
        else mp[sstr.str()][val.str_value] += 1;
        if (mp.size() >= 1000000) {
            print(cout,mp,lbl);
            mp.clear();
        }
    }
    print(cout,mp,lbl);
    return 0;
}
