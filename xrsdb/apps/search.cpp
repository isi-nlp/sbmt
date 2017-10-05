//
//# ifndef NDEBUG
//# define BOOST_ENABLE_ASSERT_HANDLER 1
//# endif
# include <tr1/unordered_map>
# include <tr1/unordered_set>
# include <search/grammar.hpp>
# include <search/info_registry.hpp>
# include <search/xedge.hpp>
# include <search/sort.hpp>

# include <sbmt/logging.hpp>
# include <iostream>
# include <iterator>
# include <algorithm>
# include <set>

# include <sbmt/hash/hash_set.hpp>
# include <sbmt/token.hpp>
# include <sbmt/sentence.hpp>
# include <sbmt/grammar/tree_tags.hpp>
# include <sbmt/grammar/lm_string.hpp>
# include <sbmt/feature/accumulator.hpp>


# include <boost/algorithm/string/join.hpp>
# include <boost/algorithm/string/split.hpp>
# include <boost/algorithm/string/classification.hpp>
# include <boost/program_options.hpp>
# include <boost/iostreams/device/file_descriptor.hpp>
# include <boost/iostreams/copy.hpp>
# include <boost/iostreams/filter/zlib.hpp>
# include <boost/iostreams/device/back_inserter.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/iterator/zip_iterator.hpp>

# include <filesystem.hpp>
# include <db_usage.hpp>
# include <numproc.hpp>
# include <boost/function_output_iterator.hpp>
# include <boost/foreach.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/convenience.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/timer/timer.hpp>
# include <boost/graph/adjacency_list.hpp>
# include <boost/graph/properties.hpp>
# include <boost/graph/graphviz.hpp>
# include <boost/thread/shared_mutex.hpp>
# include <boost/thread/locks.hpp>
# include <boost/range/adaptor/reversed.hpp>

# include <gusc/functional.hpp>
# include <gusc/trie/sentence_lattice.hpp>
# include <gusc/generator/generator_from_iterator.hpp>
# include <gusc/iterator/iterator_from_generator.hpp>
# include <gusc/filesystem/create_directories.hpp>
# include <gusc/filesystem/path_from_integer.hpp>
# include <gusc/generator/transform_generator.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <gusc/generator/union_heap_generator.hpp>
# include <gusc/generator/product_heap_generator.hpp>
# include <gusc/generator/any_generator.hpp>
# include <gusc/generator/finite_union_generator.hpp>
# include <gusc/generator/peekable_generator_facade.hpp>
# include <word_cluster.hpp>
# include <word_cluster_db.hpp>
# include <lattice_reader.hpp>
# include <forest_reader.hpp>
# include <syntax_rule_util.hpp>


# include <map>
# include <vector>
# include <stack>

# include <boost/bind.hpp>
# include <boost/regex.hpp>
# include <boost/range.hpp>

# include <boost/iostreams/filter/line.hpp>
# include <boost/iostreams/filtering_stream.hpp>
# include <boost/iostreams/device/file.hpp>
# include <boost/iostreams/filter/gzip.hpp>

# include <boost/date_time/posix_time/posix_time.hpp>

# include <sbmt/search/impl/block_lattice_tree.tpp>
# include <sbmt/hash/thread_pool.hpp>

# include <tbb/task_scheduler_init.h>
# include <tbb/parallel_for.h>

# include <decode_sequence_reader.hpp>

# include <sbmt/io/logfile_registry.hpp>
namespace sbmt {
SBMT_SET_DOMAIN_LOGFILE(root_domain, "-2" );
SBMT_SET_DOMAIN_LOGGING_LEVEL(root_domain, info);
}

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(decoder_domain,"decoder",sbmt::root_domain);




typedef uint64_t biguint_t;
typedef int64_t bigint_t;



template<typename RandomAccessIterator, typename Compare>
struct quick_sort_range {
  static const size_t grainsize = 500;
  const Compare &comp;
  RandomAccessIterator begin;
  size_t size;

  quick_sort_range( RandomAccessIterator begin_, size_t size_, const Compare &comp_ ) :
    comp(comp_), begin(begin_), size(size_) {}

  bool empty() const {return size==0;}
  bool is_divisible() const {return size>=grainsize;}

  quick_sort_range( quick_sort_range& range, tbb::split ) : comp(range.comp) {
    RandomAccessIterator array = range.begin;
    RandomAccessIterator key0 = range.begin; 
    size_t m = range.size/2u;
    std::iter_swap(array + 0, array + m);

    size_t i=0;
    size_t j=range.size;
    // Partition interval [i+1,j-1] with key *key0.
    for(;;) {
      __TBB_ASSERT( i<j, NULL );
      // Loop must terminate since array[l]==*key0.
      do {
        --j;
        __TBB_ASSERT( i<=j, "bad ordering relation?" );
      } while( comp( *key0, array[j] ));
      do {
        __TBB_ASSERT( i<=j, NULL );
        if( i==j ) goto partition;
        ++i;
      } while( comp( array[i],*key0 ));
      if( i==j ) goto partition;
      std::iter_swap(array + i, array + j);
    }
  partition:
    // Put the partition key were it belongs
    std::iter_swap(array + j, key0);
    // array[l..j) is less or equal to key.
    // array(j..r) is greater or equal to key.
    // array[j] is equal to key
    i=j+1;
    begin = array+i;
    size = range.size-i;
    range.size = j;
  }
};

template<typename RandomAccessIterator, typename Compare>
struct quick_sort_body {
  void operator()( const quick_sort_range<RandomAccessIterator,Compare>& range ) const {
    //SerialQuickSort( range.begin, range.size, range.comp );
    quick_sort( range.begin, range.begin + range.size, range.comp );
  }
};

template<typename RandomAccessIterator, typename Compare>
void parallel_quick_sort( RandomAccessIterator begin, RandomAccessIterator end, const Compare& comp ) {
  tbb::task_group_context ctxt(tbb::task_group_context::isolated);
  tbb::auto_partitioner part;
  tbb::parallel_for( quick_sort_range<RandomAccessIterator,Compare>(begin, end-begin, comp )
                   , quick_sort_body<RandomAccessIterator,Compare>()
                   , part
                   , ctxt );
}

template<typename RandomAccessIterator, typename Compare>
void parallel_sort( RandomAccessIterator begin, RandomAccessIterator end, const Compare& comp) { 
  const int min_parallel_size = 500; 
  if( end > begin ) {
    if (end - begin < min_parallel_size) { 
      quick_sort(begin, end, comp);
    } else {
      parallel_quick_sort(begin, end, comp);
    }
  }
}

using namespace xrsdb;
using namespace xrsdb::search;
using namespace sbmt;
using namespace boost::filesystem;
//using namespace boost;

using namespace gusc;
using namespace boost::posix_time;

namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace bio = boost::iostreams;



using std::make_pair;
using std::cin;
using std::cout;
using std::cerr;
using std::clog;
using std::flush;
using std::endl;
using std::getline;
using std::string;
using std::ostream_iterator;
using std::ostream;
using std::multimap;
using std::map;
using std::pair;
using std::vector;
using std::set;

typedef std::tr1::unordered_map<size_t,rule_application const*> used_rules_map;

typedef boost::graph_traits<graph_t>::edge_iterator eitr_t;
typedef boost::graph_traits<graph_t>::edge_descriptor edge_t;
typedef boost::graph_traits<graph_t>::vertex_descriptor vtx_t;

struct hash_offset {
    size_t operator()(ip::offset_ptr<char> k) const { return size_t(k.get()); }
};

struct signature_trie_data;

typedef std::tr1::unordered_map<
          ip::offset_ptr<char>
        , signature_trie_data
        , hash_offset> signature_cache;

struct word_trie_data_impl {
    struct sorttdata {
        boost::shared_mutex mutex;
        boost::mutex workmutex;
        boost::condition_variable workfinished;
    };
    word_trie const* get();
    word_trie_data_impl();
    word_trie_data_impl(boost::shared_array<char> data, size_t sz,rule_sort_f sorter);
    word_trie_data_impl(std::string fname, biguint_t offset, rule_sort_f sorter);
    ~word_trie_data_impl();
    boost::shared_array<char> data;
    word_trie const* ptr;
    boost::shared_ptr<signature_cache> cache;
    rule_sort_f sorter;
    std::string fname;
    biguint_t foffset;
    bool reloadable;
    void clear_cache(rule_sort_f s);// { cache->clear(); sorter = s; }
    void drop(bool nullify = true);
    void load();
    boost::shared_mutex mtx;
    std::vector<boost::shared_ptr<sorttdata> > smtx;
    static bool operate_cache;
};

bool word_trie_data_impl::operate_cache = false;

struct word_trie_data {
    word_trie const* get() const { return impl ? impl->get() : 0; }
    word_trie const* operator->() const { return get(); }
    word_trie const& operator*() const { return *get(); }
    signature_cache* cache() const { return impl->cache.get(); }
    rule_sort_f sorter() const { return impl->sorter; }
    void clear_cache(rule_sort_f s) { impl->clear_cache(s); }
    void drop(bool nullify = true) { if (impl) impl->drop(nullify); }
    word_trie_data(boost::shared_array<char> data, size_t sz,rule_sort_f sorter)
    : impl(new word_trie_data_impl(data,sz,sorter)) {}
    word_trie_data(std::string name, biguint_t offset, rule_sort_f sorter)
    : impl(new word_trie_data_impl(name,offset,sorter)) {}
    word_trie_data() {}
    boost::shared_mutex& mutex() { return impl->mtx; }
    word_trie_data_impl::sorttdata& sortit(ip::offset_ptr<char> sm) 
    { return *(impl->smtx[hash_offset()(sm) % 23]); }
    
private:
    boost::shared_ptr<word_trie_data_impl> impl;
};

boost::tuple<boost::shared_array<char>,size_t>
load_word_trie(std::ifstream& ifs, biguint_t offset)
{
    ifs.seekg(offset);
    biguint_t sz;
    boost::iostreams::read(ifs,(char*)(&sz),sizeof(biguint_t));
    boost::shared_array<char> data(new char[sz]);
    boost::iostreams::read(ifs,data.get(),sz);
    return boost::make_tuple(data,sz);
}

struct signature_trie_data {
    signature_trie const* get() const { return ptr; } 
    signature_trie const* operator->() const { return ptr; }
    signature_trie const& operator*() const { return *ptr; }
    
    signature_trie* get() { return ptr; } 
    signature_trie* operator->() { return ptr; }
    signature_trie& operator*() { return *ptr; }
    
    signature_trie_data() {}
    signature_trie_data(boost::shared_array<char> data, size_t sz)
    : data(data)
    , ptr(external_buffer_type(ip::open_only,data.get(),sz).find<signature_trie>("root").first)
    {}
private:
    boost::shared_array<char> data;
    signature_trie* ptr;
};

void word_trie_data_impl::load()
{
    fs::path tmpdir, cpath;
    if (operate_cache) {
        tmpdir = fs::temp_directory_path();
        cpath = tmpdir / fs::path(fname) / boost::lexical_cast<std::string>(foffset);
    }
    bool using_cache = operate_cache and fs::exists(cpath);
    if (cache->size()) throw std::runtime_error("cache non-empty while word_trie reloading");
    uint64_t sz;
    if (using_cache) {
        SBMT_VERBOSE_STREAM(decoder_domain, "loading from " << cpath);
        std::ifstream ifs(cpath.native().c_str());
        boost::tie(data,sz) = load_word_trie(ifs,0);
    } else {
        std::ifstream ifs(fname.c_str());
        SBMT_VERBOSE_STREAM(decoder_domain,"loading from " << fname << ":" << foffset);
        boost::tie(data,sz) = load_word_trie(ifs,foffset);
        if (operate_cache) {
            uint64_t avail = space(tmpdir).available;
            if ((avail > 1024 * 1024 * 1024) and (10 * sz < avail)) {
                SBMT_VERBOSE_STREAM(decoder_domain, "creating cache location " << cpath.parent_path());
                fs::create_directories(cpath.parent_path());
                SBMT_VERBOSE_STREAM(decoder_domain, "writing cache " << cpath);
                fs::path rndm = fs::unique_path(tmpdir / "%%%%%%%%");
                std::ofstream ofs(rndm.native().c_str());
                boost::iostreams::write(ofs,(char*)(&sz),sizeof(uint64_t));
                boost::iostreams::write(ofs,data.get(),sz);
                ofs.close();
                fs::rename(rndm,cpath);
            }
        } 
    }
    ptr = external_buffer_type(ip::open_only,data.get(),sz).find<word_trie>("root").first;
}

word_trie const* word_trie_data_impl::get()
{
    if ((not ptr) and reloadable) {load();}
    if (not ptr) {
        throw std::runtime_error("word_trie not loaded");
    }
    return ptr;
}

void word_trie_data_impl::drop(bool nullify)
{
    if (not reloadable) throw std::runtime_error("cannot drop non-reloadable word_trie");
    if (nullify) {
        data.reset();
        //cache.reset(new signature_cache());
        ptr = 0;
    } else {
        //boost::shared_array<char> data;
        //word_trie const* ptr;
        int sz = ptr->memory_size() + 1024;
        boost::shared_array<char> newdata(new char[sz]);
        external_buffer_type wb(ip::create_only,newdata.get(),sz);
        char_allocator alloc(wb.get_segment_manager());
        wb.construct<word_trie>("root")(*ptr,alloc);
        data = newdata;
        ptr = wb.find<word_trie>("root").first;
    }
}

word_trie_data_impl::word_trie_data_impl() {}

word_trie_data_impl::word_trie_data_impl(boost::shared_array<char> data, size_t sz,rule_sort_f sorter)
: data(data)
, ptr(external_buffer_type(ip::open_only,data.get(),sz).find<word_trie>("root").first)
, cache(new signature_cache())
, sorter(sorter)
, reloadable(false)
, smtx(23)
{
   for (size_t x = 0; x != smtx.size(); ++x) smtx[x].reset(new sorttdata());
}

word_trie_data_impl::word_trie_data_impl(std::string name, biguint_t offset, rule_sort_f sorter)
: ptr(0)
, cache(new signature_cache())
, sorter(sorter)
, fname(name)
, foffset(offset)
, reloadable(true)
, smtx(23)
{
    for (size_t x = 0; x != smtx.size(); ++x) smtx[x].reset(new sorttdata());
    load();
}

word_trie_data_impl::~word_trie_data_impl() {}

void word_trie_data_impl::clear_cache(rule_sort_f s) { cache->clear(); sorter = s; }

word_trie_data
load_word_trie( std::ifstream& ifs, biguint_t offset, rule_sort_f sorter )
{
    ifs.seekg(offset);
    biguint_t sz;
    boost::iostreams::read(ifs,(char*)(&sz),sizeof(biguint_t));
    boost::shared_array<char> data(new char[sz]);
    boost::iostreams::read(ifs,data.get(),sz);
    return word_trie_data(data,sz,sorter);
}

struct cluster_search_data {
    word_trie_data wtd;
    boost::shared_ptr<cluster_search> search;
    rule_sort_f sorter() const { return wtd.sorter(); }
    signature_cache* cache() const { return wtd.cache(); }
    
    cluster_search* get() const { return search.get(); }
    cluster_search* operator->() const { return search.get(); }
    cluster_search& operator*() const { return *search; }
    
    cluster_search_data( header const& h
                       , word_trie_data trie
                       , sbmt::graph_t const* g
                       , boost::graph_traits<sbmt::graph_t>::vertex_descriptor b 
                       )
    : wtd(trie)
    , search(new cluster_search(h,wtd.get(),g,b)) {}
                  
    cluster_search_data( header const& h
                       , word_trie_data trie
                       , sbmt::graph_t const* g
                       , boost::graph_traits<sbmt::graph_t>::vertex_descriptor b
                       , boost::graph_traits<sbmt::graph_t>::edge_descriptor e 
                       )
    : wtd(trie)
    , search(new cluster_search(h,trie.get(),g,b,e)) {}
};

signature_trie_data
load_signature_trie(compressed_signature_trie const& st)
{
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::zlib_decompressor());
    in.push(boost::make_iterator_range(st.get<0>(),st.get<0>() + st.get<1>()));
    std::vector<char> temp;
    boost::iostreams::copy(in, boost::iostreams::back_inserter(temp));
    boost::shared_array<char> data(new char[temp.size()]);
    std::copy(temp.begin(),temp.end(),data.get());
    return signature_trie_data(data,temp.size());
}








using boost::detail::atomic_count;
typedef boost::shared_ptr<atomic_count> shared_atomic_count;


struct row_hold_data {
    boost::mutex mtx;
    boost::condition cond;
};

typedef boost::shared_ptr<row_hold_data> shared_row_hold_data;

typedef std::map<uint16_t, shared_row_hold_data> row_holds;
typedef std::map<span_t, int> xchart_holds;

typedef std::map<span_t, xspan> xchart;

struct first_equal {
    template <class T>
    bool operator()(T const& t1, T const& t2) const
    {
        return t1[0] == t2[0];
    }
};

struct first_hash {
    template <class T>
    bool operator()(T const& t) const
    {
        return boost::hash<typename T::value_type>()(t[0]);
    }
};


