# include <binalgo.hpp>
# include <filesystem.hpp>
# include <collapsed_signature_iterator.hpp>
# include <syntax_rule_util.hpp>

# include <iostream>
# include <stdexcept>
# include <iterator>
# include <cstdlib>

namespace sbmt {

std::istream& operator >> (std::istream& in, indexed_token& tok)
{
    boost::uint32_t idx;
    in >> idx;
    tok = indexed_token(idx);
    return in;
}

}

using namespace boost;
using namespace boost::posix_time;
using namespace boost::filesystem;
using namespace boost::serialization;
using namespace boost::archive;
using namespace sbmt;
using namespace xrsdb;

using std::vector;
using std::string;
using std::swap;
using std::stringstream;
using std::istream_iterator;
using std::ostream_iterator;
using std::exception;
using std::cin;
using std::cerr;
using std::clog;
using std::endl;

struct lenbalance {
    explicit lenbalance(indexed_token root) : root(root) {}
    bool operator()( vector<indexed_token> const& v1
                   , vector<indexed_token> const& v2 ) const
    {
        if (v1.size() < v2.size()) return true;
        if (v1.size() > v2.size()) return false;
        size_t c1 = 0;
        size_t c2 = 0;
        BOOST_FOREACH(indexed_token r, v1) {
            if (r == root) break;
            ++c1;
        }
        BOOST_FOREACH(indexed_token r, v2) {
            if (r == root) break;
            ++c2;
        }
        double d1 = std::abs(double(v1.size())/2.0 - c1);
        double d2 = std::abs(double(v2.size())/2.0 - c2);
        return d1 < d2;
    }
    indexed_token root;
};

typedef std::multimap<vector<indexed_token>,string,lenbalance> cluster_t;

////////////////////////////////////////////////////////////////////////////////

tuple<path,indexed_token,vector<indexed_token>,string>
procline(string const& s)
{
    static regex splt("([^\\t]*)\\t([^\\t]*)\\t([^\\t]*)\\t(.*)\\s*");
    smatch what;
    regex_match(s,what,splt);

    tuple<path,indexed_token,vector<indexed_token>,string>
        tpl( lexical_cast<path>(what.str(1))
           , lexical_cast<indexed_token>(what.str(2))
           , vector<indexed_token>()
           , what.str(4)
           );

    stringstream sstr(what.str(3));
    copy( istream_iterator<indexed_token>(sstr)
        , istream_iterator<indexed_token>()
        , back_inserter(tpl.get<2>())
        );

    return tpl;
}

void proccluster(virtmap const& global, cluster_t const& cluster)
{
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    virtmap virts;
    
    std::string line;
    path groupid;
    indexed_token rarest;
    indexed_token nr;
    vector<indexed_token> keys;
    string rule;
    if (getline(cin,line)) {
        tie(groupid,rarest,keys,rule) = procline(line);
        lenbalance lb(rarest);
        cluster_t cluster(lb);
        cluster.insert(make_pair(keys,rule));
        while (getline(cin,line)) {
            tie(groupid,nr,keys,rule) = procline(line);
            if (nr == rarest) {
                cluster.insert(make_pair(keys,rule));
            } else {
                proccluster(virts,cluster);
                cluster = cluster_t(lenbalance(nr));
                rarest = nr;
            }
        }
        proccluster(virts,cluster);
    }
    return 0;
}