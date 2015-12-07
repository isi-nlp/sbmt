#ifndef SBMT__TESTING_HPP
#define SBMT__TESTING_HPP

#include <sbmt/logging.hpp>

namespace sbmt {
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(test_domain,"test",root_domain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(test_domain,error);

inline bool testing(char const* msg,io::logging_level verbose=io::lvl_verbose)
{
    using namespace io;
    logging_stream& str = SBMT_LOGSTREAM(test_domain);
    if (logging_at_level(str,verbose)) {
        str << logging_level_manip(verbose) << msg << std::endl;
        return true;
    }
    return false;
}

}


#endif
