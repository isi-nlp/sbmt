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
# include <sbmt/io/logfile_registry.hpp>

# include <boost/program_options.hpp>
# include <boost/iostreams/device/file_descriptor.hpp>
# include <boost/iostreams/copy.hpp>
# include <boost/iostreams/filter/zlib.hpp>
# include <boost/iostreams/device/back_inserter.hpp>
# include <boost/shared_ptr.hpp>

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
# include <syntax_rule_util.hpp>
# include <sbmt/edge/ngram_constructor.hpp>

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

# include <decode_sequence_reader.hpp>

template<typename IT, typename CMP> 
IT partition(IT begin, IT end, IT pivot, CMP const& cmp)
{
    std::iter_swap(pivot, (end-1));
    typename std::iterator_traits<IT>::reference piv = *(end - 1);
    
    IT store=begin;
    for(IT it=begin; it!=end-1; ++it) {
        if (cmp(*it,piv)) {
            std::iter_swap(store, it);
            ++store;
        }
    }
    std::iter_swap(end-1, store);
    return store;
}

struct pivot_median {
template<typename IT,typename CMP>
IT operator()(IT begin, IT end, CMP const& cmp) const
{
    IT pivot(begin+(end-begin)/2);
    if(cmp(*begin,*pivot) and cmp(*(end-1),*begin)) pivot=begin;
    else if(cmp(*(end-1),*pivot) and cmp(*begin,*(end-1))) pivot=end-1;
    return pivot;
}
};

template<typename IT, typename PF, typename CMP> 
void quick_sort(IT begin, IT end, PF const& pf, CMP const& cmp)
{
    if((end-begin)>1) {
        IT pivot=pf(begin, end, cmp);

        pivot=partition(begin, end, pivot, cmp);

        quick_sort(begin, pivot, pf, cmp);
        quick_sort(pivot+1, end, pf, cmp);
    }
}

template<typename IT, typename CMP>
void quick_sort(IT begin, IT end, CMP const& cmp)
{
    quick_sort(begin, end, pivot_median(), cmp);
}

using namespace xrsdb;
using namespace sbmt;
using namespace boost::filesystem;
using namespace boost;
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

typedef graph_traits<graph_t>::edge_iterator eitr_t;
typedef graph_traits<graph_t>::edge_descriptor edge_t;
typedef graph_traits<graph_t>::vertex_descriptor vtx_t;

struct xedge;
typedef gusc::shared_varray<xedge> xequiv_pre;
struct xedge_partial;

struct make_token {
    typedef lm_token<indexed_token> result_type;
    result_type operator()(fixed_rule::tree_node const& nd) const
    {
        if (nd.indexed()) return result_type(rule->rhs2var.find(nd.index())->second);
        else return result_type(nd.get_token());
    }
    rule_application const* rule;
    make_token(rule_application const* rule = 0) : rule(rule) {}
};

struct grammar_facade;

template <class Type> 
struct rule_property_op {};

template <>
struct rule_property_op<rule_length::distribution_t> {
    typedef rule_length::distribution_t const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->rldist;
    }
};

template <>
struct rule_property_op<xrsdb::fixed_rldist_varray> {
    typedef xrsdb::fixed_rldist_varray const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->vldist;
    }
};

template <>
struct rule_property_op<xrsdb::fixed_bool_varray> {
    typedef xrsdb::fixed_bool_varray const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->cross;
    }
};

struct grammar_facade {
    typedef rule_application const* rule_type;
    typedef fixed_rule syntax_rule_type;
    
    fixed_rule const& get_syntax(rule_type r) const
    {
        return r->rule;
    }
    
    bool rule_has_property(rule_type r, size_t id) const 
    {
        if (id == lm_string_id) return true;
        else if (id == taglm_string_id) return true;
        else if (id == cross_id) return not r->cross.empty();
        else if (id == vldist_id) return not r->vldist.empty();
        else if (id == rldist_id) return r->rldist.mean() >= 0;
        else throw std::runtime_error("not-a-property-id");
    }
    
    template <class T> 
    typename rule_property_op<T>::result_type 
    rule_property(rule_type r, size_t id, T* pt = 0) const
    {
        return rule_property_op<T>()(r,id,this);
    }
    
    
    sbmt::in_memory_dictionary& dict() const 
    { 
        return h->dict; 
    }
    
    sbmt::feature_dictionary& feature_names() const 
    { 
        return h->fdict; 
    }
    
    static void* id(rule_type r) { return (void*)(r); }
    
    template <class S>
    rule_type insert_terminal_rule( indexed_token
                                  , S const& svec
                                  )
    {
        return 0;
    }
    
    template <class S, class T>
    rule_type insert_terminal_rule( indexed_token
                                  , S const& svec
                                  , T const& text_feats
                                  )
    {
        return 0;
    }
    
    sbmt::weight_vector const& get_weights() const 
    { 
        return *w; 
    }
    
    sbmt::weight_vector& get_weights()
    { 
        return *w; 
    }
    
    size_t get_syntax_id(rule_type r) const 
    { 
        return r->rule.id(); 
    }
    
    sbmt::indexed_token rule_lhs(rule_type r) const 
    { 
        return r->rule.lhs_root()->get_token(); 
    }
    
    size_t rule_rhs_size(rule_type r) const 
    {
        return r->rule.rhs_size(); 
    }
    
    sbmt::indexed_token rule_rhs(rule_type r, size_t idx) const 
    { 
        return (r->rule.rhs_begin() + idx)->get_token();
    }
    
    header* h;
    sbmt::weight_vector* w;
    grammar_facade(header* h, sbmt::weight_vector* w) : h(h),w(w) {}
};

typedef std::map<boost::tuple<sbmt::indexed_token,bool>,sbmt::indexed_token> tokmap_t;

struct lmstring_adaptor {
    static std::set<indexed_token> unkset;
    struct leaf {
        bool operator()(fixed_rule::tree_node const& nd) const
        {
            return nd.is_leaf() and unkset.find(nd.get_token()) == unkset.end();
            
        }
    };
    

