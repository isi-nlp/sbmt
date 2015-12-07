# include <gusc/filesystem/create_directories.hpp>
# include <gusc/trie/traverse_trie.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/convenience.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/filesystem/operations.hpp>
# include <string>

namespace gusc {

using boost::filesystem::path;
using std::string;

////////////////////////////////////////////////////////////////////////////////

struct create_directories_visitor {      
    path p;
    
    template <class Trie>
    void at_state(Trie& tr, typename Trie::state s) const
    {
        path pp = p / tr.key(s);
        if (!pp.empty()) create_directory(pp);
        //clog << (p / tr.key(s)) << endl;
    }
    
    template <class Trie>
    void begin_children(Trie& tr, typename Trie::state s) 
    {
        p /= tr.key(s);
    }
    
    template <class Trie>
    void end_children(Trie& tr, typename Trie::state s) 
    {
        p = p.branch_path();
    }
};

////////////////////////////////////////////////////////////////////////////////

void create_directories( basic_trie<boost::filesystem::path,int> const& paths )
{
    create_directories_visitor vis;
    traverse_trie(paths,vis);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc
