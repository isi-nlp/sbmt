# include <treelib/Tree.h>
# include <iostream>
# include <string>
# include <boost/regex.hpp>

boost::regex bar("^(.*)-BAR$");

string relabel(treelib::Tree::iterator_base const& node)
{
    boost::smatch sm;
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    if (itr != end) {
        int x = 1;
        int hpos = node->getHeadPosition();
        string H;
        for (; itr != end; ++itr) {
            string h = relabel(itr);
            if (boost::regex_match(itr->getLabel(),sm,bar)) {
                if (x == 1) {
                    itr->setLabel(sm.str(1) +"-R-BAR");
                } else {
                    itr->setLabel(sm.str(1) + "-L-BAR");
                }
            }
            if (x == hpos) H = h;
            ++x;
        }
        if (boost::regex_match(node->getLabel(),sm,bar)) {
            node->setLabel(sm.str(1) + "-" + H + "-BAR");
            return H;
        } 
    }
    return node->getLabel();
}

int main()
{
    using namespace std;
    string line;
    while (getline(cin,line)) {
        if (line == "0") cout << "0\n";
        else {
            treelib::Tree tree;
            tree.read(line);
            relabel(tree.begin());
            tree.toStream(cout);
            cout << '\n';
        }
    }
}
