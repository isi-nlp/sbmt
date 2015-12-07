namespace sbmt {

template <class CharT, class TraitsT>
std::basic_ostream& 
operator << (std::basic_ostream& os, rule_topology const& rtop)
{
    os << rtop.lhs() << " -> " << rtop.rhs(0);
    for (std::size_t x = 1; x != rtop.rhs_size(); ++x) {
        os << " " << rtop.rhs(x);
    }
    return os;
}

} // namespace sbmt
