# if ! defined(UTILITIES__ANY_INFO_GRAMMAR_HPP)
# define       UTILITIES__ANY_INFO_GRAMMAR_HPP

# include <sbmt/edge/any_info.hpp>

# if PHOENIX_LIMIT < 6
# define PHOENIX_LIMIT 6
# endif

# if BOOST_SPIRIT_CLOSURE_LIMIT < 6
# define BOOST_SPIRIT_CLOSURE_LIMIT 6
# endif

# include <boost/spirit/include/classic.hpp>
# include <boost/spirit/include/phoenix1.hpp>
# include <boost/function.hpp>
/*
# include <boost/spirit/core.hpp>
# include <boost/spirit/utility/confix.hpp>
# include <boost/spirit/utility/distinct.hpp>
# include <boost/spirit/utility/escape_char.hpp>
# include <boost/spirit/attribute.hpp>
# include <boost/spirit/utility/lists.hpp>
# include <boost/spirit/phoenix/binders.hpp>
# include <boost/spirit/phoenix/primitives.hpp>
# include <boost/spirit/phoenix/operators.hpp>
# include <boost/spirit/phoenix/special_ops.hpp>
*/


# include <string>
# include <iostream>

# include <gusc/phoenix_helpers.hpp>

namespace sg {
    
template <class Type>
struct val_closure : boost::spirit::classic::closure<val_closure<Type>,Type>
{
    typename val_closure::member1 val;
};


////////////////////////////////////////////////////////////////////////////////

struct c_string_grammar 
    : boost::spirit::classic::grammar<c_string_grammar,val_closure<std::string>::context_t>
{
    
    template <class Scanner>
    struct definition
    {
        definition(c_string_grammar const& self)
        {
            using namespace boost::spirit::classic;
            using namespace phoenix;
            using namespace gusc;
            
            cstr 
                = lexeme_d[ confix_p('"',*(c_escape_ch_p[cstr.val += arg1]),'"')
                [
                     self.val = construct_<std::string>(
                                    cstr.val
                                  , 0
                                  , size_(cstr.val) - 1
                                )
                ]
                ]
                ;
        }
        //typedef typename boost::spirit::classic::lexeme_scanner<Scanner>::type lex_t;
        typedef val_closure<std::string>::context_t string_context_t;
        typedef boost::spirit::classic::rule<Scanner,string_context_t> cstr_t;
        cstr_t cstr;
        cstr_t const& start() const { return cstr; }   
    };
};

////////////////////////////////////////////////////////////////////////////////

struct optional_quote_string_grammar
    : boost::spirit::classic::grammar< optional_quote_string_grammar
                            , val_closure<std::string>::context_t
                            >
{
    template <class Scanner>
    struct definition {
        typedef typename boost::spirit::classic::lexeme_scanner<Scanner>::type lex_t;
        definition(optional_quote_string_grammar const& self)
        {
            using namespace boost::spirit::classic;
            using namespace phoenix;
            using namespace gusc;
            top
                = 
                lexeme_d[
                    (
                        c_str [ self.val = arg1 ]
                    )
                    |
                    ((*(anychar_p - (space_p|';'|','))) [ 
                            self.val = construct_<std::string>(arg1,arg2)
                        ] 
                    ) 
                ] 
                ;
        }
        boost::spirit::classic::rule<Scanner> const& start() const { return top; }
        c_string_grammar c_str;
        boost::spirit::classic::rule<Scanner> top;
    };
};

////////////////////////////////////////////////////////////////////////////////

struct use_info_grammar 
    : boost::spirit::classic::grammar<
          use_info_grammar
        , val_closure< std::vector<std::string> >::context_t
      >
{
    typedef val_closure< std::vector<std::string> >::context_t vec_t;
    template <class Scanner>
    struct definition {
        typedef typename boost::spirit::classic::lexeme_scanner<Scanner>::type lex_t;
        definition(use_info_grammar const& self)
        {
            using namespace boost::spirit::classic;
            using namespace gusc;
            using namespace phoenix;
            top 
                = 
                ( str_p("use-info") 
                >> lexeme_d[ info_g[ push_back_(self.val, arg1) ] 
                >> *(((ch_p(',')|space_p) >> *space_p) >> info_g[ push_back_(self.val, arg1) ]) ]
                )
                //[
                //    self.val = top.val
                //]
                ;
        }
        boost::spirit::classic::rule<Scanner,vec_t> const& start() const {return top;}
        boost::spirit::classic::rule<Scanner,vec_t> top;
        optional_quote_string_grammar info_g;
    };
};

////////////////////////////////////////////////////////////////////////////////

struct set_info_option_grammar : boost::spirit::classic::grammar<set_info_option_grammar>
{
    typedef val_closure<std::string> string_closure;
    
    struct command_closure 
      : boost::spirit::classic::closure<
          command_closure
        , std::string
        , std::string
        , std::string
        >
    {
        member1 name;
        member2 key;
        member3 value;
    };
    
    typedef boost::function<void(std::string,std::string,std::string)> set_option_cb;
    set_option_cb cb;
    
    set_info_option_grammar(set_option_cb f = sbmt::info_registry_set_option)
    : cb(f) {}
    
    template <class Scanner>
    struct definition 
    {
        definition(set_info_option_grammar const& self) 
        {
            using namespace boost::spirit::classic;
            using namespace phoenix;
            top 
                =   
                (   str_p("set-info-option")
                >>  (info_name[ top.name = arg1 ]) >> !(ch_p(',')) 
                >>  (info_key[ top.key = arg1 ]) >> !ch_p(',')
                >>  info_value[ top.value = arg1 ]
                )
                [ 
                    phoenix::bind(self.cb)(top.name,top.key,top.value)
                    //var(std::cout) << "set-info-option "
                    //               << '"' << top.name  << '"' << ' '
                    //               << '"' << top.key   << '"' << ' '
                    //               << '"' << top.value << '"' << std::endl
                ]
                ;
                
        }
        
        boost::spirit::classic::rule<Scanner,command_closure::context_t> const& start() const 
        { return top; }
        
        c_string_grammar c_str;
        boost::spirit::classic::rule<Scanner,command_closure::context_t> top;
        optional_quote_string_grammar info_name, info_key, info_value;
        
    };
};
}
////////////////////////////////////////////////////////////////////////////////

# endif //     UTILITIES__ANY_INFO_GRAMMAR_HPP
