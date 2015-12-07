# include <iostream>
# include <string>
# include <boost/algorithm/string/split.hpp>
# include <boost/algorithm/string/classification.hpp>
# include <boost/foreach.hpp>
# include <boost/lexical_cast.hpp>
# include <utility>
# include <vector>
# include <map>
# include <boost/tuple/tuple.hpp>

using boost::is_any_of;
using boost::tuple;
using boost::tie;
using boost::split;
using boost::lexical_cast;
using std::istream;
using std::string;
using std::vector;
using std::map;
using std::make_pair;
using std::cin;
using std::cout;
/*

# requires mapreduce be specified to use two keys for sorting, 1 for partitioning
def keyval(input):
    for line in input:
        r,k,v = line.strip().split('\t')
        yield r.strip(),k.strip(),v.strip()
*/
bool get_keyval(istream& in, tuple<string&,string&,map<string,long double>&> tpl)
{
    string line;
    if (getline(in,line)) {
        vector<string> sv;
        split(sv,line,is_any_of("\t"));
        tpl.get<0>() = sv[0];
        tpl.get<1>() = sv[1];
        tpl.get<2>().clear();
        vector<string> kvv;
        split(kvv,sv[2],is_any_of(" "));
        BOOST_FOREACH(string const& kvs, kvv) {
            vector<string> kv;
            split(kv,kvs,is_any_of("="));
            tpl.get<2>()[kv[0]] = lexical_cast<long double>(kv[1]);
        }
        return true;
    } else {
        return false;
    }
}

/*
def reducer(pairs):
    for rule, rec in itertools.groupby(pairs, lambda x : x[0]):
        total = 0
        m = collections.defaultdict(int)
        for r,k,v in rec:
            m['%s-%s' % (k,v)] += 1
            total += 1
        M = {}
        for k,w in m.iteritems():
            M[k] = float(w)/float(total)
        yield rule,M 
*/

void print(string const& rule, string const& lbl, map<string,long double> const& mp)
{
    long double total = 0;
    string k;
    long double v;
    BOOST_FOREACH(tie(k,v),mp) total += v;
    cout << rule << '\t';
    
    bool first = true;
    BOOST_FOREACH(tie(k,v), mp) {
        if (not first) {
            cout << ' ';
        }
        first = false;
        cout << lbl << '-' << k << "=10^-" << float(v/total);
    }
    cout << '\n';
}

void accumulate(map<string,long double>& accum,map<string,long double> const& mp)
{
    string k;
    uint64_t v;
    BOOST_FOREACH(tie(k,v),mp) accum[k] += v;
}

int main()
{
    map<string,long double> mp, accum;
    string rule, r, key, k;
    if (get_keyval(cin,tie(r,k,mp))) {
        rule = r;
        key = k;
        accum = mp;
        while (get_keyval(cin,tie(r,k,mp))) {
            if (rule != r) {
                print(rule,key,accum);
                accum.clear();
                //accum.swap(map<string,uint64_t>());
            }
            rule = r;
            key = k;
            accumulate(accum,mp);
        }
        print(rule,key,accum);
    }
    return 0;
}
/*          

if __name__ == '__main__':
    for rule, M in reducer(keyval(sys.stdin)):
        print rule + '\t' + ' '.join('%s=10^-%g' % p for p in M.iteritems())
*/