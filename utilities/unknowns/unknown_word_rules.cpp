#define GRAEHL__SINGLE_MAIN
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/time_space_report.hpp>
#include <graehl/shared/program_options.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/pairlist.hpp>
#include <graehl/shared/printlines.hpp>
#include <graehl/shared/maybe_update_bound.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
//#include <graehl/shared/set_difference.hpp>
#include <set>
#include <graehl/shared/string_match.hpp>
#include <sstream>
#include <xrsparse/xrs.hpp>

//TODO:    string native_unk="@UNKNOWN@"; disagrees w/ native_epsilon(), native_unknown_word(), etc. (we're using lmstring to get our desired results in decoding, and have to doctor nbest outputs ourselves after)

std::ostream& operator<< (std::ostream& out, std::vector<std::string> const& s)
{
  std::vector<std::string>::const_iterator itr = s.begin(), end = s.end();
  if (itr != end) {
    out << *itr;
    ++itr;
  }
  for (;itr != end; ++itr) out << " " << *itr;
  return out;
}


/* emulates binal:  example word->word xltn

binal input:

NNP("indonesia") -> "c1" ### derivation-id=128 *FEATURES HERE*



binal output:

NNP:VL: NNP("indonesia") -> "c1" ### id=128 *FEATURES HERE* virtual_label=no complete_subtree=yes lm_string={{{"indonesia"}}} sblm_string={{{}}} lm=yes sblm=yes rule_file_line_number=1 rhs={{{"c1"}}}

 */


/* implementation: won't use rulereader, so we're going to need revision if we change rule formats.

*** assumptions: no spaces (escaped or otherwise) in foreign tokens.  id=123 followed by space.  no token contains "}}} (or if it does, it has a space!)
***             string line,rhs_l=" rhs={{{\"",rhs_r="\"}}}",id_l=" id=",id_r=" ",attr=" ###",real_rule=" virtual_label=no";
*** uses those left and right brackets;  checks AFTER attr separator for real_rule, then grabs rhs and id.

// input: will check for rhs={{{"c1"}}} somewhere on the line (not necessarily at end).  if we see it, then we consider foreign word c1 (appearing unquoted in the foreign file) covered
// output:

*/
using namespace std;
using namespace boost;
using namespace graehl;

char const* usage_str();


std::string const &indicator_val="10^-1";

template <class O>
void indicator_feature(O&o,std::string const& name)
{
    o << " "<<name<<"="<<indicator_val;
}


bool doing(ostream_arg const&on,char const* what)
{
    if (!on.is_none()) {
        cerr << "Printing " << what << " to " << on << "...\n";
        return true;
    }
    return false;    
}

void warn(std::string const& what,char const* header="WARNING: ")
{
    cerr << "\n"<<header<<what<<"\n";
}


void warn_line(std::string const& what,std::string const& line,unsigned line_no,char const* header="WARNING: ")
{
    using namespace std;
    warn(what,header);
    cerr << "\n" << header << what << " on LINE";
    if (line_no)
        cerr << " #"<<line_no;
    cerr << ':' << line << "\n";
}

void throw_error(std::string const& what)
{
    warn(what,"ERROR: ");
    throw std::runtime_error(what);
}

typedef std::list< std::pair<std::string,std::string> > pair_list_t;
pair_list_t pair_list_construct(std::string str)
{
    typedef boost::char_separator<char> sep_t;
    typedef boost::tokenizer<sep_t> toker_t;
    pair_list_t pl;
    sep_t outer_sep(",");
    sep_t inner_sep(":");
    toker_t outer(str,outer_sep);
    for (toker_t::iterator i = outer.begin(); i != outer.end(); ++i) {
        toker_t inner(*i,inner_sep);
        toker_t::iterator j = inner.begin();
        std::string s1 = *j;
        ++j;
        std::string s2;
        if (j != inner.end()) s2 = *j;
        pl.push_back(make_pair(s1,s2));
    }
    return pl;
}

