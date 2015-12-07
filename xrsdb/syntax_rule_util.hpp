# if ! defined(XRSDB__SYNTAX_RULE_UTIL_HPP)
# define       XRSDB__SYNTAX_RULE_UTIL_HPP

# include <string>
# include <vector>
# include <sbmt/token.hpp>
# include <gusc/generator/lazy_sequence.hpp>
# include <boost/lexical_cast.hpp>

namespace xrsdb {

struct hop_compare {
    struct op {
        op(sbmt::indexed_token const& k) : k(k) {}
        
        bool operator()(sbmt::indexed_token const& t1,sbmt::indexed_token const& t2) const
        {
            if (t1.type() != sbmt::virtual_tag_token or t2.type() != sbmt::virtual_tag_token) {
                return t1 < t2;
            } else if (t1.index() > k.index() or t2.index() > k.index()) {
                return t1 < t2;
            } else {
                return false;
            }
        }
        sbmt::indexed_token k;
    };
    
    op operator()(sbmt::indexed_token const& k) const { return op(k); }
};

////////////////////////////////////////////////////////////////////////////////

std::vector<sbmt::indexed_token>
rhs_from_string(std::string const & rule, sbmt::indexed_token_factory & dict);

////////////////////////////////////////////////////////////////////////////////

std::vector<sbmt::indexed_token>
from_string( std::string const& sss
           , sbmt::indexed_token_factory& tf );

////////////////////////////////////////////////////////////////////////////////

std::vector<sbmt::indexed_token>
from_sig( std::string const& sss
        , sbmt::indexed_token_factory& tf );

////////////////////////////////////////////////////////////////////////////////

std::vector<sbmt::indexed_token>
forcetree_sig( std::string const& sss
             , sbmt::indexed_token_factory& tf );

////////////////////////////////////////////////////////////////////////////////
             
std::vector<sbmt::indexed_token>
stateful_forcetree_sig( std::string const& sss
                      , sbmt::indexed_token_factory& tf );

////////////////////////////////////////////////////////////////////////////////

} // namespace xrsdb

# endif //     XRSDB__SYNTAX_RULE_UTIL_HPP
