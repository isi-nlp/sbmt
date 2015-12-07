# if ! defined(SBMT__BUITIN_INFOS__RULE_FEATURE_CONSTRUCTORS_HPP)
# define       SBMT__BUITIN_INFOS__RULE_FEATURE_CONSTRUCTORS_HPP

# include <string>
# include <sbmt/grammar/lm_string.hpp>
# include <sbmt/span.hpp>
# include <boost/lexical_cast.hpp>

namespace sbmt {
    
struct span_constructor {
    template <class Dictionary>
    span_t operator()(Dictionary& dict, std::string const& str) const
    {
        return boost::lexical_cast<span_t>(str);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct bool_constructor {
    template <class Dictionary>
    bool operator()(Dictionary& dict, std::string const& str) const
    {
        return make_bool(str);
    }
private:
    bool make_bool(std::string const& str) const;
};

////////////////////////////////////////////////////////////////////////////////

struct lm_string_constructor {
    template <class Dictionary>
    lm_string<typename Dictionary::token_type>
    operator()(Dictionary& dict, std::string const& str) const
    {
        return lm_string<typename Dictionary::token_type>(str,dict);
    }
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__BUITIN_INFOS__RULE_FEATURE_CONSTRUCTORS_HPP
