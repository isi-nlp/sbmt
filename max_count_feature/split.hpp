# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <sstream>
# include <boost/lexical_cast.hpp>
# include <boost/cstdint.hpp>

typedef boost::uint8_t rule_offset_t;
using namespace std;

size_t count_splits(rule_data const& rd)
{
    size_t splits = 0;
    for (size_t x = 0; x != rd.rhs.size(); ++x) {
        if (rd.rhs[x].indexed) {
            if (x == 0 or rd.rhs[x-1].indexed) ++splits;
        }
    }
    if (rd.rhs.back().indexed) ++splits;
    return splits;
}
