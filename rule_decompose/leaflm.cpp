# include "binalgo.hpp"
# include <iostream>

std::map<std::string,std::string> lblmap;

int height(sbmt::fatter_syntax_rule::tree_node const* rt)
{
    int retval = 0;
    if (rt->indexed() or rt->lexical()) { 
        return retval; 
    } else {
        BOOST_FOREACH(sbmt::fatter_syntax_rule::tree_node const& nd, rt->children()) {
            retval = std::max(retval,height(&nd));
        }
        retval += 1;
        return retval;
    }
}
int height(sbmt::fatter_syntax_rule const& rule)
{
    return height(rule.lhs_root());
}

int width(sbmt::fatter_syntax_rule const& rule)
{
    int lhs = 0;
    BOOST_FOREACH(sbmt::fatter_syntax_rule::tree_node const& nd,rule.lhs()) {
        if (nd.indexed() or nd.lexical()) ++lhs;
    }
    int rhs = rule.rhs_size();
    return std::max(lhs,rhs);
}

void print_rule_tree( std::ostream& out
                    , sbmt::fatter_syntax_rule::tree_node const& nd )
{
    out << lblmap[nd.get_token().label()];
    if (nd.indexed() or nd.children_begin()->lexical()) {
        return;
    }
    else  {
        bool first = true;
        out << '(';
        BOOST_FOREACH(sbmt::fatter_syntax_rule::tree_node const& cnd,nd.children()) {
            if (not first) out << '_';
            print_rule_tree(out,cnd);
            first = false;
        }
        out << ')';
    }
}

void print_rule_token(std::ostream& out, sbmt::fatter_syntax_rule const& r)
{
    bool first = true;
    BOOST_FOREACH(sbmt::fatter_syntax_rule::rule_node const& nd, r.rhs()) {
        if (nd.lexical()) {
            if (first == false) out << '_';
            out << "'"<< nd.get_token().label() << "'";
            first = false;
        }
    }
    if (first) out << "@!@";
    out << ' ';
    print_rule_tree(out,*r.lhs_root());
    out << ' ';
    first = true;
    BOOST_FOREACH(sbmt::fatter_syntax_rule::tree_node const& nd, r.lhs()) {
        if (nd.lexical()) {
            if (first == false) out << '_';
            out << "'"<< lblmap[nd.get_token().label()] << "'";
            first = false;
        }
    }
}

std::string rule_token(sbmt::fatter_syntax_rule const& r)
{
    std::stringstream sstr;
    print_rule_token(sstr,r);
    return sstr.str();
}


boost::tuple<
  sbmt::fatter_syntax_rule::tree_node const*
, int
, std::list<subder_ptr>::const_iterator
>
find_corner( subder_ptr const& sd
           , sbmt::fatter_syntax_rule::tree_node const* r
           , int idx
           , std::list<subder_ptr>::const_iterator c )
{
    sbmt::fatter_syntax_rule::tree_node const* retval = r + 100000000;
    if (r->lexical()) retval = r;
    else if (c != sd->children.end() and (*c)->root == r) {
        ++c;
    } else {
        sbmt::fatter_syntax_rule::tree_node const* rtv;
        sbmt::fatter_syntax_rule::lhs_children_iterator ci = r->children_begin(),
                                                        ce = r->children_end();
        for (; ci != ce; ++ci) {
            boost::tie(rtv,idx,c) = find_corner(sd,&(*ci),idx,c);
            retval = std::min(retval,rtv);
        }
    }
    return boost::make_tuple(retval,idx,c);
}

sbmt::fatter_syntax_rule::tree_node const*
find_corner(subder_ptr const& sd)
{
    sbmt::fatter_syntax_rule::tree_node const* ret;
    int idx;
    std::list<subder_ptr>::const_iterator c;
    boost::tie(ret,idx,c) = find_corner(sd,sd->root,0,sd->children.begin());
    return std::min(sd->root + 100000000,ret);
}

bool subder_is_lex( subder_ptr const& sd
                  , aligned_rule const& ar
                  , sbmt::fatter_syntax_rule::tree_node const* corner )
{
    return (corner - sd->root < 100000000);
}

