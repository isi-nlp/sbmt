#include <sbmt/io/logging_level.hpp>
#include <sbmt/io/logging_stream.hpp>
#include <sbmt/io/detail/logging_public_stream.hpp>

using namespace std;

namespace sbmt { namespace io {

////////////////////////////////////////////////////////////////////////////////

static const int num_levels = logging_level::inherit + 1;
static const string lvl_string[num_levels] =
{
    string("none")
  , string("error")
  , string("warning")
  , string("terse")
  , string("info")
  , string("verbose")
  , string("debug")
  , string("pedantic")
    , string("<inherit>")
};

////////////////////////////////////////////////////////////////////////////////

detail::logging_public_stream&
operator << (logging_stream& logout, logging_level_manip const& lvl_set)
{
    return lvl_set(logout);
}

////////////////////////////////////////////////////////////////////////////////

detail::logging_public_stream&
logging_level_manip::operator()(logging_stream& log) const
{
    log.stream().set_logging_level(lvl);
    return log.stream();
}

////////////////////////////////////////////////////////////////////////////////

extern const logging_level_manip pedantic_msg(lvl_pedantic);

extern const logging_level_manip debug_msg(lvl_debug);

extern const logging_level_manip verbose_msg(lvl_verbose);

extern const logging_level_manip info_msg(lvl_info);

extern const logging_level_manip terse_msg(lvl_terse);

extern const logging_level_manip warning_msg(lvl_warning);

extern const logging_level_manip error_msg(lvl_error);

////////////////////////////////////////////////////////////////////////////////

void logging_level::read(istream& is)
{
    string str;
    is >> str;
    string const* pos = std::find( lvl_string
                                 , lvl_string + num_levels
                                 , str);
    if (pos != lvl_string + num_levels) lvl = level(pos - lvl_string);
    else {
        std::cout << "big fucking failure!" << std::endl;
        is.setstate(ios::failbit);
    }
}

////////////////////////////////////////////////////////////////////////////////

void logging_level::write(ostream& os) const
{
    assert(lvl >= 0 and lvl <= num_levels);
    os << lvl_string[lvl];
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::io
