# include <xrsparse/xrs.hpp>
# include <string>
# include <sstream>
# include <iostream>
# include <sbmt/grammar/syntax_rule.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <gusc/string/escape_string.hpp>
# include <algorithm>
# include <iterator>
# include <boost/regex.hpp>
# include <boost/algorithm/string.hpp>
# include <boost/interprocess/managed_shared_memory.hpp>
# include <boost/interprocess/containers/map.hpp>
# include <boost/interprocess/containers/string.hpp>
# include <boost/interprocess/allocators/allocator.hpp>
# include <boost/interprocess/managed_mapped_file.hpp>

namespace ip = boost::interprocess;
using namespace std;
using sbmt::fatter_syntax_rule;

boost::regex target0("~\\d+~\\d+ [0-9.-]+ ");
string target0repl = " ";

boost::regex target1("(\\S+)\\) ");
string target1repl(boost::smatch const& what)
{
    return " \"" + what.str(1) + "\") ";
}

boost::regex target2("\\(([^ \"]+)\\s+");
string target2repl(boost::smatch const& what)
{
    return what.str(1) + "(";
}

boost::regex target3("\\s+\\)");
string target3repl = ")";

boost::regex sourcestr("(\\S+)");
string sourcerepl(boost::smatch const& what)
{
    return "\"" + what.str(1) + "\"";
}

boost::regex alignpair("(\\d+)-(\\d+)");
string alignrepl(boost::smatch const& what)
{
    return "(" + what.str(1) + " " + what.str(2) + ")";
}

typedef boost::tuple<int,int> intpair;
vector<intpair> read_align(string str) {
    vector<intpair> vp;
    stringstream sstr(str);
    copy(istream_iterator<intpair>(sstr), istream_iterator<intpair>(), back_inserter(vp));
    return vp;
}
void throw_feature_not_found(rule_data const& rd, string lbl)
{
    stringstream sstr;
    sstr << "feature " << lbl << " not found in rule '" << rd << "'";
    throw runtime_error(sstr.str());
}

typedef map<fatter_syntax_rule::lhs_preorder_iterator,bool> level_set;
typedef map<int,level_set> level_sets;
typedef map<fatter_syntax_rule::lhs_preorder_iterator,int> levels;
typedef vector<fatter_syntax_rule::lhs_preorder_iterator> lhs_yield;
typedef vector<fatter_syntax_rule::rhs_iterator> rhs_yield;

lhs_yield create_lhs_yield(fatter_syntax_rule const& rule)
{
    lhs_yield lhs;
    BOOST_FOREACH(fatter_syntax_rule::tree_node const& nd, rule.lhs()) {
        if (nd.is_leaf()) lhs.push_back(&nd);
    }
    return lhs;
}

rhs_yield create_rhs_yield(fatter_syntax_rule const& rule)
{
    rhs_yield rhs;
    BOOST_FOREACH(fatter_syntax_rule::rule_node const& nd, rule.rhs()) {
        rhs.push_back(&nd);
    }
    return rhs;
}

int create_level_sets( level_sets& lss
                     , levels& lvls
                     , fatter_syntax_rule const& rule
                     , fatter_syntax_rule::lhs_preorder_iterator pos 
                     )
{
    int lvl = -1;
    BOOST_FOREACH(fatter_syntax_rule::tree_node const& nd, pos->children()) {
        lvl = max(lvl,create_level_sets(lss,lvls,rule,&nd));
    }
    ++lvl;
    lss[lvl][pos] = true;
    lvls[pos] = lvl;
    BOOST_FOREACH(fatter_syntax_rule::tree_node const& nd, pos->children()) {
        if (lvls[&nd] + 1 != lvl) lss[lvl - 1][&nd] = false; 
    }
    return lvl;
}

string node_name( level_set& ls
                , fatter_syntax_rule::lhs_preorder_iterator pos
                )
{
    if (ls[pos]) return "n" + boost::lexical_cast<string>(pos);
    else return "m" + boost::lexical_cast<string>(pos);
}

template <class Iterator>
string node_name(Iterator pos)
{
    return "n" + boost::lexical_cast<string>(pos);
}