bool subder_is_lex( subder_ptr const& sd
                  , aligned_rule const& ar )
{
    sbmt::fatter_syntax_rule::tree_node const* corner = find_corner(sd);
    return subder_is_lex(sd,ar,corner);
}

void insert_rule(aligned_rule const& ar, std::map<std::string,std::string>& printmap)
{
    std::string x;
    bool found = false;
    BOOST_FOREACH(sbmt::fatter_syntax_rule::tree_node const& nd, ar.rule.lhs()) {
        x = nd.get_token().label();
        if (nd.lexical()) { 
            found = true; 
            break; 
        }
    }
    std::string ret = x;
    int n = 0;
    if (not found) while (true) {
        std::stringstream sstr;
        sstr << std::setfill('0') << std::setw(8) << n;
        ret = x + "." + sstr.str();
        if (printmap.find(ret) == printmap.end()) break;
        ++n;
    }
    printmap.insert(std::make_pair(ret,rule_token(ar.rule)));
}

int fitness( subder_ptr const& sd
           , aligned_rule const& ar
           , int maxheight
           , int maxwidth )
{
    
    int fit = 0;
    if (subder_is_lex(sd,ar)) {
        int h = height(sd->rule.rule);
        int w = width(sd->rule.rule);
        if (h > maxheight) fit += (h - maxheight);
        if (w > maxwidth) fit += (w - maxwidth);
    }
    BOOST_FOREACH(subder_ptr const& ci, sd->children) {
        fit = std::max(fit,fitness(ci,ar,maxheight,maxwidth));
    }
    return fit;
}

void print_lex_leafs( std::map<std::string,std::string>& printmap
                    , subder_ptr const& sd
                    , aligned_rule const& ar
                    , int maxheight
                    , int maxwidth );

void print_lex_leaf( std::map<std::string,std::string>& printmap
                    , subder_ptr const& sd
                    , aligned_rule const& ar
                    , int maxheight
                    , int maxwidth  )
{
    //std::cerr << "BLL\n";
    if (height(sd->rule.rule) <= maxheight and width(sd->rule.rule) <= maxwidth) {
        insert_rule(sd->rule,printmap);
    } else {
        aligned_rule nar = sd->rule;
        subder_ptr nsd = get_subder(nar,*nar.rule.lhs_root());
        std::map<int,std::pair<subder_ptr,aligned_rule> > fitmap;
        BOOST_FOREACH(subder_ptr c, nsd->rhs) {
            //std::cerr << "_BLL\n";
            if (subder_is_lex(c)) {
                aligned_rule::align_data::right_map::iterator ritr,rnext,rend;
                boost::tie(ritr,rend) = nar.align.right.equal_range(c->source_begin);
                
                for (; ritr != rend; ++ritr) {
                    //std::cerr << "__BLL\n";
                    //std::cerr << ritr->second->get_token().label() << "-" << ritr->second->get_token().label() << '\n';
                    aligned_rule::align_data::right_map::value_type v = *ritr;
                    ritr = nar.align.right.erase(ritr);
                    //std::cerr << ritr->second->get_token().label() << "-" << ritr->second->get_token().label() << '\n';

                    //if (ritr != rend) std::cerr << ritr->second->get_token().label() << "-" << ritr->second->get_token().label() << '\n';
                    //else std::cerr << "end\n";
                    aligned_rule nnar = nar;
                    //fatter_syntax_rule::
                    //nnar.erase(nnar.rule.lhs_begin() + (v.second - nar.rule.lhs_begin()), nnar.rule.rhs_begin() + (v.first -)
                    subder_ptr nnsd = get_subder(nnar,*nnar.rule.lhs_root());
                    fitmap.insert(std::make_pair(fitness(nnsd,nnar,maxheight,maxwidth),std::make_pair(nnsd,nnar)));
                    ritr = nar.align.right.insert(v).first;
                    //std::cerr << "__ELL\n";
                }
            }
            //std::cerr << "_ELL\n";
        }
        if (fitmap.empty()) std::cerr << "warning:\n" << nar.rule << "\ncannot be made smaller. omitted\n";
        else print_lex_leafs(printmap,fitmap.begin()->second.first,fitmap.begin()->second.second,maxheight,maxwidth);
    }
    //std::cerr << "ELL\n";
}

