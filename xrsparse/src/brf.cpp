# include <xrsparse/xrs.hpp>
# include <boost/spirit.hpp>

int main()
{
    using namespace std;
    using namespace boost::spirit;
    using namespace phoenix;
    string line;
    

    std::ios_base::sync_with_stdio(false); 
    while (getline(cin,line)) {
        if (line[0] == 'X' and line[1] == ':') {
            rule_data 
                rd = parse_xrs(std::make_pair(line.begin() + 2, line.end()));
            std::cout << "X: " << rd << '\n';
        } else if (line[0] == 'V' and line[1] == ':') {
            brf_data 
                rd = parse_brf(std::make_pair(line.begin() + 2, line.end()));
            std::cout << "V: " << rd << '\n';
        }
    }
    return 0;
}