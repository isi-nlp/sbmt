# if ! defined(GUSC__CREATE_DIRECTORIES_HPP)
# define       GUSC__CREATE_DIRECTORIES_HPP

# include <gusc/trie/basic_trie.hpp>
# include <string>
# include <boost/filesystem/path.hpp>


namespace gusc {

void create_directories( basic_trie<boost::filesystem::path,int> const& paths );
////////////////////////////////////////////////////////////////////////////////
///
///  given a collection of paths, creates all necessary 
///  directories in the paths, without attempting to create any
///  directory more than once.
///
///  leaf items are not created, since it is usually unclear whether a leaf
///  is intended to be a file or a directory.
///
////////////////////////////////////////////////////////////////////////////////
template <class Itr>
void create_directories(Itr beg, Itr end)
{
  basic_trie<boost::filesystem::path,int> paths(0);
    for (Itr itr = beg; itr != end; ++itr) {
        boost::filesystem::path d = itr->branch_path();
        paths.insert(d.begin(), d.end(), 1);
    }
    create_directories(paths);
}

} // namespace gusc

# endif //     GUSC__CREATE_DIRECTORIES_HPP

