# include <string>
# include <sstream>
# include <iostream>
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <gusc/string/escape_string.hpp>
# include <gusc/trie/basic_trie.hpp>
# include <gusc/trie/fixed_trie.hpp>
# include <gusc/trie/trie_algo.hpp>
# include <algorithm>
# include <iterator>
# include <boost/regex.hpp>
# include <boost/algorithm/string.hpp>
# include <boost/interprocess/managed_shared_memory.hpp>
# include <boost/interprocess/containers/map.hpp>
# include <boost/interprocess/containers/string.hpp>
# include <boost/interprocess/containers/vector.hpp>
# include <boost/interprocess/allocators/allocator.hpp>
# include <boost/interprocess/managed_mapped_file.hpp>
# include <boost/foreach.hpp>

namespace ip = boost::interprocess;

typedef ip::allocator<char, ip::managed_mapped_file::segment_manager> char_allocator;
typedef ip::basic_string<char, std::char_traits<char>, char_allocator> mmap_string;


typedef ip::allocator<boost::tuple<boost::uint32_t,boost::uint32_t>,ip::managed_mapped_file::segment_manager> match_allocator;
typedef ip::vector<boost::tuple<boost::uint32_t,boost::uint32_t>,match_allocator> match_vector;

typedef ip::allocator<std::pair<boost::uint32_t const, boost::tuple<mmap_string,mmap_string> >
        , ip::managed_mapped_file::segment_manager
        > training_allocator;
typedef ip::map<boost::uint32_t,boost::tuple<mmap_string,mmap_string>,std::less<boost::uint32_t>,training_allocator> training_map;

typedef ip::allocator<std::pair<boost::uint32_t const,match_vector>,ip::managed_mapped_file::segment_manager> match_map_allocator;
typedef ip::map<boost::uint32_t,match_vector,std::less<boost::uint32_t>,match_map_allocator> match_map;

typedef gusc::fixed_trie<char,boost::uint32_t,char_allocator,gusc::create_plain> fixed_trie;

std::string normstr(std::string const& str)
{
    std::vector<std::string> v;
    boost::split(v,str,boost::algorithm::is_any_of("\t "),boost::token_compress_on);
    return " " + boost::algorithm::join(v," ") + " ";
}

void write_memmap(std::string const& fname, std::istream& in)
{
    {
    std::multimap<int, std::string> strset;
    std::string line;
    while(getline(in,line)) {
        strset.insert(std::make_pair(line.size(),line));
    }
    ip::managed_mapped_file mfile(ip::create_only,fname.c_str(),10UL*1024UL*1024UL*1024UL);
    char_allocator calloc(mfile.get_segment_manager());
    training_allocator talloc(mfile.get_segment_manager());
    match_allocator maalloc(mfile.get_segment_manager());
    match_map_allocator mmalloc(mfile.get_segment_manager());

    training_map* tm = mfile.construct<training_map>("training-map")(std::less<boost::uint32_t>(),talloc);
    match_map* mm = mfile.construct<match_map>("match-map")(std::less<boost::uint32_t>(),mmalloc);
    gusc::basic_trie<char,boost::uint32_t> trie;
    int sz;
    BOOST_FOREACH(boost::tie(sz,line),strset) {
        std::vector<std::string> srctgt;
        boost::split(srctgt,line,boost::algorithm::is_any_of("\t"));
        std::string src = normstr(srctgt[0]);
        std::string tgt = normstr(srctgt[1]);
        
        
        for (size_t m = 0; m != src.size(); ++m) {
            for (size_t M = m + 1; M != src.size(); ++M) {
                if (src[m] == ' ' and src[M] == ' ') {
                    bool inserted;
                    boost::uint32_t mmidx = mm->size();
                    gusc::basic_trie<char,boost::uint32_t>::state s;
                    boost::tie(inserted,s) = trie.insert(src.begin() + m + 1, src.begin() + M, mmidx);
                    if (not inserted) {
                        mmidx = trie.value(s);
                    } else {
                        mm->insert(std::make_pair(mmidx,match_vector(maalloc)));
                    }
                    match_vector& mv = mm->find(mmidx)->second;
                    if (mv.size() < 20) mv.push_back(boost::make_tuple(tm->size(),m));
                }
            }
        }
        boost::uint32_t tmidx = tm->size();
        std::cerr << tmidx << '\n';
        tm->insert( std::make_pair(
                      tmidx
                    , boost::make_tuple(
                        mmap_string(src.begin(),src.end(),calloc)
                      , mmap_string(tgt.begin(),tgt.end(),calloc)
                      )
                    )
                  )
                  ;
    }

    mfile.construct<fixed_trie>("trie")(trie,trie.start(),calloc);
    }
    
    ip::managed_mapped_file::shrink_to_fit(fname.c_str());
}

void print_line(mmap_string const& src, mmap_string const& tgt)
{
    std::cout << "SOURCE:" << src << std::endl << std::endl;
    std::cout << "TARGET:" << tgt << std::endl << std:: endl;
    std::cout << std::endl << std::endl;
}
    
int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);   
    if (argc == 3 and strcmp(argv[1],"-c") == 0) {
        write_memmap(argv[2],std::cin);
    } else {
        ip::managed_mapped_file mfile(ip::open_only,argv[1]);
        training_map* tm = mfile.find<training_map>("training-map").first;
        match_map* mm = mfile.find<match_map>("match-map").first;
        fixed_trie* trie = mfile.find<fixed_trie>("trie").first;
        std::string line;
        std::cout << "enter phrase (or \"RE \" followed by regex): ";
        while (getline(std::cin,line)) {
            if (line.substr(0,3) == "RE ") {
                boost::regex re(line.substr(3,std::string::npos));
                int count = 0;
                training_map::iterator itr = tm->begin(), end = tm->end();
                std::cout << "SEARCHING TRAINING DATA\n" << std::endl;
                for (; itr != end and count < 20; ++itr) {
                    if (boost::regex_search(itr->second.get<0>().begin(),itr->second.get<0>().end(),re)) {
                        print_line(itr->second.get<0>(),itr->second.get<1>());
                        ++count;
                    }
                }
            } else {
                gusc::trie_find_result<fixed_trie,std::string::iterator> 
                    res = trie_find(*trie,trie->start(),line.begin(),line.end());
                if (res.found) {
                
                    uint32_t idx = trie->value(res.state);
                    uint32_t sid;
                    uint32_t strt;
                    std::cout << "FOUND " << mm->find(idx)->second.size() << " examples\n\n";
                    BOOST_FOREACH(boost::tie(sid,strt), mm->find(idx)->second) {
                        print_line(tm->find(sid)->second.get<0>(),tm->find(sid)->second.get<1>());
                    }
                } else {
                    std::cout << "NOT FOUND\n" << std::endl;
                }
            }
            std::cout << "enter phrase (or \"RE \" followed by regex): ";
        }
    }
    std::cout << '\n';
}    
    
    
    
    
    

