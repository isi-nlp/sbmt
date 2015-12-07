# ifndef SBMT__SEARCH__LAZY__FIND_PARSE_ERROR_HPP
# define SBMT__SEARCH__LAZY__FIND_PARSE_ERROR_HPP 

# include <sbmt/search/parse_error.hpp>
# include <sbmt/search/lazy/chart.hpp>
# include <boost/tuple/tuple.hpp>
# include <vector>


namespace sbmt { namespace lazy {
    
template <class Chart, class Grammar>
struct find_parse_error_return {
    typedef std::vector<
      boost::tuple<
        deriv_note_ptr
      , deriv_note_ptr
      , typename chart_edge_equivalence<Chart>::type
      , std::vector<typename chart_edge_equivalence<Chart>::type>
      >
    > type;
};

template <class Chart, class Grammar>
typename find_parse_error_return<Chart,Grammar>::type
find_parse_error(deriv_note_ptr dptr, Chart const& chrt, Grammar const& gram)
{
    typedef typename find_parse_error_return<Chart,Grammar>::type retval_type;
    retval_type retval;
    indexed_token root;
    if (dptr->syn) root = gram.get_syntax(dptr->synid).lhs_root()->get_token();
    else root = gram.dict().find_virtual_tag(dptr->tok);
    
    //std::cerr << "*** fpe " << gram.dict().label(root) << " type: "<< root.type() << " span:" << dptr->span << '\n';
    
    typedef std::vector<typename chart_edge_equivalence<Chart>::type> cvector;
    cvector children;
    BOOST_FOREACH(deriv_note_ptr child, dptr->children) {
        deriv_note_ptr rchild,rparent;
        typename chart_edge_equivalence<Chart>::type req;
        cvector c;
        
        //boost::tie(rparent,rchild,req) = find_parse_error(child,chrt,gram);
        retval_type ret = find_parse_error(child,chrt,gram);
        BOOST_FOREACH(typename retval_type::value_type r,ret) {
            boost::tie(rparent,rchild,req,c) = r;
            if (req.size() == 0) {
                if (rparent->syn) retval.push_back(boost::make_tuple(rparent,rchild,req,c));
                else retval.push_back(boost::make_tuple(dptr,rchild,req,c));
            } else children.push_back(req);
        }
    }
    if (retval.size() > 0) return retval;

    //std::cerr << "*** fpe " << gram.dict().label(root) << " type: "<< root.type() << " span:" << dptr->span << " " << children.size() << " children found\n";
    // at this point, we know the children have matches
    typename chart_edge_equivalence<Chart>::type match;
    int matchcount = 0;
    BOOST_FOREACH(typename chart_cell<Chart>::type cell, chrt[dptr->span]) {
        if (cell_root(cell) == root) {
            BOOST_FOREACH(typename chart_edge_equivalence<Chart>::type eq, cell) {
                BOOST_FOREACH(typename chart_edge<Chart>::type const& e, eq) {
                    typename cvector::iterator citr = children.begin();
                    size_t x = 0;
                    for (; x != e.child_count(); ++x) {
                        if (not is_lexical(e.get_child(x).root())) {
                            if (citr == children.end() or e.get_children(x) != *citr) {
                                break;
                            } else {
                                ++citr;
                            }
                        }
                    }
                    if (citr == children.end() and x == e.child_count()) {
                        if (dptr->syn) {
                            if (dptr->synid == e.syntax_id(gram)) {
                                match = eq;
                                ++matchcount;
                                //return boost::make_tuple(dptr,eq);
                            }
                        } else {
                            match = eq;
                            ++matchcount;
                            //return boost::make_tuple(dptr,eq);
                        }
                    }
                }
            }
        }
    }
    assert(matchcount <= 1);
    //if (matchcount == 0) {
    //    std::cerr << "*** fpe " << gram.dict().label(root) << " type: "<< root.type() << " span:" << dptr->span << " no match\n";
    //}
    retval.push_back(boost::make_tuple(dptr,dptr,match,children));
    return retval;
}

} } // namespace sbmt::lazy

# endif
