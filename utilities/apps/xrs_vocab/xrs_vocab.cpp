# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/hash/oa_hashtable.hpp>
# include <sbmt/hash/functors.hpp>

# include <graehl/shared/fileargs.hpp>
# include <graehl/shared/string_match.hpp>
#include <graehl/shared/command_line.hpp>

# include <boost/program_options.hpp>
# include <boost/regex.hpp>

# include <algorithm>
# include <string>

# include <gusc/functional.hpp>

using namespace sbmt;
using namespace std;
////////////////////////////////////////////////////////////////////////////////

void report(string const& line, exception const& e)
{
    cerr << "error processing rule "<< line << endl;
    cerr << "exception: "<< e.what() << endl;
}

void die(string const& why)
{
  cerr<<"ERROR: "<<why<<"\n";
  throw std::runtime_error(why);
}

////////////////////////////////////////////////////////////////////////////////

template<class Container>
void add_words_to_dict(Container& dict, string const& line)
{
    try {
    fat_syntax_rule rule(line,fat_tf);
    fat_syntax_rule::lhs_preorder_iterator itr = rule.lhs_begin(),
                                           end = rule.lhs_end();
    for (; itr != end; ++itr) {
        if (itr->get_token().type() == native_token) {
            dict.insert(itr->get_token().label());
        }
    }
    } catch (bad_rule_format const& e) {
        report(line,e);
    } catch (ns_RuleReader::Rule::bad_format const& e) {
        report(line,e);
    }
}

////////////////////////////////////////////////////////////////////////////////

char brf_header[]="BRF version ";
char const* brf_header_begin=brf_header;
char const* brf_header_end=brf_header+sizeof(brf_header)-1;
void output_dict(istream& in, ostream& out,bool force_brf=false)
{
    using boost::regex;
    regex comment_or_blank("^\\s*(\\$\\$\\$|%%%|$)");
    string line;
    oa_hashtable< string, gusc::identity > dict;
# define DICTADD(s) dict.insert(s)
    if (force_brf or (in and getline(in,line))) {
        if (force_brf or graehl::match_begin(line.begin(),line.end(),brf_header_begin,brf_header_end)) {
          cerr<<"BRF rule file. extracting vocab from lm_string\n";
          char c;
          std::vector<char> str;
          while(in) {
# define IFC(e) if (in && in.get(c) && c==e)
          IFC('l')
          IFC('m')
          IFC('_')
          IFC('s')
          IFC('t')
          IFC('r')
          IFC('i')
          IFC('n')
          IFC('g')
          IFC('=')
          IFC('{')
          IFC('{')
            IFC('{')
            while(in) {
              IFC(' ') {
                if (!in.get(c)) break;
                if (c=='"') {
                  if (!in.get(c)) break;
                  if (c=='"') {
                    IFC('"')
                      DICTADD("\"");
                  } else {
                    str.clear();
                    str.push_back(c);
                    // ".
                    while(in) {
                      if (!in.get(c)) break;
                      if (c=='"') {
                        DICTADD(string(str.begin(),str.end()));
                        break;
                      } else str.push_back(c);
                    }
                  }
                } else if (c=='}') {
                  IFC('}')
                    IFC('}')
                  break;
                }
              }
            }
          }
        } else {
          cerr<<"XRS rule file. extracting vocab from lhs tree leaves\n";
          do {
            if (not regex_search(line,comment_or_blank))
              add_words_to_dict(dict,line);
          } while(in and getline(in,line));
        }
    }
    copy(dict.begin(), dict.end(), ostream_iterator<string>(cout,"\n"));
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    namespace po = boost::program_options;
    using boost::regex;

    using namespace graehl;

    istream_arg pin("-");
    ostream_arg pout("-");

    char const* usage =
        "display all english words in a given xrs file";

    typedef boost::program_options::options_description OD;
    bool help=false;
    bool brf=false;
    OD all; //(general_options_desc());
    using boost::program_options::bool_switch;
    all.add_options()( "help,h"
                        , bool_switch(&help)
                      , "display this help message" );
    all.add_options()("brf,b",bool_switch(&brf)
                      , "force input format to brf");
    all.add_options()( "input,i"
                        , defaulted_value(&pin)
                      , "input xrs file" );
    all.add_options()( "output,o"
                        , defaulted_value(&pout)
                      , "output dictionary file" );

    istream& in = pin.stream();
    ostream& out = pout.stream();

    boost::program_options::variables_map vm;
    store(parse_command_line(argc,argv,all),vm);
    notify(vm);

    if (help) {
        cout <<argv[0]<<'\n'<< usage<<"\n"<< all << "\n";
        return 1;
    }

    output_dict(in,out,brf);
}

////////////////////////////////////////////////////////////////////////////////
