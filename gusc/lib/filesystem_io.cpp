# include <gusc/filesystem_io.hpp>
# include <iostream>
# include <iostream>

namespace boost { namespace filesystem {

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream& os, const path& ph)
{
    os << ph.string();
    return os;
}

////////////////////////////////////////////////////////////////////////////////

std::istream& operator >> (std::istream& is, path& ph)
{
    std::string str;
    is >> str;
    ph = str;
    return is;
}

////////////////////////////////////////////////////////////////////////////////

} } // boost::filesystem

