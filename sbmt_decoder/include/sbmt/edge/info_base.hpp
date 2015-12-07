# if ! defined(SBMT__EDGE__INFO_BASE_HPP)
# define       SBMT__EDGE__INFO_BASE_HPP

# include <cstddef>
# include <vector>
# include <sbmt/logmath.hpp>

namespace sbmt {


////////////////////////////////////////////////////////////////////////////////

template <class Derived>
struct info_base {
  /* // GOES in info_factory, actually. but there's no info factory base. everyone seems to repeat this knowledge
  typedef tuple<Derived,score_t,score_t> result_type; // info, inside-score, heuristic
  static inline Derived & info(result_type &r) {
    return boost::get<0>(r);
  }
  static inline score_t & inside(result_type &r) {
    return boost::get<1>(r);
  }
  static inline score_t & h(result_type &r) {
    return boost::get<2>(r);
  }
  */
    Derived const& derived() const
    {
        return *static_cast<Derived const*>(this);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class Info>
std::size_t hash_value(info_base<Info> const& ei)
{
    return ei.derived().hash_value();
}

////////////////////////////////////////////////////////////////////////////////

template <class Info>
bool operator == (info_base<Info> const& ei1, info_base<Info> const& ei2)
{
    return ei1.derived().equal_to(ei2.derived());
}

////////////////////////////////////////////////////////////////////////////////

template <class Info>
bool operator != (info_base<Info> const& ei1, info_base<Info> const& ei2)
{
    return not ei1.derived().equal_to(ei2.derived());
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__EDGE__INFO_BASE_HPP
