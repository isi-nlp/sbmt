# if ! defined(XRSDB__DB_USAGE_HPP)
# define       XRSDB__DB_USAGE_HPP

# include <boost/enum.hpp>

namespace xrsdb {

BOOST_ENUM_VALUES(
    db_usage
  , const char*
  , (decode)("decode")
    (raw_sig)("raw-sig")
    (force_estring)("force-estring")
    (force_etree)("force-etree")
    (force_stateful_etree)("force-stateful-etree")
);

} // namespace xrsdb

# endif //     XRSDB__DB_USAGE_HPP
