# include <sbmt/token/indexed_token.hpp>

using namespace std;

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

int token_label::dict_index()
{
    static const int di_ = ios_base::xalloc();
    return di_;
}

////////////////////////////////////////////////////////////////////////////////

indexed_token_factory const* get_dict(std::ios_base& ios)
{
    return static_cast<indexed_token_factory*>(
               ios.pword(token_label::dict_index())
           );
}

////////////////////////////////////////////////////////////////////////////////

ostream& operator<< (std::ostream& os, token_label const& l)
{
    os.pword(token_label::dict_index()) = (void*)l.dict;
    return os;
}



////////////////////////////////////////////////////////////////////////////////

token_label::token_label(indexed_token_factory const& dict)
  : dict(&dict) {}
  
token_label::token_label(indexed_token_factory const* dict)
  : dict(dict) {}

////////////////////////////////////////////////////////////////////////////////

ostream& token_raw(ostream& os)
{
    return os << token_label(NULL);
}

////////////////////////////////////////////////////////////////////////////////

token_label::saver::saver(std::ios_base& ios) 
 : boost::io::ios_pword_saver(ios,dict_index()) {}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt


