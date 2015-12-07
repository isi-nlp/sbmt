# if ! defined(GUSC__PATH_FROM_ID_HPP)
# define       GUSC__PATH_FROM_ID_HPP

# include <gusc/mod_sequence.hpp>
# include <iterator>
# include <vector>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class Int1, class Int2>
boost::filesystem::path path_from_id(Int1 id, Int2 modulo, Int1 max = 0)
{
    std::vector<Int1> v;
    boost::filesystem::path p;
    mod_sequence(id,modulo,back_inserter(v),max);
    
    typename std::vector<Int1>::iterator i = v.begin(), e = v.end();
    p = boost::lexical_cast<std::string>(*i);
    ++i;
    for (; i != e; ++i) {
        p = p / boost::lexical_cast<std::string>(*i);
    }
    return p;
} 

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc 

# endif //     GUSC__PATH_FROM_ID_HPP
