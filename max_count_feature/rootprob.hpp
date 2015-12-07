# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <fstream>
# include <sstream>
# include <boost/tr1/unordered_map.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/foreach.hpp>
# include <cmath>

using namespace std;

typedef tr1::unordered_map<string,std::vector<long double> > vecmap_t;

void read(char const* nm, vecmap_t* mp)
{
    ifstream nmf(nm);
    string k;
    string line;
    long double vv;
    while (getline(nmf,line)) {
        stringstream sstr(line);
        sstr >> k;
        std::vector<long double> v;
        while (sstr >> vv) {
            v.push_back(std::log10(vv));
        }
        (*mp)[k] = v;
    }
}

long double logdiv(long double d, long double n)
{
    if (d <= 0) return -20.0;
    else return std::max(std::log10(d) - n, -20.0L);
}

void score(vecmap_t* mp, rule_data* rd, std::vector<long double>* ret) {
  size_t rpos = get_feature(*rd,"count");
  if (rpos == rd->features.size()) return;
  std::vector<long double> v;
  if (rd->features[rpos].number) v.push_back(rd->features[rpos].num_value);
  else {
    std::stringstream sstr(rd->features[rpos].str_value);
    long double vv;
    while (sstr >> vv) v.push_back(vv);
  }

  for (size_t x = 0; x != v.size(); ++x) {
    (*ret).push_back(logdiv(v[x],(long double)((*mp)[rd->lhs[0].label][x])));
  }
}