/*
bool operator == (xequiv const& xeq1, xequiv const& xeq2)
{
    return xeq1[0] == xeq2[0];
}

bool operator != (xequiv const& xeq1, xequiv const& xeq2)
{
    return !(xeq1 == xeq2);
} */

xedge tm_make_xedge(grammar_facade const* gram, tag_prior const* priormap, std::vector<xequiv> const& v, rule_application const& r)
{
    float c = 0.;
    std::vector<any_xinfo> b;
    
    BOOST_FOREACH(xequiv const& xeq, v) c += cost(xeq);
    c += r.cost;
    xedge ret(&r,v,b,c,0);
    if (gram->rule_lhs(&r).type() != sbmt::top_token){
        ret.heur += (*priormap)[gram->rule_lhs(&r)].neglog10();
    }
    return ret;
}

// given: spn, a span
//        n, number of splits
//        strie, signature trie
//        st, signature trie state
//        chrt, xchart
// return generator of xcell tuples + signature trie states

struct equiv_tree;

struct equiv_tree {
    xequiv const* equiv;
    equiv_tree* prev;
    explicit equiv_tree( xequiv const* equiv
                       , equiv_tree* prev = 0
                       ) 
    : equiv(equiv)
    , prev(prev) {}
};

struct cell_tree;

struct cell_tree {
    xcell const* cell;
    cell_tree* prev;
    explicit cell_tree( xcell const* cell
                      , cell_tree* prev = 0
                      ) 
    : cell(cell)
    , prev(prev) {}
};

struct info_tree;

struct info_tree {
    struct info_tree* p;
    any_xinfo i;
    float c;
    float h;
    
    info_tree(boost::tuple<any_xinfo,sbmt::score_t,sbmt::score_t> const& t, info_tree* p = 0)
    : p(p)
    , i(t.get<0>())
    , c(p ? p->c + t.get<1>().neglog10() : t.get<1>().neglog10())
    , h(p ? p->h + t.get<2>().neglog10() : t.get<2>().neglog10()) {}
};

info_tree* allocate_info_tree( boost::object_pool<info_tree>& pool
                             , boost::tuple<any_xinfo,sbmt::score_t,sbmt::score_t> const& t
                             , info_tree* p = 0 )
{
    info_tree* it = pool.malloc();
    if (not it) throw std::bad_alloc();
    ::new(it) info_tree(t,p);
    return it;
}


cell_tree* allocate_cell_tree(boost::pool<>& pool, xcell const* xc, cell_tree* p = 0)
{
    cell_tree* ct = (cell_tree*)pool.malloc();
    if (not ct) throw std::bad_alloc();
    ::new(ct) cell_tree(xc,p);
    return ct;
}

equiv_tree* allocate_equiv_tree(boost::pool<>& pool, xequiv const* xe, equiv_tree* p = 0)
{
    equiv_tree* et = (equiv_tree*)pool.malloc();
    if (not et) throw std::bad_alloc();
    ::new(et) equiv_tree(xe,p);
    return et;
}

typedef boost::tuple<
          equiv_tree*
        , float
        > equiv_in;

typedef 
    boost::tuple<
      cell_tree*
    , signature_trie::state
    , float
    > cell_in;

typedef gusc::any_generator<xedge,gusc::iterator_tag>
        xedge_generator;
typedef gusc::any_generator<xedge_generator,gusc::iterator_tag>
        xedge_generator_generator;
        
struct greater_cost {
    template <class T>
    bool op(T const& g1, T const& g2) const
    {
        float s1 = 0; float s2 = 0;
        BOOST_FOREACH(typename T::const_reference v, g1) {
            s1 += cost(v) + heur(v);
        }
        BOOST_FOREACH(typename T::const_reference v, g2) {
            s2 += cost(v) + heur(v);
        }
        return s1 > s2;
    }
    
    bool operator()(xedge const& e1, xedge const& e2) const
    {
        return cost(e1) + heur(e1) > cost(e2) + heur(e2);
    }
    
    bool operator()(xedge_generator const& e1, xedge_generator const& e2) const
    {
        if (not e1) return bool(e2);
        if (not e2) return false;
        return cost(*e1) + heur(*e1) > cost(*e2) + cost(*e2);
    }
    
    bool operator()(xequiv const& eq1, xequiv const& eq2) const
    {
        return cost(eq1) + heur(eq1) > cost(eq2) + heur(eq2);
    }
    
    bool operator()(xcell const& c1, xcell const& c2) const
    {
        return cost(c1) + heur(c1) > cost(c2) + heur(c2);
    }
    
    bool operator()(std::vector<xequiv> const& g1, std::vector<xequiv> const& g2) const
    {
        return op(g1,g2);
    }
    
    bool operator()(cell_in const& g1, cell_in const& g2) const
    {
        return g1.get<2>() > g2.get<2>();
    }
    
    bool operator()(equiv_in const& g1, equiv_in const& g2) const
    {
        return g1.get<1>() > g2.get<1>();
    }
    
    bool operator()(info_tree const* i1, info_tree const* i2) const
    {
        return (i1->c + i1->h) > (i2->c + i2->h);
    }
};

struct lower_cost {
    template <class T>
    bool operator()(T const& t1, T const& t2) const
    {
        return greater_cost()(t2,t1);
    }
};


std::string nbest_features(feature_vector const& f, feature_dictionary& dict);





BOOST_ENUM_VALUES(
    output_format_type
  , const char*
  , (nbest)("nbest")
    (forest)("forest")
);

BOOST_ENUM_VALUES(
   forest_format_type
 , const char*
 , (target_string)("target-string")
   (rules)("rules")
   (amr_string)("amr-string")
);

BOOST_ENUM_VALUES(
    multipass
  , const char*
  , (single)("single")
    (source)("source")
    (pipe)("pipe")
    (sink)("sink")
);

struct options {
    bool pop_newline;
    bool nondet;
    bool append_rules;
    bool keep_align;
    bool rule_dump;
    bool tee_weights;
    size_t num_threads;
    size_t histogram;
    size_t nbests;
    size_t estring_copies;
    size_t nbest_pops;
    double poplimit_multiplier;
    double softlimit_multiplier;
    size_t max_equivalents;
    size_t merge_window;
    std::string infos;
    fs::path dbdir;
    fs::path wfile;
    fs::path instructions;
    fs::path pfile;
    double prior_floor_prob;
    double prior_bonus_count;
    double weight_tag_prior;
    sbmt::tag_prior priormap;
    header h;
    output_format_type output_format;
    forest_format_type forest_format;
    multipass pass;
    size_t limit_syntax_length;
    bool no_sort_rules;
    std::istream* input;
    options() 
    : pop_newline(false)
    , nondet(false)
    , append_rules(false)
    , keep_align(false)
    , rule_dump(false)
    , tee_weights(false)
    , num_threads(numproc_online())
    , histogram(1000)
    , nbests(10)
    , estring_copies(0)
    , nbest_pops(100000)
    , poplimit_multiplier(1.0)
    , softlimit_multiplier(10.0)
    , max_equivalents(4)
    , merge_window(10)
    , prior_floor_prob(1e-7)
    , prior_bonus_count(100)
    , weight_tag_prior(1.0)
    , output_format(output_format_type::forest)
    , forest_format(forest_format_type::target_string)
    , pass(multipass::single)
    , limit_syntax_length(40)
    , no_sort_rules(false)
    , input(&std::cin) {}
    ~options()
    {
        if (input != &std::cin) delete input;
    }
};

options& parse_options(int argc, char** argv, options& opts)
{
    using namespace boost::program_options;
    options_description desc;

    bool multi_thread = false;
    desc.add_options()
        ( "dbdir,d"
        , value(&opts.dbdir)
        , "grammar database"
        )
        ( "instructions,i"
        , value(&opts.instructions)
        , "instruction file"
        )
        ( "num-threads"
        , value(&opts.num_threads)->default_value(opts.num_threads)
        , "number of decode threads"
        )
        ( "use-info,u"
        , value(&opts.infos)
        , "info types to use"
        )
        ( "span-max-edges,m"
        , value(&opts.histogram)->default_value(opts.histogram)
        , "histogram beam"
        )
        ( "unary-span-max-edges"
        , value<int>(0)
        , "ignored. whats a unary rule...?"
        )
        ( "multi-thread"
        , bool_switch(&multi_thread)
        )
        ( "nondeterministic"
        , bool_switch(&opts.nondet)
        , "force nondeterministic branch"
        )
        ( "append-rules"
        , bool_switch(&opts.append_rules)
        , "append used rules after forest or nbest list"
        )
        ( "rule-dump"
        , bool_switch(&opts.rule_dump)
        )
        ( "no-sort-rules"
        , bool_switch(&opts.no_sort_rules))
        ( "cache-grammars"
        , bool_switch(&word_trie_data_impl::operate_cache)
        , "if the grammar lives on a shared filesystem, cache grammar on local $TMPDIR"
        )
        ( "poplimit-multiplier,p"
        , value(&opts.poplimit_multiplier)->default_value(opts.poplimit_multiplier)
        , "examine p*m edges before keeping m edges"
        )
        ( "softlimit-multiplier,s"
        , value(&opts.softlimit_multiplier)->default_value(opts.softlimit_multiplier)
        , "examine at most s*m edges until m non-equivalent edges are produced"
        )
        ( "max-equivalents,e"
        , value(&opts.max_equivalents)->default_value(opts.max_equivalents)
        , "no more than e equivalent edges are kept in chart"
        )
        ( "merge-heap-decode"
        , value<string>()
        , "ignored"
        )
        ( "print-forest-em-file"
        , value<string>()
        , "ignored"
        )
        ( "print-forest-em-and-node-type"
        , value<string>()
        , "ignored"
        )
        ( "merge-heap-lookahead,r"
        , value(&opts.merge_window)->default_value(opts.merge_window)
        , "union merge-heap lookahead window"
        )
        ( "limit-syntax-length"
        , value(&opts.limit_syntax_length)->default_value(opts.limit_syntax_length)
        )
        ( "weight-file,w"
        , value(&opts.wfile)
        )
        ( "tee-weights,t"
        , bool_switch(&opts.tee_weights)
        , "use weights in current pass, and send them to the next pass"
        )
        ( "help,h"
        , "produce help message"
        )
        ( "prior-file"
        , value(&opts.pfile)
        , "file with alternating <tag> <count> e.g. NP 123478.  virtual tags ignored"
        )
        ( "prior-floor-prob"
        , value(&opts.prior_floor_prob)->default_value(opts.prior_floor_prob)
        , "minimum probability for missing or low-count tags"
        )
        ( "prior-bonus-count"
        , value(&opts.prior_bonus_count)->default_value(opts.prior_bonus_count)
        , "give every tag that appears in prior-file this many extra counts (before normalization)"
        )
        ( "weight-prior"
        , value(&opts.weight_tag_prior)->default_value(opts.weight_tag_prior)
        , "raise prior prob to this power for rule heuristic"
        )
        ( "output-format"
        , value(&opts.output_format)->default_value(opts.output_format)
        )
        ( "forest-format"
        , value(&opts.forest_format)->default_value(opts.forest_format)
        )
        ( "pass"
        , value(&opts.pass)->default_value(opts.pass)
        )
        ( "keep-align"
        , value<string>()
        , "ignored"
        )
        ( "nbests"
        , value(&opts.nbests)->default_value(opts.nbests)
        , "number of bests to generate"
        )
        ( "per-estring-nbests"
        , value(&opts.estring_copies)->default_value(opts.estring_copies)
        , "number of duplicated estrings allowed before passing over in nbest list"
        )
        ( "nbest-pops"
        , value(&opts.nbest_pops)->default_value(opts.nbest_pops)
        , "max nbests to pop before giving up"
        )
        ( "newline-after-pop"
        , value(&opts.pop_newline)->default_value(opts.pop_newline)
        , "add newline to stream after popping grammar" )
        ;
    
    desc.add(get_info_options());
    desc.add(sbmt::io::logfile_registry::instance().options());
    po::positional_options_description posdesc;
    posdesc.add("dbdir",1).add("instructions",1);
    po::basic_command_line_parser<char> cmd(argc,argv);
    variables_map vm;
    po::store(cmd.options(desc).positional(posdesc).run(),vm);
    notify(vm);
    
    opts.nbest_pops = std::max(opts.nbests,opts.nbest_pops);
    if ((not multi_thread) or (opts.num_threads <= 0)) opts.num_threads = 1;
    if (vm.count("help")) {
        cerr << desc << endl;
        exit(0);
    }
    
    if (opts.pass == multipass::source || opts.pass == multipass::pipe) {
        opts.output_format = output_format_type::forest;
        opts.forest_format = forest_format_type::rules;
        opts.append_rules = true;
    } 
    if (not vm.count("dbdir")) {
        cerr << desc << endl;
        cerr << "must provide xrs rule db" << endl;
        exit(1);
    }
    SBMT_VERBOSE_STREAM(decoder_domain, "loading header...");
    load_header(opts.h,opts.dbdir);
    SBMT_VERBOSE_STREAM(decoder_domain, "done loading header");
    if (opts.no_sort_rules) {
        cerr << "will assume rules in database are sorted consistently with weights and infos" << endl;
    }
    if (vm.count("instructions")) {
        opts.input = new std::ifstream(opts.instructions.native().c_str());
    } else {
        opts.input = &std::cin;
    }

    if (exists(opts.pfile)) {
        std::ifstream ifs(opts.pfile.native().c_str());
        opts.priormap.set(ifs,opts.h.dict,opts.prior_floor_prob,opts.prior_bonus_count);
        opts.priormap.raise_pow(opts.weight_tag_prior);
    }

    make_fullmap(opts.h.dict);



    init_info_factories(opts.h.dict);
    return opts;
}

typedef gusc::any_generator<cell_in, gusc::iterator_tag> cell_in_generator;

void print_sig_trie( signature_trie const& strie
                   , signature_trie::state st
                   , signature_trie::state spot
                   , int d )
{
    string h;
    for (int x = 0; x != d; ++x) h += ' ';
    if (strie.value(st) != strie.nonvalue()) {
        ip::offset_ptr<rule_application> ra; size_t sz;
        boost::tie(ra,sz) = strie.value(st);
        for (size_t rx = 0; rx != sz; ++rx) {
            std::cerr << h;
            if (st == spot) std::cerr << "[[[";
            std::cerr << ra[rx].rule;
            if (st == spot) std::cerr << "]]]";
            std::cerr << '\n';
        }
    }
    h += ' ';
    BOOST_FOREACH(signature_trie::state chld, strie.transitions(st)) {
        std::cerr << h;
        if (chld == spot) std::cerr << "[[[";
        std::cerr << strie.key(chld);
        if (chld == spot) std::cerr << "]]]";
        std::cerr << ":\n";
        print_sig_trie(strie,chld,spot,d+4);
    }
}

struct transitioner
: gusc::peekable_generator_facade<transitioner,cell_in> 
{    
    transitioner(signature_trie const* strie, cell_in const& ge, xspan const& xs,boost::pool<>* cell_pool)
    : strie(strie)
    , ge(ge)
    , from(ge.get<1>())
    , c(ge.get<2>())
    , xitr(xs.begin())
    , xend(xs.end())
    , sz(boost::size(strie->transitions(from)))
    , curr(0)
    , cell_pool(cell_pool) { advance(true); }
    
private:
    struct lessfirst {
        typedef bool result_type;
        
        bool operator()(signature_trie::state const& t1, signature_trie::state const& t2) const
        {
            return t1->key < t2->key;
        }

        bool operator()(signature_trie::state const& t1, indexed_token const& t2) const
        {
            return t1->key < t2;
        }

        bool operator()(indexed_token const& t1, signature_trie::state const& t2) const
        {
            return t1 < t2->key;
        }
    };
    
    bool transition(xcell const& xc, bool first)
    {
        //std::cerr << "+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+\n";
        //print_sig_trie(*strie,strie->start(),ge.get<1>(),0);
        //std::cerr << "---- " << root(xc) << " ---->\n";
        
        signature_trie::iterator tbeg, tend, tpos;
        boost::tie(tbeg,tend) = strie->transitions(from);
        tpos = std::lower_bound(tbeg,tend,root(xc),lessfirst());
        if (tpos != tend and root(xc) == (*tpos)->key) {
            if (not first) ge.get<0>() = ge.get<0>()->prev;
            ge.get<0>() = allocate_cell_tree(*cell_pool,&xc,ge.get<0>());
            ge.get<1>() = *tpos;
            ge.get<2>() = c + cost(xc) + heur(xc);
            return true;
            //print_sig_trie(*strie,strie->start(),s,0);
        } else {
            return false;
        }
        //std::cerr << "+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+\n";
    }
    
    void advance(bool first) {
        if (curr >= sz) {
            //std::cerr << "short circuit: " << xend - xitr << "\n";
            xitr = xend;
        }
        while (xitr != xend) {
            if (transition(*xitr,first)) { ++curr; break; }
            else ++xitr;
        }
    }
    
    cell_in const& peek() const { return ge; }
    
    void pop() { ++xitr; advance(false); }
    
    bool more() const { return xitr != xend; }
    
    friend class gusc::generator_access;
    
    signature_trie const* strie;
    cell_in ge;
    signature_trie::state from;
    float c;
    xspan::const_iterator xitr;
    xspan::const_iterator xend;
    size_t sz;
    size_t curr;
    boost::pool<>* cell_pool;
};