    lmstring_adaptor(rule_application const* rule, tokmap_t const* tmap = 0) : rule(rule),tmap(tmap) {}
    rule_application const* rule;
    tokmap_t const* tmap;
    typedef boost::transform_iterator<
              make_token 
            , boost::filter_iterator<
                leaf
              , fixed_rule::lhs_preorder_iterator
              >
            > const_iterator_impl;
    typedef boost::any_iterator<lm_token<indexed_token> const,boost::forward_traversal_tag,lm_token<indexed_token> const> const_iterator;
    typedef const_iterator iterator;
    size_t size() const 
    {  
        size_t sz = 0;
        const_iterator b = begin(), e = end();
        for(; b != e; ++b) ++sz;
        return sz;
    }
    const_iterator begin() const
    {
       if (tmap) {
            return const_iterator(weak_syntax_iterator( rule->rule.lhs_begin()
                                                      , rule->rule.lhs_end()
                                                      , tmap
                                                      , rule));
       } else if (rule->lmstring.size() == 1 and rule->lmstring.begin()->is_index() and rule->lmstring.begin()->get_index() == 1)
            return const_iterator(boost::make_transform_iterator(
                     boost::make_filter_iterator(leaf(),rule->rule.lhs_begin(),rule->rule.lhs_end())
                   , make_token(rule)
                   ));
        else return const_iterator(rule->lmstring.begin());
    }
    const_iterator end() const
    {
        if (tmap) {
            return const_iterator(weak_syntax_iterator( rule->rule.lhs_end()
                                                      , rule->rule.lhs_end()
                                                      , tmap
                                                      , rule ));
        } else if (rule->lmstring.size() == 1 and rule->lmstring.begin()->is_index() and rule->lmstring.begin()->get_index() == 1)
            return const_iterator(boost::make_transform_iterator(
                     boost::make_filter_iterator(leaf(),rule->rule.lhs_end(),rule->rule.lhs_end())
                   , make_token(rule)
                   ));
        else return const_iterator(rule->lmstring.end());
    }
    bool is_identity() const 
    {
        const_iterator b = begin(), e = end();
        if (b == e) return false;
        if (b->is_token()) return false;
        ++b;
        if (b != e) return false;
        return true;
    }
};

std::ostream& operator << (std::ostream& out, lmstring_adaptor const& lmstr)
{
    bool first = true;
    BOOST_FOREACH(lm_token<indexed_token> lmtok, lmstr) {
        if (not first) out << ' ';
	out << lmtok;
	first = false;
    }
    return out;
}

std::set<indexed_token> lmstring_adaptor::unkset;
std::map<boost::tuple<sbmt::indexed_token,bool>,sbmt::indexed_token> tokmap;

template <>
struct rule_property_op<lmstring_adaptor> {
    typedef lmstring_adaptor result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        if (id == lm_string_id) return lmstring_adaptor(r);
        else {
	    //std::cerr << r->rule << std::endl;
	    //std::cerr << lmstring_adaptor(r,&tokmap);
	    //std::cerr << "\n\n";
	    return lmstring_adaptor(r,&tokmap);
        }
    }
};


typedef any_type_info<grammar_facade> any_xinfo;
typedef gusc::shared_varray<any_xinfo> info_vec;

void read_weights(weight_vector& wv, std::istream& in, feature_dictionary& fdict)
{
    std::string line;
    while (getline(in,line)) {
        weight_vector _wv;
        read(_wv,line,fdict);
        wv += _wv;
    }
    std::cerr << "weights: " << print(wv,fdict) << '\n';
}



struct weak_syntax_constructor : ngram_constructor {
    weak_syntax_constructor() : ngram_constructor("taglm_string", "taglm-") {}
    template <class Grammar>
    sbmt::any_type_info_factory<Grammar>
    construct( Grammar& gram
             , sbmt::lattice_tree const& lat
             , sbmt::property_map_type const& pm
             )
    {
        sbmt::any_type_info_factory<Grammar> 
            fact = ((sbmt::ngram_constructor*)(this))->construct(gram,lat,pm);
        indexed_token_factory& d = gram.dict();
        tokmap[boost::make_tuple(d.tag("NP"),true)] = d.native_word("[NP]");
        tokmap[boost::make_tuple(d.tag("NP"),false)] = d.native_word("[/NP]");
        tokmap[boost::make_tuple(d.tag("NP-C"),true)] = d.native_word("[NP-C]");
        tokmap[boost::make_tuple(d.tag("NP-C"),false)] = d.native_word("[/NP-C]");
        tokmap[boost::make_tuple(d.tag("S"),true)] = d.native_word("[S]");
        tokmap[boost::make_tuple(d.tag("S"),false)] = d.native_word("[/S]");
        return fact;
    }
    
    /*
    sbmt::options_map get_options()
    {
        sbmt::options_map newopts("weak syntax lm options");
        sbmt::options_map opts = ((sbmt::ngram_constructor*)(this))->get_options();
        std::string oname;
        sbmt::described_option oval;
        BOOST_FOREACH(boost::tie(oname,oval), opts) {
            newopts.insert(std::make_pair("taglm-" + oname, oval));
        }
        return newopts;
    }
    */
    
};



template <class C, class S>
std::basic_ostream<C,S>& print(std::basic_ostream<C,S>& os, grammar_facade::rule_type r, grammar_facade const& g)
{
    print(os,r->rule,g.dict());
    return os;
}

typedef boost::function<
          void( rule_application_array_adapter::iterator
              , rule_application_array_adapter::iterator
              )
        > rule_sort_f;

struct hash_offset {
    size_t operator()(ip::offset_ptr<char> k) const { return size_t(k.get()); }
};

struct signature_trie_data;

typedef std::tr1::unordered_map<
          ip::offset_ptr<char>
        , signature_trie_data
        , hash_offset> signature_cache;

struct word_trie_data_impl {
    word_trie const* get();
    word_trie_data_impl();
    word_trie_data_impl(boost::shared_array<char> data, size_t sz,rule_sort_f sorter);
    word_trie_data_impl(std::string fname, uint64_t offset, rule_sort_f sorter);
    ~word_trie_data_impl();
    boost::shared_array<char> data;
    word_trie const* ptr;
    boost::shared_ptr<signature_cache> cache;
    rule_sort_f sorter;
    std::string fname;
    uint64_t foffset;
    bool reloadable;
    void clear_cache(rule_sort_f s);// { cache->clear(); sorter = s; }
    void drop();
    void load();
    boost::shared_mutex mtx;
};

struct word_trie_data {
    word_trie const* get() const { return impl ? impl->get() : 0; }
    word_trie const* operator->() const { return get(); }
    word_trie const& operator*() const { return *get(); }
    signature_cache* cache() const { return impl->cache.get(); }
    rule_sort_f sorter() const { return impl->sorter; }
    void clear_cache(rule_sort_f s) { impl->clear_cache(s); }
    void drop() { if (impl) impl->drop(); }
    word_trie_data(boost::shared_array<char> data, size_t sz,rule_sort_f sorter)
    : impl(new word_trie_data_impl(data,sz,sorter)) {}
    word_trie_data(std::string name, uint64_t offset, rule_sort_f sorter)
    : impl(new word_trie_data_impl(name,offset,sorter)) {}
    word_trie_data() {}
    boost::shared_mutex& mutex() { return impl->mtx; }
private:
    boost::shared_ptr<word_trie_data_impl> impl;
};

