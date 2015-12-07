# include "rootprob.hpp"
# include <boost/algorithm/string/classification.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/algorithm/string/split.hpp>

using boost::tuple;
using boost::split;
using boost::tie;

// TODO: for each line matching key, get the scores for the rule and add them
// to a running set, then print out the sums of scores with the key per mapper
// approach and reset.


// TODO: could do field manipulation here and not in the separate python mapper
bool get_keyval(istream& in, tuple<string&,string&> tpl)
{
    string line;
    if (getline(in,line)) {
        vector<string> sv;
        boost::split(sv,line,boost::is_any_of("\t"));
        tpl.get<0>() = sv.front();
        tpl.get<1>() = sv.back();
        return true;
    } else {
        return false;
    }
}


int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    vecmap_t mp;
    read(argv[1],&mp);
    string k, v, key;
    string featurename="rprobdecomp";
    std::vector<long double> scores;
    if (get_keyval(cin,tie(k,v))) {
      // set key and process v
      key = k;
      rule_data rd;
      try {
        rd = parse_xrs(v);
      } catch(...) { }
      score(&mp, &rd, &scores);
      bool cleared=false;
      while (get_keyval(cin,tie(k,v))) {
        if (key != k) {
          cout << key << "\t" << featurename << "=10^" << scores[0];
          for (size_t x = 1; x < scores.size(); ++x)
            cout << ' ' << "\t" << featurename << "." <<  x-1 << "=10^" << scores[x];
          cout << '\n';
          scores.clear();
          cleared=true;
        }
        key = k;
        try {
          rd = parse_xrs(v);
        } catch(...) { continue; }
        if (cleared) {
          cleared=false;
          score(&mp, &rd, &scores);
        }
        else {
          std::vector<long double> subscores;
          score(&mp, &rd, &subscores);
          for (size_t x = 0; x < subscores.size(); ++x)
            scores[x]+=subscores[x];
        }
      }
      cout << key << "\t" << featurename << "=10^" << scores[0];
      for (size_t x = 1; x < scores.size(); ++x)
        cout << ' ' << "\t" << featurename << "." << x-1 << "=10^" << scores[x];
      cout << '\n';
    }
    return 0;
}
