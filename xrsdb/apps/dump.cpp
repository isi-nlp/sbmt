# include <filesystem.hpp>
# include <word_cluster.hpp>
# include <iostream>
# include <boost/filesystem/operations.hpp>
# include <boost/filesystem/convenience.hpp>
# include <map>
# include <string>

using namespace boost::filesystem;
using namespace xrsdb;

void dump(std::ostream& os, header& h, path p,sbmt::indexed_token tok)
{
    typedef std::pair<sbmt::indexed_token,uint64_t> pt_type;
    if (tok != sbmt::indexed_token()) {
        std::cerr << "cluster: " << tok << '\n';
        load_word_cluster(p,h,tok).dump(os);
    } else if (not h.old_version()) {
        BOOST_FOREACH(pt_type pt,h.offsets) {
            std::cerr << "cluster: " << pt.first << '\n';
            load_word_cluster(p,h,pt.first).dump(os);
        }
    } else {
        if (is_directory(p)) {
            directory_iterator itr(p), end;
            for (; itr != end; ++itr) {
                dump(os,h,*itr,tok);
            }
        } else if (p.leaf() != "header") {
            load_word_cluster(p,0).dump(os);
        }
    }
}

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    path root(argv[1]);
    header h;
    load_header(h,root);
    sbmt::indexed_token tok;
    std::cerr << sbmt::token_label(h.dict);
    if (argc > 2) {
        std::string line;
        while (getline(std::cin,line)) {
            tok = h.dict.foreign_word(line);
            dump(std::cout,h,root,tok);
        }
    } else {
        dump(std::cout,h,root,tok);
    }
    return 0;
}
