# include <xrsparse/xrs.hpp>
# include <string>
# include <map>
# include <vector>
# include <boost/foreach.hpp>
# include <iostream>
# include <sstream>
# include <fstream>
#include <boost/numeric/conversion/bounds.hpp>

typedef std::map< std::string, std::vector<std::string> > split_map_t;

////////////////////////////////////////////////////////////////////////////////

void split_rule( rule_data& rule
               , split_map_t& split
               , rule_data::lhs_list::iterator curr
               , std::ostream& out
               , rule_data::rule_id_type& m
               , rule_data::rule_id_type& M
               , bool& orig )
{
    using namespace std;

    if (curr == rule.lhs.end()) {
        if (not orig) {
            if (rule.id > 0) {
                ++M;
                rule.id = M;
            } else {
                --m;
                rule.id = m;
            }
        } else {
            orig = false;
        }
        out << rule << '\n';
        return;
    }

    split_map_t::const_iterator pos = split.find(curr->label);

    rule_data::lhs_list::iterator next = curr;
    ++next;
    for (; next != rule.lhs.end() and not next->indexed; ++next) {}

    //split_rule(rule,split,next,out,m,M,orig);
    if (pos == split.end()) 
	pos = split.insert(make_pair(curr->label,vector<string>(1,curr->label))).first;

    string oldlabel = curr->label;
    BOOST_FOREACH(string const& newlabel, pos->second) {
        curr->label = newlabel;
        split_rule(rule,split,next,out,m,M,orig);
    }
    curr->label = oldlabel;
}

void split_rule( rule_data& rule
               , split_map_t& split
               , std::ostream& out
               , rule_data::rule_id_type& m
               , rule_data::rule_id_type& M  )
{
    bool orig = true;
    split_rule(rule,split,rule.lhs.begin(),out,m,M,orig);
}

////////////////////////////////////////////////////////////////////////////////

split_map_t read_split_map(std::istream& in)
{
    using namespace std;
    split_map_t split;
    string line;

    while(getline(in,line)) {
        string key;
        stringstream sstr;
        sstr.str(line);
        sstr >> key;
        vector<string> v;
        typedef istream_iterator<string> splitter_t;
        typedef ostream_iterator<string> printer_t;
        copy(splitter_t(sstr), splitter_t(), back_inserter(v));
        split.insert(make_pair(key,v));

        // this prevents rules with already-split variables from disappearing
        BOOST_FOREACH(string vv, v) {
            split.insert(make_pair(vv,vector<string>(1,vv)));
        }
        cerr << key << " -> ";
        copy(v.begin(),v.end(),printer_t(cerr," "));
        cerr << '\n';
    }
    return split;
}

////////////////////////////////////////////////////////////////////////////////

void exit_usage()
{
    std::cerr << "\nusage: splitnt <splitfilename>\n\n";
    exit(1);
}

int main(int argc, char** argv)
{
    using namespace std;
    if (argc != 2) {
        exit_usage();
    }
    ifstream ifs(argv[1]);
    if (!ifs) {
        exit_usage();
    }
    split_map_t split = read_split_map(ifs);

    rule_data::rule_id_type M = boost::numeric::bounds<rule_data::rule_id_type>::lowest();
    rule_data::rule_id_type m = boost::numeric::bounds<rule_data::rule_id_type>::highest();

    vector<rule_data> rdv;

    string line;
    while (getline(cin,line)) {
        try {
            rule_data rd = parse_xrs(line);
            if (rd.id > 0) M = std::max(M,rd.id);
            else m = std::min(m,rd.id);
            rdv.push_back(rd);
        } catch (runtime_error const& e) {
            cerr << e.what() << '\n';
        }
    }

    BOOST_FOREACH(rule_data& rd, rdv) {
        split_rule(rd,split,cout,m,M);
    }
}


