# include <iostream>
# include <treelib/Tree.h>
# include <string>
# include <boost/foreach.hpp>
# include <tr1/unordered_map>
# include <boost/tuple/tuple.hpp>
//# include <rule_head/at_replace.hpp> 

std::string at_replace(std::string str);

typedef std::tr1::unordered_map<std::string,size_t> tag_counts;

typedef std::tr1::unordered_map<std::string, tag_counts> backoff_counts;

void add_entries(treelib::Tree::iterator_base const& node, backoff_counts& bc, bool tag) 
{
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    
    if (itr != end and not node->getIsTerminal()) {
        std::string label = node->getLabel();
        std::string hword;
        if (tag) {
            hword = node->getIsPreterminal() ? node->getLabel() : node->getHeadPOS() ;
        } else {
            hword = node->getIsPreterminal() ? at_replace(node.begin()->getLabel()) : at_replace(node->getHeadword()) ;
        }
        bc[label][hword] += 1;
        for (; itr != end; ++itr) add_entries(itr,bc,tag);
    }
}

int main(int argc, char** argv)
{
    bool pos = false;
    if (argc >= 2 and argv[1] == std::string("-t")) pos = true;
    std::ios::sync_with_stdio(false);
    backoff_counts bc;
    
    std::string line;
    while (getline(std::cin,line)) {
        treelib::Tree tree;
        tree.read(line);
        add_entries(tree.begin(),bc,pos);

    }
    
    for (backoff_counts::iterator itr = bc.begin(); itr != bc.end(); ++itr) {
        for (tag_counts::iterator jtr = itr->second.begin(); jtr != itr->second.end(); ++jtr) {
            std::cout << itr->first << '\t' << jtr->first << '\t' << jtr->second << '\n';
        }
    }
    return 0;
}
