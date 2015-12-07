#ifndef SBMT__SEARCH__PARSE_ERROR_HPP
#define SBMT__SEARCH__PARSE_ERROR_HPP
# include <string>
# include <vector>
# include <sbmt/span.hpp>
# include <sbmt/logmath.hpp>
# include <boost/shared_ptr.hpp>
# include <iosfwd>

namespace sbmt {

struct deriv_note;
typedef boost::shared_ptr<deriv_note> deriv_note_ptr;
typedef std::vector<deriv_note_ptr> deriv_note_children;

struct deriv_note {
    bool syn;
    int synid;
    std::string tok;
    span_t span;
    score_t total;
    score_t inside;
    deriv_note_children children;
    
    deriv_note( span_t span
              , std::string tok
              , score_t total
              , score_t inside ) 
    : syn(false)
    , tok(tok)
    , span(span)
    , total(total)
    , inside(inside) {}
    
    deriv_note( span_t span
              , int synid
              , score_t total
              , score_t inside ) 
    : syn(true)
    , synid(synid)
    , span(span)
    , total(total)
    , inside(inside) {} 
};

std::ostream& operator<< (std::ostream& out, deriv_note_ptr d);
deriv_note_ptr parse_binarized_derivation(std::string dstr);

} // namespace sbmt

#endif