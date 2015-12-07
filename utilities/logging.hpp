# if ! defined(UTILITIES__LOGGING_HPP)
# define       UTILITIES__LOGGING_HPP

# include <sbmt/logging.hpp>

namespace sbmt {
SBMT_SET_DOMAIN_LOGFILE(root_domain, "-2" );
SBMT_SET_DOMAIN_LOGGING_LEVEL(root_domain, info);
}

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(app_domain,"app",sbmt::root_domain);

# endif //     UTILITIES__LOGGING_HPP