boost::tuple<boost::shared_array<char>,size_t>
load_word_trie(std::ifstream& ifs, uint64_t offset)
{
    ifs.seekg(offset);
    uint64_t sz;
    boost::iostreams::read(ifs,(char*)(&sz),sizeof(uint64_t));
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
    if (cache->size()) throw std::runtime_error("cache non-empty while word_trie reloading");
    size_t sz;
    std::ifstream ifs(fname.c_str());
    boost::tie(data,sz) = load_word_trie(ifs,foffset);
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

void word_trie_data_impl::drop()
{
    if (not reloadable) throw std::runtime_error("cannot drop non-reloadable word_trie");
    data.reset(0);
    ptr = 0;
}

word_trie_data_impl::word_trie_data_impl() {}

word_trie_data_impl::word_trie_data_impl(boost::shared_array<char> data, size_t sz,rule_sort_f sorter)
: data(data)
, ptr(external_buffer_type(ip::open_only,data.get(),sz).find<word_trie>("root").first)
, cache(new signature_cache())
, sorter(sorter)
, reloadable(false)
{}

word_trie_data_impl::word_trie_data_impl(std::string name, uint64_t offset, rule_sort_f sorter)
: ptr(0)
, cache(new signature_cache())
, sorter(sorter)
, fname(name)
, foffset(offset)
, reloadable(true)
{load();}

word_trie_data_impl::~word_trie_data_impl() {}

void word_trie_data_impl::clear_cache(rule_sort_f s) { cache->clear(); sorter = s; }

word_trie_data
load_word_trie( std::ifstream& ifs, uint64_t offset, rule_sort_f sorter )
{
    ifs.seekg(offset);
    uint64_t sz;
    boost::iostreams::read(ifs,(char*)(&sz),sizeof(uint64_t));
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
    , search(new cluster_search(h,wtd.get(),g,b,e)) {}
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

struct xequiv;

struct xedge_partial {
    rule_application const* rule;
    gusc::shared_varray<xequiv> children;
    double cost;
    xedge_partial(xedge const& xe);
    xedge_partial();
};

struct xedge {
    rule_application const* rule;
    gusc::shared_varray<xequiv> children;
    gusc::shared_varray<any_xinfo> infos;
    double cost;
    double heur;
    template <class Range, class InfoRange>
    xedge( rule_application const* rule
         , Range const& rng
         , InfoRange const& inforange
         , double cost
         , double heur);
    xedge() : rule(0), cost(0), heur(0) {}
};

struct xequiv {
    struct iterator 
    : boost::iterator_facade<iterator,xedge const,boost::random_access_traversal_tag> {
        xequiv const* equiv;
        mutable boost::optional<xedge> val;
        gusc::shared_varray<xedge_partial>::const_iterator itr;
        xedge const& dereference() const;
        void advance(std::ptrdiff_t n);
        void increment();
        void decrement();
        bool equal(iterator const& other) const;
        iterator(xequiv const* equiv, gusc::shared_varray<xedge_partial>::const_iterator itr);
    };
    typedef iterator const_iterator;
    typedef xedge value_type;
    typedef xedge const& reference;
    typedef reference const_reference;
    gusc::shared_varray<xedge_partial> edges;
    gusc::shared_varray<any_xinfo> infos;
    double heur;
    template <class Range>
    explicit xequiv(Range const& rng);
    xequiv();
    iterator begin() const;
    iterator end() const;
    size_t  size() const;
    bool empty() const;
};

void const* id(xequiv const& xeq) { return xeq.edges.get(); }

xedge_partial::xedge_partial(xedge const& xe)
: rule(xe.rule)
, children(xe.children)
, cost(xe.cost)
{}

xedge_partial::xedge_partial() : rule(0), cost(0) {}

xequiv::xequiv() : heur(0) {}
template <class Range>
xequiv::xequiv(Range const& rng)
: edges(rng.size())
, infos(rng[0].infos)
, heur(rng[0].heur)
{
    for (size_t x = 0; x != edges.size(); ++x) edges[x] = xedge_partial(rng[x]);
}

xedge const& xequiv::iterator::dereference() const
{
    if (not val) val = xedge(itr->rule,itr->children,equiv->infos,itr->cost,equiv->heur);
    return *val;
}
        
void xequiv::iterator::advance(std::ptrdiff_t n)
{
    itr += n;
    if (n != 0) val = boost::none;
}
        
void xequiv::iterator::increment() { return advance(1); }

void xequiv::iterator::decrement() { return advance(-1); }
        
bool xequiv::iterator::equal(iterator const& other) const
{
    return other.itr == itr;
}

xequiv::iterator::iterator( xequiv const* equiv
                          , gusc::shared_varray<xedge_partial>::const_iterator itr )
: equiv(equiv)
, itr(itr) {}

xequiv::iterator xequiv::begin() const 
{ 
    return iterator(this,edges.begin()); 
}

xequiv::iterator xequiv::end() const 
{ 
    return iterator(this,edges.end()); 
}

size_t xequiv::size() const 
{ 
    return edges.size(); 
}

bool xequiv::empty() const 
{ 
    return edges.empty(); 
}


typedef gusc::shared_varray<xequiv> xcell;

typedef gusc::shared_varray<xcell> xspan;

using boost::detail::atomic_count;
typedef boost::shared_ptr<atomic_count> shared_atomic_count;

typedef std::map<span_t, boost::shared_ptr<boost::detail::atomic_count> > xchart_holds;
typedef std::map<span_t, xspan> xchart;

indexed_token root(xedge const& xe) { return xe.rule->rule.lhs_root()->get_token(); }
indexed_token root(xequiv const& xeq) { return xeq.begin()->rule->rule.lhs_root()->get_token(); }
indexed_token root(xcell const& xc) { return root(xc[0]); }

double cost(xedge const& xe) { return xe.cost; }
double cost(xequiv const& xeq) { return xeq.begin()->cost; }
double cost(xcell const& xc) { return cost(xc[0]); }
double cost(xspan const& x) {return x.size() > 0 ? cost(x[0]) : 0.; }

double heur(xedge const& xe) { return xe.heur; }
double heur(xequiv const& xeq) { return xeq.heur; }
double heur(xcell const& xc) { return heur(xc[0]); }
double heur(xspan const& x) {return x.size() > 0 ? heur(x[0]) : 0.; }

info_vec const& infos(xedge const& xe) { return xe.infos; }
info_vec const& infos(xequiv const& xeq) { return xeq.infos; }

template <class Range, class InfoRange>
xedge::xedge( rule_application const* rule
            , Range const& rng
            , InfoRange const& inforng
            , double cost
            , double heur )
: rule(rule)
, children(rng)
, infos(inforng)
, cost(cost)
, heur(heur) 
{
    #ifndef NDEBUG
    size_t x = 0;
    BOOST_FOREACH(fixed_rule::rule_node const& nd, this->rule->rule.rhs()) {
        if (nd.indexed()) {
            assert(nd.get_token() == root(children[x]));
            ++x;
        }
    }
    assert(x == children.size());
    #endif
}

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

bool operator == (xedge const& xe1, xedge const& xe2)
{
    if (root(xe1) != root(xe2)) return false;
    for (size_t x = 0; x != xe1.infos.size(); ++x) if (xe1.infos[x] != xe2.infos[x]) return false;
    return true;
}

size_t hash_value(xedge const& xe) {
    size_t ret = boost::hash<sbmt::indexed_token>()(root(xe));
    boost::hash_combine(ret,boost::hash_range(xe.infos.begin(),xe.infos.end()));
    return ret;
}

bool operator != (xedge const& xe1, xedge const& xe2)
{
    return !(xe1 == xe2);
}
/*
bool operator == (xequiv const& xeq1, xequiv const& xeq2)
{
    return xeq1[0] == xeq2[0];
}

bool operator != (xequiv const& xeq1, xequiv const& xeq2)
{
    return !(xeq1 == xeq2);
} */

xedge tm_make_xedge(std::vector<xequiv> const& v, rule_application const& r)
{
    double c = 0.;
    std::vector<any_xinfo> b;
    
    BOOST_FOREACH(xequiv const& xeq, v) c += cost(xeq);
    c += r.cost;
    return xedge(&r,v,b,c,0);
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
        , double
        > equiv_in;

typedef 
    boost::tuple<
      cell_tree*
    , signature_trie::state
    , double
    > cell_in;


struct greater_cost {
    template <class T>
    bool op(T const& g1, T const& g2) const
    {
        double s1 = 0; double s2 = 0;
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
};

struct lower_cost {
    template <class T>
    bool operator()(T const& t1, T const& t2) const
    {
        return greater_cost()(t2,t1);
    }
};


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
    double c;
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
    return gusc::generate_union_heap( 
             gusc::make_peekable(
               gusc::generate_transform(
                 gen
               , make_transitioner(&strie,spn,&cell_pool)
               )
             )
           , greater_cost()
           , 10
           );
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
                cgen = gen_transitions(
                         chrt.find(span_t(x,x+k))->second
                       , strie
                       , gusc::generate_from_range(lzy)
                       , cell_pool
                       );
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
        return gusc::generate_finite_union(splits,greater_cost());
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
        gen = gusc::generate_single_value(
                cell_in(
                  0
                , strie.start()
                , 0.0
                )
              );
    span_t spn; size_t n;
    //std::cerr << "cells_in";
    BOOST_FOREACH(boost::tie(spn,n),v) {
        //std::cerr << " x "<< spn << ":" << n;
        gen = gen_cells_in(spn,n,strie,chrt,gen,cell_pool);
    }
    //std::cerr << "\n";
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

typedef gusc::any_generator<xedge,gusc::iterator_tag>
        xedge_generator;
typedef boost::function<xedge(std::vector<xequiv> const&,rule_application const&)> 
        xedge_construct_f;
typedef boost::function<sbmt::weight_vector(xedge)> xedge_components_f;

struct apply_rule_equiv {
    apply_rule_equiv(xedge_construct_f func) : func(func) {}
    xedge_construct_f func;
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

xedge_generator
make_edges1( cell_in const& ge
           , signature_trie const& strie
           , xedge_construct_f func
           , boost::pool<>& equiv_pool
           )
{
    typedef
        gusc::any_generator<equiv_in,gusc::iterator_tag> 
        children_generator;
    typedef
        gusc::generator_from_range<xcell> xcell_generator;
        children_generator 
            gen = gusc::generate_single_value(
                    equiv_in(
                      0
                    , 0.0
                    )
                  );
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
    return generate_product_heap(
             apply_rule_equiv(func)
           , greater_cost()
           , lzygen
           , rules
           )
           ;
}

struct make_res {
    typedef signature_trie const* result_type;
    void sort_res (signature_trie* trie, signature_trie::state s, rule_sort_f sorter)
    {
        if (trie->value(s) != trie->nonvalue()) {
            rule_application_array_adapter rules(trie->value(s));
            sorter(rules.begin(),rules.end());
        }
        BOOST_FOREACH(signature_trie::state ss, trie->transitions(s)) {
            sort_res(trie,ss,sorter);
        }
    }
    result_type operator()(compressed_signature_trie const& sr, rule_sort_f sorter)
    {
        signature_cache* sc = wtd.cache();
        boost::shared_mutex& mtx = wtd.mutex();
        signature_cache::iterator pos;
        { 
            boost::shared_lock<boost::shared_mutex> readlock(mtx);
            pos = sc->find(sr.get<0>());
            if (pos != sc->end()) return 0;
        }
        { 
            boost::upgrade_lock<boost::shared_mutex> mightwritelock(mtx);
            pos = sc->find(sr.get<0>());
            if (pos != sc->end()) return 0;
            else {
                boost::upgrade_to_unique_lock<boost::shared_mutex> 
                    writelock(mightwritelock);
                signature_trie_data sigdata = load_signature_trie(sr);
                sort_res(sigdata.get(),sigdata.get()->start(),sorter);
                sc->insert(std::make_pair(sr.get<0>(),sigdata));
                return sigdata.get();
            }
        }
    }
    make_res(word_trie_data wtd) : wtd(wtd) {}
    word_trie_data wtd;
};

xedge_generator
make_edges( cluster_search_data& csd
          , cluster_search::single_result_type const& bt
          , xchart const& chrt
          , xedge_construct_f func
          , boost::pool<>& cell_pool
          , boost::pool<>& edge_pool
          )
{
    make_res mr(csd.wtd);
    cell_in_generator cgen = gen_cells_in(bt.get<0>(),*mr(bt.get<1>(),csd.sorter()),chrt,cell_pool);
    return
    gusc::generate_union_heap(
      gusc::make_peekable( 
        gusc::generate_transform(
          cgen
        , boost::bind(make_edges1,_1,boost::cref(*mr(bt.get<1>(),csd.sorter())),func,boost::ref(edge_pool))
        )
      )
    , greater_cost()
    , 10
    );
}

xedge_generator
make_edges( cluster_search_data& csd
          , cluster_search::result_type const& res 
          , xchart const& chrt
          , xedge_construct_f func
          , boost::pool<>& cell_pool
          , boost::pool<>& edge_pool )
{
    xedge_generator retval;
    std::vector<xedge_generator> vec(res.size());
    int x = 0;
    BOOST_FOREACH(cluster_search::single_result_type const& cb, res)
    {
        xedge_generator xeg = make_edges(csd,cb,chrt,func,cell_pool,edge_pool);
        vec[x] = xeg;
        ++x;
    }
    retval = gusc::generate_finite_union(vec, greater_cost());
    return retval;
}


xedge_generator
make_edges( std::vector<cluster_search_data>& vcs
          , vtx_t v
          , xchart const& chrt
          , xedge_construct_f func
          , boost::pool<>& cell_pool
          , boost::pool<>& edge_pool )
{
    std::vector<xedge_generator> vcsresults(vcs.size());
    size_t x = 0;
    BOOST_FOREACH(cluster_search_data& cs, vcs) {
        cluster_search::result_type crs((*cs)(v));
        xedge_generator xegf = make_edges(cs,crs,chrt,func,cell_pool,edge_pool);
        vcsresults[x] = xegf;
        ++x;
    }
    return gusc::generate_finite_union(vcsresults,greater_cost());
}

void print_rules( std::vector<cluster_search_data>& vcs, vtx_t v )
{
    BOOST_FOREACH(cluster_search_data& cs, vcs) {
        cluster_search::result_type crs((*cs)(v));
        make_res mr(cs.wtd);
        xedge_generator xegf = make_edges(cs,crs,chrt,func,cell_pool,edge_pool);
        BOOST_FOREACH(cluster_search::single_result_type const& cb, crs) {
            signature_trie const* sigtrie = mr(cb.get<1>(),null_sorter());
            
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

typedef map< tuple<vtx_t,vtx_t>, vector<edge_t> > addums_t;
typedef map<sbmt::indexed_token, word_trie_data> wordtrie_map;

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
                            ad[make_tuple(vx,vy)].push_back(e);
                        }
                    } else if (pvy == vx or ad.find(make_tuple(vx,pvy)) != ad.end()) {
                        ad[make_tuple(vx,vy)].push_back(e);
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

namespace sbmt {
template <>
struct ngram_rule_data<grammar_facade> {
    typedef lmstring_adaptor type;
    typedef type return_type;
    static return_type value( grammar_facade const& grammar
                            , grammar_facade::rule_type r
                            , size_t lmstrid )
    {
        return grammar.rule_property<type>(r,lmstrid);
    }
};
}

namespace rule_length {
template <>
struct var_distribution<grammar_facade> {
    typedef fixed_rldist_varray type;
};
}

/*
template <>
struct ngram_rule_data<grammar_facade const> {
    typedef lmstring_adaptor type;
    typedef type return_type;
    static return_type value( grammar_facade const& grammar
                            , grammar_facade::rule_type r
                            , size_t lmstrid )
    {
        return grammar.rule_property<type>(r,lmstrid);
    }
};*/

/*
struct xedge {
    rule_application const* rule;
    gusc::varray<any_xinfo> infos;
    gusc::varray<xequiv> children;
    double cost;
    double heur;
    template <class Range>
    xedge(rule_application const& rule, Range const& rng, double cost, double heur);
    xedge() : rule(0), cost(0), heur(0) {}
};
*/
struct xtree;
typedef boost::shared_ptr<xtree> xtree_ptr;
typedef std::vector<xtree_ptr> xtree_children;
struct xtree {
    double cost;
    xedge root;
    typedef gusc::varray<xtree_ptr> children_type;
    //typedef std::tr1::unordered_map<int,xtree_ptr> children_type;
    children_type children;
    template <class RNG>
    xtree(xedge const& root, RNG c)
    : cost(root.cost)
    , root(root)
    , children(boost::begin(c),boost::end(c))
    {/*
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
        double s1;
        BOOST_FOREACH(xtree_ptr const& t1, c1) { s1 += t1->cost; }
        double s2;
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
            ++x;
        }
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

xtree_generator 
xtrees_from_xedge(xedge const& hyp, ornode_map& omap)
{
    if (hyp.children.empty()) {
        return gusc::make_single_value_generator(
                 xtree_ptr(new xtree(hyp,xtree_children()))
               );
    } else {
        return gusc::generator_as_iterator(
                 gusc::generate_transform(
                   generate_xtree_children(hyp,omap)
                 , make_xtree(hyp)
                 )
               );
    }
}

struct make_xtree_generator_from_xedge {
    make_xtree_generator_from_xedge(ornode_map& omap) 
    : omap(&omap) {}
    ornode_map* omap;
    typedef xtree_generator result_type;
    xtree_generator operator()(xedge const& hyp) const
    {
        return xtrees_from_xedge(hyp,*omap);
    }
};

xtree_list 
xtrees_from_xequiv(xequiv const& forest, ornode_map& omap)
{
    ornode_map::iterator pos = omap.find(id(forest));
    if (pos != omap.end()) return pos->second;
    else {
        xtree_list lst(
         xtree_generator(
          gusc::generate_union_heap(
            gusc::generator_as_iterator(
              gusc::generate_transform(
                gusc::generate_from_range(forest)
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
    return gusc::generator_as_iterator(toplevel_generator(forest));
}

feature_vector features(rule_data const& rd, feature_dictionary& fdict);

void accum(xtree_ptr const& t,weight_vector& f, xedge_components_f func)
{
    f += func(t->root);
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

std::string 
hyptree(xtree_ptr const& t,fixed_rule::tree_node const& n, in_memory_dictionary& dict)
{
    using namespace sbmt;
    std::stringstream sstr;
    sstr << sbmt::token_label(dict);
    if (n.indexed()) {
        xtree_ptr ct = t->children[t->root.rule->rhs2var.find(n.index())->second];
        assert(n.get_token() == ct->root.rule->rule.lhs_root()->get_token());
        sstr << hyptree(ct,*(ct->root.rule->rule.lhs_root()),dict);
    } else if (n.lexical()) {
        sstr << n.get_token();
    } else {
        sstr << '(' << n.get_token();
        BOOST_FOREACH(fixed_rule::tree_node const& c, n.children()) {
            sstr << ' ' << hyptree(t,c,dict);
        }
        sstr << ')';
    }
    return sstr.str();
}

std::string 
hyptree(xtree_ptr const& t, in_memory_dictionary& dict)
{
    return hyptree(t,*(t->root.rule->rule.lhs_root()),dict);
}

std::string nbest_features(feature_vector const& f, feature_dictionary& dict);

typedef std::map< std::string
        , sbmt::any_type_info_factory_constructor<grammar_facade> 
        > info_registry_type;
info_registry_type info_registry;
property_map_type pmap;

struct initer {
initer()
{
    any_type_info_factory_constructor<grammar_facade> ng = ngram_constructor();
    info_registry.insert(std::make_pair("ngram",ng));
    
    any_type_info_factory_constructor<grammar_facade> sg = weak_syntax_constructor();
    info_registry.insert(std::make_pair("taglm",sg));
    
    any_type_info_factory_constructor<grammar_facade> dt = distortion_constructor<fixed_bool_varray>();
    info_registry.insert(std::make_pair("distortion",dt));
    
    any_type_info_factory_constructor<grammar_facade> rl = rule_length::rlinfo_factory_constructor();
    info_registry.insert(std::make_pair("rule-length",rl));
    
    pmap["lm_string"] = lm_string_id;
    pmap["taglm_string"] = taglm_string_id;
    pmap["cross"] = cross_id;
    pmap["rldist"] = rldist_id;
    pmap["vldist"] = vldist_id;
}
};

initer in;

boost::program_options::options_description get_info_options()
{
    boost::program_options::options_description opts(":: info-type options :");

    info_registry_type::iterator itr = info_registry.begin(), 
                                 end = info_registry.end();

    for (; itr != end; ++itr) {
        namespace po = boost::program_options;
        sbmt::options_map omap = itr->second.get_options();
        po::options_description o(omap.title() + " [" + itr->first + "]");
        for (options_map::iterator i = omap.begin(); i != omap.end(); ++i) {
            o.add_options()
              ( i->first.c_str()
              , po::value<std::string>()->notifier(i->second.opt)
              , i->second.description.c_str()
              )
              ;
        }
        opts.add(o);
    }

    return opts;
}

BOOST_ENUM_VALUES(
    output_format_type
  , const char*
  , (nbest)("nbest")
    (forest)("forest")
);

struct options {
    bool keep_align;
    size_t num_threads;
    size_t histogram;
    size_t nbests;
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
    size_t limit_syntax_length;
    std::istream* input;
    options() 
    : keep_align(false)
    , num_threads(numproc_online())
    , histogram(1000)
    , nbests(10)
    , poplimit_multiplier(1.0)
    , softlimit_multiplier(10.0)
    , max_equivalents(4)
    , merge_window(10)
    , prior_floor_prob(1e-7)
    , prior_bonus_count(100)
    , weight_tag_prior(1.0)
    , output_format(output_format_type::forest)
    , limit_syntax_length(40)
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
        ( "keep-align"
        , value<string>()
        , "ignored"
        )
        ( "nbests"
        , value(&opts.nbests)->default_value(opts.nbests)
        , "unique string nbests"
        )
        ( "per-estring-nbests"
        , value<string>()
        , "ignored"
        )
        ;
    
    desc.add(get_info_options());
    desc.add(sbmt::io::logfile_registry::instance().options());
    po::positional_options_description posdesc;
    posdesc.add("dbdir",1).add("instructions",1);
    po::basic_command_line_parser<char> cmd(argc,argv);
    variables_map vm;
    po::store(cmd.options(desc).positional(posdesc).run(),vm);
    notify(vm);
    
    if (not multi_thread) opts.num_threads = 1;
    if (vm.count("help")) {
        cerr << desc << endl;
        exit(0);
    }
    if (not vm.count("dbdir")) {
        cerr << desc << endl;
        cerr << "must provide xrs rule db" << endl;
        exit(1);
    }
    load_header(opts.h,opts.dbdir);
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
    return opts;
}

typedef any_type_info_factory<grammar_facade> any_xinfo_factory;

std::vector<std::string> info_names(std::string info_names_str)
{
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(",");
    tokenizer tokens(info_names_str, sep);
    std::vector<std::string> retval;
    std::copy(tokens.begin(),tokens.end(),std::back_inserter(retval));
    return retval;
}

gusc::shared_varray<any_xinfo_factory> 
get_info_factories( std::string info_names_str
                  , grammar_facade& gram
                  , lattice_tree& ltree 
                  , property_map_type& pmap )
{
    std::vector<std::string> keys = info_names(info_names_str);
    std::vector<any_xinfo_factory> facts;
    BOOST_FOREACH(std::string key, keys) {
        facts.push_back(info_registry.find(key)->second.construct(gram,ltree,pmap));
    }
    return gusc::shared_varray<any_xinfo_factory>(facts.begin(),facts.end());
}

struct null_sorter {
    void operator()( rule_application_array_adapter::iterator beg
                   , rule_application_array_adapter::iterator end ) 
    { return; }
};

struct info_rule_sorter {
    bool operator()(rule_application const& r1, rule_application const& r2) const
    {
        return (r1.cost + r1.heur) < (r2.cost + r2.heur);
    }
    void operator()( rule_application_array_adapter::iterator beg
                   , rule_application_array_adapter::iterator end )
    {
        BOOST_FOREACH(rule_application& ra, std::make_pair(beg,end)) {
            ra.cost = dot(*weights,ra.costs);
            ra.heur = (*priormap)[gram->rule_lhs(&ra)].neglog10();
            BOOST_FOREACH(any_xinfo_factory& fact, factories) {
                ra.heur += fact.rule_heuristic(*gram,&ra).neglog10();
            }
        }
        quick_sort(beg,end,*this);
        //BOOST_FOREACH(rule_application& ra, std::make_pair(beg,end)) {
        //    std::cerr << '(' << ra.cost <<','<< ra.heur <<')' << ':' << ra.cost + ra.heur << ' ';
        //}
        //std::cerr << '\n';
    }
    info_rule_sorter( grammar_facade const* gram
                    , gusc::shared_varray<any_xinfo_factory> factories
                    , weight_vector const* weights
                    , tag_prior* priormap )
    : gram(gram)
    , factories(factories)
    , weights(weights)
    , priormap(priormap) {}
    
    grammar_facade const* gram;
    gusc::shared_varray<any_xinfo_factory> factories;
    weight_vector const* weights;
    tag_prior* priormap;
};

struct info_make_xedge {
    xedge operator()(std::vector<xequiv> const& v, rule_application const& r)
    {
        xedge ret(tm_make_xedge(v,r));
        gusc::shared_varray<any_xinfo> ri(factories.size());
        gusc::varray<constituent<any_xinfo> > cia(v.size());
        any_xinfo info; sbmt::score_t scr, heur;
        for (size_t x = 0; x != factories.size(); ++x) {
            for (size_t y = 0; y != v.size(); ++y) {
                cia[y] = make_constituent(&(v[y].infos[x]),root(v[y]));
            }
            boost::tie(info,scr,heur) = (factories[x].create_info(*gram,&r,spn,cia))();
            ret.cost += scr.neglog10();
            if (gram->rule_lhs(&r).type() != sbmt::top_token) ret.heur += heur.neglog10();
            ri[x] = info;
        }
        if (gram->rule_lhs(&r).type() != sbmt::top_token){
            ret.heur += (*priormap)[gram->rule_lhs(&r)].neglog10();
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
                cia[y] = make_constituent(&(xe.children[y].infos[x]),root(xe.children[y]));
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
                   , tag_prior* priormap )
    : gram(gram)
    , spn(spn)
    , factories(factories)
    , priormap(priormap) {}
    
    grammar_facade* gram;
    span_t spn;
    gusc::shared_varray<any_xinfo_factory> factories;
    tag_prior* priormap;
};

typedef vector<word_trie_data> word_trie_stack;

void set_weights(header& h, weight_vector& weights, fat_weight_vector const& fatweights)
{
    weights = index(fatweights,h.fdict);
}

void pop_grammar( word_trie_stack& wts )
{
    wts.pop_back();
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
    std::cerr << "grammar " << fname << " pushed\n";
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
    BOOST_FOREACH(sbmt::weight_vector::value_type v, fv) {
        if (not first) {
            out << ',';
        } else {
            first = false;
        }
        out << h.fdict.get_token(v.first) << ':' << v.second;
    }
    out << '>';
    return out;
}

std::string derivation(xtree_ptr tree, xedge_components_f func, header& h)
{
    std::stringstream deriv;
    deriv << token_label(h.dict);
    lmstring_adaptor lsa(tree->root.rule);
    if (tree->children.empty()) {
        deriv << tree->root.rule->rule.id();
        deriv << '[';
        copy(lsa.begin(),lsa.end(),std::ostream_iterator< lm_token<indexed_token> >(deriv,","));
        deriv << ']';
        print_features(deriv,tree->root,func,h);
    } else {
        deriv << '(' << tree->root.rule->rule.id();
        deriv << '[';
        copy(lsa.begin(),lsa.end(),std::ostream_iterator< lm_token<indexed_token> >(deriv,","));
        deriv << ']';
        print_features(deriv,tree->root,func,h);
        BOOST_FOREACH(xtree::children_type::const_reference c, tree->children) {
            deriv << ' ' << derivation(c,func,h);
        }
        deriv << ')';
    } 
    return deriv.str();
}

set<int64_t>& used_rules(xtree_ptr tree, set<int64_t>& used)
{
    used.insert(tree->root.rule->rule.id());
    BOOST_FOREACH(xtree_ptr ctree, tree->children) used_rules(ctree,used);
    return used;
}

set<int64_t> used_rules(xtree_ptr tree)
{
    set<int64_t> used;
    used_rules(tree,used);
    return used;
}

typedef std::tr1::unordered_map<void const*,size_t> idmap;

void print_hyperedge( std::ostream& out
                    , xedge const& xe
                    , idmap& idm
                    , header& h
                    , xedge_components_f func );


void print_forest( std::ostream& out
                 , xequiv const& xeq
                 , idmap& idm
                 , header& h
                 , xedge_components_f func )
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
        BOOST_FOREACH(xedge const& alt, xeq) {
            if (xeq.size() > 1 or not_first) out << ' ';
            print_hyperedge(out,alt,idm,h,func);
            not_first = true;
        }
        if (xeq.size() > 1) out << " )";
    }
}


void print_hyperedge( std::ostream& out
                    , xedge const& xe
                    , idmap& idm
                    , header& h
                    , xedge_components_f func )
{
    out << '(';
    out << xe.rule->rule.id();
    print_features(out,xe,func,h);
    BOOST_FOREACH(fixed_rule::tree_node const& nd, xe.rule->rule.lhs()) {
        if (nd.is_leaf()) {
            if (nd.lexical()) {
                out << ' ' << '"' << gusc::escape_c(h.dict.label(nd.get_token())) << '"';
            } else {
                out << ' ';
                print_forest(out,xe.children[xe.rule->rhs2var.find(nd.index())->second],idm,h,func);
            }
        }
    }
    out << " )";
}

void print_forest(std::ostream& out, xequiv const& xeq, header& h,xedge_components_f func)
{
    idmap idm;
    print_forest(out,xeq,idm,h,func);
}

typedef std::priority_queue<xedge,std::vector<xedge>,lower_cost> xequiv_construct;
typedef std::tr1::unordered_map<xedge,xequiv_construct,boost::hash<xedge> > 
        xcell_construct;
typedef std::tr1::unordered_map<indexed_token,xcell_construct,boost::hash<indexed_token> > 
        xspan_construct;

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
    xchart chrt;
    xchart_holds chrthlds;
    mapsvtx::iterator end;
    mapsvtx::reverse_iterator rbegin;
    map< vtx_t, vector<cluster_search_data> > vcs;
    sbmt::thread_pool spanpool;
    boost::mutex mutex;
    boost::condition jobs_done;
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
    , factories(get_info_factories(opts.infos,gram,ltree,pmap))
    , spanpool(opts.num_threads)
    {
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
        for (right = smap.begin(); right != end; ++right) {
            bool initial = true;
            left = mapsvtx::reverse_iterator(right);
            for (;left != rbegin; ++left) {
                spn=span_t(skipg[left->second],skipg[right->second]);
                chrt[spn] = xspan();

                chrthlds.insert(std::make_pair(spn,shared_atomic_count(new atomic_count(initial ? 0 : 2))));
                initial = false;
            }
        }
        end = smap.end();
        
    }
};

template <class K, class V, class S, class CK>
V& get(std::map<K,V,S>& mp, CK const& k)
{
    return mp.find(k)->second;
}

void decode_span( size_t id
                , header& h
                , sbmt::weight_vector& weights
                , options& opts 
                , decode_data& dd
                , decode_data::mapsvtx::reverse_iterator left
                , decode_data::mapsvtx::iterator right
                , span_t total )
{
    try {
        span_t spn=span_t(left->first,right->first);
        if (long(*(dd.chrthlds.find(spn)->second)) != 0) {
            throw std::logic_error("calculating span with hold on it");
        }
        if (not (spn.left() != 0 and spn.right() - spn.left() > (int)opts.limit_syntax_length)) {
            addums_t::iterator apos = dd.ad.find(make_tuple(left->second,right->second));
            if (apos != dd.ad.end()) {
                BOOST_FOREACH(edge_t e, apos->second){
                    indexed_token lookup = dd.skipg[e];
                    if (cluster_exists(opts.dbdir,h,lookup)) {
                        get(dd.vcs,left->second).push_back(cluster_search_data(h,get(dd.wtm,lookup),&dd.skipg,left->second,e));
                    }
                }
            }

            xspan_construct scons;
            boost::pool<> equiv_pool(sizeof(equiv_tree));
            boost::pool<> cell_pool(sizeof(cell_tree));
            xedge_generator gxe = make_edges( get(dd.vcs,left->second)
                                            , right->second
                                            , dd.chrt
                                            , info_make_xedge(&dd.gram,spn,dd.factories,&opts.priormap) 
                                            , cell_pool
                                            , equiv_pool
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
                } else if (xec.size() < opts.max_equivalents) {
                    xec.push(xe);
                } else if (cost(xec.top()) > cost(xe)) {
                    xec.pop();
                    xec.push(xe);
                }
                ++pops;
            }
            equiv_pool.purge_memory();
            cell_pool.purge_memory();
    
            cerr << "% " << id <<"\t["<< left->first << ',' << right->first << "] of block " << total << ' ';
            cerr << "[";
            if (apos != dd.ad.end()) { BOOST_FOREACH(edge_t e, apos->second) { cerr << ' ' << dd.skipg[e]; } }
            cerr << " ] %%% (" << equivs << " equivalences)";
            xspan xs(scons.size());
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
                    ++x;
                }
                std::sort(xc.begin(),xc.end(),lower_cost());
                xs[y] = xc;
                ++y;
            }
            std::sort(xs.begin(),xs.end(),lower_cost());
            cerr << " (" << xs.size() <<  " cells) ("<< pops << " pops) (cost "<<cost(xs)<<")\n";
            get(dd.chrt,spn) = xs;
            if (spn.left() == 0) {
                BOOST_FOREACH(indexed_token wd, get(dd.freelists,spn.right())) {
                    cerr << "dropping cluster data for "<< wd << '\n';
                    wordtrie_map::iterator pos = dd.wtm.find(wd);
                    if (pos != dd.wtm.end()) pos->second.drop();
                }
            }
        }
        decode_data::mapsvtx::reverse_iterator rnext = left; ++rnext;
        if (rnext != dd.rbegin) {
            span_t spn1(rnext->first,right->first);
            size_t holds = --(*(dd.chrthlds.find(spn1)->second));
            cerr << "decrement " << spn1 << "(holds=" << holds <<")\n";
            if (holds == 0) {
                dd.spanpool.add( boost::bind( decode_span
                                            , id
                                            , boost::ref(h)
                                            , boost::ref(weights)
                                            , boost::ref(opts)
                                            , boost::ref(dd)
                                            , rnext
                                            , right
                                            , total
                                            )
                               );
            }
        }
        decode_data::mapsvtx::iterator next = right; ++next;
        if (next != dd.end) {
            span_t spn2(left->first,next->first);
            size_t holds = --(*(dd.chrthlds.find(spn2)->second));
            cerr << "decrement " << spn2 << "(holds=" << holds <<")\n";
            if (holds == 0) {
                dd.spanpool.add( boost::bind( decode_span
                                            , id
                                            , boost::ref(h)
                                            , boost::ref(weights)
                                            , boost::ref(opts)
                                            , boost::ref(dd)
                                            , left
                                            , next
                                            , total
                                            )
                               );
            }
        }
        if (spn == total) {
            boost::mutex::scoped_lock lk(dd.mutex);
            dd.jobs_done.notify_all();
        }
    } catch(...) {
        boost::mutex::scoped_lock lk(dd.mutex);
        dd.jobs_done.notify_all();
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
        size_t rtidx = dd.skipg[right->second];
        BOOST_FOREACH(indexed_token lookup, get(dd.loadlists,rtidx)) {
            if (get(dd.wtm,lookup).get() == 0 and cluster_exists(opts.dbdir,h,lookup)) {
                fs::path p; string ignore;
                boost::tie(p,ignore) = structure_from_token(lookup);
                fs::path loc = opts.dbdir / p;
                std::cerr << "loading cluster data for "<< lookup << '\n';
                get(dd.wtm,lookup) = word_trie_data(loc.native(),h.offset(lookup),info_rule_sorter(&dd.gram,dd.factories,&weights,&opts.priormap));
            }
        }
        if (left != dd.rbegin) {
            dd.spanpool.add( boost::bind( decode_span
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
        dd.jobs_done.wait(lk);
    }
    dd.spanpool.wait();

}

void decode( gusc::lattice_ast const& lat
           , size_t id
           , header& h
           , sbmt::weight_vector& weights
           , word_trie_stack& wts
           , options& opts )
{
    lmstring_adaptor::unkset.clear();
    BOOST_FOREACH(indexed_token ut, h.dict.native_words()) {
      if ("@UNKNOWN@" == h.dict.label(ut).substr(0,9)) {
	lmstring_adaptor::unkset.insert(ut);
      }
    }
    decode_data dd(lat,id,h,weights,wts,opts);
    decode_data::mapsvtx::iterator right;
    decode_data::mapsvtx::reverse_iterator left;
    std::cerr << "weights: " << print(weights,h.fdict) << '\n';
    for (right = dd.smap.begin();right != dd.end; ) {
        decode_data::mapsvtx::iterator beg = right;
        for (size_t blk = 0; blk != opts.limit_syntax_length and right != dd.end; ++blk, ++right) {}
        decode_data::mapsvtx::iterator end = right; --end;
        span_t total(dd.smap.begin()->first,end->first);
        decode_range(id,h,weights,opts,dd,beg,right,total);
    }
}

int main(int argc, char** argv)
{
    try {
        options opts;
        parse_options(argc,argv,opts);
        std::istream& fin = *opts.input;
    
        //lmstring_adaptor::unk = opts.h.dict.native_word("@UNKNOWN@");
        cerr << sbmt::token_label(opts.h.dict);
        cout << sbmt::token_label(opts.h.dict);
        wildcard_array wc(opts.h.dict);
        gusc::lattice_ast lat;
    
        sbmt::weight_vector weights;
        // for globals, per-sentence pushed grammars.
        word_trie_stack wts;
        indexed_token vartok = wc[0];
        fs::path p; string ignore;
        boost::tie(p,ignore) = structure_from_token(vartok);
        fs::path loc = opts.dbdir / p;
        std::ifstream ifs(loc.native().c_str());
        uint64_t offset = opts.h.offset(vartok);
    
        wts.push_back(load_word_trie(ifs,offset,null_sorter())); 
    
        decode_sequence_reader reader;
        reader.set_pop_grammar_callback(boost::bind(pop_grammar,boost::ref(wts)));
        reader.set_decode_callback(boost::bind( decode
                                              , _1
                                              , _2
                                              , boost::ref(opts.h)
                                              , boost::ref(weights)
                                              , boost::ref(wts)
                                              , boost::ref(opts) 
                                              ));
        reader.set_push_grammar_callback(boost::bind( push_grammar
                                                    , boost::ref(wts)
                                                    , _1
                                                    , _2
                                                    , boost::cref(weights)
                                                    , boost::ref(opts.h)
                                                    ));

        reader.read(fin);
        return 0;
    } catch(std::exception& e) {
        std::cerr << "abnormal exit: " << e.what() << "\n";
        return 1;
    } catch(...) {
        std::cerr << "unknown abnormal exit\n";
        return 1;
    }
    
}
