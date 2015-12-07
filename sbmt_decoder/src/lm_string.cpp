#include <sbmt/grammar/lm_string.hpp>

namespace sbmt {

boost::regex index_string_regex("[x]?([0-9]+)");

boost::regex lexical_string_regex("\"([^\"]+|\")\"");

boost::regex nonterminal_string_regex("^\\((.*)\\)$");

} // namespace sbmt

