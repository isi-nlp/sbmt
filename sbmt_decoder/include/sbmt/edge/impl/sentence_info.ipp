
namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class ItrT> std::pair<span_t,bool>
join_span_range(span_string const& sstr, ItrT fbegin, ItrT fend)
{
    using namespace std;
    
    pair<span_t,bool> retval(span_t(0,0), false);
    span_index_t spn_left  = 0;
    span_index_t spn_right = 0;
    span_string::iterator itr = sstr.begin(),
                          end = sstr.end();
    bool initialized = false;
    
    for (; itr != end; ++itr) {
        if (!initialized) {
            if (itr->is_index()) {
                ItrT pos = fbegin + itr->get_index();
                assert(pos < fend);
                if (length(*(pos)) == 0) continue;
                spn_left = pos->left();
                spn_right = pos->right();
            } else {
                span_t const& ss = itr->get_span();
                spn_left = ss.left();
                spn_right = ss.right();
            }
            initialized = true;
        } else {
            if (itr->is_index()) {
                ItrT pos = fbegin + itr->get_index();
                assert(pos < fend);
                if (length(*pos) == 0) continue;
                if (pos->left() != spn_right) {
                    retval = make_pair(span_t(spn_left,spn_right),false);
                    return retval;
                }
                spn_right = pos->right();
            } else {
                span_t const& ss = itr->get_span();
                if (ss.left() != spn_right) {
                    retval = make_pair(span_t(spn_left,spn_right),false);
                    return retval;
                }
                spn_right = ss.right();
            }
        }
    }
    retval = make_pair(span_t(spn_left,spn_right),true);
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

