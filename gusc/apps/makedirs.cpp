# include <iostream>
# include <vector>
# include <gusc/filesystem/create_directories.hpp>
# include <boost/filesystem/path.hpp>
# include <gusc/filesystem_io.hpp>

template <class Itr>
void makedirs(Itr beg, Itr end)
{
    gusc::basic_trie<boost::filesystem::path,int> paths(0);
    for (Itr itr = beg; itr != end; ++itr) {
        paths.insert(itr->begin(), itr->end(), 1);
    }
    gusc::create_directories(paths);
}

int main(int argc, char** argv)
{
    using namespace std;
    using namespace boost::filesystem;
    
    ios_base::sync_with_stdio(false);
    makedirs(istream_iterator<path>(cin), istream_iterator<path>(/*end*/));
                  
    return 0;
}
