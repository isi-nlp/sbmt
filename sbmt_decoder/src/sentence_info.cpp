# include <sbmt/edge/sentence_info.hpp>
# include <stdexcept>

namespace sbmt {

extern boost::regex index_string_regex;    
boost::regex span_string_regex("\\[.+\\]");

span_string::token::token(span_t const& s)
: var(s) 
{
    if (s.left() >= s.right()) 
        throw std::invalid_argument("non increasing spans");
}

span_t const& span_string::token::get_span() const { return boost::get<span_t>(var); }

unsigned int  span_string::token::get_index() const { return boost::get<unsigned int>(var); }

span_string::token::token(unsigned int x)
: var(x) {}

void throw_unless(bool x, const char* msg)
{
    if (!x) throw std::invalid_argument(msg);
}
    
span_string::span_string(std::string const& str)
{
    boost::char_delimiters_separator<char> sep(false,""," \t");
    boost::tokenizer<> toker(str,sep);
    boost::tokenizer<>::iterator itr = toker.begin();
    boost::tokenizer<>::iterator end = toker.end();
    
    boost::smatch s;
    bool seen_span = false;
    span_index_t idx = 0;
    
    for (; itr != end; ++itr) {
        if (boost::regex_match(*itr, s, index_string_regex)) {
            seen_span = false;
            unsigned int x =(boost::lexical_cast<unsigned int>(s.str(1)));
            vec.push_back(token(x));
        } 
        else if (boost::regex_match(*itr, s, span_string_regex)) {
            throw_unless(seen_span == false,"two spans in a row");
            span_t spn(boost::lexical_cast<span_t>(s.str(0)));
            throw_unless( idx <= spn.left() and spn.left() < spn.right()
                        , "non increasing spans" );
            seen_span = true;
            idx = spn.right();
            vec.push_back(token(spn));
        }
    }
}

void span_string::pop_back()
{
  vec.pop_back();
}

void span_string::pop_back(span_t const& s)
{
    if( not( vec.back().is_span() and 
	     (vec.back().get_span().left() <= s.left()) and
	     (vec.back().get_span().right() == s.right())
           )
      ) {
        std::stringstream sstr;
	sstr << "span cannot be popped. s="<<s<<", back()=";
	if(vec.back().is_span()) sstr << vec.back().get_span();
	else sstr << vec.back().get_index();
	throw std::logic_error(sstr.str());
    }
    span_t old = vec.back().get_span();
    if (s.left() == old.left()) vec.pop_back();
    else vec.back() = span_t(old.left(),s.left());
}

void span_string::push_back(token const& tok)
{
    if (vec.size() == 0) vec.push_back(tok);
    else if (vec.back().is_span() and tok.is_span()) {
        throw_unless( adjacent(vec.back().get_span(), tok.get_span())
                    , "non-adjacent consecutive spans" );
        vec.back() = combine(vec.back().get_span(),tok.get_span());
    }
    else vec.push_back(tok);
}

std::pair<int,int> gap(span_string const& sstr)
{
    std::pair<int,int> retval(0,0);
    span_string::iterator itr = sstr.begin(), end = sstr.end();
    bool first_seen = false, reversing = false;
    
    for (;itr != end; ++itr) {
        if (itr->is_span()) {
            if (first_seen) retval.first += length(itr->get_span());
        } else {
            if (!first_seen) {
                reversing = itr->get_index() != 0;
                first_seen = true;
            } else {
                if (reversing) retval.second = -1;
                else retval.second = 1;
                return retval;
            }
        }
    }
    return retval;
}

span_sig signature(span_string const& sstr, unsigned int idx)
{
    bool found = false, open_left = true, open_right = false;
    span_index_t left = 0;
    span_string::iterator itr = sstr.begin(), end = sstr.end();
    for (;itr != end; ++itr) {
        if (itr->is_index()) {
            if (found) open_right = true;
            else if (itr->get_index() == idx) found = true;
            else open_left = true;
        } else if (!found) {
            left = itr->get_span().right();
            open_left = false;
        } else {
            if (open_left and open_right) 
                return span_sig(left,itr->get_span().left(), span_sig::open_both);
            if (open_left)
                return span_sig(left,itr->get_span().left(), span_sig::open_left);
            if (open_right)
                return span_sig(left,itr->get_span().left(), span_sig::open_right);
            return span_sig(left,itr->get_span().left(), span_sig::open_none);
        }
    }
    if (open_left) return span_sig(left,USHRT_MAX,span_sig::open_both);
    else return span_sig(left,USHRT_MAX,span_sig::open_right);
}

std::ostream& operator<<(std::ostream& out, span_sig const& s)
{
    switch (s.st) {
        case span_sig::open_both:  
            out << '(' << s.lt << ',' << s.rt << ')'; 
            break;                       
        case span_sig::open_left:  
            out << '(' << s.lt << ',' << s.rt << ']';
            break;
        case span_sig::open_right: 
            out << '[' << s.lt << ',' << s.rt << ')';
            break;
        case span_sig::open_none:  
            out << '[' << s.lt << ',' << s.rt << ']';
            break;
    }
    return out;
}

////////////////////////////////////////////////////////////////////////////////

std::pair<lazy_join_sentence_iterator, lazy_join_sentence_iterator>
lazy_join_sentence( indexed_lm_string const& lmstr
                  , indexed_sentence const& str )
{
    return std::make_pair( lazy_join_sentence_iterator(lmstr,str)
                         , lazy_join_sentence_iterator(lmstr,str,false)
                         );
}

std::pair<lazy_join_sentence_iterator, lazy_join_sentence_iterator>
lazy_join_sentence( indexed_lm_string const& lmstr
                  , indexed_sentence const& str1
                  , indexed_sentence const& str2 )
{
    return std::make_pair( lazy_join_sentence_iterator(lmstr,str1,str2)
                         , lazy_join_sentence_iterator(lmstr,str1,str2,false)
                         );
}

////////////////////////////////////////////////////////////////////////////////

indexed_sentence join_sentence( indexed_lm_string const& lmstr
                              , indexed_sentence const& str ) 
{
    indexed_sentence retval(native_token);
    assert (str.type() == native_token);
    indexed_lm_string::iterator itr = lmstr.begin() ,
                                end = lmstr.end();
    
    for (; itr != end; ++itr) {
        if (itr->is_index()) {
            assert (itr->get_index() == 0);
            retval += str;
        } else {
            retval += itr->get_token();
        }
    }
    return retval;
}

std::pair<span_t,bool> join_spans(span_string const& sstr, span_t const& s)
{
    span_t vec[1] = { s };
    return join_span_range(sstr, &vec[0], &vec[0] + 1);
    /*
    using namespace std;
    
    span_index_t spn_left  = 0;
    span_index_t spn_right = 0;
    pair<span_t,bool> retval;
    bool initialized = false;
    span_string::iterator itr = sstr.begin(),
                          end = sstr.end();

    for (; itr != end; ++itr) {
        if (!initialized) {
            if (itr->is_index()) {
                throw_unless(itr->get_index() == 0,
                             "span_string index doesnt match" );
                if (length(s) == 0) continue;
                spn_left = s.left();
                spn_right = s.right();
            } else {
                span_t const& ss = itr->get_span();
                spn_left = ss.left();
                spn_right = ss.right();
            }  
            initialized = true;
        } else {
            if (itr->is_index()) {
                throw_unless( itr->get_index() == 0
                            , "span_string index doesnt match" );
                if (length(s) == 0) continue;
                if (s.left() != spn_right) {
                    retval = make_pair(span_t(spn_left,spn_right),false);
                    return retval;
                }
                spn_right = s.right();
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
    */
}

std::pair<span_t,bool> 
join_spans(span_string const& sstr, span_t const& s0, span_t const& s1)
{
    span_t vec[2] = {s0, s1};
    return join_span_range(sstr,vec, vec + 2);
    /*
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
                unsigned idx = itr->get_index();
                throw_unless( idx == 0 or idx == 1
                            , "span_string index doesnt match" );
                if (idx == 0) {
                    if (length(s0) == 0) continue;
                    spn_left = s0.left();
                    spn_right = s0.right();
                } else {
                    if (length(s1) == 0) continue;
                    spn_left = s1.left();
                    spn_right = s1.right();
                }
                
            } else {
                span_t const& ss = itr->get_span();
                spn_left = ss.left();
                spn_right = ss.right();
            }
            initialized = true;
        } else {
            if (itr->is_index()) {
                unsigned idx = itr->get_index();
                throw_unless( idx == 0 or idx == 1
                            , "span_string index doesnt match" );
                if (idx == 0) {
                    if (length(s0) == 0) continue;
                    if (s0.left() != spn_right) {
                        retval = make_pair(span_t(spn_left,spn_right),false);
                        return retval;
                    }
                    spn_right = s0.right();
                } else {
                    if (length(s1) == 0) continue;
                    if (s1.left() != spn_right) {
                        retval = make_pair(span_t(spn_left,spn_right),false);
                        return retval;
                    }
                    spn_right = s1.right();
                }
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
    */
}

////////////////////////////////////////////////////////////////////////////////

indexed_sentence join_sentence( indexed_lm_string const& lmstr
                              , indexed_sentence const& str0 
                              , indexed_sentence const& str1 ) 
{
    indexed_sentence retval(native_token);
    assert(str0.type() == native_token);
    assert(str1.type() == native_token);
    indexed_lm_string::iterator itr = lmstr.begin() ,
                                end = lmstr.end();
    
    for (; itr != end; ++itr) {
        if (itr->is_index()) {
            assert(itr->get_index() == 0 or itr->get_index() == 1);
            retval += itr->get_index() == 0 ? str0 : str1;
        } else {
            retval += itr->get_token();
        }
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

bool 
lazy_join_sentence_iterator::equal(lazy_join_sentence_iterator const& o) const
{
    return lm_str == o.lm_str and
           sent0  == o.sent0  and
           sent1  == o.sent1;
}

////////////////////////////////////////////////////////////////////////////////

void lazy_join_sentence_iterator::increment()
{
    switch (s) {
        case lm_active:    
            ++lm_str.first;
            if (lm_str.first != lm_str.second) {
                if(lm_str.first->is_index()) {
                    
                    if (lm_str.first->get_index() == 0) {
                        if (sent0.first != sent0.second)
                            s = sent0_active;
                        else increment();
                    } else if (lm_str.first->get_index() == 1) {
                        if (sent1.first != sent1.second)
                            s = sent1_active;
                        else increment();
                    }
                }
            }
            break;
        case sent0_active: 
            ++sent0.first;
            if (sent0.first == sent0.second) {
                s = lm_active;
                if (lm_str.first != lm_str.second) increment();
            }
            break;
        case sent1_active: 
            ++sent1.first;
            if (sent1.first == sent1.second) {
                s = lm_active;
                if (lm_str.first != lm_str.second) increment();
            }
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

lazy_join_sentence_iterator::lazy_join_sentence_iterator( 
                                 indexed_lm_string const& lm
                               , indexed_sentence const& s0
                               , indexed_sentence const& s1 
                               , bool start
                             ) 
: lm_str(start ? lm.begin() : lm.end(), lm.end())
, sent0(start ? s0.begin() : s0.end(), s0.end())
, sent1(start ? s1.begin() : s1.end(), s1.end())
, s(lm_active)
{
    init();
}

void lazy_join_sentence_iterator::init()
{
    if (lm_str.first != lm_str.second) {
        if (lm_str.first->is_index()) {
            if (lm_str.first->get_index() == 0) {
                if (sent0.first == sent0.second) increment();
                else s = sent0_active;
            } else if (lm_str.first->get_index() == 1) {
                if (sent1.first == sent1.second) increment();
                else s = sent1_active;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

lazy_join_sentence_iterator::lazy_join_sentence_iterator( 
                                 indexed_lm_string const& lm
                               , indexed_sentence const& s0
                               , bool start
                             ) 
: lm_str(start ? lm.begin() : lm.end(), lm.end())
, sent0(start ? s0.begin() : s0.end(), s0.end())
, sent1(s0.end(), s0.end())
, s(lm_active)
{
    init();
}

////////////////////////////////////////////////////////////////////////////////

indexed_token const& lazy_join_sentence_iterator::dereference() const
{
    switch (s) {
        case lm_active:    return lm_str.first->get_token(); break;
        case sent0_active: return *(sent0.first);            break;
        case sent1_active: return *(sent1.first);            break;
        default:           return lm_str.first->get_token(); break;
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
