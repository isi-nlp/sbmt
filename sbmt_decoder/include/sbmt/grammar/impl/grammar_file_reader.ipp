#include <set>
#include <string>

namespace sbmt {

namespace detail {

extern std::set<std::string> excluded_property_list;

////////////////////////////////////////////////////////////////////////////////

template <class ItrT, class OutItrT>
void extract_scores(ItrT begin, ItrT end, OutItrT& out)
{
    for (ItrT itr = begin; itr != end; ++itr) {
        if (excluded_property_list.find(itr->first) == 
            excluded_property_list.end()) {
            out = lexical_cast<score_t>(itr->second);
            ++out;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class CT>
void extract_scores(ns_RuleReader::Rule& r, CT& c)
{
    extract_scores< ns_RuleReader::Rule::attribute_map::iterator
                  , input_iterator<CT> > ( r.getAttributes()->begin()
                                         , r.getAttributes()->end()
                                         , std::inserter(c, c.end()) );
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt::detail

} // namespace sbmt