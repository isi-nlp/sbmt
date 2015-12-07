# if ! defined(SBMT__EDGE__SCORED_INFO_ARRAY_HPP)
# define       SBMT__EDGE__SCORED_INFO_ARRAY_HPP

# include <cstddef>
# include <vector>
# include <boost/iterator/iterator_traits.hpp>
# include <boost/range.hpp>
# include <sbmt/token/token.hpp>

namespace sbmt {

//struct skip_foreign_token {  };
template <class Info>
struct scored_info_array : public std::vector<Info const*> {
  typedef Info info_type;
  typedef std::vector<Info const*> V;
  template <class ConstituentIterator>
  scored_info_array(boost::iterator_range<ConstituentIterator> rng,bool keep_foreign_token=true) {
    ConstituentIterator itr = boost::begin(rng), end = boost::end(rng);
    for (; itr != end; ++itr) {
      if (itr->root().type() != foreign_token) {
        V::push_back(itr->info());
      } else if (keep_foreign_token) {
        V::push_back(itr->info()); // unknown if foreign tokens have a NULL info pointer
      }
    }
  }
};

} // namespace sbmt

# endif //     SBMT__EDGE__SCORED_INFO_ARRAY_HPP
