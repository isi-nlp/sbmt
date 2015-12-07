# include <xrsparse/xrs.hpp>
# include <boost/spirit.hpp>

int main()
{
    using namespace std;
    using namespace boost::spirit;
    using namespace phoenix;
    string line;
    xrs_grammar g;
    rule_data rd;
    std::ios_base::sync_with_stdio(false); 
    while (getline(cin,line)) {
        parse_info<std::string::iterator> 
            info = parse(line.begin(),line.end(), (!(g[cout << arg1 << '\n'])) >> (!end_p), space_p);
        if (info.full) {
           continue;// cout << line << '\n';
        } else {
            cerr << "epic fail:" << endl <<  string(info.stop,line.end()) << endl;
            return 1;
        }
    }
    return 0;
}