struct make_transitioner {
    typedef transitioner result_type;
    result_type operator()(cell_in const& ge) const { return transitioner(strie,ge,xspn,cell_pool); }
    make_transitioner( signature_trie const* strie,xspan const& xspn,boost::pool<>* cell_pool) 
    : strie(strie)
    , xspn(xspn)
    , cell_pool(cell_pool) {}
    signature_trie const* strie;
    xspan xspn;
    boost::pool<>* cell_pool;
};

cell_in_generator
gen_transitions( xspan const& spn
               , signature_trie const& strie
               , cell_in_generator gen
               , boost::pool<>& cell_pool )
{
    return cell_in_generator(gusc::generate_union_heap( 
             gusc::make_peekable(
               gusc::generate_transform(
                 gen
               , make_transitioner(&strie,spn,&cell_pool)
               )
             )
           , greater_cost()
           , 10
           ));
}

cell_in_generator
gen_cells_in( span_t spn
            , size_t n
            , signature_trie const& strie
            , xchart const& chrt
            , cell_in_generator gen
            , boost::pool<>& cell_pool )
{
    if (not gen) return gen;
    // main bit
    if (n == 1) {
        return gen_transitions(chrt.find(spn)->second,strie,gen,cell_pool);
    } else {
        gusc::shared_lazy_sequence< cell_in_generator > lzy(gen);
        size_t x = spn.left();
        size_t y = spn.right();
        std::vector<cell_in_generator> splits;
        for (size_t k = 1; y - x - k >= n - 1; ++k) {
            cell_in_generator 
                cgen = cell_in_generator(gen_transitions(
                         chrt.find(span_t(x,x+k))->second
                       , strie
                       , cell_in_generator(gusc::generate_from_range(lzy))
                       , cell_pool
                       ));
            if (not cgen) continue;
            splits.push_back(
              gen_cells_in(
                span_t(x+k,y)
              , n-1
              , strie
              , chrt
              , cgen
              , cell_pool
              )
            );
        }
        return cell_in_generator(gusc::generate_finite_union(splits,greater_cost()));
    }
    
}

cell_in_generator
gen_cells_in( std::vector< boost::tuple<span_t,size_t> > const& v
            , signature_trie const& strie
            , xchart const& chrt
            , boost::pool<>& cell_pool
            )
{
    //std::cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    //print_sig_trie(strie,strie.start(),0);
    //std::cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    cell_in_generator 
        gen = cell_in_generator(gusc::generate_single_value(
                cell_in(
                  0
                , strie.start()
                , 0.0
                )
              ));
    span_t spn; size_t n;
    //std::cerr << "cells_in";
    BOOST_FOREACH(boost::tie(spn,n),v) {
    //    std::cerr << " x "<< spn << ":" << n;
       gen = gen_cells_in(spn,n,strie,chrt,gen,cell_pool);
    }
    //std::cerr << "\n";
    //std::cerr << strie << '\n';
    return gen;
}

struct push_back_equiv {
    typedef equiv_in result_type;
    result_type operator()(equiv_in ret, xequiv const& s) const
    {
        ret.get<0>() = allocate_equiv_tree(*equiv_pool,&s,ret.get<0>());
        ret.get<1>() += cost(s) + heur(s);
        return ret;
    }
    push_back_equiv(boost::pool<>* equiv_pool) : equiv_pool(equiv_pool) {}
private:
    boost::pool<>* equiv_pool;
};

typedef boost::function<xedge(std::vector<xequiv> const&,rule_application const&)> 
        xedge_construct_det_f;
typedef boost::function<xedge_generator(std::vector<xequiv> const&,rule_application const&)> 
        xedge_construct_f;
typedef boost::function<sbmt::weight_vector(xedge)> xedge_components_f;

struct apply_rule_equiv_det {
    apply_rule_equiv_det(xedge_construct_det_f func) : func(func) {}
    xedge_construct_det_f func;
    typedef xedge result_type;
    result_type operator()(equiv_in const& in, rule_application const& r) const
    {
        size_t sz = 0;
        equiv_tree* etree = in.get<0>();
        while (etree) {
            ++sz;
            etree = etree->prev;
        }
        std::vector<xequiv> vec(sz);
        etree = in.get<0>();
        sz = 0;
        while (etree) {
            vec[sz] = *(etree->equiv);
            ++sz;
            etree = etree->prev;
        }
        return func(vec,r);
    }
};

struct apply_rule_equiv {
    apply_rule_equiv(xedge_construct_f func) : func(func) {}
    xedge_construct_f func;
    typedef xedge_generator result_type;
    result_type operator()(equiv_in const& in, rule_application const& r) const
    {
        size_t sz = 0;
        equiv_tree* etree = in.get<0>();
        while (etree) {
            ++sz;
            etree = etree->prev;
        }
        std::vector<xequiv> vec(sz);
        etree = in.get<0>();
        sz = 0;
        while (etree) {
            vec[sz] = *(etree->equiv);
            ++sz;
            etree = etree->prev;
        }
        return func(vec,r);
    }
};


xedge_generator
make_edges_det1( cell_in const& ge
               , signature_trie const& strie
               , xedge_construct_det_f func
               , boost::pool<>& equiv_pool
               )
{
    typedef
        gusc::any_generator<equiv_in,gusc::iterator_tag> 
        children_generator;
        //typedef
        //gusc::generator_from_range<xcell> xcell_generator;
        children_generator 
            gen = children_generator(gusc::generate_single_value(
                    equiv_in(
                      0
                    , 0.0
                    )
                  ));
    cell_tree* ctree = ge.get<0>();
    while (ctree) {
        gusc::shared_lazy_sequence<children_generator> lzygen(gen);
        gen = generate_product_heap(push_back_equiv(&equiv_pool),greater_cost(),lzygen,*(ctree->cell));
        //gusc::shared_lazy_sequence<xcell_generator> lzycell(generate_from_range(*(ctree->cell)));
        //gen = generate_product_heap(push_back_equiv(&equiv_pool),greater_cost(),lzygen,lzycell);
        ctree = ctree->prev;
    }

    rule_application_array_adapter rules(strie.value(ge.get<1>()));
    
    //std::cerr << "make_edges1:bool(gen) == " << bool(gen) << " rules.size() == " << rules.size() << '\n';
    
    gusc::shared_lazy_sequence<children_generator> lzygen(gen);
    return xedge_generator(
               gusc::generate_product_heap(
                 apply_rule_equiv_det(func)
               , greater_cost()
               , lzygen
               , rules
               )
               );
}


xedge_generator_generator
make_edges1( cell_in const& ge
           , signature_trie const& strie
           , xedge_construct_f func
           , boost::pool<>& equiv_pool
           , options& opts
           )
{
    typedef
        gusc::any_generator<equiv_in,gusc::iterator_tag> 
        children_generator;
        //typedef
        //gusc::generator_from_range<xcell> xcell_generator;
        children_generator 
            gen = children_generator(gusc::generate_single_value(
                    equiv_in(
                      0
                    , 0.0
                    )
                  ));
    cell_tree* ctree = ge.get<0>();
    while (ctree) {
        gusc::shared_lazy_sequence<children_generator> lzygen(gen);
        gen = generate_product_heap(push_back_equiv(&equiv_pool),greater_cost(),lzygen,*(ctree->cell));
        //gusc::shared_lazy_sequence<xcell_generator> lzycell(generate_from_range(*(ctree->cell)));
        //gen = generate_product_heap(push_back_equiv(&equiv_pool),greater_cost(),lzygen,lzycell);
        ctree = ctree->prev;
    }

    rule_application_array_adapter rules(strie.value(ge.get<1>()));
    //BOOST_FOREACH(rule_application const& ra, rules) {
    //    std::cerr << '<'<<ra.cost<<','<<ra.heur<<'>' << ' ';
    //}
    //std::cerr << '\n';
    
    //std::cerr << "make_edges1:bool(gen) == " << bool(gen) << " rules.size() == " << rules.size() << '\n';
    
    gusc::shared_lazy_sequence<children_generator> lzygen(gen);
    return xedge_generator_generator(
             gusc::generate_product_heap(
               apply_rule_equiv(func)
             , greater_cost()
             , lzygen
             , rules
             )
               );
}

struct make_res {
    
    void sortit( rule_sort_f sorter
               , std::vector<rule_application_array_adapter> ruless
               , boost::mutex* pmtx = 0
               , boost::condition_variable* pcond = 0
               , int* pcount = 0
               )
    {
        BOOST_FOREACH(rule_application_array_adapter rules, ruless) sorter(rules.begin(),rules.end());
        //std::cerr << "FINISHED OFF A SORT " << ruless.size() << "\n";
        if (pmtx) { 
            boost::mutex::scoped_lock lk(*pmtx); 
            --(*pcount); 
            if (*pcount == 0) pcond->notify_all();
        }
    }
    
    typedef signature_trie const* result_type;
    void sort_res ( signature_trie* trie
                  , signature_trie::state s
                  , rule_sort_f sorter
                  , std::vector<rule_application_array_adapter>& ruless
                  , int& sz
                  , sbmt::thread_pool* sortpool = 0
                  , boost::mutex* pmtx = 0
                  , boost::condition_variable* pcond = 0
                  , int* pcount = 0
                  )
    {
        if (trie->value(s) != trie->nonvalue()) {
            rule_application_array_adapter rules(trie->value(s));
            ruless.push_back(rules);
            sz += rules.size();
            if ( sortpool ) {
                if (sz > 1000) {
                    { 
                        boost::mutex::scoped_lock lk(*pmtx); 
                        ++(*pcount); 
                    }
                    //std::cerr << "FORKING OFF A SORT " << ruless.size() << "\n";
                    sortpool->add(boost::bind(&make_res::sortit,this,sorter,ruless,pmtx,pcond,pcount));
                    ruless.clear();
                    sz = 0;
                }     
            } 
        }
        BOOST_FOREACH(signature_trie::state ss, trie->transitions(s)) {
            sort_res(trie,ss,sorter,ruless,sz,sortpool,pmtx,pcond,pcount);
        }
    }
    result_type operator()(compressed_signature_trie const& sr, rule_sort_f sorter,bool dupok = true)
    {
        signature_cache::iterator pos;
        signature_cache* sc = wtd.cache();
        boost::shared_mutex& mtx = wtd.mutex();
        boost::shared_mutex& smtx = wtd.sortit(sr.get<0>()).mutex;
        boost::mutex& wmtx = wtd.sortit(sr.get<0>()).workmutex;
        boost::condition_variable& wcond = wtd.sortit(sr.get<0>()).workfinished;
        int count = 0;
        {
            boost::shared_lock<boost::shared_mutex> readlock(mtx);
            pos = sc->find(sr.get<0>());
            if (pos != sc->end()) return dupok ? pos->second.get() : 0;
        }
        {
            boost::upgrade_lock<boost::shared_mutex> smightwritelock(smtx);
            { 
                boost::shared_lock<boost::shared_mutex> readlock(mtx);
                pos = sc->find(sr.get<0>());
                if (pos != sc->end()) return dupok ? pos->second.get() : 0;
            }
            {
                boost::upgrade_to_unique_lock<boost::shared_mutex> swritelock(smightwritelock);
                //std::stringstream sstr;
                //sstr << "writing " << sr.get<0>() << "\n";
                //std::cerr << sstr.str();
                signature_trie_data sigdata = load_signature_trie(sr);
                std::vector<rule_application_array_adapter> ruless;
                int rsz = 0;
                if (do_sort) {
                    sort_res(sigdata.get(),sigdata.get()->start(),sorter,ruless,rsz,sortpool,&wmtx,&wcond,&count);
                    sortit(sorter,ruless);
                    if (sortpool) {
                      boost::mutex::scoped_lock wlk(wmtx);
                      while (count > 0) wcond.wait(wlk);
                    } 
                }
                {
                    boost::shared_lock<boost::shared_mutex> readlock(mtx);
                    pos = sc->find(sr.get<0>());
                    if (pos != sc->end()) throw std::logic_error("signature_tree_data should not be loaded");
                }
                {
                    boost::upgrade_lock<boost::shared_mutex> mightwritelock(mtx);
                    boost::upgrade_to_unique_lock<boost::shared_mutex> writelock(mightwritelock);
                    pos = sc->find(sr.get<0>());
                    if (pos != sc->end()) throw std::logic_error("signature_tree_data should not be loaded here either");
                    sc->insert(std::make_pair(sr.get<0>(),sigdata));
                    return sigdata.get();
                }
            }
        }
    }
    
    make_res(word_trie_data wtd,bool do_sort) : wtd(wtd), sortpool(0), do_sort(do_sort) {}
    make_res(word_trie_data wtd,sbmt::thread_pool& sp, bool do_sort) : wtd(wtd), sortpool(&sp), do_sort(do_sort) {}
    word_trie_data wtd;
    sbmt::thread_pool* sortpool;
    bool do_sort;
};


xedge_generator
make_edges_det( cluster_search_data& csd
              , cluster_search::single_result_type const& bt
              , xchart const& chrt
              , xedge_construct_det_f func
              , boost::pool<>& cell_pool
              , boost::pool<>& edge_pool
              , sbmt::thread_pool& sortpool
              , options& opts
              )
{
    make_res mr(csd.wtd,sortpool,not opts.no_sort_rules);
    cell_in_generator 
        cgen = gen_cells_in(bt.get<0>(),*mr(bt.get<1>(),csd.sorter()),chrt,cell_pool);
    return
    xedge_generator(gusc::generate_union_heap(
      gusc::make_peekable( 
        gusc::generate_transform(
          cgen
        , boost::bind(make_edges_det1,_1,boost::cref(*mr(bt.get<1>(),csd.sorter())),func,boost::ref(edge_pool))
        )
      )
    , greater_cost()
    , 10
    ));
}


xedge_generator_generator
make_edges( cluster_search_data& csd
          , cluster_search::single_result_type const& bt
          , xchart const& chrt
          , xedge_construct_f func
          , boost::pool<>& cell_pool
          , boost::pool<>& edge_pool
          , sbmt::thread_pool& sortpool
          , options& opts
          )
{
    make_res mr(csd.wtd,sortpool,not opts.no_sort_rules);
    cell_in_generator cgen = gen_cells_in(bt.get<0>(),*mr(bt.get<1>(),csd.sorter()),chrt,cell_pool);
    return
    xedge_generator_generator(gusc::generate_union_heap(
      gusc::make_peekable( 
        gusc::generate_transform(
          cgen
        , boost::bind(
            make_edges1
          , _1
          , boost::cref(*mr(bt.get<1>(),csd.sorter()))
          , func
          , boost::ref(edge_pool)
          , boost::ref(opts)
          )
        )
      )
    , greater_cost()
    , 10
    ));
}

xedge_generator
make_edges_det( cluster_search_data& csd
              , cluster_search::result_type const& res 
              , xchart const& chrt
              , xedge_construct_det_f func
              , boost::pool<>& cell_pool
              , boost::pool<>& edge_pool
              , sbmt::thread_pool& sortpool
              , options& opts )
{
    xedge_generator retval;
    std::vector<xedge_generator> vec(res.size());
    int x = 0;
    BOOST_FOREACH(cluster_search::single_result_type const& cb, res)
    {
        xedge_generator 
            xeg = make_edges_det(csd,cb,chrt,func,cell_pool,edge_pool,sortpool,opts);
        vec[x] = xeg;
        ++x;
    }
    retval = gusc::generate_finite_union(vec, greater_cost());
    return retval;
}

xedge_generator_generator
make_edges( cluster_search_data& csd
          , cluster_search::result_type const& res 
          , xchart const& chrt
          , xedge_construct_f func
          , boost::pool<>& cell_pool
          , boost::pool<>& edge_pool
          , sbmt::thread_pool& sortpool
          , options& opts )
{
    xedge_generator_generator retval;
    std::vector<xedge_generator_generator> vec(res.size());
    int x = 0;
    BOOST_FOREACH(cluster_search::single_result_type const& cb, res)
    {
        xedge_generator_generator xeg = make_edges(csd,cb,chrt,func,cell_pool,edge_pool,sortpool,opts);
        vec[x] = xeg;
        ++x;
    }
    retval = gusc::generate_finite_union(vec, greater_cost());
    return retval;
}


