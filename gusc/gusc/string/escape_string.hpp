# if ! defined(GUSC__STRING__ESCAPE_STRING_HPP)
# define       GUSC__STRING__ESCAPE_STRING_HPP

# include <string>

namespace gusc {

std::string escape_c(std::string const& str);
std::string unescape_c(std::string const& str);

} // namespace gusc

# endif //     GUSC__STRING__ESCAPE_STRING_HPP
