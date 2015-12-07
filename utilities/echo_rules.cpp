# include <sbmt/grammar/syntax_rule.hpp>
# include <boost/lexical_cast.hpp>

int main(int argc, char** argv)
{
    bool lhs = true;
    bool rhs = true;
    bool id = true;
    if (argc >= 2) lhs = boost::lexical_cast<bool>(argv[1]);
    if (argc >= 3) rhs = boost::lexical_cast<bool>(argv[2]);
    if (argc >= 4) id  = boost::lexical_cast<bool>(argv[3]);
    using namespace std;
    string line; 
    cout << sbmt::print_rule_id(id) << sbmt::print_rule_rhs(rhs) << sbmt::print_rule_lhs(lhs);
    while (getline(cin,line)) {
        sbmt::fat_syntax_rule rule(line,sbmt::fat_tf);
        cout << rule << '\n';
    }
}