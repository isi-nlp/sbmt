# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <sstream>
# include <boost/lexical_cast.hpp>

using namespace std;



int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }

        size_t min_rpos = get_feature(rd,"min");
        if (min_rpos == rd.features.size()) continue;

        if ((rd.features[min_rpos].number && 
             rd.features[min_rpos].num_value == 1) ||
            (boost::lexical_cast<int>(rd.features[min_rpos].str_value)) == 1)
          cout << line << endl;
    }
    return 0;
}
