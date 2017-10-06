# if ! defined(EXTRA_RULES_HPP)
# define       EXTRA_RULES_HPP

# include <iosfwd>
# include <string>
# include <set>
# include <map>
# include <xrsparse/xrs.hpp>

namespace {
std::string const top_string="TOP";
std::string const glue_string="GLUE";
std::string const derivation_size_feat=" derivation-size=10^-1";
std::string const glue_rule_feat=" glue-rule=10^-1";
std::string const top_glue_rule_feat=" top-glue-rule=10^-1";
std::string const id_feat="id";
std::string const id_feature=" "+id_feat+"=";
std::string const attr_sep=" ###";
std::string const lhs_rhs_arrow=" -> ";
std::string const maroon_feature=" maroon=10^-1";
std::string const align_feat="align";
std::string const unary_align=" "+align_feat+"={{{[#s=1 #t=1 0,0]}}}";
std::string const binary_align=" "+align_feat+"={{{[#s=2 #t=2 0,0 1,1]}}}";
std::string const top_align_fss=" "+align_feat+"={{{[#s=2 #t=1 1,0]}}}";
} // unnamed namespace

////////////////////////////////////////////////////////////////////////////////

rule_data::rule_id_type
glue_rules( std::ostream& output
          , std::set<std::string> rhs_set
          , rule_data::rule_id_type max_id
          , bool align
          , std::string fs
          , bool add_headmarker
          , bool mira_features 
          , std::string efs = "");

////////////////////////////////////////////////////////////////////////////////

rule_data::rule_id_type
maroon_rules( std::ostream& out
            , std::set<std::string> const& rhs_set
            , std::set<std::string> const& corpus_lines 
	    , std::map<std::string,double> const& probs
            , rule_data::rule_id_type max_id
            , bool add_headmarker );

////////////////////////////////////////////////////////////////////////////////

# endif //     EXTRA_RULES_HPP


