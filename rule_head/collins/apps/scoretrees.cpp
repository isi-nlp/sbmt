# include <collins/lm.hpp>
# include <iostream>
# include <string>
# include <boost/lexical_cast.hpp>

double score_tree(treelib::Tree::iterator_base const& node, collins::model const& lm) 
{
    double logprob = 0;
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    if ((not node->getIsPreterminal()) and itr != end) {
        collins::strrule sr = collins::make_strrule(node);
        collins::idxrule ir = collins::str2idxrule(sr,lm);
        std::cerr << sr << " --> " << ir << " --> " << collins::idx2strrule(ir,lm) << '\n';
        logprob += lm.logprob(ir);
        itr = node.begin();
        for (; itr != end; ++itr) logprob += score_tree(itr,lm);
    }
    return logprob;
}

double score_tree_cfg(treelib::Tree::iterator_base const& node, collins::model const& lm) 
{
    double logprob = 0;
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    if ((not node->getIsPreterminal()) and itr != end) {
        collins::strrule sr = collins::make_strrule(node);
        collins::idxrule ir = collins::str2idxrule(sr,lm);
        std::cerr << sr << " --> " << ir << " --> " << collins::idx2strrule(ir,lm) << '\n';
        logprob += lm.logprob_amr(ir);
        itr = node.begin();
        for (; itr != end; ++itr) logprob += score_tree_cfg(itr,lm);
    }
    return logprob;
}

int main(int argc, char** argv)
{
    double wb_rh_rate = 1.0;
    double wb_dr_rate = 1.0;
    double wb_hr_rate = 1.0;
    double wb_cf_rate = 1.0;
    wb_rh_rate = boost::lexical_cast<double>(argv[2]);
    wb_dr_rate = boost::lexical_cast<double>(argv[3]);
    wb_hr_rate = boost::lexical_cast<double>(argv[4]);
    wb_cf_rate = boost::lexical_cast<double>(argv[5]);
    std::ios::sync_with_stdio(false);
    collins::model lm(argv[1],wb_rh_rate,wb_dr_rate,wb_hr_rate,wb_cf_rate);
    std::string line;
    double lmscr = 0.0;
    double tmscr = 0.0;
    size_t n = 0;
    while (getline(std::cin,line)) {
        treelib::Tree tree;
        tree.read(line);
        double l = score_tree(tree.begin(),lm);
        double t = score_tree_cfg(tree.begin(),lm);
        std::cout << l << '\t' << t <<  std::endl;
        lmscr += l;
        tmscr += t;
        ++n;
    }
    std::cout << "avg: " << lmscr/n << '\t' << tmscr/n << std::endl;
}