# if ! defined(SBMT__SEARCH__CKY_LOGGING_HPP)
# define       SBMT__SEARCH__CKY_LOGGING_HPP

# include <sbmt/search/logging.hpp>

namespace sbmt {

// return number of [i,j,k] considered so far in CKY, up to cur_size, cur_leftpos
double cky_progress(int chart_size,int cur_size=-1,int cur_leftpos=-1);

} // namespace sbmt


# endif //     SBMT__SEARCH__CKY_LOGGING_HPP