xedge_generator
make_edges_det( std::vector<cluster_search_data>& vcs
              , vtx_t v
              , xchart const& chrt
              , xedge_construct_det_f func
              , boost::pool<>& cell_pool
              , boost::pool<>& edge_pool
              , sbmt::thread_pool& sortpool
              , options& opts )
{
    std::vector<xedge_generator> vcsresults(vcs.size());
    size_t x = 0;
    std::vector<cluster_search_data> vcsr;
    BOOST_FOREACH(cluster_search_data& cs, vcs) {
        vcsr.push_back(cs);
    }
    random_shuffle(vcsr.begin(),vcsr.end());
    BOOST_FOREACH(cluster_search_data& cs, vcsr) {
        cluster_search::result_type crs((*cs)(v));
        xedge_generator xegf = make_edges_det(cs,crs,chrt,func,cell_pool,edge_pool,sortpool,opts);
        vcsresults[x] = xegf;
        ++x;
    }
    return xedge_generator(gusc::generate_finite_union(vcsresults,greater_cost()));
}

xedge_generator
make_edges( std::vector<cluster_search_data>& vcs
          , vtx_t v
          , xchart const& chrt
          , xedge_construct_f func
          , boost::pool<>& cell_pool
          , boost::pool<>& edge_pool
          , sbmt::thread_pool& sortpool
          , options& opts )
{
    std::vector<xedge_generator_generator> vcsresults(vcs.size());
    size_t x = 0;
    std::vector<cluster_search_data> vcsr;
    BOOST_FOREACH(cluster_search_data& cs, vcs) {
        vcsr.push_back(cs);
    }
    random_shuffle(vcsr.begin(),vcsr.end());
    BOOST_FOREACH(cluster_search_data& cs, vcsr) {
        cluster_search::result_type crs((*cs)(v));
        xedge_generator_generator xegf = make_edges(cs,crs,chrt,func,cell_pool,edge_pool,sortpool,opts);
        vcsresults[x] = xegf;
        ++x;
    }
    return xedge_generator(gusc::generate_union_heap(
             gusc::generate_finite_union(vcsresults,greater_cost())
           , greater_cost()
           , 10
           , opts.histogram
           ));
}

////////////////////////////////////////////////////////////////////////////////

typedef std::map< boost::tuple<vtx_t,vtx_t>, std::vector<edge_t> > addums_t;
typedef std::map<sbmt::indexed_token, word_trie_data> wordtrie_map;
typedef std::map<sbmt::indexed_token, boost::shared_ptr<boost::mutex> > wordtrie_mutex_map;

addums_t addums(graph_t const& g)
{
    addums_t ad;
    map<size_t,vtx_t> smap;
    BOOST_FOREACH(vtx_t vtx, vertices(g)) {
        smap.insert(make_pair(g[vtx],vtx));
    }
    vtx_t vfront = smap.begin()->second;
    vtx_t vback = smap.rbegin()->second;
    map<size_t,vtx_t>::iterator left = smap.begin(), end = smap.end(), right;
    for (; left != end; ++left) {
        right = left; ++right;
        vtx_t vx = left->second;
        for (; right != end; ++right) {
            vtx_t vy = right->second;
            BOOST_FOREACH(edge_t e, in_edges(vy,g)) {
                vtx_t pvy = source(e,g);
                vtx_t pve = target(e,g);
                if (g[e].type() != sbmt::virtual_tag_token) {
                    if (left->first == 0) {
                        if (pvy == vfront or pve == vback) {
                            ad[boost::make_tuple(vx,vy)].push_back(e);
                        }
                    } else if (pvy == vx or ad.find(boost::make_tuple(vx,pvy)) != ad.end()) {
                        ad[boost::make_tuple(vx,vy)].push_back(e);
                    }
                }
            }
        }
    }
    return ad;
}

indexed_token keyword(indexed_token tok, wildcard_array& wc) {
    if (tok.type() == sbmt::virtual_tag_token) {
        tok = wc[0];
    }
    return tok;
}

sbmt::indexed_token headword(xedge const& xe, std::vector<sbmt::indexed_token> c)
{
    if (xe.rule->hwd->indexed()) {
        return c[xe.rule->rhs2var.find(xe.rule->hwd->index())->second];
    } else {
        return xe.rule->hwd->get_token();
    }
}

struct xtree;
typedef boost::shared_ptr<xtree> xtree_ptr;
typedef std::vector<xtree_ptr> xtree_children;
struct xtree {
    float cost;
    xedge root;
    sbmt::indexed_token hword;
    typedef gusc::varray<xtree_ptr> children_type;
    //typedef std::tr1::unordered_map<int,xtree_ptr> children_type;
    children_type children;
    template <class RNG>
    xtree(xedge const& root, RNG c)
    : cost(root.cost)
    , root(root)
    , children(boost::begin(c),boost::end(c))
    {
        if (root.rule->hwd->indexed()) {
            hword = children[root.rule->rhs2var.find(root.rule->hwd->index())->second]->hword;
        }
        else {
            hword = root.rule->hwd->get_token();
        }
        //std::cerr << "before cost: " << cost << "\n";
        BOOST_FOREACH(xequiv xeq, root.children) if (::root(xeq).type() != sbmt::foreign_token) cost -= ::cost(xeq);
        BOOST_FOREACH(xtree_ptr ptr, children) cost += ptr->cost;
        BOOST_FOREACH(fixed_rule::tree_node const& nd, root.rule->rule.lhs()) {
            //if (nd.indexed() and ::root(children[nd.index()]->root) != nd.get_token()) {
            //    throw std::logic_error("xtree roots dont match");
            //}
            if (nd.indexed() and ::root(children[root.rule->rhs2var.find(nd.index())->second]->root) != nd.get_token()) {
                throw std::logic_error("xtree roots dont match");
            }
        }
        //std::cerr << "after cost: " << cost << "\n";
        /*
        typename boost::range_const_iterator<RNG>::type 
             itr = boost::begin(c),
             end = boost::end(c);
        fixed_rule::rhs_iterator
             rhsitr = rule->rule.rhs_begin(),
             rhsend = rule->rule.rhs_end();
        size_t x = 0;
        for (; rhsitr != rhsend; ++rhsitr, ++x) {
            if (rhsitr->indexed()) {
                children.insert(std::make_pair(x,*itr));
                cost += (*itr)->cost;
                ++itr;
             }
         }
         */
     }
};

typedef gusc::any_generator<boost::shared_ptr<xtree>,gusc::iterator_tag> xtree_generator;
xtree_generator xtrees_from_xequiv(xequiv const& forest);

struct greater_xtree_cost {
    bool operator()( xtree_ptr const& t1, xtree_ptr const& t2 ) const
    {
        return t1->cost > t2->cost;
    }
};


struct greater_xtree_children_cost {
    bool operator()(xtree_children const& c1, xtree_children const& c2) const
    {
        float s1(0);
        BOOST_FOREACH(xtree_ptr const& t1, c1) { s1 += t1->cost; }
        float s2(0);
        BOOST_FOREACH(xtree_ptr const& t2, c2) { s2 += t2->cost; }
        return s1 > s2;
    }
};

struct append_xtree_children {
    typedef xtree_children result_type;
    xtree_children operator()(xtree_children c, xtree_ptr const& p) const
    {
        c.push_back(p);
        return c;
    }
};

struct make_xtree_children {
    typedef xtree_children result_type;
    xtree_children operator()(xtree_ptr const& p) const
    {
        return xtree_children(1,p);
    }
};

std::ostream& operator << (std::ostream& out, xtree_ptr const& t);


typedef gusc::any_generator<std::vector< boost::shared_ptr<xtree> >,gusc::iterator_tag> xtree_children_generator;
typedef gusc::shared_lazy_sequence<xtree_children_generator> xtree_children_list;
typedef gusc::shared_lazy_sequence<xtree_generator> xtree_list;
typedef std::tr1::unordered_map<void const*,xtree_list> ornode_map;

xtree_list
xtrees_from_xequiv(xequiv const& forest, ornode_map& omap);
xtree_generator 
xtrees_from_xedge(xedge const& hyp, ornode_map& omap);

xtree_children_generator 
generate_xtree_children(xedge const& hyp, ornode_map& omap)
{
    xtree_children_generator ret;
    bool first = true;
    fixed_rule::rhs_iterator
        rhsitr = hyp.rule->rule.rhs_begin(),
        rhsend = hyp.rule->rule.rhs_end();
    size_t x = 0;
    for (; rhsitr != rhsend; ++rhsitr) {
        if (rhsitr->indexed()) {
            if (first) {
                xequiv xf0 = hyp.children[x];
                ret = gusc::generator_as_iterator(
                        gusc::generate_transform(
                          gusc::generate_from_range(xtrees_from_xequiv(xf0,omap))
                        , make_xtree_children()
                        )
                      );
                first = false;
            } else {
                typedef gusc::shared_lazy_sequence<xtree_children_generator> 
                        xtree_children_list;
                typedef gusc::shared_lazy_sequence<xtree_generator> 
                        xtree_list;
                xequiv xfx = hyp.children[x];
                ret = gusc::generate_product_heap(
                        append_xtree_children()
                      , greater_xtree_children_cost()
                      , xtree_children_list(ret)
                      , xtrees_from_xequiv(xfx,omap)
                      );
            }
        }
        ++x;
    }
    return ret; 
}

struct make_xtree {
    xedge hyp;
    make_xtree(xedge const& h)
    : hyp(h){}
    
    typedef xtree_ptr result_type;
    xtree_ptr operator()(xtree_children const& c) const
    {
        return xtree_ptr(new xtree(hyp,c));
    }
};

bool variable_free(xedge const& hyp)
{
    fixed_rule::rhs_iterator
        rhsitr = hyp.rule->rule.rhs_begin(),
        rhsend = hyp.rule->rule.rhs_end();
    for (; rhsitr != rhsend; ++rhsitr) if (rhsitr->indexed()) return false;
    return true;
}

xtree_generator 
xtrees_from_xedge(xedge const& hyp, ornode_map& omap)
{
    if (variable_free(hyp)) {
        return xtree_generator(gusc::make_single_value_generator(
                 xtree_ptr(new xtree(hyp,xtree_children()))
               ));
    } else {
        return xtree_generator(gusc::generator_as_iterator(
                 gusc::generate_transform(
                   generate_xtree_children(hyp,omap)
                 , make_xtree(hyp)
                 )
               ));
    }
}

std::string 
hyptree(xtree_ptr const& t,fixed_rule::tree_node const& n, in_memory_dictionary const& dict)
{
    using namespace sbmt;
    std::stringstream sstr;
    sstr << sbmt::token_label(dict);
    if (n.indexed()) {
        //xtree_ptr ct = t->children[n.index()];
        xtree_ptr ct = t->children[t->root.rule->rhs2var.find(n.index())->second];
        assert(n.get_token() == ct->root.rule->rule.lhs_root()->get_token());
        sstr << hyptree(ct,*(ct->root.rule->rule.lhs_root()),dict);
    } else if (n.lexical()) {
        sstr << n.get_token();
    } else {
        bool is_pre = false;
        int nc = 0;
        sstr << '(' << n.get_token();
        BOOST_FOREACH(fixed_rule::tree_node const& c, n.children()) {
            if (c.lexical() and nc == 0) is_pre = true;
            else is_pre = false;
            sstr << ' ' << hyptree(t,c,dict);
            ++nc;
        }
        if (not is_pre) sstr << ' ';
        sstr << ')';
    }
    return sstr.str();
}

std::string 
hyptree(xtree_ptr const& t, in_memory_dictionary const& dict)
{
    return hyptree(t,*(t->root.rule->rule.lhs_root()),dict);
}

struct make_xtree_generator_from_xedge {
    make_xtree_generator_from_xedge(ornode_map& omap) 
    : omap(&omap) {}
    ornode_map* omap;
    typedef xtree_generator result_type;
    xtree_generator operator()(xedge const& hyp) const
    {
        # if 0
        xtree_generator gen = xtrees_from_xedge(hyp,*omap);
        xtree_ptr tree = gen();
        while (gen) {
            xtree_ptr t = gen();
            if (t->cost < tree->cost) {
                std::cerr << "TREE ORDER VIOLATION:\n";
                gen = xtrees_from_xedge(hyp,*omap);
                in_memory_dictionary const* dict = sbmt::get_dict(std::cerr); 
                while (gen) {
                    std::cerr << hyptree(gen(),*dict) << '\n';
                }
                std::cerr << "/TREE ORDER VIOLATION:\n";
                break;
            }
        }
        # endif
        return xtrees_from_xedge(hyp,*omap);
    }
};


struct lcost {
    template <class X, class Y>
    bool operator()(X x, Y y) const { return cost(x) < cost(y); }
};

xtree_list 
xtrees_from_xequiv(xequiv const& forest, ornode_map& omap)
{
    ornode_map::iterator pos = omap.find(id(forest));
    if (pos != omap.end()) return pos->second;
    else {
        xequiv f2 = forest;
        quick_sort(f2.begin(),f2.end(),lcost());
        # if 0
        xequiv::iterator x = forest.begin(), y = forest.begin(), end = forest.end();
        std::cerr << '\n';
        for (; y != end; ++y) {
            if (x->cost > y->cost) std::cerr << "order violation!\n";
            if (x != y and x->cost == y->cost) std::cerr << "order same!\n";
            x = y;
            std::cerr << "%% " << y->rule->rule.id() << ' ' << y->cost << '\n';
        }
        # endif
        xtree_list lst(
         xtree_generator(
          gusc::generate_union_heap(
            gusc::generator_as_iterator(
              gusc::generate_transform(
                gusc::generate_from_range(f2)
              , make_xtree_generator_from_xedge(omap)
              )
            )
          , greater_xtree_cost()
          )
         )
        );
        omap.insert(std::make_pair(id(forest),lst));
        return lst;
    }
}

struct toplevel_generator {
    typedef xtree_ptr result_type;
    boost::shared_ptr<ornode_map> omap;
    xtree_generator gen;
    toplevel_generator(xequiv const& forest)
    : omap(new ornode_map())
    , gen(gusc::generate_from_range(xtrees_from_xequiv(forest,*omap)))
    {
        
    }
    operator bool() const { return bool(gen); }
    xtree_ptr operator()() { return gen(); }
};

xtree_generator xtrees_from_xequiv(xequiv const& forest)
{
    return xtree_generator(gusc::generator_as_iterator(toplevel_generator(forest)));
}

feature_vector features(rule_data const& rd, feature_dictionary& fdict);

void accum(xtree_ptr const& t,weight_vector& f, xedge_components_f func)
{
    f += func(t->root);
    BOOST_FOREACH(xequiv const& xeq, t->root.children) if (root(xeq).type() == sbmt::foreign_token) f += sbmt::weight_vector(xeq.begin()->rule->costs);
    BOOST_FOREACH(xtree::children_type::value_type const& c, t->children) {
        accum(c,f,func);
    }
}

weight_vector accum(xtree_ptr const& t, xedge_components_f func)
{
    weight_vector f;
    accum(t,f,func);
    return f;
}

struct info_make_xedges {
    
    struct combine_info {
        typedef info_tree* result_type;
        info_tree* operator()(boost::tuple<any_xinfo,sbmt::score_t,sbmt::score_t> const& t,info_tree* p = 0) const
        {
            return allocate_info_tree(*pool,t,p);
        }
        boost::object_pool<info_tree>* pool;
        combine_info(boost::object_pool<info_tree>* pool) : pool(pool) {} 
    };
    
    struct xedge_transform {
        typedef xedge result_type;
        xedge operator()(info_tree* p) const
        {
            xedge ret(tm);
            if (p) {
                if (include_heur) ret.heur += p->h;
                ret.cost += p->c;
                ret.infos = gusc::shared_varray<any_xinfo>(sz);
                for (int x = sz - 1; x != -1; --x) {
                    ret.infos[x] = p->i;
                    p = p->p;
                }
            }
            return ret;
        }
        xedge tm;
        size_t sz;
        bool include_heur;
        xedge_transform(xedge const& tm, size_t sz, bool ih)
        : tm(tm)
        , sz(sz)
        , include_heur(ih) {}
    };
    
    typedef gusc::any_generator<info_tree*, gusc::iterator_tag> info_tree_generator;
    typedef gusc::shared_lazy_sequence<info_tree_generator>  info_tree_sequence;
    typedef gusc::shared_lazy_sequence<gusc::peekable_generator<any_xinfo_factory::result_generator> > info_sequence;
    xedge_generator operator()(std::vector<xequiv> const& v, rule_application const& r)
    {
        xedge_generator gen;
        //std::cerr << "creating generator from " << r.rule << " and";
        //BOOST_FOREACH(xequiv vv, v) {
        //    std::cerr << ' ' << vv.span << ' ' << root(vv) << ':' << id(vv);
        //}
        //std::cerr << '\n';
        gusc::shared_varray<any_xinfo> ri(factories.size());
        gusc::varray<constituent<any_xinfo> > cia(v.size());
        
        if (factories.size()) {
            for (size_t y = 0; y != v.size(); ++y) {
                cia[y] = make_constituent(&(v[y].infos[0]),root(v[y]),v[y].span);
            }
            info_tree_generator igen = info_tree_generator(gusc::make_peekable(gusc::generate_transform(factories[0].create_info(*gram,&r,spn,cia),combine_info(pool))));
            for (size_t x = 1; x != factories.size(); ++x) {
                for (size_t y = 0; y != v.size(); ++y) {
                    cia[y] = make_constituent(&(v[y].infos[x]),root(v[y]),v[y].span);
                }
            
                igen = info_tree_generator(gusc::generate_product_heap(
                         combine_info(pool)
                       , greater_cost()
                       , info_sequence(gusc::make_peekable(factories[x].create_info(*gram,&r,spn,cia)))
                       , info_tree_sequence(igen)
                       ));
            }
            gen =  xedge_generator(gusc::make_peekable(gusc::generate_transform(
                     igen
                   , xedge_transform(
                       tm_make_xedge(gram,priormap,v,r)
                     , factories.size()
                     , gram->rule_lhs(&r).type() != sbmt::top_token
                     )
                   )));
        } else {
            gen = xedge_generator(gusc::generate_single_value(tm_make_xedge(gram,priormap,v,r)));
        }
        //std::cerr << "created generator, first element: " << root(*gen) << '\n';
        return gen;
    }
    
