# if ! defined(XRSDB__WORD_CLUSTER_DB_HPP)
# define       XRSDB__WORD_CLUSTER_DB_HPP

////////////////////////////////////////////////////////////////////////////////

# include <boost/filesystem/path.hpp>
# include <filesystem.hpp>

namespace sbmt { class indexed_token; }

////////////////////////////////////////////////////////////////////////////////

namespace xrsdb {
    
class cluster_mru;

////////////////////////////////////////////////////////////////////////////////

class word_cluster_db
{
public:
    word_cluster_db(boost::filesystem::path const& p, size_t cache_sz_in_mb);
    word_cluster get(sbmt::indexed_token word);
    bool exists(sbmt::indexed_token word) const;
    ~word_cluster_db();
private:
    boost::scoped_ptr<cluster_mru> mru;
    boost::filesystem::path dbdir;
    header h;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace xrsdb

# endif //     XRSDB__WORD_CLUSTER_DB_HPP
