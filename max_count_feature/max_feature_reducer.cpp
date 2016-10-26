# include <iostream>
# include <string>
# include <boost/algorithm/string/join.hpp>
# include <boost/algorithm/string/split.hpp>
# include <boost/algorithm/string/classification.hpp>
# include <boost/lexical_cast.hpp>
# include <utility>
# include <vector>
# include <boost/tuple/tuple.hpp>
# include <sstream>

using boost::tuple;
using boost::tie;
using boost::join;
using boost::split;
using boost::lexical_cast;
using std::istream;
using std::string;
using std::vector;
using std::make_pair;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::stringstream;
/*
# requires mapreduce be specified to use two keys for sorting, 1 for partitioning

def keyval(input):
    for line in input:
        args = line.strip().split('\t')
        k = args[0]
        w = args[-1]
        v = '\t'.join(args[1:len(args)-1])
        yield k.strip(),v.strip(),Decimal(w)
*/

bool get_keyval(istream& in, tuple<string&,string&,long double&> tpl)
{
    string line;
    if (getline(in,line)) {
        vector<string> sv;
        boost::split(sv,line,boost::is_any_of("\t"));
        tpl.get<0>() = sv.front();
        stringstream sstr(sv.back());
        string dd("errg");
        sstr >> dd;
        try {
        tpl.get<2>() = lexical_cast<long double>(dd);
        tpl.get<1>() = join(make_pair(sv.begin() + 1, sv.end() -1), "\t");
        } catch(...) {
	  cerr << "line: " << line << endl;
          cerr << "dd: " << dd << endl;
	  throw;
	}
        return true;
    } else {
        return false;
    }
}


/*
def reducer(pairs):
    for rule, rec in itertools.groupby(pairs, lambda x : x[0]):
        occ = Decimal(0)
        val = ''
        for fval, sub in itertools.groupby(rec, lambda x : x[1]):
            fvoc = Decimal(0)
            for ignore1,ignore2,w in sub:
                fvoc += w
            if fvoc > occ:
                occ = fvoc
                val = fval
        yield rule, val

if __name__ == '__main__':
    for rule, val in reducer(keyval(sys.stdin)):
        print "%s\t%s" % (rule,val)
*/

int main(int argc, char** argv)
{
    bool combiner = argc > 1 and std::string(argv[1]) == "-c";
    std::ios_base::sync_with_stdio(false);
    long double count(0), sum(0), candsum(0);
    string rule, r, val, v, candval;
    if (get_keyval(cin,tie(r,v,count))) {
        rule = r;
        sum = count;
        val = v;
        candsum = 0;
        candval = "";
        while (get_keyval(cin,tie(r,v,count))) {
            if (val != v or rule != r) {
	        if (combiner) cout << rule << '\t' << val << '\t' << sum << '\n';
                if (sum > candsum) {
                    candsum = sum;
                    candval = val;
                }
                sum = 0;
            }
            if (rule != r) {
	        if (not combiner) cout << rule << '\t' << candval << '\n';
                candsum = 0;
                candval = "";
            }
            rule = r;
            val = v;
            sum += count;
        }
        if (combiner) cout << rule << '\t' << val << '\t' << sum <<'\n';
        if (sum > candsum) {
            candsum = sum;
            candval = val;
        }
        if (not combiner) cout << rule << '\t' << candval << '\n';
    }
    return 0;
}