    sbmt::weight_vector operator()(xedge const& xe)
    {
        sbmt::weight_vector out(xe.rule->costs);
        sbmt::feature_vector scores;
        sbmt::ignore_accumulator hout;
        sbmt::multiply_accumulator<sbmt::feature_vector> sout(scores);
        gusc::varray<constituent<any_xinfo> > cia(xe.children.size());
        for (size_t x = 0; x != factories.size(); ++x) {
            for (size_t y = 0; y != xe.children.size(); ++y) {
                cia[y] = make_constituent(&(xe.children[y].infos[x]),root(xe.children[y]),xe.children[y].span);
            }
            factories[x].component_scores( *gram
                                         , xe.rule
                                         , spn
                                         , cia
                                         , xe.infos[x]
                                         , boost::make_function_output_iterator(sout)
                                         , boost::make_function_output_iterator(hout)
                                         );
        }
        BOOST_FOREACH(sbmt::feature_vector::value_type v, scores) {
            out[v.first] += v.second.neglog10();
        }
        return out;
    }
    
    info_make_xedges( grammar_facade* gram
                    , span_t spn
                    , gusc::shared_varray<any_xinfo_factory> factories
                    , tag_prior const* priormap
                    , boost::object_pool<info_tree>* pool )
    : gram(gram)
    , spn(spn)
    , factories(factories)
    , priormap(priormap)
    , pool(pool) {}
    
    grammar_facade* gram;
    span_t spn;
    gusc::shared_varray<any_xinfo_factory> factories;
    tag_prior const* priormap;
    boost::object_pool<info_tree>* pool;
};

struct info_make_xedge {
    typedef xedge result_type;
    xedge operator()(std::vector<xequiv> const& v, rule_application const& r)
    {
        xedge ret(tm_make_xedge(gram,priormap,v,r));
        gusc::shared_varray<any_xinfo> ri(factories.size());
        gusc::varray<constituent<any_xinfo> > cia(v.size());
        any_xinfo info; sbmt::score_t scr, heur;
        for (size_t x = 0; x != factories.size(); ++x) {
            for (size_t y = 0; y != v.size(); ++y) {
                cia[y] = make_constituent(&(v[y].infos[x]),root(v[y]),v[y].span);
            }
            boost::tie(info,scr,heur) = (factories[x].create_info(*gram,&r,spn,cia))();
            ret.cost += scr.neglog10();
            if (gram->rule_lhs(&r).type() != sbmt::top_token) ret.heur += heur.neglog10();
            ri[x] = info;
        }
        
        ret.infos = ri;
        return ret;
    }
    
    
    sbmt::weight_vector operator()(xedge const& xe)
    {
        sbmt::weight_vector out(xe.rule->costs);
        sbmt::feature_vector scores;
        sbmt::ignore_accumulator hout;
        sbmt::multiply_accumulator<sbmt::feature_vector> sout(scores);
        gusc::varray<constituent<any_xinfo> > cia(xe.children.size());
        for (size_t x = 0; x != factories.size(); ++x) {
            for (size_t y = 0; y != xe.children.size(); ++y) {
                cia[y] = make_constituent(&(xe.children[y].infos[x]),root(xe.children[y]),xe.children[y].span);
            }
            factories[x].component_scores( *gram
                                         , xe.rule
                                         , spn
                                         , cia
                                         , xe.infos[x]
                                         , boost::make_function_output_iterator(sout)
                                         , boost::make_function_output_iterator(hout)
                                         );
        }
        BOOST_FOREACH(sbmt::feature_vector::value_type v, scores) {
            out[v.first] += v.second.neglog10();
        }
        return out;
    }
    
    info_make_xedge( grammar_facade* gram
                   , span_t spn
                   , gusc::shared_varray<any_xinfo_factory> factories
                   , tag_prior const* priormap
                   , boost::object_pool<info_tree>* pool  = 0)
    : gram(gram)
    , spn(spn)
    , factories(factories)
    , priormap(priormap) {}
    
    grammar_facade* gram;
    span_t spn;
    gusc::shared_varray<any_xinfo_factory> factories;
    tag_prior const* priormap;
};

typedef vector<word_trie_data> word_trie_stack;

void set_weights(header& h, weight_vector& weights, fat_weight_vector const& fatweights)
{
    weights = index(fatweights,h.fdict);
    SBMT_VERBOSE_STREAM(decoder_domain, "weights: " <<  print(weights,h.fdict));
    //std::cerr << "weights: " << print(weights,h.fdict) << '\n';
}

void echo_weights(fat_weight_vector const& fatweights)
{
    std::cout << "weights \"";
    bool first = true;
    std::string nm;
    double vl;
    BOOST_FOREACH(boost::tie(nm,vl),fatweights) {
        if (not first) std::cout << ',';
        std::cout << gusc::escape_c(nm) << ':' << vl;
        first = false;
    }
    std::cout << "\";\n" << std::endl ;
}

void tee_weights(header& h, weight_vector& weights, fat_weight_vector const& fatweights)
{
    set_weights(h,weights,fatweights);
    echo_weights(fatweights);
}

void pop_grammar( word_trie_stack& wts, bool newline)
{
    wts.pop_back();
    if (newline) std::cout << std::endl;
}

void push_inline_grammar( word_trie_stack& wts
                        , std::vector<std::string> const& fvec
                        , sbmt::weight_vector const& weights
                        , header& h )
{
    std::stringstream sstr;
    BOOST_FOREACH(std::string const& str, fvec) {
        sstr << str;
    }
    size_t sz;
    boost::shared_array<char> buf;
    boost::tie(buf,sz) = create_word_trie(sstr,weights,h);
    wts.push_back(word_trie_data(buf,sz,null_sorter()));
    SBMT_INFO_STREAM(decoder_domain, "inline grammar pushed");
}

void push_grammar( word_trie_stack& wts
                 , std::string const& fname
                 , sbmt::archive_type ar
                 , sbmt::weight_vector const& weights
                 , header& h )
{
    std::ifstream ifs(fname.c_str());
    size_t sz;
    boost::shared_array<char> buf;
    boost::tie(buf,sz) = create_word_trie(ifs,weights,h);
    wts.push_back(word_trie_data(buf,sz,null_sorter()));
    SBMT_INFO_STREAM(decoder_domain, "grammar " << fname << " pushed");
}
void echo_grammar( std::string const& fname
                 , sbmt::archive_type ar )
{
    std::ifstream ifs(fname.c_str());
    std::string line;
    while(getline(ifs,line)) std::cout << line << '\n';
    std::cout << std::flush;
}

std::string sentence_yield(xtree_ptr tree, header& h) 
{
    std::stringstream sent;
    bool first(true);
    sent << sbmt::token_label(h.dict);
    fixed_rule const& r = tree->root.rule->rule;
    BOOST_FOREACH(fixed_rule::tree_node const& nd, r.lhs()) {
        if (nd.is_leaf()) {
            if (not first) sent << ' ';
            if (nd.lexical()) {
                sent << nd.get_token(); 
            }
            else if (nd.indexed()) {
                //sent << sentence_yield(tree->children[nd.index()],h);
                sent << sentence_yield(tree->children[tree->root.rule->rhs2var.find(nd.index())->second],h); 
            }
            first = false;
        }
    }
    return sent.str();
}

std::ostream& print_features(std::ostream& out, xedge const& xe, xedge_components_f func, header& h)
{
    out << '<';
    bool first = true;
    sbmt::weight_vector fv = func(xe);
    BOOST_FOREACH(xequiv const& xeq, xe.children) if (root(xeq).type() == sbmt::foreign_token) fv += sbmt::weight_vector(xeq.begin()->rule->costs);
    BOOST_FOREACH(sbmt::weight_vector::value_type v, fv) {
      if (h.fdict[v.first] != "rawcount") {
        if (not first) {
            out << ',';
        } else {
            first = false;
        }
        out << h.fdict.get_token(v.first) << ':' << v.second;
      }
    }
    out << '>';
    return out;
}

std::string headword_derivation(xtree_ptr tree, header& h)
{
    std::stringstream deriv;
    deriv << token_label(h.dict);
    //lmstring_adaptor lsa(tree->root.rule,0);
    if (tree->children.empty()) {
        deriv << tree->root.rule->rule.id() << ":" << tree->hword;
    } else {
        deriv << '(' << tree->root.rule->rule.id() << ":" << tree->hword;
        BOOST_FOREACH(xtree::children_type::const_reference c, tree->children) {
            deriv << ' ' << headword_derivation(c,h);
        }
        deriv << ')';
    } 
    return deriv.str();
}

std::string derivation(xtree_ptr tree, xedge_components_f func, header& h)
{
    std::stringstream deriv;
    deriv << token_label(h.dict);
    //lmstring_adaptor lsa(tree->root.rule,0);
    if (tree->children.empty()) {
        deriv << tree->root.rule->rule.id();
    } else {
        deriv << '(' << tree->root.rule->rule.id();
        BOOST_FOREACH(xtree::children_type::const_reference c, tree->children) {
            deriv << ' ' << derivation(c,func,h);
        }
        deriv << ')';
    } 
    return deriv.str();
}

set<bigint_t>& used_rules(xtree_ptr tree, set<bigint_t>& used, used_rules_map* umap)
{
    used.insert(tree->root.rule->rule.id());
    if (umap) umap->insert(std::make_pair(tree->root.rule->rule.id(),tree->root.rule));
    BOOST_FOREACH(xtree_ptr ctree, tree->children) used_rules(ctree,used,umap);
    return used;
}

set<bigint_t> used_rules(xtree_ptr tree,used_rules_map* umap)
{
    set<bigint_t> used;
    used_rules(tree,used,umap);
    return used;
}

typedef std::tr1::unordered_map<void const*,sbmt::indexed_token> headwordmap;

sbmt::indexed_token headword(xequiv const& xeq, headwordmap& hwm);
sbmt::indexed_token headword(xedge const& xe, headwordmap& hwm);

sbmt::indexed_token headword(xequiv const& xeq, headwordmap& hwm)
{
    headwordmap::iterator pos = hwm.find(id(xeq));
    if (pos != hwm.end()) {
        return pos->second;
    } else {
        sbmt::indexed_token tok = headword(*xeq.begin(),hwm);
        hwm.insert(std::make_pair(id(xeq),tok));
        return tok;
    }
}

sbmt::indexed_token headword(xedge const& xe, headwordmap& hwm)
{
    if (xe.rule->hwd->indexed()) {
        return headword(xe.children[xe.rule->hwd->index()],hwm);
    } else {
        return xe.rule->hwd->get_token();
    }
}

typedef std::tr1::unordered_map<void const*,size_t> idmap;

void print_hyperedge( std::ostream& out
                    , xedge const& xe
                    , idmap& idm
                    , headwordmap& hwm
                    , header& h
                    , used_rules_map* umap
                    , xedge_components_f func
                    , options const& opts );


void print_forest( std::ostream& out
                 , xequiv const& xeq
                 , idmap& idm
                 , headwordmap& hwm
                 , header& h
                 , used_rules_map* umap
                 , xedge_components_f func
                 , options const& opts )
{
    idmap::iterator pos = idm.find(id(xeq));
    if (pos != idm.end()) {
        out << '#' << pos->second;
    } else {
        size_t id_ = idm.size() + 1;
        idm.insert(std::make_pair(id(xeq),id_));
        out << '#' << id_;
        if (xeq.size() > 1) out << "(OR";
        bool not_first = false;
        size_t N = 0;
        BOOST_FOREACH(xedge const& alt, xeq) {
            if (xeq.size() > 1 or not_first) out << ' ';
            print_hyperedge(out,alt,idm,hwm,h,umap,func,opts);
            not_first = true;
            ++N;
            if (N >= 20) break;
        }
        if (xeq.size() > 1) out << " )";
    }
}

std::string cptstr(sbmt::indexed_token t, header& h)
{
    std::string s = h.dict.label(t);
    if (s[0] == '/' or s[0] == ':') return s.substr(1,std::string::npos);
    if (s[0] == '`' and s[s.size() - 1] == '`') return s.substr(1,s.size() - 2);
    else return s;
}

sbmt::indexed_token
print_hyperedge_treenode( std::ostream& out
                        , xedge const& xe
                        , fixed_rule::tree_node const& nd
                        , idmap& idm
                        , headwordmap& hwm
                        , header& h
                        , used_rules_map* umap
                        , xedge_components_f func
                        , options const& opts )
{
    if (nd.indexed()) {
        out << ' ';
        print_forest(out,xe.children[nd.index()],idm,hwm,h,umap,func,opts);
        return headword(xe.children[nd.index()],hwm);
    } else if (nd.children_begin()->lexical()) {
        sbmt::indexed_token w = nd.children_begin()->get_token();
        if (h.dict.label(w)[0] == '/') {
            out << ' ' << "\"inst(" << cptstr(w,h) << ")\"";
        }
        return w;
    } else {
        uint32_t hpos = xe.rule->headmarker[&nd - xe.rule->rule.lhs_begin()];
        std::vector<sbmt::indexed_token> c;
        BOOST_FOREACH(fixed_rule::tree_node const& cnd, nd.children()) {
            c.push_back(print_hyperedge_treenode(out,xe,cnd,idm,hwm,h,umap,func,opts));
        }
        if (c.size() == 3) {
            sbmt::indexed_token hc, dc, rl;
            uint32_t xx = 0;
            BOOST_FOREACH(sbmt::indexed_token cc, c) {
                ++xx;
                if (hpos == xx) hc = cc;
                else if (h.dict.label(cc)[0] == ':') rl = cc;
                else dc = cc;
            }
            out << " \"" << cptstr(rl,h) << "(" << cptstr(hc,h) << "," << cptstr(dc,h) << ")\"";
        }
        return c[hpos - 1];
    }
}


void print_hyperedge( std::ostream& out
                    , xedge const& xe
                    , idmap& idm
                    , headwordmap& hwm
                    , header& h
                    , used_rules_map* umap
                    , xedge_components_f func
                    , options const& opts )
{
    out << '(';
    out << xe.rule->rule.id();
    if (umap) umap->insert(std::make_pair(xe.rule->rule.id(),xe.rule));
    if (opts.forest_format == forest_format_type::target_string or 
        opts.forest_format == forest_format_type::amr_string) print_features(out,xe,func,h);
    else {
        float cst = cost(xe);
        BOOST_FOREACH(xequiv xeq, xe.children) { cst = cst - cost(xeq); }
        out << "<pass:" << cst << ">";
    }
    if (opts.forest_format == forest_format_type::target_string) {
        BOOST_FOREACH(fixed_rule::tree_node const& nd, xe.rule->rule.lhs()) {
            if (nd.is_leaf()) {
                if (nd.lexical()) {
                    out << ' ' << '"' << gusc::escape_c(h.dict.label(nd.get_token())) << '"';
                } else {
                    out << ' ';
                    print_forest(out,xe.children[nd.index()],idm,hwm,h,umap,func,opts);
                }
            }
        } 
    } else if (opts.forest_format == forest_format_type::amr_string) {
        print_hyperedge_treenode(out,xe,*xe.rule->rule.lhs_root(),idm,hwm,h,umap,func,opts);
    } else {
        BOOST_FOREACH(xequiv cxe, xe.children) {        
            if (root(cxe).type() != sbmt::foreign_token) {
                out << ' ';
                print_forest(out,cxe,idm,hwm,h,umap,func,opts);
            } else {
                out << " " << cxe.span;// span(cxe);
            }
        }
    }
    out << " )";
}

void print_forest( std::ostream& out
                 , xequiv const& xeq
                 , header& h
                 , used_rules_map* umap
                 , xedge_components_f func
                 , options const& opts )
{
    idmap idm;
    headwordmap hwm;
    if (opts.forest_format == forest_format_type::rules) {
        out << "forest\n";
    }
    if (xeq.begin() == xeq.end()) {
        if (opts.forest_format == forest_format_type::rules) {
            out << "#0(0<noparse:1> [0,1] )";
        } else {
            out << "(0<noparse:1> \"NOPARSE\" )";
        }
    } else {
        print_forest(out,xeq,idm,hwm,h,umap,func,opts);
    }
}

typedef std::priority_queue<xedge,std::vector<xedge>,lower_cost> xequiv_construct;
typedef std::tr1::unordered_map<xedge,xequiv_construct,boost::hash<xedge> > 
        xcell_construct;
