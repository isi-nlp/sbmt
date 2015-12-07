# include <boost/coro/generator.hpp>
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

typedef boost::coro::generator<int> var_length_generator;

int gen_var_lengths(var_length_generator::self& gen, rule_data const& rd)
{
    std::string lenstr;
    BOOST_FOREACH(feature const& f, rd.features) {
        if (f.key == "srcb") {
            stringstream sstr(f.str_value);
            sstr << tuples::set_open('[') 
                 << tuples::set_delimiter(',') 
                 << tuples::set_close(']');

            istream_iterator<span_t> itr(sstr), end;
            BOOST_FOREACH(rhs_node const& rnode, rd.rhs) {
                if (itr == end) throw runtime_error("feature srcb too short");
                if (rnode.indexed) {
                    gen.yield(len(*itr));
                }
                ++itr;
            }
            if (itr != end) throw runtime_error("feature srcb too long");
        }
    }
    
    gen.exit();
    return -1; // dummy
}

var_length_generator var_lengths(rule_data const& rd)
{
    return var_length_generator(bind(gen_var_lengths, _1, cref(rd)));
}

var_length_generator var_lengths() { return var_length_generator(); }




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
        copy(var_lengths(rd),var_lengths(),ostream_iterator<int>(cout," "));
        cout << '\n';
    }
}
