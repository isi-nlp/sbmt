# if ! defined(SBMT__LOGGING_HPP)
# define       SBMT__LOGGING_HPP

# include <sbmt/io/logging_fwd.hpp>
# include <sbmt/io/logging_stream.hpp>
# include <sbmt/io/logging_macros.hpp>
# include <sbmt/io/formatted_logging_msg.hpp>
# include <sbmt/io/logfile_registry.hpp>
# include <sbmt/io/log_auto_report.hpp>

namespace sbmt {

SBMT_REGISTER_LOGGING_DOMAIN_NAME(root_domain,"sbmt");

}

# endif //     SBMT__LOGGING_HPP

