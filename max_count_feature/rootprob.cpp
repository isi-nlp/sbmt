# include "rootprob.hpp"

int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    vecmap_t mp;
    read(argv[1],&mp);
    string line;
    string featurename="rprob";
    while (getline(cin,line)) {
      rule_data rd;
      try {
        rd = parse_xrs(line);
      } catch(...) { continue; }
      if (get_feature(rd, "count") == rd.features.size()) continue;
      std::vector<long double> scores;
      score(&mp, &rd, &scores);
      cout << rd.id << "\t" << featurename << "=10^" << scores[0];
      for (size_t x = 1; x != scores.size(); ++x)
        cout << ' ' << "\t" << featurename << "." << x-1 << "=10^" << scores[x];
      cout << '\n';
    }
    return 0;
}
