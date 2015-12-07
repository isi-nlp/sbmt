# include <xrsparse/xrs.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/bind.hpp>
# include <iostream>

using namespace std;
using namespace boost;

typedef tuple<int,int> span_t;

int len(span_t const& s) { return get<1>(s) - get<0>(s); }

////////////////////////////////////////////////////////////////////////////////

void gen_var_lengths(std::ostream& out, rule_data const& rd)
{
    std::string lenstr;
    BOOST_FOREACH(feature const& f, rd.features) {
        if (f.key == "srcb") {
            stringstream sstr(f.str_value);
            sstr << tuples::set_open('[') 
                 << tuples::set_delimiter(',') 
                 << tuples::set_close(']');

            istream_iterator<span_t> itr(sstr), end;
            int rl = 0;
            BOOST_FOREACH(rhs_node const& rnode, rd.rhs) {
                if (itr == end) throw runtime_error("feature srcb too short");
                if (rnode.indexed) {
                    rl += len(*itr);
                    out << '(' << 1 << ',' << len(*itr) << ',' << len(*itr) * len(*itr) << ')' << ' ';
                }
                ++itr;
            }
            out << "(1," << rl << ',' << rl*rl << ')';
            if (itr != end) throw runtime_error("feature srcb too long");
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////


int main() 
{
    ios::sync_with_stdio(false);
    cout << print_rule_data_features(false);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }

        cout << rd << '\t';
        gen_var_lengths(cout,rd);
        cout << '\n';
    }
}

