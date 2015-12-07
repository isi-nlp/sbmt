# if ! defined(GUSC__FILESYSTEM_IO_HPP)
# define       GUSC__FILESYSTEM_IO_HPP

# include <boost/filesystem/path.hpp>
# include <iosfwd>

/// \todo these iostream operations are added to boost::filesystem official 
/// library in 1.34.0, so this inclusion should be conditionally removed when
/// boost/version.hpp indicates 1.34.0 or above
namespace boost { namespace filesystem {

std::ostream& operator << (std::ostream& os, const path& ph);
std::istream& operator >> (std::istream& is, path& ph);

} } // boost::filesystem

# endif //     GUSC__FILESYSTEM_IO_HPP