void print_lex_leafs( std::map<std::string,std::string>& printmap
                    , subder_ptr const& sd
                    , aligned_rule const& ar
                    , int maxheight
                    , int maxwidth )
{
    bool print_self = subder_is_lex(sd,ar);
    BOOST_FOREACH(subder_ptr const& ci, sd->children) {
        print_lex_leafs(printmap,ci,ar,maxheight,maxwidth);
    }
    if (print_self) print_lex_leaf(printmap,sd,ar,maxheight,maxwidth);

}



struct options {
    int  maxheight;
    int  maxwidth;
    bool lexonly;
    bool ruleformat;
    bool empties;
    options() 
    : maxheight(1000000000)
    , maxwidth(1000000000)
    , lexonly(false)
    , ruleformat(false)
    , empties(false) {}
};

std::auto_ptr<options> parse_options(int argc, char** argv)
{
    std::string ptrfeats;
    using namespace boost::program_options;
    options_description desc;
    std::auto_ptr<options> opts(new options());
    desc.add_options()
        ( "maxheight,h"
        , graehl::defaulted_value(&opts->maxheight)
        )
        ( "maxwidth,w"
        , graehl::defaulted_value(&opts->maxwidth)
        )
        ( "help,?"
        , "produce help message"
        )
        ( "lexonly,l"
        , bool_switch(&opts->lexonly)
        )
        ( "rule-format,r"
        , bool_switch(&opts->ruleformat)
        )
        ( "empty-lines,e"
	, bool_switch(&opts->empties)
	)
        ;
    
    variables_map vm;
    basic_command_line_parser<char> cmd(argc,argv);
    store(cmd.options(desc).run(),vm);
    notify(vm);
    
    if (vm.count("help")) {
        std::cerr << desc << std::endl;
        exit(0);
    }

    return opts;
}

void map_tokens(rule_data& rule)
{
    lblmap.clear();
    BOOST_FOREACH(lhs_node& tn, rule.lhs) {
        std::stringstream sstr;
        sstr << std::setfill('0') << std::setw(8) << lblmap.size();
        lblmap.insert(std::make_pair(sstr.str(),tn.label));
        tn.label = sstr.str();
    }
}

void insert_variables(std::map<std::string,std::string>& printmap, aligned_rule const& ar)
{
    BOOST_FOREACH(sbmt::fatter_syntax_rule::tree_node const& nd, ar.rule.lhs()) {
        if (nd.indexed()) {
            std::stringstream sstr;
            sstr << '#' << nd.index();
            printmap.insert(std::make_pair(nd.get_token().label(),sstr.str()));
        }
    }
}



int main(int argc, char** argv)
{
    std::auto_ptr<options> opts = parse_options(argc,argv);
    std::string line;
    while (getline(std::cin,line)) {
        std::map<std::string,std::string> printmap;
        rule_data rule;
        try {
            rule = parse_xrs(line);
        } catch (std::exception const& e) { 
            std::cerr << e.what() << '\n'; 
            if (opts->empties) std::cout << '\n';
            continue; 
        }
        map_tokens(rule);
        insert_variables(printmap,rule);
        int nvars = printmap.size();
        aligned_rule ar(rule);
        subder_ptr sd = get_subder(ar,*ar.rule.lhs_root(),false,false,false,false,true);
        
        print_lex_leafs(printmap,sd,ar,opts->maxheight,opts->maxwidth);
        
        std::string v;
        std::string a;
        bool first = true;
        if (opts->ruleformat) std::cout <<rule.id << "\tleaflm_string={{{";
        BOOST_FOREACH(boost::tie(v,a), printmap) {
            if (not first) std::cout << ' ';
            std::cout << a;
            first = false;
        }
        if (opts->ruleformat) std::cout << "}}} leaf-length=" << printmap.size() - nvars;
        std::cout << '\n';
    }
}
