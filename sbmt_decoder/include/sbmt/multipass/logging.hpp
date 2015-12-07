#ifndef SBMT__MULTIPASS__LOGGING_HPP
#define SBMT__MULTIPASS__LOGGING_HPP

# include <sbmt/logging.hpp> // for root_domain and logging interface

namespace sbmt {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(multipass,root_domain);
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME( limit_cells
                                       , "limit_cells"
                                       , multipass );
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME( compare_cells
                                       , "compare_cells"
                                       , multipass );

} // namespace sbmt



#endif
