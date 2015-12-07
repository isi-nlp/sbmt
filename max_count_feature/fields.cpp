/* kind of like cut, but reordering is allowed */ 

# include <iostream>
# include <boost/algorithm/string.hpp>
# include <boost/lexical_cast.hpp>
# include <string>
# include <vector>
# include <map>
# include <gusc/string/escape_string.hpp>

using namespace boost;
using namespace std;

typedef split_iterator<string::iterator> string_split_iterator;

void print(ostream& out, iterator_range<string::iterator> const& rng)
{
    copy(rng.begin(),rng.end(),ostream_iterator<char>(out));
}

string delimiter="\t";

int main(int argc, char** argv) {
    vector<int> fields;
    string line;
    vector< iterator_range<string::iterator> > splits;
    for (int x = 1; x != argc; ++x) {
        string argvx = argv[x];
        if (argv[x] == string("-d")) {
            ++x;
            delimiter = gusc::unescape_c(argv[x]);
        } else {
            fields.push_back(lexical_cast<int>(argvx));
        }
    }
    ios::sync_with_stdio(false);
    cin.tie(0);
    while (getline(cin,line)) {
        splits.clear();
        string_split_iterator 
            itr = make_split_iterator(line,token_finder(is_any_of(delimiter))), 
            end;
        copy(itr,end,back_inserter(splits));
        bool first = true;
        for (size_t x = 0; x != fields.size(); ++x) {
            if (not first) {
                cout << '\t';
            }
            print(cout,splits.at(fields.at(x)));
            first = false;
        }
        cout << '\n';
    }
    return 0;
}
