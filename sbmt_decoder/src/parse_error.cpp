# include <boost/regex.hpp>
# include <sbmt/search/parse_error.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/span.hpp>
# include <sbmt/logmath.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/algorithm/string/split.hpp>
# include <boost/algorithm/string/classification.hpp>

using namespace std;
namespace sbmt {

boost::regex synre("\\[([-0-9\\.]+),([-0-9\\.]+)\\]\\[(\\d+),(\\d+)\\]([-0-9]+)");
boost::regex virtre("\\[([-0-9\\.]+),([-0-9\\.]+)\\]\\[(\\d+),(\\d+)\\](V\\S+)"); 


deriv_note_ptr
parse(string const& str)
{
    boost::smatch m;
    span_t span;
    int synid;
    string tok;
    score_t total, inside;
    if (regex_match(str,m,synre)) {
        total = score_t(boost::lexical_cast<float>(m.str(1)),as_neglog10());
        inside  = score_t(boost::lexical_cast<float>(m.str(2)),as_neglog10());
        span  = span_t(boost::lexical_cast<int>(m.str(3)),boost::lexical_cast<int>(m.str(4)));
        synid = boost::lexical_cast<int>(m.str(5));
        return deriv_note_ptr(new deriv_note(span,synid,total,inside));
    } else if (regex_match(str,m,virtre)) {
        total = score_t(boost::lexical_cast<float>(m.str(1)),as_neglog10());
        inside  = score_t(boost::lexical_cast<float>(m.str(2)),as_neglog10());
        span  = span_t(boost::lexical_cast<int>(m.str(3)),boost::lexical_cast<int>(m.str(4)));
        tok   = m.str(5);
        return deriv_note_ptr(new deriv_note(span,tok,total,inside));
    }
    else {
        throw runtime_error("in parse_error module: could not parse "+str);
    }
}

boost::tuple<deriv_note_ptr,vector<string>::const_iterator>
parse( vector<string>::const_iterator curr
     , vector<string>::const_iterator end )
{
    assert(curr != end);
    if ((*curr)[0] != '(') {
        return boost::make_tuple(parse(*curr),curr + 1);
    } else {
        deriv_note_ptr rt = parse(curr->substr(1));
        ++curr;
        for (; curr != end and *curr != ")";) {
            deriv_note_ptr cd;
            boost::tie(cd,curr) = parse(curr,end);
            rt->children.push_back(cd);
        }
        assert(curr != end);
        return boost::make_tuple(rt,curr + 1);
    }
}

std::ostream& operator << (std::ostream& out, deriv_note const& d)
{
    out << d.span;
    if (d.syn) out << d.synid;
    else out << d.tok;
    return out;
}

std::ostream& operator << (std::ostream& out, deriv_note_ptr d)
{
    if (d->children.size() > 0) out << '(';
    out << *d;
    for (size_t x = 0; x != d->children.size(); ++x) {
        out << ' ' << d->children[x];
    }
    if (d->children.size() > 0) out << " )";
    return out;
}

deriv_note_ptr parse_binarized_derivation(std::string dstr)
{
    vector<string> dvec;
    boost::split(dvec,dstr,boost::is_space(),boost::token_compress_on);
    vector<string>::const_iterator c;
    deriv_note_ptr rt;
    boost::tie(rt,c) = parse(dvec.begin(),dvec.end());
    assert(c == dvec.end());
    return rt;
}

} // namespace sbmt
