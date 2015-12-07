# if ! defined(SBMT__SEARCH__LOGGING_HPP)
# define       SBMT__SEARCH__LOGGING_HPP

# include <sbmt/logging.hpp> // for root_domain and logging interface

namespace sbmt {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(search,root_domain);
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(filter_domain,"filter",search);
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(cky_domain,"cky",search);
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(parse_error_domain,"parse-error",cky_domain);

} // namespace sbmt


# endif //     SBMT__SEARCH__LOGGING_HPP
