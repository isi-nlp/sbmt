# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <sstream>

using namespace std;

int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    
    cout << print_rule_data_features(false);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        long id;
        try {
            rd = parse_xrs(line);
            id = rd.id;
        } catch(...) { continue; }

        cout << rd << '\t' << id << '\n';
    }
    return 0;
}