# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <sstream>
# include <boost/tr1/unordered_map.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/foreach.hpp>
# include <boost/lexical_cast.hpp>

using namespace std;
typedef tr1::unordered_map<string, std::vector<long double> > vecmap_t;

void print(vecmap_t& full_mp, vecmap_t& min_mp)
{
    string k;
    std::vector<long double> v;
    BOOST_FOREACH(boost::tie(k,v), full_mp) {
        cout << k << '\t' << v[0];
        for (size_t x = 1; x != v.size(); ++x) cout << ' ' << v[x];
        cout << "\t";
        vecmap_t::iterator pos = min_mp.find(k);
        double oval = (pos == min_mp.end()) ? 0 : pos->second[0];
        cout << oval;
        for (size_t x = 1; x != v.size(); ++x) {
          oval = (pos == min_mp.end()) ? 0 : pos->second[x];
          cout << ' ' << oval;
        }
        cout << '\n';
    }
    full_mp.clear();
    min_mp.clear();
}

void sum( vecmap_t& mp
        , std::string const& lbl
        , std::vector<long double> const& v )
{
    vecmap_t::iterator pos = mp.find(lbl);
    if (pos == mp.end()) {
      mp.insert(std::make_pair(lbl,v));
    }
    else {
      for (size_t x = 0; x != v.size(); ++x) pos->second[x] += v[x];
    }
}

int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    vecmap_t full_mp;
    vecmap_t min_mp;
    cout << print_rule_data_features(false);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }

        size_t count_rpos = get_feature(rd,"count");
        if (count_rpos == rd.features.size()) continue;

        std::vector<long double> v;
        if (rd.features[count_rpos].number) v.push_back(rd.features[count_rpos].num_value);
        // multiple values for count collapsed
        else {
            std::stringstream sstr(rd.features[count_rpos].str_value);
            long double vv;
            while (sstr >> vv) v.push_back(vv);
        }
        sum(full_mp,rd.lhs[0].label,v);
        // is min defined and is it {{{1}}}?
        size_t min_rpos = get_feature(rd, "min");
        if (min_rpos != rd.features.size() &&
            boost::lexical_cast<bool>(rd.features[min_rpos].str_value)) {
          sum(min_mp, rd.lhs[0].label, v);
        }
    }
    print(full_mp, min_mp);
    return 0;
}
