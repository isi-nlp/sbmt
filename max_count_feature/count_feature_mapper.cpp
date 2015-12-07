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
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }
        
        size_t rpos = get_feature(rd,"cwt");
        
        cout << rd << '\t';
        if (rpos != rd.features.size()) {
            if (rd.features[rpos].number) {
                cout << rd.features[rpos].num_value;
            } else {
                cout << rd.features[rpos].str_value;
            }
        }
        else cout << 1; 
        cout << '\n';
    }
    return 0;
}