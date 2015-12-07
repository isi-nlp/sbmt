# include <iostream>
# include <boost/algorithm/string.hpp>
# include <boost/lexical_cast.hpp>
# include <string>
# include <vector>
# include <map>

using namespace boost;
using namespace std;

typedef split_iterator<string::iterator> string_split_iterator;

void print(ostream& out, iterator_range<string::iterator> const& rng)
{
    copy(rng.begin(),rng.end(),ostream_iterator<char>(out));
}

int main(int argc, char** argv) {
    map<int,int> permute;
    string line;
    vector< iterator_range<string::iterator> > splits;
    for (int x = 1; x != argc; ++x) {
        string argvx = argv[x];
        string_split_iterator 
            itr = make_split_iterator(argvx,first_finder(",")),
            end;
        vector<int> p;
        for (; itr != end; ++itr) {
            p.push_back(lexical_cast<int>(copy_range<string>(*itr)));
        }
        if (p.size() != 2) throw std::logic_error("bad permute descriptor");
        permute.insert(make_pair(p[0],p[1]));
    }
    ios::sync_with_stdio(false);
    cin.tie(0);
    while (getline(cin,line)) {
        splits.clear();
        string_split_iterator 
            itr = make_split_iterator(line,first_finder("\t",is_equal())), 
            end;
        copy(itr,end,back_inserter(splits));
        bool first = true;
        for (size_t x = 0; x != splits.size(); ++x) {
            map<int,int>::iterator pos = permute.find(x);
            int to = x;
            if (pos != permute.end()) { to = pos->second; }
            if (to >= 0) {
                if (not first) {
                    cout << '\t';
                }
                print(cout,splits.at(to));
                first = false;
            }
        }
        cout << '\n';
    }
    return 0;
}