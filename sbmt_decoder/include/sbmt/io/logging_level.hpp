#ifndef   SBMT_IO_LOGGING_LEVEL_HPP
#define   SBMT_IO_LOGGING_LEVEL_HPP

#include  <sbmt/io/logging_fwd.hpp>

namespace sbmt { namespace io {

////////////////////////////////////////////////////////////////////////////////

class logging_level_manip
{
public:
    logging_level_manip(logging_level lvl)
    : lvl(lvl) {}
    
    detail::logging_public_stream& operator()(logging_stream& logout) const;
private:
    logging_level lvl;
};

////////////////////////////////////////////////////////////////////////////////

detail::logging_public_stream& 
operator << (logging_stream& logout, logging_level_manip const& level_setter);

inline detail::logging_public_stream& 
operator << (logging_stream& logout, logging_level_manip& level_setter)
{
    return (logout << const_cast<logging_level_manip const&>(level_setter));
}

////////////////////////////////////////////////////////////////////////////////
///
/// you should be able to use these as follows:
/// \code
///     my_log << pedantic_msg << "this is a pedantic message" << endl;
///     my_log << error_msg << "this is an error message" << endl;
/// \endcode
///
////////////////////////////////////////////////////////////////////////////////

extern const logging_level_manip pedantic_msg;

extern const logging_level_manip debug_msg;

extern const logging_level_manip verbose_msg;

extern const logging_level_manip info_msg;

extern const logging_level_manip terse_msg;

extern const logging_level_manip warning_msg;

extern const logging_level_manip error_msg;

/////////////////////////////////////////////////////////////////////////////////



} } // namespace sbmt::io

#endif // SBMT_IO_LOGGING_LEVEL_HPP
