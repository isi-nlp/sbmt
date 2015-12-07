# include <sbmt/grammar/rule_feature_constructors.hpp>

namespace sbmt {

bool bool_constructor::make_bool(std::string const& str) const
{
    if (str == "yes" || str == "true" || str == "1") {
        return true;
    } else if (str == "no" || str == "false" || str == "0") {
        return false;
    } else {
        throw std::runtime_error("cannot parse "+str+" as boolean expression");
    }
    return false;
}

} // namespace sbmt
