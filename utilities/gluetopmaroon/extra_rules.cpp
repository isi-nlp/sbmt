# include "extra_rules.hpp"
# include <iostream>
# include <sstream>
# include <string>
# include <set>
# include <boost/regex.hpp>
# include <boost/algorithm/string.hpp>
# include <boost/foreach.hpp>
# include <boost/lexical_cast.hpp>
# include <string>

using namespace std;

std::string escape_feat(std::string const& raw)
{
    std::string ret;
    BOOST_FOREACH(char c, raw) {
        if (boost::algorithm::is_alnum()(c) or c == '-' or c == '_')
            ret.push_back(c);
        else 
            ret += "ESC_" + boost::lexical_cast<std::string>((unsigned int)(c));
    }
    return ret;
}

string glue_var_feat(string const& lbl) 
{
    return " glue-" + escape_feat(lbl) + "=10^-1";
}

string fss_glue( string lhs
               , string fs
               , string rhs
               , rule_data::rule_id_type id
               , string align
               , bool add_headmarker
               , bool mira_features
               )
{
    stringstream sstr;
    sstr << lhs << "(x0:" << rhs << ")"
         << lhs_rhs_arrow 
         << "\"" << fs << "\" x0"
         << attr_sep 
         << glue_rule_feat 
         <<( mira_features ? glue_var_feat(rhs) : "")
         << derivation_size_feat
         << id_feature 
         << id 
         << align;
    if(add_headmarker){ sstr<< " headmarker={{{ R(H) }}}";}
    return sstr.str();
}

string binary_glue( string lhs
                  , string rhs1
                  , string rhs2
                  , rule_data::rule_id_type& id
                  , string align
                  , bool add_headmarker
                  , bool mira_features 
                  )
{
    stringstream sstr;
    if(!add_headmarker){
        sstr << lhs << "(x0:" << rhs1 << " x1:" << rhs2 <<")"
             << lhs_rhs_arrow 
             << "x0 x1"
             << attr_sep 
             << glue_rule_feat 
             << ( mira_features ? glue_var_feat(rhs2) : "")
             << derivation_size_feat
             << id_feature 
             << id 
             << align;
    } else {
// we use one-way glue rules.
#if 0
        sstr << lhs << "(x0:" << rhs1 << " x1:" << rhs2 <<")"
             << lhs_rhs_arrow 
             << "x0 x1"
             << attr_sep 
             << glue_rule_feat 
             << derivation_size_feat
             << id_feature 
             << id 
             << align
             << " headmarker={{{ R(H D) }}}"
             <<endl;

        ++id;
#endif

        sstr << lhs << "(x0:" << rhs1 << " x1:" << rhs2 <<")"
             << lhs_rhs_arrow 
             << "x0 x1"
             << attr_sep 
             << glue_rule_feat 
             << ( mira_features ? glue_var_feat(rhs2) : "")
             << derivation_size_feat
             << id_feature 
             << id 
             << align
             << " headmarker={{{ R(D H) }}}";
    }
    return sstr.str();
}

rule_data::rule_id_type
glue_rules( ostream& output
          , set<string> rhs_set
          , rule_data::rule_id_type max_id
          , bool align
          , string fs
          , bool add_headmarker
          , bool mira_features
          , string efs )
{
    string glue0 = "GLUE0";
    string glue1 = "GLUE1";
    string glue  = "GLUE";
    string talign, ualign, balign;
    if (align) {
        talign = top_align_fss;
        balign = binary_align;
        ualign = unary_align;
    }
    string fss_rhs = "\"" + fs + "\" x0";
    rhs_set.insert("TOP");
    set<string>::iterator itr=rhs_set.begin(), end=rhs_set.end();
    for (;itr != end; ++itr) {
        output << fss_glue(glue0,fs,*itr,++max_id,talign, add_headmarker, mira_features) 
               << " lm_string={{{ \"<s>\" 0 }}}" << endl;
        output << binary_glue(glue,glue0,*itr,++max_id,balign, add_headmarker, mira_features) 
               << endl;
        output << binary_glue(glue,glue,*itr,++max_id,balign, add_headmarker, mira_features) 
               << endl;
    }
    output << top_string << "(x0:" << glue_string << ")"
           << lhs_rhs_arrow << "x0"; 
    if (efs != "") output << ' ' << '"' << efs << '"';
    output << attr_sep
           << glue_rule_feat << top_glue_rule_feat << derivation_size_feat
           << id_feature << (++max_id) << ualign ;

    if(add_headmarker){ output << " headmarker={{{ R(D) }}}";}
    output << " lm_string={{{ 0 \"</s>\" }}}";

    output << endl;
           
    return max_id;
}

rule_data::rule_id_type 
maroon_rules( ostream& out
            , set<string> const& rhs_set
            , set<string> const& corpus_lines 
            , rule_data::rule_id_type max_id
            , bool add_headmarker )
{
    using namespace std;
    using namespace boost;

    set<string> rule_set;  
    regex split_re("\\s+");


    set<string>::const_iterator line_itr = corpus_lines.begin(),
                                line_end = corpus_lines.end();
    for (; line_itr != line_end; ++line_itr) {
        sregex_token_iterator itr( line_itr->begin()
                                 , line_itr->end()
                                 , split_re
                                 , -1
                                 );
        sregex_token_iterator end;
        for (; itr != end; ++itr) if (*itr != "<foreign-sentence>") {
            set<string>::const_iterator symbol_itr = rhs_set.begin(),
                                        symbol_end = rhs_set.end();

            for (; symbol_itr != symbol_end; ++symbol_itr) {
                string rule = *symbol_itr + "(x0:" + *symbol_itr + ") -> \""
                    + itr->str() + "\" x0 ### maroon=10^-1 maroon-" + escape_feat(*symbol_itr) + "=10^-1 ";
                if(add_headmarker){ rule += "headmarker={{{ R(H) }}} ";}
                rule_set.insert(rule);
            }
        }
    }

    set<string>::iterator rule_itr = rule_set.begin(), 
                          rule_end = rule_set.end();
    for (; rule_itr != rule_end; ++rule_itr) {
        out << *rule_itr << "id=" << ++max_id << endl;
    }
    return max_id;
}