typedef std::tr1::unordered_map<indexed_token,xcell_construct,boost::hash<indexed_token> > 
        xspan_construct;

void set_infos(std::string& s, std::vector<std::string> const& incoming1)
{
    std::deque<std::string> incoming;
    BOOST_FOREACH(std::string in, incoming1) if (in != "") incoming.push_back(in);
    SBMT_VERBOSE_STREAM(decoder_domain, "old info set: \"" << s << "\"");
    SBMT_VERBOSE_STREAM(decoder_domain, "incoming info set: \"" << boost::algorithm::join(incoming,",") << "\"");
    std::deque<std::string> current;
    boost::algorithm::split(current,s,boost::is_any_of(", "),boost::token_compress_on);
    if (incoming[0] == "+") {
        std::set<std::string> infos(current.begin(),current.end());
        for (size_t x = 1; x != incoming.size(); ++x) if (infos.find(incoming[x]) == infos.end()) {
            current.push_back(incoming[x]);
            infos.insert(incoming[x]);
        }
        s = boost::algorithm::join(current,",");
    } else if (incoming.back() == "+") {
        std::set<std::string> infos(current.begin(),current.end());
        for (int x = incoming.size() - 2; x > -1; --x) if (infos.find(incoming[x]) == infos.end()) {
            current.push_front(incoming[x]);
            infos.insert(incoming[x]);
        }
        s = boost::algorithm::join(current,",");
    } else if (incoming[0] == "-") {
        std::set<std::string> infos(incoming.begin(),incoming.end());
        std::vector<std::string> newcurrent;
        BOOST_FOREACH(std::string info, current) {
            if (infos.find(info) == infos.end()) newcurrent.push_back(info);
        }
        s = boost::algorithm::join(newcurrent,",");
    } else {
        s = boost::algorithm::join(incoming,",");
    }
    SBMT_INFO_STREAM(decoder_domain, "new info set: \"" << s << "\"");
}

void echo_infos(std::vector<std::string> const& incoming)
{
    std::cout << "use-info " << boost::algorithm::join(incoming,",") << " ;\n" << std::endl;
}

template <class R> size_t size_(R const& r)
{
    size_t x = 0;
    BOOST_FOREACH(typename boost::range_reference<R const>::type rr, r)
    {
        ++x;
    }
    return x;
}

struct decode_data {
    wildcard_array wc;
    indexed_token vartok;
    graph_t skipg;
    addums_t ad;
    grammar_facade gram;
    sbmt::lattice_tree ltree;
    gusc::shared_varray<any_xinfo_factory> factories;
    typedef map<size_t,vtx_t> mapsvtx;
    mapsvtx smap;
    typedef map<size_t,vector<indexed_token> > usage_lists;
    usage_lists freelists;
    usage_lists loadlists;
    wordtrie_map wtm;
    wordtrie_mutex_map wtmtxm;
    xchart chrt;
    xchart_holds chrthlds;
    row_holds rowhlds;
    size_t active_rows;
    size_t max_active_rows;
    mapsvtx::iterator end;
    mapsvtx::reverse_iterator rbegin;
    map< vtx_t, vector<cluster_search_data> > vcs;
    sbmt::thread_pool spanpool;
    sbmt::thread_pool sortpool;
    boost::mutex mutex;
    boost::condition jobs_done;
    boost::condition row_finished;
    bool abort;
    bool deterministic;
    std::multimap<sbmt::indexed_token,edge_t> mbyedges;
    chart_initial_rule_map cirm;
    boost::tuple<boost::shared_array<char>,size_t> cirmdata;
    decode_data( gusc::lattice_ast const& lat 
               , size_t id
               , header& h
               , sbmt::weight_vector& weights
               , word_trie_stack& wts
               , options& opts )
    : wc(h.dict)
    , vartok(wc[0])
    , skipg(skip_lattice(to_dag(lat,h.dict),h.dict))
    , ad(addums(skipg))
    , gram(&h,&weights)
    , ltree(convert(gram,lat))
    , factories(get_info_factories(opts.infos,gram,ltree,get_property_map()))
    , active_rows(0)
    , max_active_rows(opts.num_threads)
    , spanpool(opts.num_threads)
    , sortpool(std::min(int(opts.num_threads),4))
    , abort(false)
    , deterministic(not opts.nondet)
    , cirmdata(create_lattice_rules(lat,weights,h,cirm))
    {
        BOOST_FOREACH(any_xinfo_factory const& xf, factories) if (not xf.deterministic()) deterministic = false;
        
        SBMT_INFO_STREAM(decoder_domain, "deterministic: " << deterministic);
        SBMT_INFO_STREAM(decoder_domain, "num-threads: " << opts.num_threads);
        typedef map<indexed_token,size_t> usage_map;
        usage_map usage_limits;
        BOOST_FOREACH(edge_t e, edges(skipg)) { 
            if (skipg[boost::source(e,skipg)] != 0) {
                usage_limits[skipg[e]] = std::max(usage_limits[skipg[e]],skipg[target(e,skipg)]);
            }
        }
        BOOST_FOREACH(vtx_t v, vertices(skipg)) freelists[skipg[v]] = vector<indexed_token>();
        BOOST_FOREACH(usage_map::value_type v, usage_limits) {
            freelists[v.second + opts.limit_syntax_length].push_back(v.first);
        }
        usage_map usage_begins;
        BOOST_FOREACH(edge_t e, edges(skipg)) {
            if (skipg[e].type() == sbmt::foreign_token) {
                wtm[skipg[e]] = word_trie_data();
                wtmtxm[skipg[e]] = boost::shared_ptr<boost::mutex>(new boost::mutex());
                if (usage_begins.find(skipg[e]) == usage_begins.end()) {
                    usage_begins[skipg[e]] = skipg[target(e,skipg)];
                } else {
                    usage_begins[skipg[e]] = std::min(usage_begins[skipg[e]],skipg[target(e,skipg)]);
                }
            }
        }
        BOOST_FOREACH(vtx_t v, vertices(skipg)) loadlists[skipg[v]] = vector<indexed_token>();
        BOOST_FOREACH(usage_map::value_type v, usage_begins) {
            loadlists[v.second].push_back(v.first);
        }
        wtm[vartok] = wts[0];
        BOOST_FOREACH(word_trie_data& wt, wts) {
            wt.clear_cache(info_rule_sorter(&gram,factories,&weights,&opts.priormap));
        }
        // search space created for global, per-sent grammars. not 
        BOOST_FOREACH(vtx_t vtx, vertices(skipg)) {
            fs::path p;
            BOOST_FOREACH(word_trie_data wt, wts)
                vcs[vtx].push_back(cluster_search_data(h,wt,&skipg,vtx));
            smap.insert(make_pair(skipg[vtx],vtx));
        }
        end = smap.end(); 
        rbegin = mapsvtx::reverse_iterator(smap.begin());
        mapsvtx::iterator right;
        mapsvtx::reverse_iterator left;
        span_t spn;
        
        multimap<span_t,sbmt::indexed_token> initmap;
        BOOST_FOREACH(edge_t ed, edges(skipg)) if (skipg[ed].type() == sbmt::foreign_token) {
            span_t s(skipg[source(ed,skipg)],skipg[target(ed,skipg)]);
            initmap.insert(make_pair(s,skipg[ed]));
        }
        
        for (right = smap.begin(); right != end; ++right) {
            bool initial = true;
            left = mapsvtx::reverse_iterator(right);
            for (;left != rbegin; ++left) {
                spn=span_t(skipg[left->second],skipg[right->second]);
                size_t nequivs = size_(initmap.equal_range(spn));
                chrt[spn] = xspan(nequivs);
                size_t x = 0;
                sbmt::indexed_token fw;
                sbmt::span_t s_;
                BOOST_FOREACH(boost::tie(s_,fw), initmap.equal_range(spn)) {
                    SBMT_VERBOSE_STREAM(decoder_domain, "populating " << spn << " with " << fw);
                    assert(s_ == spn);
                    chrt[spn][x] = xcell(1);
                    chrt[spn][x][0] = xequiv(spn,fw,factories.size(),cirm[spn][fw].get());
                    ++x; 
                }
                chrthlds.insert(std::make_pair(spn,initial ? 0 : 2));
                initial = false;
            }
            rowhlds.insert(std::make_pair(right->first,shared_row_hold_data(new row_hold_data())));
        }
        end = smap.end();
        
        BOOST_FOREACH(edge_t e, edges(skipg)) if (sbmt::is_lexical(skipg[e])) {
            mbyedges.insert(std::make_pair(skipg[e],e));
        }
    }

    xedge_components_f weight_maker(sbmt::span_t spn, options const& opts, boost::object_pool<info_tree>& info_pool)
    {
        return info_make_xedges(&gram,spn,factories,&opts.priormap,&info_pool);
    }
    
    xedge_generator 
    make_edges( std::vector<cluster_search_data>& vcs
              , vtx_t v
              , xchart const& chrt
              , sbmt::span_t spn
              , boost::pool<>& cell_pool
              , boost::pool<>& edge_pool
              , boost::object_pool<info_tree>& info_pool
              , options& opts )
    {
        if (deterministic) 
            return ::make_edges_det( 
                               vcs
                             , v
                             , chrt
                             , info_make_xedge(&gram,spn,factories,&opts.priormap,&info_pool)
                             , cell_pool
                             , edge_pool
                             , sortpool
                             , opts
                             );
        else
            return ::make_edges( 
                               vcs
                             , v
                             , chrt
                             , info_make_xedges(&gram,spn,factories,&opts.priormap,&info_pool)
                             , cell_pool
                             , edge_pool
                             , sortpool
                             , opts
                             );
    }
    void send_abort()
    {
        {
            boost::mutex::scoped_lock lk(mutex);
            abort = true;
        }
        size_t ignr;
        shared_row_hold_data sd;
        BOOST_FOREACH(boost::tie(ignr,sd),rowhlds) { 
            boost::mutex::scoped_lock lk(sd->mtx);
            sd->cond.notify_all();
        }
        {
            boost::mutex::scoped_lock lk(mutex);
            row_finished.notify_all();
            jobs_done.notify_all();
        }
    }
    ~decode_data()
    {
        send_abort();
    }
};

void source_string(std::string& str, xtree const& e, header& h)
{
    //assert(e.children.size() == e.root.rule->rule.rhs_size());
    for (size_t x = 0, nx = 0; x != e.root.rule->rule.rhs_size(); ++x) {
        if (not (e.root.rule->rule.rhs_begin() + x)->indexed()) {
            str += " " + h.dict.label((e.root.rule->rule.rhs_begin() + x)->get_token());
        } else {
            assert((e.root.rule->rule.rhs_begin() + x)->get_token() == 
                e.children[nx]->root.rule->rule.lhs_root()->get_token());
            source_string(str, *e.children[nx],h);
            ++nx;
        }
    }
    //if (e.children.size() == 0) str += " " + h.dict.label(e.root->rule.lhs_root()->get)
}

std::string source_string(xtree const& e, header& h)
{
    std::string ret;
    source_string(ret,e,h);
    return ret;
}

void compute_align_rec(sbmt::alignment& a, xtree const& e )
{

    //precondition_children p(d,grammar());
    unsigned ns=e.root.rule->rule.rhs_size();
    std::vector<alignment> ca(ns);
    typedef sbmt::alignment::substitution sub;
    std::vector<sub> subs;
    for (unsigned vi = 0, ni = 0; ni != ns; ++ni) {
        
        if ((e.root.rule->rule.rhs_begin() + ni)->indexed()) {
            xtree const& ce = *e.children[vi];
            compute_align_rec(ca[ni],ce);
            subs.push_back(sub(ca[ni],ni));
            ++vi;
        }
    }
    
    // convert decoder alignment into jgraehls alignment string, and parse
    std::stringstream sstr;
    sstr << "[ #s=" << e.root.rule->rule.rhs_size();
    sstr << " #t=" << e.root.rule->rule.lhs_yield_size();

    int s , t;
    BOOST_FOREACH(boost::tie(t,s), e.root.rule->tgt_src_aligns) {
        sstr << ' ' << s << ',' << t;
    }
    sstr << " ]";
    //std::cerr << e.root.rule->rule;
    //std::cerr << " rulealign: " << sstr.str() << '\n';
    sbmt::alignment(sstr.str()).substitute_into(a,subs);
    //get_scored_syntax(e).align.substitute_into(a,subs);
}

sbmt::alignment align(xtree const& d)
{
    sbmt::alignment a;
    compute_align_rec(a,d);
    return a;
}

void print_results( xequiv xeq
                  , size_t id
                  , decode_data& dd
                  , options const& opts
                  , header& h
                  , std::string errormsg = "didn't build any toplevel items")
{
    used_rules_map umap;
    used_rules_map* pumap = opts.append_rules ? &umap : 0;
    sbmt::span_t spn = xeq.span;
    boost::object_pool<info_tree> info_pool;
    
    if (opts.output_format == output_format_type::nbest) {
        if (xeq.begin() == xeq.end()) {
            std::cout << "NBEST sent=" 
                      << id << ' '
                      << "nbest=0 totalcost=0 kbest=0 hyp={{{NOPARSE}}} failed-parse=1 tree={{{(TOP NOPARSE)}}} "
                      << "derivation={{{0}}} align={{{0-0}}} source={{{NOPARSE}}} "
                      << "used-rules={{{0}}} fail-msg={{{ "<<errormsg<<" }}}"
                      << std::endl;
        } else {
            xtree_generator treegen = xtrees_from_xequiv(xeq);
            size_t m = 0;
            size_t M = 0;
            map<string,size_t> uniqlist;
        
            while (treegen and m != opts.nbests and M != opts.nbest_pops) {
                xtree_ptr tree = *treegen;
                string sent = sentence_yield(tree,h);
                map<string,size_t>::iterator upos = uniqlist.find(sent);
                if (upos == uniqlist.end() or (opts.estring_copies > 0 and upos->second < opts.estring_copies) or opts.estring_copies == 0) {
                    if (upos == uniqlist.end() and opts.estring_copies > 0) uniqlist.insert(make_pair(sent,1));
                    else if (opts.estring_copies > 0) upos->second += 1;
                    std::cout << "NBEST sent=" << id << " nbest=" << m << " totalcost=" << std::flush << tree->cost << ' ' << std::flush;
                    std::cout << "kbest=" << M << ' ' << std::flush;
                    std::cout << "hyp={{{"<< sent << "}}} " << std::flush;
                    std::cout << "tree={{{" << hyptree(tree,h.dict) << "}}} " << std::flush;
                    std::cout << "headword_derivation={{{" << headword_derivation(tree,h) << "}}} " << std::flush;
                    std::cout << "derivation={{{" << derivation( tree
                                                               , dd.weight_maker(spn,opts,info_pool)
                                                               , h
                                                               ) << "}}} " << std::flush;
                    std::cout << "used-rules={{{ ";
                    BOOST_FOREACH(bigint_t r, used_rules(tree,pumap)) std::cout << r << ' ';
                    std::cout << "}}} " << std::flush;
                    std::cout << "align={{{";
                    sbmt::alignment al = ::align(*tree);
                    al.sort();
                    for (unsigned i=0,e=al.n_src();i!=e;++i)
                        for (sbmt::alignment::tars::const_iterator j=al.sa[i].begin(),je=al.sa[i].end();j!=je;++j) {
                            std::cout << ' ' << *j << '-' << i;
                        }
                    std::cout << " }}} " << std::flush; 
                    
                    std::cout << "source={{{";
                    std::cout << source_string(*tree,h);
                    std::cout << " }}}";
                    
                    weight_vector wv = accum( tree
                                            , dd.weight_maker(spn,opts,info_pool)
                                            );
                    
                    BOOST_FOREACH(weight_vector::const_reference v, wv) {
		      if (h.fdict[v.first] != "rawcount") {
                        std::cout << ' ' << h.fdict.get_token(v.first) << '=' << v.second;
		      }
                    }
                    std::cout << std::endl;
                    ++m;
                }
                ++M;
                ++treegen;
            }
        }
    } else {
        SBMT_INFO_STREAM(decoder_domain, "writing forest for sentence " << id << " beginning");
        print_forest( std::cout
                    , xeq
                    , h
                    , pumap
                    , dd.weight_maker(spn,opts,info_pool)
                    , opts
                    );
        SBMT_INFO_STREAM(decoder_domain, "writing forest for sentence " << id << " complete");
        std::cout << std::endl;
    }
    if (opts.append_rules) {
        if (xeq.begin() == xeq.end()) {
	  std::cout << "TOP(\"NOPARSE\") -> \"NOPARSE\" ### id=0 noparse=1 sent="<<id<<"\n";
        }
        size_t ruid; rule_application const* rule;
        BOOST_FOREACH(boost::tie(ruid,rule), umap) {
            rule->print(std::cout,h);
            std::cout << " sent="<<id<<std::endl;
        }
        if (not opts.pop_newline) std::cout << '\n' << std::flush;
        if (opts.pass == multipass::source or opts.pass == multipass::pipe) {
            std::cout << ";\n\n" << std::flush;
        }
        std::cout << std::flush;
    }
    
    SBMT_INFO_STREAM(decoder_domain, "results for sentence " << id << " processed");
}


