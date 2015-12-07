# include <treelib/Tree.h>
# include <iostream>
# include <string>
# include <boost/program_options.hpp>
# include <boost/enum.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/foreach.hpp>
# include <tr1/unordered_map>
# include <collins/lm.hpp>
# include <boost/algorithm/string.hpp>

typedef std::map< int, std::vector<int> > alignment;
typedef std::vector<std::string> source;

source read_source(std::string s)
{
    std::vector<std::string> v;
    boost::split(v,s,boost::is_any_of(" "));
    for (int x = 0; x != v.size(); ++x) v[x] = collins::at_replace(v[x]);
    return v;
}

alignment read_alignment(std::string s)
{
    alignment a;
    std::vector<std::string> v;
    boost::split(v,s,boost::is_any_of(" "));
    BOOST_FOREACH(std::string ss, v) {
        std::vector<std::string> vv;
        boost::split(vv,ss,boost::is_any_of("-"));
        a[boost::lexical_cast<int>(vv[0])].push_back(boost::lexical_cast<int>(vv[1]));
    }
    for (alignment::iterator itr = a.begin(); itr != a.end(); ++itr) {
        std::sort(itr->second.begin(),itr->second.end());
    }
    return a;
}

typedef std::tr1::unordered_map<std::string,size_t> count_map;

void dictionary(collins::strrule const& rule, count_map& cmap)
{
    BOOST_FOREACH(collins::strrule::variable const & v, std::make_pair(rule.r,rule.r + rule.sz)) {
        cmap[v.label] += 1;
        cmap[v.head] += 1;
    }
}

void make_rule(treelib::Tree::iterator_base const& node) 
{
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    
    if ((not node->getIsPreterminal()) and (itr != end)) {
        std::cout << collins::make_strrule(node) << '\n';
        itr = node.begin();
        for (; itr != end; ++itr) make_rule(itr);
    }
}

void count_preterm(treelib::Tree::iterator_base const& node, int& x) 
{
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    if (node->getIsPreterminal()) ++x;
    else if ((not node->getIsPreterminal()) and (itr != end)) {
        for (; itr != end; ++itr) {
            count_preterm(itr,x);
        }
    }
}

int count_preterm(treelib::Tree::iterator_base const& node)
{
    int x = 0;
    count_preterm(node,x);
    return x;
}

std::vector<std::string> affiliated_source(source const& src, alignment const& a, int d)
{
    std::vector<std::string> f(11);
    if (d < 0) {
        for (int c = 0; c != 11; ++c) f[c] = "NULL";
    } else {
        alignment::const_iterator pos = a.find(d);
        if (pos == a.end()) pos = a.upper_bound(d);
        if (pos == a.end()) {
            for (int dd = d; dd != -1; --dd) {
                pos = a.find(dd);
                if (pos != a.end()) break;
            }
        }
        if (pos == a.end()) throw std::logic_error("BRRK " + boost::lexical_cast<std::string>(d));
        int af = pos->second[pos->second.size()/2];
        for (int c = 5, a = af; c != 11; ++c, ++a) {
            if (a >= src.size()) f[c] = "</source_s>";
            else f[c] = src[a];
        }
        for (int c = 4, a = af -1; c != -1; --c, --a) {
            if (a < 0) f[c] = "<source_s>";
            else f[c] = src[a];
        }
    }
    return f;
}

