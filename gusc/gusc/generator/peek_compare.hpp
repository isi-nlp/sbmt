# if ! defined(GUSC__GENERATOR__PEEK_COMPARE_HPP)
# define       GUSC__GENERATOR__PEEK_COMPARE_HPP

namespace gusc {
    
////////////////////////////////////////////////////////////////////////////////
///
///  compares two generators (input iterators), by comparing the current 
///  elements in the generators.
///
///  i.e. for generators g1,g2, 
///  \code
///  Cmp cmp;
///  peek_compare<Cmp>(cmp)(g1,g2) <==> cmp(*g1,*g2)
///  \endcode
///
////////////////////////////////////////////////////////////////////////////////
template <class Compare>
struct peek_compare {
    peek_compare(Compare const& compare = Compare())
      : compare(compare) {}
    
    template <class Peekable>
    bool operator()(Peekable const& p1, Peekable const& p2) const
    {
        if (!p1) return bool(p2);
        else if (!p2) return false;
        else return compare(*p1,*p2);
    }

    Compare const compare;
};

} // namespace gusc

# endif //     GUSC__GENERATOR__PEEK_COMPARE_HPP

