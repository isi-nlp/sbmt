namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  \todo implement i/o manipulators to change pre/post decoration of tokens in
///        i/o streams.  could be something like
///            os << token_decorator<myobj> << mytoken
///        where myobj implements methods 
///            string pre(token) 
///        and 
///            string post(token)
///
////////////////////////////////////////////////////////////////////////////////
template <class CharT, class TraitsT>
std::basic_ostream<CharT,TraitsT>& 
operator << (std::basic_ostream<CharT,TraitsT>& out, token const& tok)
{
    /// not committed to this as actual output:  just need something to make
    /// my test code compile. --michael
    switch (tok.type()) {
        case token::foreign:
            out << "foreign:" << tok.label();;
            break;
        case token::native: 
            out << "native:" << tok.label();
            break;
        case token::tag:
            out << "tag:" << tok.label();
            break;
        case token::virtual_tag:
            out << "virtual_tag:" << tok.label();
            break;
        case token::top:
            out << "top:"<<top_token_text;
            break;
    }
    return out;
}

////////////////////////////////////////////////////////////////////////////////

inline bool operator==(token const& t1, token const& t2)
{
    return t1.idx == t2.idx;
}

inline bool operator!=(token const& t1, token const& t2)
{
    return !(t1 == t2);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
