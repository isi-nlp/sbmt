# include <xrsparse/xrs_grammar_.hpp>
# include <xrsparse/xrs.hpp>
# include <gusc/iterator/ostream_iterator.hpp>

template class std::vector<lhs_node>;
template class std::vector<rhs_node>;
template class std::vector<feature>;

typedef std::pair<std::string::iterator,std::string::iterator> pair_t;
typedef boost::iterator_range<std::string::iterator> range_t;


template brf_data parse_brf<std::string>(std::string const& line);
template brf_data parse_brf<pair_t>(pair_t const& line);
template brf_data parse_brf<range_t>(range_t const& line);

template rule_data parse_xrs<std::string>(std::string const& line);
template rule_data parse_xrs<pair_t>(pair_t const& line);
template rule_data parse_xrs<range_t>(range_t const& line);

std::ptrdiff_t get_feature(rule_data const& rd, std::string const& name)
{
    rule_data::feature_list::const_iterator pos = rd.features.begin();
    for (; pos != rd.features.end(); ++pos) {
        if (pos->key == name) return pos - rd.features.begin();
    }
    return pos - rd.features.begin();
}

std::ostream& operator<< (std::ostream& os, lhs_pos const& r)
{
    if (r.pos->indexed) {
        os << 'x' << r.pos->index << ':' << r.pos->label;
    } else if(not r.pos->children) {
        os << '"' << r.pos->label << '"';
    } else {
        os << r.pos->label;
        os << '(';
        lhs_pos rr(*r.vec, r.pos + 1);
        os << rr;
        while (rr.pos->next != 0) {
            assert(rr.vec->begin() + rr.pos->next > rr.pos);
            rr.pos = rr.vec->begin() + rr.pos->next;
            assert(rr.pos < rr.vec->end());
            os << ' ' << rr;
        }
        os << ')';
    }
    return os;
}

std::ostream& operator<< (std::ostream& os, rhs_node const& r)
{
    if (r.indexed) {
        os << 'x' << r.index;
    } else {
        os << '"' << r.label << '"';
    }
    return os;
}

std::ostream& operator<< (std::ostream& os, feature const& f)
{
    if (f.key == "") return os;
    os << f.key << '='; 
        if (f.compound) {
            os << "{{{" << f.str_value << "}}}";
        } else {
	    if (f.number) os << "10^" << (-f.num_value);
            else os << f.str_value;
        }
    return os;
}

size_t rhs_position(rule_data const& rd, size_t idx)
{
    size_t ret = 0;
    BOOST_FOREACH(rhs_node const& rhs, rd.rhs) {
        if (rhs.indexed and rhs.index == idx) return ret;
        ++ret;
    }
    return ret;
}

std::string label(rule_data const& rd, rhs_node const& rhs)
{
    if (rhs.indexed) {
        BOOST_FOREACH(lhs_node lhs, rd.lhs) {
            if (lhs.indexed and lhs.index == rhs.index) {
                return lhs.label;
            }
        }
        throw std::logic_error("index out of bounds");
    } else {
        return rhs.label;
    }
}

std::ostream& operator<< (std::ostream& os, rule_data const& rd)
{
    bool print_lhs = rule_data_iomanip::print(os,rule_data_iomanip::lhs),
         print_rhs = rule_data_iomanip::print(os,rule_data_iomanip::rhs),
         print_features = rule_data_iomanip::print(os,rule_data_iomanip::features);
    lhs_pos r(rd.lhs, rd.lhs.begin());
    if (print_lhs) os << r;
    if (print_lhs and print_rhs) os << " -> ";
    if (print_rhs) copy(rd.rhs.begin(),rd.rhs.end(),gusc::ostream_iterator(os," "));
    if (print_features) {
        if (print_lhs or print_rhs) os << " ###";
        if (rd.id) os << " id=" << rd.id;
        if (rd.features.size()) os << ' ';
        copy(rd.features.begin(),rd.features.end(),gusc::ostream_iterator(os," "));
    }
    return os;
}

std::ostream& operator<< (std::ostream& out, brf_data::var const& rd)
{
    if (rd.nt) return out << rd.label;
    else return out << '"' << rd.label << '"';
}

std::ostream& operator<< (std::ostream& out, brf_data const& rd)
{
    out << rd.lhs << " -> " << rd.rhs[0];
    if (rd.rhs_size > 1) out << ' ' << rd.rhs[1];
    if (not rd.features.empty()) {
        out << " ###";
        copy( rd.features.begin()
            , rd.features.end()
            , gusc::ostream_iterator(out," ")
            );
    }
    return out;
}