pair_list_t pair_list_construct_from_file(std::string filename)
{
    typedef boost::char_separator<char> sep_t;
    typedef boost::tokenizer<sep_t> toker_t;
    pair_list_t pl;
    ifstream in(filename.c_str());
    if(!in) {  std::cerr<<"Cannot open "<<filename<<" for reading!\n"; exit(1); return pl;}

    sep_t inner_sep(":");
    string line;
    string ss;
    while(getline(in, line)){
        toker_t inner(line,inner_sep);
        toker_t::iterator j = inner.begin();
        std::string ss = *j;
        std::istringstream ist(ss);
        std::string s1;
        ist >> s1;
        ++j;
        if (j != inner.end()) ss = *j;
        std::istringstream ist1(ss);
        std::string s2;
        ist1 >> s2;
        pl.push_back(make_pair(s1,s2));
    }
    return pl;
}

int main(int argc, char** argv)
{
    using namespace boost::program_options;
    typedef options_description OD;
    istream_arg template_file,rule_file,foreign_file("-");
    ostream_arg output_file("-"),template_log("-2"),single_word_rules_log("-0"),unknown_words_log("-0");
    bool help=false,copy_rules=false,lm_skip_unk=true,tm_skip_unk=false;
    string native_unk="@UNKNOWN@";
    string foreign_unk_template="\"\"UNK_F_HERE\"\"",id_template="\"\"RULE_ID_HERE\"\"";
    string unk_indicator_f="unk-rule",unk_prob_f="unk-prob",foreign_length_f="foreign-length";
    //typedef pairlist_c<string,string,',',':'> unk_tags_t;
    typedef pair_list_t unk_tags_t;
    string default_unk_tags="NNP";
    boost::int64_t id_offset=0;
    boost::int64_t max_id=0;
    boost::int64_t id_origin=0;
    unk_tags_t unk_tags = pair_list_construct(default_unk_tags);
    string unk_tags_s=default_unk_tags;
    string unk_tags_file = "";
    bool xrs_rule_format = false;
//    unk_tags_t unk_tags=boost::lexical_cast<unk_tags_t>(default_unk_tags);
    //std::istringstream is(default_unk_tags);
    //unk_tags.read(is);

    vector<string> ignored_foreigns;
    ignored_foreigns.push_back(string("<foreign-sentence>"));
    
    typedef vector<string> templates_t;
    templates_t templates;
    
    OD general;
    OD oldrule("Existing rule-file options");
    OD rtemp("Rule template options");
    OD rtemp_generate("Template generation (list of tag:prob,tag2,tag3:prob2,...)");
    ostream &log=cerr;
    general.add_options()
        ("help,h", bool_switch(&help), "show usage/documentation")
        ("foreign-file,f", defaulted_value<istream_arg>(&foreign_file), "read foreign words (each one not known in rule-file generates rules from templates")
        ("output,o", defaulted_value<ostream_arg>(&output_file), "output binary rules file")
        ("unknown-log",  defaulted_value<ostream_arg>(&unknown_words_log), "copy here: list of unknown foreign words")
        ("id-offset", defaulted_value<boost::int64_t>(&id_offset), "use id=[id-offset]+max+1, where max is the highest seen in [rule-file] or 0, whichever is more")
        ("id-origin", defaulted_value<boost::int64_t>(&id_origin), "if [id-origin] is greater than max seen ruleid, then ignore --id-offset and start ruleids here")
        ("ignore-foreign",value< vector<string> >(&ignored_foreigns), "do not make an unknown rule for this foreign word (defaults to [<foreign-sentence>])")
        ;
    oldrule.add_options()
        ("rule-file,r", defaulted_value<istream_arg>(&rule_file), "preexisting *binary* rules")
        ("copy-rules,c", defaulted_value<bool>(&copy_rules), "repeat preexisting rules on output (before new rules)")
        ("single-word-rules-log", defaulted_value<ostream_arg>(&single_word_rules_log), "write preexisting single-foreign word rules to this file")
            ;
    rtemp.add_options()
        ("template-file,t", defaulted_value<istream_arg>(&template_file), "binarized rule templates (optional)")
        ("unk-template", defaulted_value<string>(&foreign_unk_template), "in templates, [foreign-unk-template] -> actual (unquoted) unknown foreign word")
        ("id-template", defaulted_value<string>(&id_template), "in templates, [id-template] -> rule id")
        ("template-log", defaulted_value<ostream_arg>(&template_log), "write final templates here")
        ;
    rtemp_generate.add_options()
        ("unk-tags", defaulted_value<string>(&unk_tags_s), "list of key:val,key:val,... with key=native tag (val=prob) - may omit :val part")
        ("unk-tag-file", defaulted_value<string>(&unk_tags_file), "file containing lines of form key:val with key=native tag (val=prob) - may omit :val part")
        ("lm-skip-unk", defaulted_value<bool>(&lm_skip_unk), "have lm skip over the unknown word (lmstring=empty)")
        ("tm-skip-unk", defaulted_value<bool>(&tm_skip_unk), "(UNIMPLEMENTED) ensure that the unknown foreign word translation doesn't appear in the decoder's output string (also implies it will not appear in the lmstring)")
        ("native-unk", defaulted_value<string>(&native_unk), "english word to use as unknown (for lm/tm output)")
        ("foreign-length-feature", defaulted_value<string>(&foreign_length_f), "count feature: [foreign-length]=10^-1")
        ("unk-indicator-feature", defaulted_value<string>(&unk_indicator_f), "indicator feature: [unk-indicator]=10^-1")
        ("unk-prob-feature", defaulted_value<string>(&unk_prob_f), "probability (from unk-tags) feature: [unk-prob]=p")
        ("xrs-rule-format", bool_switch(&xrs_rule_format), "print xrs (not brf) rules")
        ;
    
    OD all;
    all.add(general).add(rtemp).add(oldrule).add(rtemp_generate);
    try {
        variables_map vm;
        store(parse_command_line(argc,argv,all),vm);
        notify(vm);
        log << "### COMMAND LINE:\n" << graehl::get_command_line(argc,argv,NULL) << "\n\n";
        if (help) {
            cerr << all << endl;
            return 1;
        }
        if (vm.count("unk-tags") > 0) unk_tags = pair_list_construct(unk_tags_s);
        ostream &output=output_file.stream();
        
        if(unk_tags_file != ""){
            unk_tags.clear();
            unk_tags = pair_list_construct_from_file(unk_tags_file);
        }

        for (unk_tags_t::iterator i=unk_tags.begin(),e=unk_tags.end();
             i!=e;++i) {
            std::string const& tag=i->first;
            ostringstream o;
            if (xrs_rule_format) {
	        o << tag <<"(\"" << native_unk << "\") -> \"" << foreign_unk_template<< "\" ### id=" << id_template;
	    }
            else {
                o << "X: "<< tag <<"(\"" << native_unk << "\") -> \"" << foreign_unk_template
                  << "\" ### id="<<id_template << '\n';
                o << "V: "<< tag << " -> \"" << foreign_unk_template << "\" ### id=" << id_template;
	    }
            indicator_feature(o,unk_indicator_f);
            indicator_feature(o,foreign_length_f);
            indicator_feature(o,"unk-"+tag);
            if (!i->second.empty())
                o << " "<<unk_prob_f<<"="<<i->second;
            o <<" lm_string={{{";
            if (!lm_skip_unk)
                o << '"' << native_unk << '"';
            o << "}}}";
            o << " hwpos={{{\"" << native_unk << "\"}}}";
            templates.push_back(o.str());
        }

        if (doing(template_log,"final templates"))
            printlines(template_log.stream(),templates);
        
//            for (templates_t::iterator i=templates.begin(),e=templates.end();i!=e;++i) {
        typedef set<string> unk_words_t;
        unk_words_t unk_words;
        if (!foreign_file.is_none()) {
            std::string f;
            while (foreign_file.stream() >> f)
              if (find(ignored_foreigns.begin(),ignored_foreigns.end(),f) == ignored_foreigns.end())
                unk_words.insert(f);
        }
        if (!rule_file.is_none()) {
            graehl::time_space_report report(cerr,"Finished parsing preexisting rules: ");
            //FIXME: set difference of single-word rules in rules-file and foreign
            string line,rhs_l=" rhs={{{\"",rhs_r="\"}}}",id_l=" id=",id_r=" ",attr=" ###",real_rule=" virtual_label=no";
            doing(single_word_rules_log,"(preexisting) rules for single foreign words");
            unsigned line_no=0;
            while (std::getline(rule_file.stream(),line)) {
                if (copy_rules)
                    output<< line << endl;
                ++line_no;
                if (line.substr(0,2) == "X:" or xrs_rule_format) {
		  size_t offst = xrs_rule_format ? 0 : 2;
                    rule_data xrule = parse_xrs(make_pair(line.begin() + offst, line.end()));
                    if (xrule.rhs.size() == 1 and not xrule.rhs[0].indexed) {
                        unk_words.erase(xrule.rhs[0].label);
                    }
                    maybe_increase_max(max_id,xrule.id);
                }
            }
        }
        if (doing(unknown_words_log,"unknown words"))
            printlines(unknown_words_log.stream(),unk_words);

        
        boost::int64_t next_id=1+max_id+id_offset;
        if (id_origin > next_id)
            next_id=id_origin;
        cerr << "Highest seen id in input rules was id="<<max_id<<".\n";
        cerr << "Using id="<<next_id<<" for first unknown word rule.\n";
        size_t n_unk=unk_words.size(),n_temp=templates.size();
        
        cerr << "Found " << n_unk << " unknown foreign words.  Applying each to "<<
            n_temp << " templates for a total of " << n_unk*n_temp << " new rules.\n";
        
        {
            graehl::time_space_report report(cerr,"Finished creating unknown-word rules: ");
            doing(output_file,"generated rules");
            for (templates_t::iterator ti=templates.begin(),te=templates.end();ti!=te;++ti) {
                for (unk_words_t::iterator ui=unk_words.begin(),ue=unk_words.end();ui!=ue;++ui) {
                    std::string new_rule=*ti;
                    //FIXME: store find indices outside loop!  interesting because we have two types of find running, id and unk
                    if (0==replace_all(new_rule,id_template,boost::lexical_cast<string>(next_id++)))
                        throw_error("Found no id= template in "+*ti);
                    if (0==replace_all(new_rule,foreign_unk_template,*ui))
                        throw_error("Found no @UNKNOWN@ template in "+*ti);
                    output << new_rule << "\n";
                }
            }
        }
        
    } catch (std::exception &e) {
        log << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
        throw;
    }
    return 0;
}

char const *usage_str()
{
    return
        "generates binary rules for parsing all the words in a foreign file as @UNKNOWN@:\n"
        "   * optionally scans a rule file and skips foreign words which already have *single-word* translations\n"
        "         - assigns syntax rule ids starting at some offset from the next highest number seen in the rule file\n"
        "   * you can supply a list of rule templates (with the foreign word and syntax rule id being filled in\n"
        "       - or a list of english tags and probabilities, which will generate appropriate templates\n"
        "       - the generated templates will have an indicator function (unk_rule=10^-1) and a prob (unk_prob=10^-.5)\n"
        "\nThe program operates in two separable stages: generation of templates into which new foreign words (and rule ids) are placed, and selection of foreign words which lack single-word translations.\n"
        "Different choices for LM scoring (skip or unk word) are possible since the program operates on binarized rules and can play w/ the lmstring.\n"
        ;
    
}

