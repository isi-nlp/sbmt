#include <sbmt/grammar/rule_input.hpp>

using namespace std;
using namespace ns_RuleReader;

namespace sbmt {

void throw_bad_rule_format(char const * const msg)
{
    bad_rule_format e(msg);
    throw e;
}

void throw_bad_rule_format(std::string const &msg)
{
    bad_rule_format e(msg);
    throw e;
}

} // namespace sbmt
