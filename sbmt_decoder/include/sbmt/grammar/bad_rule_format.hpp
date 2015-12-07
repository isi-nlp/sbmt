#ifndef   SBMT_GRAMMAR_BAD_RULE_FORMAT_HPP
#define   SBMT_GRAMMAR_BAD_RULE_FORMAT_HPP

#include <exception>
#include <string>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

class bad_rule_format : public std::exception
{
public:
    bad_rule_format(char const* const msg="not a valid brf rule"):msg(msg) {}
    bad_rule_format(std::string const& m):s(m),msg(s.c_str()) {}
    virtual const char* what() const throw()
    { return msg; }
    
    virtual ~bad_rule_format() throw() {}
private:
    std::string s;
    const char * msg;
};

////////////////////////////////////////////////////////////////////////////////

void throw_bad_rule_format(char const * const msg);
void throw_bad_rule_format(std::string const& msg);

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT_GRAMMAR_BAD_RULE_FORMAT_HPP
