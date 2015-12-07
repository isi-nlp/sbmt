# include <boost/multi_index_container.hpp>
# include <boost/multi_index/hashed_index.hpp>
# include <boost/multi_index/identity.hpp>
# include <boost/multi_index/sequenced_index.hpp>
# include <boost/filesystem/path.hpp>
# include <sbmt/token.hpp>
# include <sbmt/token/indexed_token.hpp>
# include <word_cluster.hpp>
# include <word_cluster_db.hpp>
# include <filesystem.hpp>
# include <boost/filesystem/operations.hpp>

namespace mi = boost::multi_index;
namespace fs = boost::filesystem;
using namespace boost;
using namespace sbmt;
using namespace std;

namespace xrsdb {

////////////////////////////////////////////////////////////////////////////////

struct root_word {
    typedef indexed_token result_type;
    template <class X>
    indexed_token operator()(X const& x) const 
    {
        return x.root_word();
    }
};

////////////////////////////////////////////////////////////////////////////////

class cluster_mru {
    
    typedef multi_index_container<
        word_cluster
      , mi::indexed_by<
            mi::sequenced<>
          , mi::hashed_unique< root_word >
        >
    > mru_t;
    
    typedef mru_t::iterator sequence_iterator;
    typedef mru_t::nth_index<1>::type::iterator hash_iterator;
    
    mru_t mru;  
    size_t max_memory;
    size_t total_memory;          
    
public:
    cluster_mru(size_t max = 1024 * 1024 * 1024);
    word_cluster get(indexed_token tok);
    bool exists(indexed_token tok) const;
    void put(word_cluster const& wc);
};

////////////////////////////////////////////////////////////////////////////////

word_cluster cluster_mru::get(indexed_token tok)
{
    hash_iterator pos = mru.get<1>().find(tok);
    word_cluster wc = *pos;
    mru.relocate(mru.begin(),mru.project<0>(pos));
    return wc;
}

bool cluster_mru::exists(indexed_token tok) const
{
    hash_iterator pos = mru.get<1>().find(tok);
    return pos != mru.get<1>().end();
}

void cluster_mru::put(word_cluster const& wc)
{
    pair<sequence_iterator,bool> p = mru.push_front(wc);
    if (not p.second) {
        mru.relocate(mru.begin(), p.first);
    } else {
        total_memory += wc.memory_size();
        while (total_memory > max_memory and mru.size() > 1) {
            total_memory -= mru.back().memory_size();
            mru.pop_back();
        }
    }
}

cluster_mru::cluster_mru(size_t max)
 : max_memory(max)
 , total_memory(0) {}

////////////////////////////////////////////////////////////////////////////////

word_cluster_db::word_cluster_db(fs::path const& p, size_t in_mb)
  : mru(new cluster_mru(in_mb * 1024 * 1024))
  , dbdir(p) 
{
    load_header(h,dbdir);
}

word_cluster_db::~word_cluster_db() {}

////////////////////////////////////////////////////////////////////////////////

word_cluster word_cluster_db::get(indexed_token tok)
{
    if (mru->exists(tok)) return mru->get(tok);
    else {
        word_cluster wc = load_word_cluster(dbdir,h,tok);
        mru->put(wc);
        return wc;
    }
}

////////////////////////////////////////////////////////////////////////////////

bool word_cluster_db::exists(indexed_token tok) const
{
    if (mru->exists(tok)) return true;
    else return cluster_exists(dbdir,h,tok);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace xrsdb