template <class K, class V, class S, class CK>
V& get(std::map<K,V,S>& mp, CK const& k)
{
    return mp.find(k)->second;
}

void decode_row( size_t id
               , header& h
               , sbmt::weight_vector& weights
               , options& opts 
               , decode_data& dd
               , decode_data::mapsvtx::reverse_iterator left
               , decode_data::mapsvtx::iterator right
               , span_t total )
{
    try { 
      while (true) {
        
        bool breakit = false;
        span_t spn=span_t(left->first,right->first);
        if (dd.chrthlds.find(spn)->second != 0) {
            SBMT_TERSE_STREAM(decoder_domain, "logic error: calculating span with hold on it");
            throw std::logic_error("calculating span with hold on it");
        }
        if (not (spn.left() != 0 and spn.right() - spn.left() > (int)opts.limit_syntax_length)) {
            addums_t::iterator apos = dd.ad.find(boost::make_tuple(left->second,right->second));
            if (apos != dd.ad.end()) {
                BOOST_FOREACH(edge_t e, apos->second){
                    indexed_token lookup = dd.skipg[e];
                    if (cluster_exists(opts.dbdir,h,lookup)) {
                        get(dd.vcs,left->second).push_back(cluster_search_data(h,get(dd.wtm,lookup),&dd.skipg,left->second,e));
                        //cluster_search_data csd(h,get(dd.wtm,lookup,&dd.skipg,left->second,e));
                        //load_rules(dd.skipg,csd,dd.smap,left->second,h);
                    }
                }
            }

            xspan_construct scons;
            boost::pool<> equiv_pool(sizeof(equiv_tree));
            boost::pool<> cell_pool(sizeof(cell_tree));
            boost::object_pool<info_tree> info_pool;
            xedge_generator gxe = dd.make_edges( get(dd.vcs,left->second)
                                               , right->second
                                               , dd.chrt
                                               , spn
                                               , cell_pool
                                               , equiv_pool
                                               , info_pool
                                               , opts
                                               );
            size_t equivs = 0; // number of equivalences kept
            size_t pops = 0; // number of pops
            //boost::random::mt19937 gen(std::time(0));
            //boost::random::uniform_int_distribution<> dist(1, 100);
            //if (dist(gen) == 95) throw std::bad_alloc();
            while ( ((pops < opts.poplimit_multiplier*opts.histogram) or
                     ((equivs < opts.histogram) and (pops < opts.softlimit_multiplier*opts.histogram)))
                     and bool(gxe) ) {
                xedge xe = gxe();
                xequiv_construct& xec = scons[root(xe)][xe];
                if (xec.empty()) {
                    ++equivs;
                    xec.push(xe);
                } else if (xec.size() < opts.max_equivalents or root(xe).type() == sbmt::top_token) {
                    xec.push(xe);
                } else if (cost(xec.top()) > cost(xe)) {
                    xec.pop();
                    xec.push(xe);
                }
                ++pops;
            }
            equiv_pool.purge_memory();
            cell_pool.purge_memory();
            std::stringstream slog;
            slog << "sent " << id <<" ["<< left->first << ',' << right->first << "] ";
            slog << "[" << equivs << " equivalences]";
            xspan xs(scons.size() + dd.chrt[spn].size());
            size_t y = 0;
            BOOST_FOREACH(xspan_construct::value_type& p, scons) {
                xcell xc(p.second.size());
                int x = 0;
                BOOST_FOREACH(xcell_construct::value_type& pp, p.second) {
                    xequiv_construct& xeq = pp.second;
                    std::vector<xedge> xcx(xeq.size());
                    //xc[x] = xequiv(xeq.size());
            
                    size_t z = xeq.size();
                    while (not xeq.empty()) {
                        --z;
                        xcx[z] = xeq.top();
                        //xc[x].assign(z,xeq.top());
                        xeq.pop();
                    }
                    xc[x] = xequiv(xcx);
                    //std::cerr << root(xc[x]) << '\t' << cost(xc[x]) << '\t' << heur(xc[x]) << '\n';
                    ++x;
                }
                std::sort(xc.begin(),xc.end(),lower_cost());
                xs[y] = xc;
                ++y;
            }
            BOOST_FOREACH(xcell xc, dd.chrt[spn]) {
                xs[y] = xc;
                ++y;
            }
            std::sort(xs.begin(),xs.end(),lower_cost());
            slog << "[" << xs.size() <<  " cells]["<< pops << " pops][cost "<<cost(xs)<<"]";
            get(dd.chrt,spn) = xs;
            if (spn.left() == 0) {
                BOOST_FOREACH(indexed_token wd, get(dd.freelists,spn.right())) {
                    slog << " dropping cluster data for "<< wd;
                    wordtrie_map::iterator pos = dd.wtm.find(wd);
                    if (pos != dd.wtm.end()) pos->second.drop();
                }
                {
                    boost::mutex::scoped_lock lk(dd.mutex);
                    dd.active_rows -= 1;
                    dd.row_finished.notify_all();
                }
            }
            SBMT_INFO_STREAM(decoder_domain, slog.str());
        }
        
        decode_data::mapsvtx::iterator next = right; ++next;
        if (next != dd.end) {
            span_t spn2(left->first,next->first);
            size_t holds;
            {
                boost::mutex::scoped_lock lk(dd.rowhlds.find(next->first)->second->mtx);
                holds = --dd.chrthlds.find(spn2)->second;
            }
            std::stringstream sstr;
            sstr << spn << " -= " << spn2 << " (holds=" << holds <<")\n";
            SBMT_VERBOSE_STREAM(decoder_domain, sstr.str());
            if (holds == 0) {
                boost::mutex::scoped_lock lk(dd.rowhlds.find(next->first)->second->mtx);
                dd.rowhlds.find(next->first)->second->cond.notify_all();
            }
        }
        decode_data::mapsvtx::reverse_iterator rnext = left; ++rnext;
        if (rnext != dd.rbegin) {
            span_t spn1(rnext->first,right->first);
            size_t holds;
            {
                boost::mutex::scoped_lock lk(dd.rowhlds.find(right->first)->second->mtx);
                holds = --dd.chrthlds.find(spn1)->second;
            }
            std::stringstream sstr;
            sstr << spn << " -= " << spn1 << " (holds=" << holds <<")\n";
            SBMT_VERBOSE_STREAM(decoder_domain, sstr.str());
            if (holds != 0) {
                boost::mutex::scoped_lock lk(dd.rowhlds.find(right->first)->second->mtx);
                holds = dd.chrthlds.find(spn1)->second;
                while (holds != 0 and not dd.abort) {
                    sstr << spn << " -| " << spn1 << " (holds=" << holds << ")\n";
                    SBMT_VERBOSE_STREAM(decoder_domain, sstr.str());
                    dd.rowhlds.find(right->first)->second->cond.wait(lk);
                    holds = dd.chrthlds.find(spn1)->second;
                }
                if (dd.abort) {
                    breakit = true;
                    SBMT_WARNING_STREAM(decoder_domain,spn << " aborting");
                }
                //sstr << spn << " -> " << spn1 << "\n";
            }
        } else {
            breakit = true;
        }
        if (spn == total) {
            boost::mutex::scoped_lock lk(dd.mutex);
            dd.jobs_done.notify_all();
        }
        if (breakit) break;
        else ++left;
    } } catch(std::exception const& e) {
        dd.send_abort();
        SBMT_ERROR_STREAM(decoder_domain,  "caught exception: " << e.what());
        throw;
    }   catch(...) {
        dd.send_abort();
        SBMT_ERROR_STREAM(decoder_domain, "caught unknown exception");
        throw;
    }
}

void decode_range( size_t id
                 , header& h
                 , sbmt::weight_vector& weights
                 , options& opts
                 , decode_data& dd
                 , decode_data::mapsvtx::iterator begin
                 , decode_data::mapsvtx::iterator end
                 , span_t total )
{
    decode_data::mapsvtx::iterator right;
    decode_data::mapsvtx::reverse_iterator left;

    for (right = begin;right != end; ++right) {
        left = decode_data::mapsvtx::reverse_iterator(right);
        
        {
            boost::mutex::scoped_lock lk(dd.mutex);
            while (dd.active_rows >= dd.max_active_rows and not dd.abort) {
                dd.row_finished.wait(lk);
            }
            if (dd.abort) break;
        }
        
        size_t rtidx = dd.skipg[right->second];
        BOOST_FOREACH(indexed_token lookup, get(dd.loadlists,rtidx)) {
            if (get(dd.wtm,lookup).get() == 0 and cluster_exists(opts.dbdir,h,lookup)) {
                fs::path p; string ignore;
                boost::tie(p,ignore) = structure_from_token(lookup);
                fs::path loc = opts.dbdir / p;
                std::stringstream sstr;
                SBMT_INFO_STREAM(decoder_domain, "loading cluster data for "<< lookup);
                /*loadrules( dd.skipg
                         , dd.smap
                         , dd.wtm
                         , lookup
                         , dd.mbyedges
                         , h
                         , dbdir 
                         , info_rule_sorter(&dd.gram,dd.factories,&weights,&opts.priormap) 
                         );*/
                rule_sort_f sorter = null_sorter();
                if (not opts.no_sort_rules) {
                    sorter = info_rule_sorter(&dd.gram,dd.factories,&weights,&opts.priormap);
                }
                get(dd.wtm,lookup) = word_trie_data(loc.native(),h.offset(lookup),sorter);
            }
        }
        
        if (left != dd.rbegin) {
            {
                boost::mutex::scoped_lock lk(dd.mutex);
                dd.active_rows += 1;
            }
            dd.spanpool.add( boost::bind( decode_row
                                        , id
                                        , boost::ref(h)
                                        , boost::ref(weights)
                                        , boost::ref(opts)
                                        , boost::ref(dd)
                                        , left
                                        , right
                                        , total
                                        )
                           );
        }
    }
    {
        boost::mutex::scoped_lock lk(dd.mutex);
        if (not dd.abort) dd.jobs_done.wait(lk);
    }
    dd.spanpool.wait();

}



void store_decode_data( gusc::lattice_ast const& inlat
                      , size_t inid
                      , gusc::lattice_ast& outlat
                      , size_t & outid )
{
    SBMT_VERBOSE_STREAM(decoder_domain, "storing " << inlat);
    outlat = inlat;
    outid = inid;
}

typedef std::map< exmp::forest_ptr
               , gusc::shared_varray<xequiv>
               > rescore_map;


gusc::shared_varray<xequiv>
decode_forest( rule_application_map const& rmap
             , rescore_map& rsmap
             , options& opts
             , decode_data& dd
             , xedge_construct_f func
             , exmp::forest_ptr inforest
             , sbmt::indexed_token intoken )
             ;

template <class Range1, class Range2> 
boost::iterator_range<
  boost::zip_iterator<
    boost::tuple<
      typename boost::range_iterator<Range1>::type
    , typename boost::range_iterator<Range2>::type
    >
  >
> zip_range(Range1 rng1, Range2 rng2)
{
    return 
    boost::make_iterator_range(
      boost::make_zip_iterator(boost::make_tuple(boost::begin(rng1),boost::begin(rng2)))
    , boost::make_zip_iterator(boost::make_tuple(boost::end(rng1),boost::end(rng2)))
    );
}

struct bind_apply_rule_equiv {
    typedef xedge_generator result_type;
    ip::offset_ptr<rule_application> ra;
    apply_rule_equiv are;
    bind_apply_rule_equiv(ip::offset_ptr<rule_application> ra, xedge_construct_f func)
    : ra(ra)
    , are(func) {}
    
    xedge_generator operator()(equiv_in const & in) const
    {
        return are(in,*ra);
    }
};

xedge_generator
from_rule_hyp( exmp::hyp_ptr inhyp
             , xedge_construct_f func
             , rule_application_map const& rmap
             , rescore_map& rsmap
             , boost::pool<>& equiv_pool
             , options& opts
             , decode_data& dd )
{
    typedef
        gusc::any_generator<equiv_in,gusc::iterator_tag> 
        children_generator;
    children_generator 
        gen = children_generator(gusc::generate_single_value(
                equiv_in(
                  0
                , 0.0
                )
              ))
    ;
    
    ip::offset_ptr<rule_application> ra = rmap.find(inhyp->ruleid())->second;
    exmp::forest_ptr inchild;
    fixed_rule::rule_node innd;
    assert(ra->rule.rhs_size() == boost::size(*inhyp));
    //std::cerr << "enter from_rule_hyp: ";
    //ra->print(std::cerr,opts.h);
    //std::cerr << '\n';
    //std::cerr << "*cgen: " << gen->get<0>() << "\n";

    
    BOOST_FOREACH( boost::tie(inchild,innd), zip_range(boost::adaptors::reverse(*inhyp),boost::adaptors::reverse(ra->rule.rhs()))) {
        assert(((inchild->begin() == inchild->end()) and innd.get_token().type() == sbmt::foreign_token) or
               (innd.get_token().type() == sbmt::tag_token)
              );
        gusc::shared_varray<xequiv> svxe = decode_forest(rmap,rsmap,opts,dd,func,inchild,innd.get_token());
        //std::cerr << "svxe.size=" << svxe.size() << '\n';
        assert(svxe.begin() != svxe.end());
        assert(root(svxe[0]) == innd.get_token());
        assert(svxe[0].span == inchild->span());
        gusc::shared_lazy_sequence<children_generator> lzygen(gen);
        //std::cerr << "lzygen.size=" << size_(lzygen);
        gen = generate_product_heap( push_back_equiv(&equiv_pool)
                                   , greater_cost()
                                   , lzygen
                                   , svxe );
        //std::cerr << "*cgen: " << gen->get<0>() << root(*(gen->get<0>()->equiv)) << "\n";
    }
    apply_rule_equiv are(func); 
    
    
    xedge_generator 
    ret =  gusc::generate_union_heap( 
             gusc::make_peekable(
               gusc::generate_transform(
                 gen
               , boost::bind(are,_1,boost::cref(*ra))
               )
             )
           , greater_cost()
           , 5
           );
    //std::cerr << "exit from_rule_hyp: first: " << root(*ret) << ' ';
    //ra->print(std::cerr,opts.h);
    //std::cerr << '\n';
    return ret;
}

xedge_generator
from_rule_hyps( exmp::forest_ptr inforest
              , xedge_construct_f func 
              , rule_application_map const& rmap
              , rescore_map& rsmap
              , boost::pool<>& equiv_pool
              , options& opts
              , decode_data& dd )
{
    //std::cerr << "enter or-node maker: " << inforest << ' ' << inforest->span() << "size=" << boost::size(*inforest) << '\n';
    assert(inforest->begin() != inforest->end());
    xedge_generator 
        gen = gusc::generate_union_heap(
                gusc::make_peekable(
                  gusc::generate_transform(
                    generate_from_range(boost::make_iterator_range(*inforest))
                  , boost::bind( from_rule_hyp
                               , _1
                               , func
                               , boost::cref(rmap)
                               , boost::ref(rsmap)
                               , boost::ref(equiv_pool)
                               , boost::ref(opts)
                               , boost::ref(dd)
                               )
                  )
                )
              , greater_cost()
              , 10
              ); 
    //std::cerr << "exit or-node maker: " << inforest << ' ' << inforest->span() << '\n';
    return gen;
}

gusc::shared_varray<xequiv>
decode_forest( rule_application_map const& rmap
             , rescore_map& rsmap
             , options& opts
             , decode_data& dd
             , xedge_construct_f func
             , exmp::forest_ptr inforest
             , sbmt::indexed_token intoken )
{
    //std::cerr << "enter decode_forest, for " << inforest << ' ' << intoken << ' ' << inforest->span() << '\n';
    rescore_map::iterator pos = rsmap.find(inforest);
    if (pos == rsmap.end()) {
        if (inforest->begin() == inforest->end()) {
            assert(intoken.type() == sbmt::foreign_token);
            gusc::shared_varray<xequiv> ret(1);
            ret[0] = xequiv(inforest->span(),intoken,dd.factories.size());
            pos = rsmap.insert(std::make_pair(inforest,ret)).first;

        } else {
            assert(is_native_tag(intoken));
            // lazy union of rule applications supplied by hyps in inforest
            // each rule application becomes a product of vectors of xequivs
            boost::pool<> equiv_pool(sizeof(equiv_tree));
            xedge_generator gen = from_rule_hyps(inforest,func,rmap,rsmap,equiv_pool,opts,dd);
            typedef std::tr1::unordered_map<xedge,xequiv_construct, boost::hash<xedge> >
                    hyp_map;
            hyp_map hmap;
            unsigned int pops = 0;
            unsigned int equivs = 0;
            while ( ((pops < opts.poplimit_multiplier*opts.histogram) or
                     ((equivs < opts.histogram) and (pops < opts.softlimit_multiplier*opts.histogram)))
                     and bool(gen) ) {
                xedge xe = gen();
                //std::cerr << "edge: " << xe.rule->rule << '\n';
                xequiv_construct& xec = hmap[xe];
                if (xec.empty()) {
                    ++equivs;
                    xec.push(xe);
                } else if (xec.size() < opts.max_equivalents or root(xe).type() == sbmt::top_token) {
                    xec.push(xe);
                } else if (cost(xec.top()) > cost(xe)) {
                    xec.pop();
                    xec.push(xe);
                }
                ++pops;
            }
            gusc::shared_varray<xequiv> ret(hmap.size());
            int x = 0;
            for (hyp_map::iterator itr = hmap.begin(); itr != hmap.end(); ++itr){
                xequiv_construct& xeq = itr->second;
                std::vector<xedge> xcx(xeq.size());
                //xc[x] = xequiv(xeq.size());

                size_t z = xeq.size();
                while (not xeq.empty()) {
                    --z;
                    xcx[z] = xeq.top();
                    //xc[x].assign(z,xeq.top());
                    xeq.pop();
                }
                ret[x] = xequiv(xcx);
                //std::cerr << root(xc[x]) << '\t' << cost(xc[x]) << '\t' << heur(xc[x]) << '\n';
                ++x;
            }
            std::sort(ret.begin(),ret.end(),lower_cost());
            pos = rsmap.insert(std::make_pair(inforest,ret)).first;
        }
    } else {
        assert(not pos->second.empty());
        assert(root(pos->second[0]) == intoken);
        assert(pos->second[0].span == inforest->span());
    }
    //std::cerr << "exit decode_forest, for " << inforest << ' ' << intoken << ' ' << inforest->span() << '\n';
    return pos->second;
}               

