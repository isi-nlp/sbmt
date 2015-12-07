# include <xrsparse/xrs.hpp>
# include <sbmt/grammar/alignment.hpp>
# include <vector>
# include <utility>
# include <string>
# include <iostream>

typedef std::vector< std::pair<std::string,std::string> > alignments;

alignments make_alignments(rule_data& r)
{
    ptrdiff_t p = get_feature(r,"align");
    
    sbmt::alignment prealign;
    alignments align;
    
    if (p < ptrdiff_t(r.features.size())) {
        prealign = sbmt::alignment(r.features[p].str_value);
    } else {
        throw std::runtime_error("alignment missing");
    }

    std::vector<rule_data::lhs_list::iterator> tarpos;
    std::vector<bool> tarcovered;
    rule_data::lhs_list::iterator
        itr = r.lhs.begin(),
        end = r.lhs.end();
    for (; itr != end; ++itr) {
        if (itr->children == false) { tarpos.push_back(itr); tarcovered.push_back(false); }
    }
    
    for (size_t s = 0; s != prealign.n_src(); ++s) {
        rule_data::rhs_list::iterator ritr = r.rhs.begin() + s;
        if (prealign.sa[s].empty()) {
            if (ritr->indexed == false) {
                align.push_back(std::make_pair(std::string("NULL"),ritr->label));
            }
        }
        BOOST_FOREACH(sbmt::alignment::index t, prealign.sa[s]) {
            rule_data::lhs_list::iterator litr = tarpos[t];
            if (litr->indexed == false and ritr->indexed == false) {
                tarcovered[t] = true;
                align.push_back(std::make_pair(litr->label,ritr->label));
            }
        }
    }
    
    for (size_t t = 0; t != tarcovered.size(); ++t) {
        if (not tarcovered[t] and not tarpos[t]->indexed) {
            align.push_back(std::make_pair(tarpos[t]->label,std::string("NULL")));
        }
    }
    
    if (align.empty()) align.push_back(std::make_pair(std::string("NULL"),std::string("NULL")));
    
    return align;
}


int main(int argc, char** argv)
{
    using namespace std;
    bool invert = ((argc == 2) and (string(argv[1]) == "invert"));
    string line;
    while (std::getline(cin,line)) {
        try {
            rule_data r = parse_xrs(line);
            alignments align = make_alignments(r);
            BOOST_FOREACH(alignments::reference a, align) {
                if (invert) a.first.swap(a.second);
            }
            BOOST_FOREACH(alignments::reference a, align) {
                cout << a.first << '\t' << a.second << '\t' << r.id;
                BOOST_FOREACH(alignments::reference a, align) {
                    cout << ' ' << a.first << ' ' << a.second;
                }
                cout << '\n';
            }
        } catch(std::exception const& e) {
            cerr << e.what() << '\n';
        }
    }
    return 0;
}