feature find_feature(rule_data const& rd, string const& lbl)
{
    BOOST_FOREACH(feature const& f, rd.features)
    {
        if (f.key == lbl) {
            return f;
        }
    }
    throw_feature_not_found(rd,lbl);
    return rd.features[0]; // will not reach -- warning supression
}

void print_clusters(fatter_syntax_rule::lhs_preorder_iterator pos)
{
    if (not pos->is_leaf()) cout << "subgraph cluster_" << node_name(pos) << " {\n";
    BOOST_FOREACH(fatter_syntax_rule::tree_node const& cnd, pos->children()) {
        print_clusters(&cnd);
        cout << node_name(pos) << " -> " 
             << node_name(&cnd)
             //<< node_name(lss[lid - 1],&cnd)
             << " [weight=" << 5 << "]\n";
    }
    if (not pos->is_leaf()) cout << "}\n";
}

void print_graphviz(rule_data const& rd)
{
    fatter_syntax_rule rule(rd,sbmt::fat_tf);
    vector<intpair> align = read_align(find_feature(rd,"align").str_value);
    level_sets lss;
    levels lvls;
    int maxlvl = create_level_sets(lss,lvls,rule,rule.lhs_root());
    lhs_yield lyld = create_lhs_yield(rule);
    rhs_yield ryld = create_rhs_yield(rule);
    
    
    cout << 
    "digraph instance {\n"
    "epsilong=0.001\n"
    "fixedsize=true\n"
    "nodesep=0.5\n"
    "splines=false\n"
    "center=true\n"
    "clusterrank=local\n"
    "node [style=\"rounded,filled\",width=0.01,height=0.01,shape=box,fillcolor=\"#e5e5e5\"]\n"
    "edge [arrowhead=none]\n";
    
    int lid;
    level_set ls;
    
    cout << "{ rank=same\n";
    BOOST_FOREACH(fatter_syntax_rule::rhs_iterator rpos, ryld) {
        cout << node_name(rpos) << " [label=\"" <<  gusc::escape_c(rpos->get_token().label()) << "\"]\n";
    }
    if (ryld.size() > 1) {
        bool first = true;
        BOOST_FOREACH(fatter_syntax_rule::rhs_iterator rpos, ryld) {
            if (not first) cout << " -> ";
            cout << node_name(rpos);
            first = false;
        }
        cout << " [style=invis,weight=10000]\n";
    }
    cout << "}\n";
    
    
    BOOST_FOREACH(boost::tie(lid,ls), lss) {
        fatter_syntax_rule::lhs_preorder_iterator pos;
        bool cond;
        cout << "{ rank=same\n";
        BOOST_FOREACH(boost::tie(pos,cond),ls) {
            cout << node_name(ls,pos);
            if (ls[pos]) cout << " [label=\"" << gusc::escape_c(pos->get_token().label()) << "\"]\n";
            else cout << " [shape=point]\n";
        }
        bool first = true;
        if (ls.size() > 1 and lid == 0) {
            int w = 1;
            if (lid ==0) w = 10000;
            BOOST_FOREACH(boost::tie(pos,cond),ls) {
                if (not first) cout << " -> ";
                cout << node_name(ls,pos);
                first = false;
            }
            cout << " [style=invis,weight="<< w << "]\n";
        }
        cout << "}\n";
    }
    
    bool headfound = false;
    bool tailfound = false;
    //bool rheadfound = false;
    //bool rtailfound = false;
    
    int src,tgt;
    BOOST_FOREACH(boost::tie(tgt,src),align) {
        int weight=50;
        if (src == 0 and tgt == 0) { 
            headfound = true; 
            weight=5000; 
        }
        if ((src == int(lyld.size()) - 1) and (tgt == 0)) {
            tailfound = true;
            weight=5000;
        }
        cout << node_name(lyld[tgt]) << " -> " << node_name(ryld[src]) << " [weight="<<weight<<",minlen=3]\n"; 
    }
    
    if (not headfound) {
        cout << node_name(lyld[0]) << " -> " << node_name(ryld[0]) << " [weight=5000,style=invis]\n"; 
    }
    if (not tailfound) {
        cout << node_name(lyld[0]) << " -> " << node_name(ryld.back()) << " [weight=5000,style=invis]\n";
    }
    
    //print_clusters(rule.lhs_root());
    
    BOOST_FOREACH(boost::tie(lid,ls),lss) {
        fatter_syntax_rule::lhs_preorder_iterator pos;
        bool cond;
        BOOST_FOREACH(boost::tie(pos,cond),ls) {
            int w = 5 * (maxlvl - lid + 1);
            if (cond) {
                BOOST_FOREACH(fatter_syntax_rule::tree_node const& cnd, pos->children()) {
                    cout << node_name(pos) << " -> " 
                         << node_name(lss[lid - 1],&cnd)
                         << " [weight=" << w << "]\n";
                }
            } else {
                cout << node_name(ls,pos) << " -> " 
                     << node_name(pos) 
                     << " [weight=" << w << "]\n";
            }
        }
    }
    
    cout << "}\n\n";
}

