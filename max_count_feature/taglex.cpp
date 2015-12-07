# include <tr1/unordered_map>
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_comparison.hpp>
# include <gusc/varray.hpp>
# include <iostream>
# include <fstream>
# include <string>
# include <sbmt/token/in_memory_token_storage.hpp>
# include <vector>
# include <boost/algorithm/string.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/functional/hash/extensions.hpp>
# include <boost/foreach.hpp>

namespace boost { namespace tuples {
    std::size_t hash_value(tuple<uint32_t,uint32_t> const& t)
    {
        std::size_t seed = 0;
        hash_combine(seed, t.get<0>());
        hash_combine(seed, t.get<1>());
        return seed;
    }
} }

typedef gusc::varray< std::pair<uint32_t,float> > featvec;

typedef std::tr1::unordered_map<
          boost::tuple<uint32_t,uint32_t>
        , featvec
        , boost::hash< boost::tuple<uint32_t,uint32_t> >
        > taglexmap;
        
void read_map(std::istream& in, taglexmap& tm, sbmt::in_memory_token_storage& tf)
{
    std::string line;
    while (getline(in,line)) {
        std::set< std::pair<uint32_t,float> > mp;
        std::vector<std::string> v;
        uint32_t f;
        uint32_t e;
        boost::split(v,line,boost::is_any_of("\t"));
        f = tf.get_index(v[0]);
        e = tf.get_index(v[1]);
        for (size_t x = 2; x != v.size(); ++x) {
            uint32_t t = tf.get_index(v[x]);
            ++x;
            assert(x != v.size());
            float p = boost::lexical_cast<float>(v[x]);
            mp.insert(std::make_pair(t,p));
        }
        tm.insert(std::make_pair(boost::make_tuple(f,e),featvec(mp.begin(),mp.end())));
    }
}

int main(int argc, char** argv)
{
    std::ifstream ifs(argv[1]);
    sbmt::in_memory_token_storage tf;
    taglexmap tm;
    read_map(ifs,tm,tf);
    
    std::string line;
    while (getline(std::cin,line)) {
        std::vector<std::string> v;
        uint32_t f;
        uint32_t e;
        boost::split(v,line,boost::is_any_of("\t"));
        f = tf.get_index(v[0]);
        e = tf.get_index(v[1]);
        taglexmap::iterator pos = tm.find(boost::make_tuple(f,e));
        if (pos != tm.end()) {
            std::cout << line;
            uint32_t t;
            float p;
            BOOST_FOREACH(boost::tie(t,p),pos->second) {
                std::cout << '\t' << tf.get_token(t) << '\t' << p;
            }
            std::cout << '\n';
        }
    }
    
    return 0;
}
