# include <xrsparse/xrs.hpp>
# include <string>
# include <sstream>
# include <iostream>
# include <map>
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_comparison.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/regex.hpp>

using namespace std;

boost::regex re("\\d");
boost::regex replus("\\d+");

int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    
    string line;
    typedef boost::tuple<string,string> key;
    map<key,size_t> mp;
    while (getline(cin,line)) {
        rule_data rd;
        feature val;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }
        
        BOOST_FOREACH(lhs_node const& lhs, rd.lhs) {
            if (lhs.children or lhs.indexed) {
                mp[key("tag",lhs.label)] += 1;
                mp[key("tag",boost::regex_replace(lhs.label,re,"@"))] += 0;
                mp[key("tag",boost::regex_replace(lhs.label,replus,"@"))] += 0;
            }
            else {
                mp[key("target",lhs.label)] += 1;
                mp[key("target",boost::regex_replace(lhs.label,re,"@"))] += 0;
                mp[key("target",boost::regex_replace(lhs.label,replus,"@"))] += 0;

            }
        }
        
        int varcount = 0;
        BOOST_FOREACH(rhs_node const& rhs, rd.rhs) {
            if (not rhs.indexed) {
                if (varcount > 0) {
                    mp[key("virt",boost::lexical_cast<string>(varcount))] += 1;
                    //std::cout << "virt " << varcount << '\t' << 1 << '\n';
                    varcount = 0;
                }
                mp[key("source",rhs.label)] += 1;
                mp[key("source",boost::regex_replace(rhs.label,re,"@"))] += 0;
                mp[key("source",boost::regex_replace(rhs.label,replus,"@"))] += 0;
                //std::cout << "source " << rhs.label << '\t' << 1 << '\n';
            } else {
                ++varcount;
            }
        }
        
        BOOST_FOREACH(feature const& feat, rd.features) {
            mp[key("feat",feat.key)] += 1;
            if (feat.key == "froot") mp[key("tag",feat.str_value)] += 1;
            if (feat.key == "fvars") {
                std::stringstream sstr(feat.str_value);
                std::string line;
                while (sstr >> line) {
                    mp[key("tag",line)] += 1;
                }
            } if (feat.key == "leaflm_string") {
                std::stringstream sstr(feat.str_value);
                std::string line;
                while (sstr >> line) {
                    if (line[0] != '#') mp[key("target",line)] += 1;
                }
            }
            //std::cout << "feat " << feat.key << '\t' << 1 << '\n';
        }
    }
    size_t count;
    key k;
    BOOST_FOREACH(boost::tie(k,count), mp) {
        std::cout << k.get<0>() << ' ' << k.get<1>() << '\t' << count << '\n';
    }
    return 0;
}