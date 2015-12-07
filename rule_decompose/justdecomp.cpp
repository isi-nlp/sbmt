# include <binalgo.hpp>
# include <iostream>
# include <boost/lexical_cast.hpp>

int main(int argc, char** argv)
{
    using namespace std;
    string line;
    // set printing options for rule
//     cout << sbmt::print_rule_id(true)
//          << sbmt::print_rule_lhs(false)
//          << sbmt::print_rule_rhs(false);
    while (getline(cin,line)) {
        aligned_rule ar(line);
        subder_ptr s = get_subder(ar);
        print_subders_mr(cout,s,ar);
        //        cout << endl;
    }
    return 0;
}
