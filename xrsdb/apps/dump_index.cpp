# include <filesystem.hpp>
# include <word_cluster.hpp>
# include <iostream>
# include <boost/filesystem/operations.hpp>
# include <boost/filesystem/convenience.hpp>
# include <map>
# include <string>

using namespace boost::filesystem;
using namespace xrsdb;

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    path root(argv[1]);
    header h;
    load_header(h,root);
    typedef std::pair<sbmt::indexed_token,uint64_t> pt_type;
    BOOST_FOREACH(pt_type pt,h.offsets) {
        std::cout << h.dict.label(pt.first) << std::endl;
    }
    return 0;
}
