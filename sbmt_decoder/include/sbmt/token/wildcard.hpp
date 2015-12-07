# ifndef SBMT__TOKEN__WILDCARD_HPP
# define SBMT__TOKEN__WILDCARD_HPP

# include <boost/lexical_cast.hpp>
# include <sbmt/token/indexed_token.hpp>
# include <gusc/generator/lazy_sequence.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

class wildcard_generator {
public:
    typedef sbmt::indexed_token result_type;
    wildcard_generator(sbmt::indexed_token_factory& dict)
      : x(0)
      , dict(&dict) {}
    operator bool() const { return true; }
    sbmt::indexed_token operator()()
    {
        return dict->virtual_tag(boost::lexical_cast<std::string>(x++));
    }

private:
    int x;
    sbmt::indexed_token_factory* dict;
};

////////////////////////////////////////////////////////////////////////////////

struct wildcard_array : gusc::lazy_sequence<wildcard_generator>
{
    wildcard_array(sbmt::indexed_token_factory& dict)
     : gusc::lazy_sequence<wildcard_generator>(wildcard_generator(dict)) {}
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif // SBMT__TOKEN__WILDCARD_HPP
