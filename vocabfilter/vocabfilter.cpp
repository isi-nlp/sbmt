# include <iostream>
# include <fstream>
# include <string>
# include <vector>
# include <iterator>
# include <boost/tr1/unordered_set.hpp>
# include <xrsparse/xrs.hpp>
# include <sbmt/grammar/syntax_rule.hpp>

using namespace boost;
using namespace std;

void read(char* fname,tr1::unordered_set<string>& st)
{
    ifstream fs(fname);
    return st.insert(istream_iterator<string>(fs),istream_iterator<string>());
}

void keys(sbmt::fat_syntax_rule::tree_node const& rt, vector<string>& k)
{
    string v = rt.get_token().label();
    BOOST_FOREACH(sbmt::fat_syntax_rule::tree_node const& nt, rt.children()) {
        keys(nt,k);
        v += '_' + nt.get_token().label();
    }
    k.push_back(v);
}

void insert(tr1::unordered_set<string>& st, sbmt::fat_syntax_rule const& r)
{
    vector<string> v; keys(*r.lhs_root(),v);
    st.insert(v.begin(),v.end());
}

bool valid(tr1::unordered_set<string> const& st, sbmt::fat_syntax_rule const& r)
{
    vector<string> v; keys(*r.lhs_root(),v);
    BOOST_FOREACH(string const& vv, v) {
        if (st.find(vv) == st.end()) return false;
    }
    return true;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(0);
    string line;
    tr1::unordered_set<string> dict;
    if (argc > 1) {
        for (int x = 1; x != argc; ++x) read(argv[x],dict);
    }
    while (getline(cin,line)) {
        if (argc > 1) {
           if (valid(dict,sbmt::fat_syntax_rule(parse_xrs(line),sbmt::fat_tf))) {
               cout << line << '\n';
           }
        } else {
           insert(dict,sbmt::fat_syntax_rule(parse_xrs(line),sbmt::fat_tf));
        }
    }
    if (argc <= 1) {
        copy(dict.begin(),dict.end(),ostream_iterator<string>(cout,"\n"));
    }
    return 0;
}
