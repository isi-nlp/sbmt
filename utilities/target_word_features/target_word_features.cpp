# include <sbmt/grammar/syntax_rule.hpp>
# include <string>
# include <iostream>
# include <fstream>
# include <boost/foreach.hpp>
# include <set>
# include <map>

# include <boost/algorithm/string.hpp>
# include <boost/foreach.hpp>
# include <boost/lexical_cast.hpp>
# include <string>

std::string escape_feat(std::string const& raw)
{
    std::string ret;
    BOOST_FOREACH(char c, raw) {
        if (boost::algorithm::is_alnum()(c) or c == '-' or c == '_')
            ret.push_back(c);
        else 
            ret += "ESC_" + boost::lexical_cast<std::string>((unsigned int)(c));
    }
    return ret;
}

int main(int argc, char** argv)
{
    using namespace std;
    using namespace sbmt;
    
    set<string> words;
    ifstream wordfile(argv[1]);
    string word;
    
    while (wordfile >> word) words.insert(word);
    
    string line;
    
    
    ios_base::sync_with_stdio(false); 
    
    while (getline(cin,line)) {
        try {
            fat_syntax_rule rl(line,fat_tf);
            map<string,size_t> counts;
            fat_syntax_rule::lhs_preorder_iterator itr = rl.lhs_begin(),
                                                   end = rl.lhs_end();
            for(; itr != end; ++itr) {
                if (itr->lexical() and words.find(itr->get_token().label()) != words.end()) {
                    counts[itr->get_token().label()]++;
                } 
            }
            cout << rl.id() << '\t';
            if (counts.size() > 0) {
                typedef pair<string,size_t> pair_t;
                BOOST_FOREACH(pair_t const& p, counts) {
                    cout << " count_" << escape_feat(p.first) << '='<< p.second;
                }
            } else {
	        cout << " notargetcount=0";
            }
            cout << "\n";
        } catch(std::exception const& e) {
            cerr << e.what();
            continue;
        }
    }
    return 0;
}