/*
int make_amr_neural( int sentid
                   , treelib::Tree::iterator_base const& node
                   , source const& src
                   , alignment const& a
                   , double wt
                   , int& x )
{
   treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();

   if (node->getIsPreterminal()) {
       return x++;
   }

   else if ((not node->getIsPreterminal()) and (itr != end)) {
       collins::strrule r = collins::make_strrule(node);
       itr = node.begin();
       std::vector<int> headidx;
       for (; itr != end; ++itr) {
           headidx.push_back(make_neural(sentid,itr,src,a,wt,x));
       }
       std::cout << sentid << '\t' << r.lhs().label << ' ' << r.lhs().tag << ' ' << r.lhs().head;

       std::vector<std::string> f;
       if (r.sz == 4) {
           int d = headidx[r.sz - r.hpos - 1];
           f = affiliated_source(src,a,d);
       } else {
           f = affiliated_source(src,a,-1);
       }
       BOOST_FOREACH(std::string ff, f) std::cout << ' ' << ff;
       std::cout << '\t' << r.hpos;
       for (size_t x = 1; x != 3; ++x) {
           if (x < r.sz) {
               std::cout << '_' << r.r[x].label;
           } else {
               std::cout << '_' << "NULL";
           }
       }
       std::cout << ' ';
       if (r.sz == 3) {
           size_t d = r.sz - r.hpos;
           std::cout << r.r[d].tag << ' ' << r.r[d].head;
       } else {
           std::cout << "NULL NULL";
       }
       std::cout << '\t'<< wt << '\n';

       if (r.lhs().label == "TOP") {
           std::cout << sentid << "\tTOP NULL NULL";
           f = affiliated_source(src,a,headidx[r.hpos - 1]);
           BOOST_FOREACH(std::string ff, f) std::cout << ' ' << ff;
           std::cout << "\t0_NULL_NULL " << r.lhs().tag << ' ' << r.lhs().head <<'\t'<<wt<< '\n';
       }

       return headidx[r.hpos - 1];
   } else {
       throw std::logic_error("AAAAH!");
   }
}
*/
int make_neural( int sentid
               , treelib::Tree::iterator_base const& node
               , source const& src
               , alignment const& a
               , double wt
               , int& x ) 
{
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    
    if (node->getIsPreterminal()) {
        return x++;
    }
    
    else if ((not node->getIsPreterminal()) and (itr != end)) {
        collins::strrule r = collins::make_strrule(node);
        itr = node.begin();
        std::vector<int> headidx;
        for (; itr != end; ++itr) {
            headidx.push_back(make_neural(sentid,itr,src,a,wt,x));
        }
        std::cout << sentid << '\t' << r.lhs().label << ' ' << r.lhs().tag << ' ' << r.lhs().head;
        
        std::vector<std::string> f;
        if (r.sz == 3) {
            int d = headidx[r.sz - r.hpos - 1];
            f = affiliated_source(src,a,d);
        } else {
            f = affiliated_source(src,a,-1);
        }
        BOOST_FOREACH(std::string ff, f) std::cout << ' ' << ff;
        std::cout << '\t' << r.hpos;
        for (size_t x = 1; x != 3; ++x) {
            if (x < r.sz) {
                std::cout << '_' << r.r[x].label;
            } else {
                std::cout << '_' << "NULL";
            }
        }
        std::cout << ' ';
        if (r.sz == 3) {
            size_t d = r.sz - r.hpos;
            std::cout << r.r[d].tag << ' ' << r.r[d].head;
        } else {
            std::cout << "NULL NULL";
        }
        std::cout << '\t'<< wt << '\n';
        
        if (r.lhs().label == "TOP") {
            std::cout << sentid << "\tTOP NULL NULL";
            f = affiliated_source(src,a,headidx[r.hpos - 1]);
            BOOST_FOREACH(std::string ff, f) std::cout << ' ' << ff;
            std::cout << "\t0_NULL_NULL " << r.lhs().tag << ' ' << r.lhs().head <<'\t'<<wt<< '\n';
        }
        
        return headidx[r.hpos - 1];
    } else {
        throw std::logic_error("AAAAH!");
    }
}

void make_neural(int sentid, treelib::Tree::iterator_base const& node, source const& src, alignment const& a, double wt) 
{
    int x = 0;
    make_neural(sentid,node,src,a,wt,x);
}

void rule_given_root_headtag_headword(collins::strrule const& rule, count_map& cmap) 
{
    std::stringstream sstr;
    sstr << rule.lhs().label << ' ' << rule.lhs().tag << ' ' << rule.lhs().head << '\t';
    sstr << rule.hpos;
    BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
        sstr << ' ' << v.label;
    }
    cmap[sstr.str()] += 1;
}

void rule_given_root_headtag(collins::strrule const& rule, count_map& cmap) 
{
    std::stringstream sstr;
    sstr << rule.lhs().label << ' ' << rule.lhs().tag << '\t';
    sstr << rule.hpos;
    BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
        sstr << ' ' << v.label;
    }
    cmap[sstr.str()] += 1;
}

