# include <xrsparse/xrs.hpp>
# include <sbmt/grammar/alignment.hpp>
# include <vector>
# include <utility>
# include <string>
# include <iostream>

std::string alignment_string(rule_data& r)
{
    ptrdiff_t p = get_feature(r,"align");
    sbmt::alignment align;
    std::stringstream sstr;
    
    if (p < ptrdiff_t(r.features.size())) {
        align = sbmt::alignment(r.features[p].str_value);
    } else {
        throw std::runtime_error("alignment missing");
    }
    
    bool first = true;
    
    for (size_t s = 0; s != align.n_src(); ++s) {
        BOOST_FOREACH(sbmt::alignment::index t, align.sa[s]) {
            if (not first) sstr << ' ';
            sstr << s << '-' << t;
            first = false;
        }
    }
    return sstr.str();
}

std::string target_string(rule_data& r)
{
    bool first = true;
    std::stringstream sstr;
    BOOST_FOREACH(rule_data::lhs_list::value_type v, r.lhs)
    {
        if (v.children == false) {
            if (not first) sstr << ' ';
            first = false;
            if (v.indexed) {
                sstr << '0';
            } else {
                sstr << '"' << v.label << '"';
            }
        }
    }
    return sstr.str();
}

std::string source_string(rule_data& r)
{
    bool first = true;
    std::stringstream sstr;
    BOOST_FOREACH(rule_data::rhs_list::value_type v, r.rhs)
    {
        if (not first) sstr << ' ';
        first = false;
        if (v.indexed) {
            sstr << '0';
        } else {
            sstr << '"' << v.label << '"';
        }
    }
    return sstr.str();
}

int main(int argc, char** argv)
{
    using namespace std;
    string line;
    while (std::getline(cin,line)) {
        try {
            rule_data r = parse_xrs(line);
            string a = alignment_string(r),
                   e = target_string(r),
                   f = source_string(r);
            cout << r.id << '\t' << f << '\t' << e << '\t' << a << '\n';
        } catch(std::exception const& e) {
            cerr << e.what() << '\n';
        }
    }
    return 0;
}
