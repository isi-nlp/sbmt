# include <treelib/Tree.h>
# include <iostream>
# include <string>
# include <boost/program_options.hpp>
# include <boost/enum.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/foreach.hpp>
# include <tr1/unordered_map>
# include <collins/model1.hpp>
# include <boost/regex.hpp>

boost::regex isbar("^(.*)-BAR$");
boost::regex isverb("^V.*");

typedef std::tr1::unordered_map<std::string,size_t> count_map;

std::string getHeadPOS(treelib::Tree::iterator_base const& base)
{
    if (base->getHeadPOS() == "") return base->getLabel();
    else return base->getHeadPOS();
}

std::string getHeadword(treelib::Tree::iterator_base const& base)
{
    if (base->getHeadword() == "") return base.begin()->getLabel();
    else return base->getHeadword();
}

collins::model1::state<std::string> make_m1_rule(treelib::Tree::iterator_base const& node)
{
    typedef collins::model1::state<std::string> strstate;
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    
    if (itr != end) {
        if (node->getIsPreterminal()) {
            //introduce state
            bool isvb = boost::regex_match(node->getLabel(),isverb);
            return strstate(node->getLabel(),node->getLabel(),node->getLabel(),itr->getLabel(),isvb);
        }
        
        std::map< int, boost::tuple<strstate,treelib::Tree::sibling_iterator> > statemap;
        for (size_t hp = 1; itr != end; ++itr,++hp) statemap[hp] = boost::make_tuple(make_m1_rule(itr),itr);
        itr = node.begin();
        
        size_t hpos = node->getHeadPosition();
        assert(hpos != 0);
        
        std::cerr << "# " << node->getLabel() << "(" << getHeadPOS(node) << "," << getHeadword(node) << ") " << hpos << "->";
        for (; itr != end;++itr) {
            std::cerr << ' ' << itr->getLabel() << "(" << getHeadPOS(itr) << "," << getHeadword(itr) << ")";
        }
        std::cerr << '\n';
        boost::smatch smh, smr;
        bool isintro = not boost::regex_match(statemap[hpos].get<1>()->getLabel(),smh,isbar);
        bool iscompl = not boost::regex_match(node->getLabel(),smr,isbar);
        std::string R = iscompl ? node->getLabel() : smr.str(1);
        std::string H = isintro ? statemap[hpos].get<0>().root : statemap[hpos].get<0>().constit;
        std::string T = statemap[hpos].get<0>().tag;
        std::string h = statemap[hpos].get<0>().word;
        if (isintro) {
            std::cout << "H\t" << R << '\t' << T << '\t' << h << '\t' << H << '\n';
            statemap[hpos].get<0>() = introduce_head(R,statemap[hpos].get<0>());
        } 
        strstate::distance_type dist = statemap[hpos].get<0>().distance;
        if (statemap.size() > 1) {
            std::string D  = statemap[3-hpos].get<0>().root;
            std::string dt = statemap[3-hpos].get<0>().tag;
            std::string dw = statemap[3-hpos].get<0>().word;
            if (hpos == 1) {
                std::cout << "R1\t" << R << ' ' << H << ' ' << dist[strstate::verb_under] << dist[strstate::right_adjacent]
                          << '\t' << T << '\t' << h << '\t' << D << ' ' << dt << '\n';
                std::cout << "R2\t" << dt << '\t' << D << '\t' << R << ' ' << H << ' ' << dist[strstate::verb_under] << dist[strstate::right_adjacent] 
                          << ' ' << T << '\t' << h << '\t' << dw << '\n';
                statemap[hpos].get<0>() = attach_right(statemap[1].get<0>(),statemap[2].get<0>());
            } else {
                std::cout << "L1\t" << R << ' ' << H << ' ' << dist[strstate::verb_under] << dist[strstate::left_adjacent] 
                          << '\t' << T << '\t' << h << '\t' << D << ' ' << dt << '\n';
                std::cout << "L2\t" << dt << '\t' << D << '\t' << R << ' ' << H << ' ' << dist[strstate::verb_under] << dist[strstate::left_adjacent] 
                          << ' ' << T << '\t' << h << '\t' << dw << '\n';
                statemap[hpos].get<0>() = attach_left(statemap[1].get<0>(),statemap[2].get<0>());
            }
        }
        dist = statemap[hpos].get<0>().distance;
        if (iscompl) {
            std::cout << "R1\t" << R << ' ' << H << ' ' << dist[strstate::verb_under] << dist[strstate::right_adjacent]
                      << '\t' << T << '\t' << h << '\t' << "NULL NULL\n";
            std::cout << "L1\t" << R << ' ' << H << ' ' << dist[strstate::verb_under] << dist[strstate::left_adjacent] 
                      << '\t' << T << '\t' << h << '\t' << "NULL NULL\n";
        }
        return statemap[hpos].get<0>();
    }
}

void print(count_map& cmap)
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
    collins::output_type type(collins::output_type::ruleheadroot);
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
            treelib::Tree tree;
            tree.read(line);
            std::cerr << "## " << line << '\n';
            make_m1_rule(tree.begin());

            if (cmap.size() > 5000000) print(cmap);
            if (linecount % 1000000 == 0) std::cerr << type << '\t' << linecount/1000000 << " million\n";
        } 
    } catch(...) {
        std::cerr << "exception line " << linecount << '\n';
        std::cerr << line << '\n';
    }
    print(cmap);
    return 0;
}