# include <sbmt/grammar/syntax_rule.hpp>
# include <string>
# include <iostream>
# include <stdexcept>
# include <gusc/iterator/ostream_iterator.hpp>
# include <collins/lm.hpp>

void throw_feature_not_found(rule_data const& rd, std::string lbl)
{
    std::stringstream sstr;
    sstr << "feature " << lbl << " not found in rule '" << rd << "'";
    throw std::runtime_error(sstr.str());
}

feature find_feature(rule_data const& rd, std::string const& lbl)
{
    BOOST_FOREACH(feature const& f, rd.features)
    {
        if (f.key == lbl) {
            return f;
        }
    }
    throw_feature_not_found(rd,lbl);
    return rd.features[0]; // will not reach -- warning supression
}

std::string recover(sbmt::fat_syntax_rule const& r, sbmt::fat_syntax_rule::tree_node const& n, std::vector<int> const& hpos, bool head = false)
{
    std::string ret;
    size_t x = &n - r.lhs_root();
    if (n.lexical()) return ret;
    else if (x == 0) ret += 'R';
    else if (head) ret += 'H';
    else ret += 'D';
    bool has_children = not n.indexed() and not n.children_begin()->lexical();
    if (has_children) {
        ret += '(';
        size_t p = 0;
        BOOST_FOREACH(sbmt::fat_syntax_rule::tree_node const& cn, n.children()) ret += recover(r,cn,hpos,++p == hpos[x]);
        ret += ')';
    }
    return ret;
}

int main()
{
    using namespace std;
    string line;
    while (getline(cin,line)) {
        rule_data rd = parse_xrs(line);
        string hm = find_feature(rd,"headmarker").str_value;
        //cerr << rd << '\n';
        sbmt::fat_syntax_rule r(rd,sbmt::fat_tf);
        vector<int> ha = collins::hpos_array(r,hm);
        cout << sbmt::print_rule_id(false) << r << '\t';
        copy(ha.begin(),ha.end(),gusc::ostream_iterator(cout," "));
        
        cout << '\t' << hm << '\t' << recover(r,*r.lhs_root(),ha) << std::endl;
    }
}