void rule_given_root(collins::strrule const& rule, count_map& cmap) 
{
    std::stringstream sstr;
    sstr << rule.lhs().label << '\t';
    sstr << rule.hpos;
    BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
        sstr << ' ' << v.label;
    }
    cmap[sstr.str()] += 1;
}

void deptag_given_rule_headtag_headword(collins::strrule const& rule, count_map& cmap)
{
    if (rule.hpos > 0) {
        size_t x = 1;
        BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
            if (x != rule.hpos) {
                std::stringstream sstr;
                sstr << rule.lhs().label << ' ' << rule.hpos;
                BOOST_FOREACH(collins::strrule::variable const& cv, rule.rhs()) {
                    sstr << ' ' << cv.label;
                }
                sstr << ' ' << rule.lhs().tag << ' ' << rule.lhs().head;
                sstr << '\t' << v.tag;
                cmap[sstr.str()] += 1;
            }
            ++x;
        }
    }
}

void deptag_given_rule_headtag(collins::strrule const& rule, count_map& cmap)
{
    if (rule.hpos > 0) {
        size_t x = 1;
        BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
            if (x != rule.hpos) {
                std::stringstream sstr;
                sstr << rule.lhs().label << ' ' << rule.hpos;
                BOOST_FOREACH(collins::strrule::variable const& cv, rule.rhs()) {
                    sstr << ' ' << cv.label;
                }
                sstr << ' ' << rule.lhs().tag;
                sstr << '\t' << v.tag;
                cmap[sstr.str()] += 1;
            }
            ++x;
        }
    }
}

void deptag_given_rule(collins::strrule const& rule, count_map& cmap)
{
    if (rule.hpos > 0) {
        size_t x = 1;
        BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
            if (x != rule.hpos) {
                std::stringstream sstr;
                sstr << rule.lhs().label << ' ' << rule.hpos;
                BOOST_FOREACH(collins::strrule::variable const& cv, rule.rhs()) {
                    sstr << ' ' << cv.label;
                }
                sstr << '\t' << v.tag;
                cmap[sstr.str()] += 1;
            }
            ++x;
        }
    }
}

void depword_given_deptag_rule_headtag_headword(collins::strrule const& rule, count_map& cmap)
{
    if (rule.hpos > 0) {
        size_t x = 1;
        BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
            if (x != rule.hpos) {
                std::stringstream sstr;
                sstr << v.tag << ' '<< rule.lhs().label << ' ' << rule.hpos;
                BOOST_FOREACH(collins::strrule::variable const& cv, rule.rhs()) {
                    sstr << ' ' << cv.label;
                }
                sstr << ' ' << rule.lhs().tag << ' ' << rule.lhs().head;
                sstr << '\t' << v.head;
                cmap[sstr.str()] += 1;
            }
            ++x;
        }
    }
}

void depword_given_deptag_rule_headtag(collins::strrule const& rule, count_map& cmap)
{
    if (rule.hpos > 0) {
        size_t x = 1;
        BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
            if (x != rule.hpos) {
                std::stringstream sstr;
                sstr << v.tag << ' ' << rule.lhs().label << ' ' << rule.hpos;
                BOOST_FOREACH(collins::strrule::variable const& cv, rule.rhs()) {
                    sstr << ' ' << cv.label;
                }
                sstr << ' ' << rule.lhs().tag;
                sstr << '\t' << v.head;
                cmap[sstr.str()] += 1;
            }
            ++x;
        }
    }
}

void depword_given_deptag_var(collins::strrule const& rule, count_map& cmap)
{
    if (rule.hpos > 0) {
        size_t x = 1;
        BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
            if (x != rule.hpos) {
                std::stringstream sstr;
                sstr << v.tag <<' '<< v.label<< '\t' << v.head;
                cmap[sstr.str()] += 1;
            }
            ++x;
        }
    }
}

void depword_given_deptag(collins::strrule const& rule, count_map& cmap)
{
    if (rule.hpos > 0) {
        size_t x = 1;
        BOOST_FOREACH(collins::strrule::variable const& v, rule.rhs()) {
            if (x != rule.hpos) {
                std::stringstream sstr;
                sstr << v.tag << '\t' << v.head;
                cmap[sstr.str()] += 1;
            }
            ++x;
        }
    }
}

