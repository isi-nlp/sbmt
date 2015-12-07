# include <boost/regex.hpp>
# include <string>

namespace {
boost::regex digits("[0-9]+");
std::string at("@");
}

std::string at_replace(std::string str)
{
  return regex_replace(str, digits, at);
}

