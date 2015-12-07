# include <iostream>
# include <boost/algorithm/string.hpp>
# include <string>
# include <vector>

using namespace boost;
using namespace std;


/***********************************************************
    #!/usr/bin/env python
    import sys
    try:
        for line in sys.stdin:
            v = line.rstrip('\n').split('\t')
            print '\t'.join(v[3:4] + v[0:2] + v[4:])
    except:
        print >> sys.stderr, 'failed to process:',line
        raise
 ************************************************************/

string line;
typedef split_iterator<string::iterator> string_split_iterator;
vector< iterator_range<string::iterator> > space(2);

void print(ostream& out, iterator_range<string::iterator> const& rng)
{
    copy(rng.begin(),rng.end(),ostream_iterator<char>(out));
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(0);
    while (getline(cin,line)) {
        string_split_iterator itr = make_split_iterator(line,first_finder("\t",is_equal())), 
                              end;
        int x = 0;
        for (; itr != end; ++itr,++x) {
            if (x < 2) {
                space[x] = *itr;
            } else if (x == 3) {
                print(cout,*itr);
                for (size_t y = 0; y != 2; ++y) {
                    cout << '\t';
                    print(cout,space[y]);
                }
            } else if (x > 3) {
                cout << '\t'; 
                print(cout,*itr);
            }
        }
        cout << '\n';
    }
    return 0;
}