string triple_to_rule(string tgt, string src, string tgtsrca)
{
    //tgt
    tgt = regex_replace(tgt,target0,target0repl);
    cerr << tgt << endl << endl;
    tgt = regex_replace(tgt,target1,target1repl);
    cerr << tgt << endl << endl;
    tgt = regex_replace(tgt,target2,target2repl);
    cerr << tgt << endl << endl;
    tgt = regex_replace(tgt,target3,target3repl);
    cerr << tgt << endl << endl;
    
    // src
    src = regex_replace(src,sourcestr,sourcerepl);
    
    // align
    tgtsrca = regex_replace(tgtsrca,alignpair,alignrepl);
    
    return tgt + " -> " + src + " ### align={{{" + tgtsrca + "}}}";
}

typedef ip::allocator<char, ip::managed_mapped_file::segment_manager> char_allocator;
typedef ip::basic_string<char, char_traits<char>, char_allocator> mmap_string;
typedef ip::allocator<pair<const int,mmap_string>, ip::managed_mapped_file::segment_manager> string_allocator;
typedef ip::map<int,mmap_string,less<int>,string_allocator> mmap_string_map;

void write_memmap(string const& fname, istream& in)
{
    {
    ip::managed_mapped_file mfile(ip::create_only,fname.c_str(),100UL*1024UL*1024UL*1024UL);
    string line;
    string_allocator salloc(mfile.get_segment_manager());
    char_allocator calloc(mfile.get_segment_manager());
    mmap_string_map* mp = mfile.construct<mmap_string_map>("mp")(less<int>(),salloc);
    while (getline(in,line)) {
        vector<string> v;
        boost::split(v,line,boost::algorithm::is_any_of("\t"));
        string rls = triple_to_rule(v.at(1),v.at(2),v.at(3));
        mp->insert(std::make_pair(boost::lexical_cast<int>(v.at(4)), mmap_string(rls.begin(),rls.end(),calloc)));
    }
    }
    ip::managed_mapped_file::shrink_to_fit(fname.c_str());
}

void read_memmap(string const& fname, istream& in)
{
    ip::managed_mapped_file mfile(ip::open_only,fname.c_str());
    int id;
    mmap_string_map* mp = mfile.find<mmap_string_map>("mp").first;
    while (in >> id) {
        try {
            if (mp->find(id) == mp->end()) throw runtime_error("sentence not found");
            mmap_string& mdata = mp->find(id)->second;
            string rdata(mdata.begin(),mdata.end());
            rule_data rd = parse_xrs(rdata);
            print_graphviz(rd);
            cout << flush;
        } catch (std::exception const& e) {
            cout << "digraph error {\n";
            cout << "n0 [label=\"" << gusc::escape_c(e.what())  << "\"]\n}\n\n";
        }
    }
}

int main(int argc, char** argv)
{    
    ios::sync_with_stdio(false);   
    string line;
    if (argc == 3 and strcmp(argv[1],"-c") == 0) write_memmap(argv[2],cin);
    else if (argc == 2) read_memmap(argv[1],cin);
    else return 1;
    return 0;
    /*
    while (getline(cin,line)) {
        rule_data rd;
        feature val;
        vector<string> v;
        boost::split(v,line,boost::algorithm::is_any_of("\t"));
        string rls = triple_to_rule(v.at(0),v.at(1),v.at(2));
        cerr << rls << endl;
        rd = parse_xrs(rls);
        print_graphviz(rd);
    }
    return 0;
    */
}

