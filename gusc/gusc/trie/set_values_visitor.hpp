# if ! defined(GUSC__TRIE__SET_VALUES_VISITOR_HPP)
# define       GUSC__TRIE__SET_VALUES_VISITOR_HPP

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class Transform>
struct set_values_visitor
{
    template <class Trie>
    void at_state(Trie& tr, typename Trie::state s) const
    {
        tr.set_value(s,f(tr.value(s)));
    }
    
    template <class Trie>
    void begin_children(Trie& tr, typename Trie::state s) const {}
    
    template <class Trie>
    void end_children(Trie& tr, typename Trie::state s) const {}
    
    set_values_visitor(Transform const& f) : f(f) {}
    
    Transform f;
};

////////////////////////////////////////////////////////////////////////////////

template <class Transform>
set_values_visitor<Transform> 
set_values_via(Transform const& f) 
{
    return set_values_visitor<Transform>(f);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__TRIE__SET_VALUES_VISITOR_HPP