void decode_forest_main( gusc::lattice_ast const& lat
                       , size_t const & id
                       , std::vector<std::string> const& forestrules
                       , header& h
                       , sbmt::weight_vector& weights
                       , word_trie_stack& wts
                       , options& opts )
{
    SBMT_INFO_STREAM(decoder_domain, "begin forest-rescoring of sentence " << id);
//    std::cerr << "received lattice:\n" << lat << '\n';
    std::stringstream sstr(forestrules[0]);
    
//    std::cerr << "received data:\n";
//    BOOST_FOREACH(std::string s, forestrules) std::cerr << s << '\n';
    
//    std::cerr << "\nreceived forest:\n";
    exmp::forest fst;
    getforest(sstr,fst,h.fdict);
//    std::cerr << fst << '\n';
    sstr.str("");
    for (int x = 1; x != forestrules.size(); ++x) {
        sstr << forestrules[x] << '\n';
    }
    rule_application_map rmap;
    boost::shared_array<char> rstore;
    create_word_map(rmap,rstore,sstr,fst,weights,h);
//    std::cerr << "received rules: (" << forestrules.size() -1 << ")\n";
    ip::offset_ptr<rule_application> rptr;
    boost::int64_t rid;
    
//    BOOST_FOREACH(boost::tie(rid,rptr), rmap) {
//        std::cerr << rid;
//        std::cerr << '\t'; 
//        rptr->print(std::cerr, h);
//        std::cerr << '\n';
//    }
    
    decode_data dd(lat,id,h,weights,wts,opts);
    rescore_map rsmap;
    boost::object_pool<info_tree> info_pool;
    
    xequiv rt(fst.span(),h.dict.toplevel_tag(),dd.factories.size());
    try {
        gusc::shared_varray<xequiv> 
            ff = decode_forest( rmap
                              , rsmap
                              , opts
                              , dd
                              , info_make_xedges(&dd.gram,fst.span(),dd.factories,&opts.priormap,&info_pool)
                              , exmp::forest_ptr(new exmp::forest(fst))
                              , h.dict.toplevel_tag() );
        
        if (ff.size() > 0) rt = ff[0];
        print_results(rt,id,dd,opts,h);
    } catch(std::exception const& exptn) {
        print_results(rt,id,dd,opts,h,exptn.what());
        throw;
    } catch(...) {
        print_results(rt,id,dd,opts,h,"unknown error");
        throw;
    }
}

void decode( gusc::lattice_ast const& lat
           , size_t id
           , header& h
           , sbmt::weight_vector& weights
           , word_trie_stack& wts
           , options& opts )
{
    if (opts.pass == multipass::source or opts.pass == multipass::pipe) {
        std::cout << lat << '\n' << std::endl;
    }
    
    //make_fullmap(h.dict);
    //boost::pool<> info_pool(sizeof(info_tree));
    decode_data dd(lat,id,h,weights,wts,opts);
    span_t topspn(dd.smap.begin()->first,(--dd.smap.end())->first);
    xequiv rootequiv(topspn,h.dict.toplevel_tag(),dd.factories.size());
    try {
        decode_data::mapsvtx::iterator right;
        decode_data::mapsvtx::reverse_iterator left;
        //std::cerr << "weights: " << print(weights,h.fdict) << '\n';
        for (right = dd.smap.begin();right != dd.end; ) {
            decode_data::mapsvtx::iterator beg = right;
            for (size_t blk = 0; right != dd.end; ++blk, ++right) {}
            decode_data::mapsvtx::iterator end = right; --end;
            span_t total(dd.smap.begin()->first,end->first);
            decode_range(id,h,weights,opts,dd,beg,right,total);
        }
        
        //std::cerr << std::endl;
        if (not dd.chrt[topspn].empty()) rootequiv = dd.chrt[topspn][0][0];
        dd.chrt.clear();
        SBMT_INFO_STREAM(decoder_domain, "printing results for sentence " << id);
        print_results(rootequiv,id,dd,opts,h);
    } catch(std::exception const& exptn) {
        SBMT_INFO_STREAM(decoder_domain, "printing results for sentence " << id << " after exception: " << exptn.what());
        print_results(rootequiv,id,dd,opts,h,exptn.what());
        throw;
    } catch(...) {
        SBMT_INFO_STREAM(decoder_domain, "printing results for sentence " << id << " after unknown exception");
        print_results(rootequiv,id,dd,opts,h,"unknown error");
        throw;
    }
}

typedef map<size_t,vtx_t> mapsvtx;



void print_rules(signature_trie const& sigtrie, signature_trie::state st, header& h)
{
    if (sigtrie.value(st) != sigtrie.nonvalue()) {
        rule_application_array_adapter rules(sigtrie.value(st));
        BOOST_FOREACH(rule_application const& ra, rules) {
            std::stringstream sstr;
            sstr << sbmt::token_label(h.dict);
            ra.print(sstr,h);
            sstr << '\n';
            std::cout << sstr.str();
            //std::cout << ra.rule;
            //uint32_t k; float v;
            //BOOST_FOREACH(boost::tie(k,v), ra.costs) {
            //    std::cout << ' ' << h.fdict.get_token(k) << '=' << v;
            //}
            //std::cout << "\n";
        }
    }
    BOOST_FOREACH(signature_trie::state sst, sigtrie.transitions(st)) {
        print_rules(sigtrie,sst,h);
    }
}

void print_rules(cluster_search_data& cs, vtx_t v, header& h)
{
    cluster_search::result_type crs((*cs)(v));
    make_res mr(cs.wtd,true);
    BOOST_FOREACH(cluster_search::single_result_type const& cb, crs) {
        signature_trie const* sigtrie = mr(cb.get<1>(),null_sorter(),false);
        if (sigtrie) print_rules(*sigtrie,sigtrie->start(),h);
    }
}



void print_rules( graph_t const& skipg
                , cluster_search_data& csd
                , mapsvtx& smap
                , vtx_t v
                , header& h )
{
    mapsvtx::iterator pos = smap.find(skipg[v]);
    for (; pos != smap.end(); ++pos) {
        //std::cerr << "       csd(" << pos->first << ")\n";
        print_rules(csd,pos->second,h);
    }
}

void print_rules( graph_t const& skipg
                , wordtrie_map& wtm
                , mapsvtx& smap
                , header& h
                , fs::path dbdir
                , vtx_t v
                , edge_t e )
{  
    //std::cerr << "print_rules " << skipg[v] << " --> [" << skipg[source(e,skipg)] << "] " << skipg[e] << " [" << skipg[target(e,skipg)] << "]\n";
    if (sbmt::is_lexical(skipg[e])) {
         
        wordtrie_map::iterator pos = wtm.find(skipg[e]);
        if (pos == wtm.end()) {
            if (cluster_exists(dbdir,h,skipg[e])) {
                fs::path p; string ignore;
                boost::tie(p,ignore) = structure_from_token(skipg[e]);
                fs::path loc = dbdir / p;
                wtm[skipg[e]] = word_trie_data(loc.native(),h.offset(skipg[e]),null_sorter());
                pos == wtm.find(skipg[e]);
            } 
        }
        if (pos != wtm.end()) {
            cluster_search_data csd(h,pos->second,&skipg,v,e);
            print_rules(skipg,csd,smap,target(e,skipg),h);
        }
    }
    BOOST_FOREACH(edge_t ee, out_edges(target(e,skipg),skipg)) {
        print_rules(skipg,wtm,smap,h,dbdir,v,ee);
    }
}

void printrules( gusc::lattice_ast const& lat
               , header& h
               , fs::path dbdir
               )
{
    graph_t skipg(skip_lattice(to_dag(lat,h.dict),h.dict));
    wordtrie_map wtm;
    mapsvtx smap;
    BOOST_FOREACH(vtx_t vtx, vertices(skipg)) {
        //std::cerr << "v:" << skipg[vtx] << '\n';
        smap.insert(make_pair(skipg[vtx],vtx));
    }
    wildcard_array wc(h.dict);
    indexed_token vartok = wc[0];
    BOOST_FOREACH(edge_t e, edges(skipg)) {
        vtx_t v;
        size_t s;
        if (sbmt::is_lexical(skipg[e])) {
            //std::cerr << "+++E:" << skipg[e] << "["<< skipg[source(e,skipg)] << ","<< skipg[target(e,skipg)] << "]\n";
            BOOST_FOREACH(boost::tie(s,v),smap) {
                //std::cerr << "  ===V:" << s << "\n";
                if (s <= skipg[source(e,skipg)]) {
                    wordtrie_map::iterator pos = wtm.find(skipg[e]);
                    if (pos == wtm.end()) {
                        if (cluster_exists(dbdir,h,skipg[e])) {
                            fs::path p; string ignore;
                            boost::tie(p,ignore) = structure_from_token(skipg[e]);
                            fs::path loc = dbdir / p;
                            wtm[skipg[e]] = word_trie_data(loc.native(),h.offset(skipg[e]),null_sorter());
                            pos = wtm.find(skipg[e]);
                        } 
                    }
                    if (pos != wtm.end()) {
                        cluster_search_data csd(h,pos->second,&skipg,v,e);
                        //std::cerr << "    print " << skipg[v] << " -- " << skipg[e] << ":\n";
                        print_rules(skipg,csd,smap,target(e,skipg),h);
                    }
                } 
            }
            wordtrie_map::iterator pos = wtm.find(skipg[e]);
            if (wtm.end() != pos) wtm.erase(pos);
        }
    }
    //nonlex
    fs::path p; string ignore;
    boost::tie(p,ignore) = structure_from_token(vartok);
    fs::path loc = dbdir / p;
    std::ifstream ifs(loc.native().c_str());
    biguint_t offset = h.offset(vartok);
    word_trie_data wtd(loc.native(),offset,null_sorter());
    vtx_t lv, rv;
    size_t ls, rs;
    BOOST_FOREACH(boost::tie(lv,ls),smap) {
        cluster_search_data csd(h,wtd,&skipg,lv);
        BOOST_FOREACH(boost::tie(rv,rs),smap) {
            if (ls < rs) {
                print_rules(csd,rv,h);
            }
        }
    }
    
}
/*
void printrules( gusc::lattice_ast const& lat
               , header& h
               , fs::path dbdir
               )
{
    graph_t skipg(skip_lattice(to_dag(lat,h.dict),h.dict));
    wordtrie_map wtm;
    mapsvtx smap;
    BOOST_FOREACH(vtx_t vtx, vertices(skipg)) smap.insert(make_pair(skipg[vtx],vtx));
    vtx_t lv;
    size_t ls;
    BOOST_FOREACH(boost::tie(ls,lv),smap) {
        BOOST_FOREACH(edge_t e, out_edges(lv,skipg)) {
            print_rules(skipg,wtm,smap,h,dbdir,lv,e);
        }
    }
}
*/
struct do_nothing_ {
    typedef void result_type;
    void operator()() {}
    template <class A> void operator()(A const& a) {}
    template <class A, class B> void operator()(A const& a, B const& b) {}
};

do_nothing_ do_nothing;


int main(int argc, char** argv)
{
    tbb::task_scheduler_init init;
    try {
        options opts;
        parse_options(argc,argv,opts);
        std::istream& fin = *opts.input;
        //make_fullmap(opts.h.dict);
        //lmstring_adaptor::unkset.clear();
        //BOOST_FOREACH(indexed_token ut, opts.h.dict.native_words()) {
        //    if ("@UNKNOWN@" == opts.h.dict.label(ut).substr(0,9)) {
        //        lmstring_adaptor::unkset.insert(ut);
        //    }
        //}
        //lmstring_adaptor::unk = opts.h.dict.native_word("@UNKNOWN@");
        cerr << sbmt::token_label(opts.h.dict);
        cout << sbmt::token_label(opts.h.dict);
        wildcard_array wc(opts.h.dict);
        gusc::lattice_ast lat;
        size_t id;
    
        sbmt::weight_vector weights;
        if (exists(opts.wfile)) {
            std::ifstream wfs(opts.wfile.c_str());
            read_weights(weights,wfs,opts.h.fdict);
        }
    
        // for globals, per-sentence pushed grammars.
        word_trie_stack wts;
        indexed_token vartok = wc[0];
        fs::path p; string ignore;
        boost::tie(p,ignore) = structure_from_token(vartok);
        fs::path loc = opts.dbdir / p;
        std::ifstream ifs(loc.native().c_str());
        biguint_t offset = opts.h.offset(vartok);
    
        wts.push_back(load_word_trie(ifs,offset,null_sorter())); 
    
        decode_sequence_reader reader;
        if (opts.pass == multipass::single or opts.pass == multipass::sink) {
            reader.set_weights_callback(boost::bind(set_weights,boost::ref(opts.h),boost::ref(weights),_1));
            reader.set_info_option_callback(set_info_option);
            reader.set_use_info_callback(boost::bind(set_infos,boost::ref(opts.infos),_1));
        } else {
            if (opts.tee_weights == true) {
                reader.set_weights_callback(boost::bind(tee_weights,boost::ref(opts.h),boost::ref(weights),_1));
            } else {
                reader.set_weights_callback(echo_weights);
            }
            reader.set_info_option_callback(tee_info_option);
            reader.set_use_info_callback(echo_infos);
        }
        if (opts.rule_dump) {
            reader.set_decode_callback(boost::bind(printrules, _1, boost::ref(opts.h),opts.dbdir));
            reader.set_push_grammar_callback(boost::bind(echo_grammar,_1,_2));
            reader.set_pop_grammar_callback(do_nothing);
        } else if (opts.pass == multipass::single or opts.pass == multipass::source) {
            reader.set_pop_grammar_callback(boost::bind(pop_grammar,boost::ref(wts),opts.pop_newline));
            reader.set_decode_callback(boost::bind( decode
                                                  , _1
                                                  , _2
                                                  , boost::ref(opts.h)
                                                  , boost::ref(weights)
                                                  , boost::ref(wts)
                                                  , boost::ref(opts) 
                                                  )
                                      );
            reader.set_push_inline_rules_callback(boost::bind( push_inline_grammar
                                                             , boost::ref(wts)
                                                             , _1
                                                             , boost::cref(weights)
                                                             , boost::ref(opts.h)
                                                             )
                                                 );
            reader.set_push_grammar_callback(boost::bind( push_grammar
                                                        , boost::ref(wts)
                                                        , _1
                                                        , _2
                                                        , boost::cref(weights)
                                                        , boost::ref(opts.h)
                                                        )
                                            );
        } else {
            reader.set_pop_grammar_callback(do_nothing);
            reader.set_push_grammar_callback(do_nothing);
            reader.set_decode_callback(boost::bind( store_decode_data
                                                  , _1
                                                  , _2
                                                  , boost::ref(lat)
                                                  , boost::ref(id)
                                                  )
                                      );
            reader.set_decode_forest_callback(boost::bind( decode_forest_main
                                                         , boost::cref(lat)
                                                         , boost::cref(id)
                                                         , _1
                                                         , boost::ref(opts.h)
                                                         , boost::ref(weights)
                                                         , boost::ref(wts)
                                                         , boost::ref(opts) 
                                                         )
                                             );
        }
        SBMT_INFO_STREAM(decoder_domain, "\ndecoder ready");
        reader.read(fin);
        return 0;
    } catch(std::exception& e) {
        SBMT_ERROR_STREAM(decoder_domain, "abnormal exit: " << e.what());
        return 1;
    } catch(...) {
        SBMT_VERBOSE_STREAM(decoder_domain,"unknown abnormal exit");
        return 1;
    }
    
}
