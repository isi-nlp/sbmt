//# define BOOST_SPIRIT_DEBUG 1

# include <boost/spirit/include/classic_core.hpp>
# include <boost/spirit/include/classic_confix.hpp>
//# include <boost/spirit/utility/confix.hpp>
# include <iostream>
# include <string>

using namespace std;
using namespace boost::spirit::classic;


struct xrs_grammar : public grammar<xrs_grammar> {
    
    template <class Scanner> 
    struct definition {
        definition(xrs_grammar const& self)
        {
            s = (
            ////////////////////////////////////////////////////////////////////
            //
            //  xrs ::= lhs "->" rhs "###" features
            //  [ semantic conditions:
            //    - set of lhs indices match set of rhs indices.
            //  ]
            //
            ////////////////////////////////////////////////////////////////////
            xrs = lhs >> "->" >> rhs >> "###" >> features,
            
            ////////////////////////////////////////////////////////////////////
            //
            //  lhs ::= nt '(' ((lhs | indexed_nt)+ | terminal) ')'
            //  [ semantic conditions:
            //    - sequence of indices must start at 0 and increase 
            //      left to right without gaps
            //  ]
            //
            ////////////////////////////////////////////////////////////////////
            lhs = nt >> '(' >> ((+(lhs | indexed_nt)) | terminal) >> ')',
            
            ////////////////////////////////////////////////////////////////////
            //
            //  rhs ::= ( index | terminal )+
            //
            ////////////////////////////////////////////////////////////////////
            rhs = +(index | terminal),
            
            ////////////////////////////////////////////////////////////////////
            //
            //  features ::= feature+
            //  [ semantic conditions:
            //    - one of the features has a label "id" and an integer value
            //  ]
            //
            ////////////////////////////////////////////////////////////////////
            features = +feature,
            
            //=== lexical rules ================================================
            
            ////////////////////////////////////////////////////////////////////
            //
            //  terminal ::= ('"' (char - '"')+ '"') | ('"' '"' '"')
            //
            ////////////////////////////////////////////////////////////////////
            terminal = lexeme_d[ "\"\"\"" | confix_p('"',*anychar_p,'"') ],
            
            ////////////////////////////////////////////////////////////////////
            //
            //  nt ::= (anychar - ('(' | ')' | '"'))
            //
            ////////////////////////////////////////////////////////////////////
            nt = lexeme_d[ +(anychar_p - (space_p | '(' | ')' | '"')) ],
            
            ////////////////////////////////////////////////////////////////////
            //
            //  index ::= 'x' unsigned_number
            //
            ////////////////////////////////////////////////////////////////////
            index = lexeme_d[ 'x' >> uint_p ],
            
            ////////////////////////////////////////////////////////////////////
            //
            //  indexed_nt ::= index ':' nt
            //
            ////////////////////////////////////////////////////////////////////
            indexed_nt = lexeme_d[ index >> ':' >> nt ],
            
            ////////////////////////////////////////////////////////////////////
            //
            //  feature ::= key '=' (simple_value | compound_value)
            //
            ////////////////////////////////////////////////////////////////////
            feature = lexeme_d[ key >> '=' >> (compound_value | simple_value) ],
            
            ////////////////////////////////////////////////////////////////////
            //
            //  key ::= alpha *(alpha_num | '-' | '_')
            //
            ////////////////////////////////////////////////////////////////////
            key = lexeme_d[ alpha_p >> *(alnum_p | '-' | '_') ],
            
            ////////////////////////////////////////////////////////////////////
            //
            //  simple_value ::= (~space)+
            //
            ////////////////////////////////////////////////////////////////////
            simple_value = lexeme_d[ +(~space_p) ],
            
            ////////////////////////////////////////////////////////////////////
            //
            //  compound_value ::= "{{{" (char - "}}}")* "}}}"
            //
            ////////////////////////////////////////////////////////////////////
            compound_value = lexeme_d[ confix_p("{{{",*anychar_p,"}}}") ]
            
            );
        }
        
        rule<Scanner> s;
        
        subrule<0>  xrs;
        subrule<1>  lhs;
        subrule<2>  rhs;
        subrule<3>  features;
        subrule<4>  terminal;
        subrule<5>  nt;
        subrule<6>  index;
        subrule<7>  indexed_nt;
        subrule<8>  feature;
        subrule<9>  key;
        subrule<10> simple_value;
        subrule<11> compound_value;
               
        rule<Scanner> const& start() const { return s; }
    };
};

int main() 
{
    std::ios_base::sync_with_stdio(false);
    xrs_grammar xrs;
    string line;
    while (getline(cin,line)) {
        parse_info<> info = parse(line.c_str(), xrs, space_p);
        if (info.full) {
            cout << line << endl;
        } else {
            cerr << "[parse failure at "<< info.stop << "] " << line << endl;
        }
    }
    return 0;
}

/* 
======== notes =================================================================

still have to test lots of corner cases.

suspect that in rule, RuleReader requires that "->" and "###" actually be
" -> " and " ### ", and that these sequences not appear anywhere else in the 
rule, owing to its stupid two pass read.

probably do do not have the exact allowable pattern for non-terminals (nt) or 
feature keys (key)

*/
