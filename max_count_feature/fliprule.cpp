# include <xrsparse/xrs.hpp>
# include <string>
# include <sstream>
# include <iostream>
# include <map>
# include <set>
# include <sbmt/grammar/alignment.hpp>

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
    pair<const char*,const char*> tformv[] = { make_pair("tphrase","sphrase")
                                             , make_pair("sphrase","tphrase")
                                             , make_pair("tgtb","srcb")
                                             , make_pair("srcb","tgtb") };
    const char* omitv[] = { "hwpos", "hwf", "ht", "hw" };
    set<string> omit(omitv,omitv+4);
    map<string,string> tform(tformv,tformv+4);

    if (argc != 2) {
        cerr << "usage: ratio_feature_mapper <feature_name>" << endl;
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
            cout << rd.lhs[0].label << '(';
            int count = 0;
            bool first = true;
            std::map<int,int> mp;
            BOOST_FOREACH(rhs_node nd,rd.rhs) {
                if (not first) cout << ' ';
                first = false;
                if (nd.indexed) {
                    mp[nd.index] = count;
                    cout << 'x' << count++ << ':' << label(rd,nd);
                } else {
                    cout << "X(\"" << label(rd,nd) << "\")";
                }
            }
            cout << ')';
            cout << " ->";
            BOOST_FOREACH(lhs_node nd,rd.lhs) {
                if (not nd.children) {
                    cout << ' ';
                    if (nd.indexed) cout << 'x' << mp[nd.index];
                    else cout << '"' << nd.label << '"';
                } 
            }
            cout << " ###";
            BOOST_FOREACH(feature feat,rd.features) {
                if (tform.find(feat.key) != tform.end()) {
                    feat.key = tform[feat.key];
                    cout << ' ' << feat;
                } else if (feat.key == "align") {
                    std::stringstream sstr;
                    sbmt::alignment a(sbmt::alignment(feat.str_value),sbmt::as_inverse());
                    sstr << a;
                    feat.str_value = sstr.str();
                    cout << ' ' << feat;
                } else if (omit.find(feat.key) == omit.end()) {
                    cout << ' ' << feat;
                }
            }
            cout << '\n';
        } catch(...) { continue; }
    }
    return 0;
}

