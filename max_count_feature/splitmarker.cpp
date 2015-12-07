# include "split.hpp"

int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    size_t n = boost::lexical_cast<size_t>(argv[1]);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }
        if (rd.lhs.size() >= std::numeric_limits<rule_offset_t>::max()) continue;
        if (rd.rhs.size() >= std::numeric_limits<rule_offset_t>::max()) continue;
        size_t s = count_splits(rd);
        cout << line << " filter={{{"; 
        cout << ((s > n) ? "true" : "false") << "}}} \n";; 

    }
    return 0;
}