void head_given_root(collins::strrule const& rule, count_map& cmap)
{
    cmap[rule.lhs().label + "\t" + rule.lhs().head] += 1;
}

void printcmap(count_map& cmap)
{
    std::string str;
    size_t sz;
    BOOST_FOREACH(boost::tie(str,sz), cmap) {
        std::cout << str << '\t' << sz << '\n';
    }
    cmap.clear();
}

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    
    namespace po = boost::program_options;
    po::options_description desc;
    collins::output_type type(collins::output_type::ruleroottagword);
    bool help(false);
    desc.add_options()
          ( "help,h"
          , po::bool_switch(&help)->default_value(false)
          , "display this message"
          )
          ( "pass,p"
          , po::value(&type)->default_value(type)
          , "which model to map"
          );
    po::variables_map vm;
    store(parse_command_line(argc,argv,desc), vm);
    notify(vm);
    if (help) {
        cerr << desc << endl;
        exit(0);
    }
    
    std::string line;
    count_map cmap;
    size_t linecount = 0;
    try { 
        while (getline(std::cin,line)) {
            ++linecount;
            if (type == collins::output_type::ruleinfo) {
                treelib::Tree tree;
                tree.read(line);
                make_rule(tree.begin());
            } else if (type == collins::output_type::neural or type == collins::output_type::amr_neural) {
                std::vector<std::string> v;
                boost::split(v,line,boost::is_any_of("\t"));
                if (v.size() < 4 or v[0] == "0" or v[0] == "" or v[1] == "" or v[2] == "" or v[3] == "") {
                    std::cout << linecount << '\n';
                    continue;
                }
                source src = read_source(v[1]);
                alignment a = read_alignment(v[2]);
                double wt = boost::lexical_cast<double>(v[3]);
                if (src.size() == 0 or a.size() == 0 ) {
                    std::cout << linecount << '\n';
                    continue;
                }
                treelib::Tree tree;
                tree.read(v[0]);
                int pt = count_preterm(tree.begin());
                if (pt < a.rbegin()->first) {
                    std::cout << linecount << '\n';
                    continue;
                }
                make_neural(linecount,tree.begin(),src,a,wt);
            } else {
                std::stringstream sstr(line);
                collins::strrule rule;
                sstr >> rule;
                if (type == collins::output_type::dictionary) dictionary(rule,cmap);
                /*
                
                if (type == collins::output_type::ruleroottagword) rule_given_root_headtag_headword(rule,cmap);
                else if (type == collins::output_type::ruleroottag) rule_given_root_headtag(rule,cmap);
                else if (type == collins::output_type::ruleroot) rule_given_root(rule,cmap);
                
                else if (type == collins::output_type::tagruletagword) deptag_given_rule_headtag_headword(rule,cmap);
                else if (type == collins::output_type::tagruletag) deptag_given_rule_headtag(rule,cmap);
                else if (type == collins::output_type::tagrule) deptag_given_rule(rule,cmap);
                //else if (type == collins::output_type::tagvar) deptag_given_var(rule,cmap);
                
                else if (type == collins::output_type::wordtagruletagword) depword_given_deptag_rule_headtag_headword(rule,cmap);
                else if (type == collins::output_type::wordtagruletag) depword_given_deptag_rule_headtag(rule,cmap);
                else if (type == collins::output_type::wordtagvar) depword_given_deptag_var(rule,cmap);
                else if (type == collins::output_type::wordtag) depword_given_deptag(rule,cmap);
                
                
                */
                else throw std::runtime_error("unsupported function");
            }
            if (cmap.size() > 5000000) printcmap(cmap);
            if (linecount % 1000000 == 0) std::cerr << type << '\t' << linecount/1000000 << " million\n";
        } 
    } catch(std::exception const& e) {
        std::cerr << "exception line " << linecount << '\n';
        std::cerr << line << '\n';
        std::cerr << e.what() << '\n';
    } catch(...) {
        std::cerr << "exception line " << linecount << '\n';
        std::cerr << line << '\n';
    }
    printcmap(cmap);
    return 0;
}