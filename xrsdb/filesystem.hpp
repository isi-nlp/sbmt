# if ! defined(XRSDB__FILESYSTEM_HPP)
# define       XRSDB__FILESYSTEM_HPP

//# define BOOST_ENABLE_ASSERT_HANDLER 1
# define XRSDB_SENTIDS 1
// boost/serialization/map.hpp includes
// boost/serialization/utility.hpp,
// which needs but does not include boost/mpl/and.hpp
# include <boost/mpl/and.hpp>
# include <boost/serialization/map.hpp>
# include <boost/tuple/tuple.hpp>
# include <sbmt/token.hpp>
# include <boost/filesystem/path.hpp>
# include <sbmt/feature/feature_vector.hpp>
# include <map>
# include <vector>
# include <iostream>
# include <sstream>
# include <iterator>
# include <gusc/filesystem_io.hpp>
# include <boost/serialization/version.hpp>
# include <boost/serialization/set.hpp>
# include <boost/serialization/map.hpp>
# include <boost/archive/binary_iarchive.hpp>
# include <boost/preprocessor/slot/counter.hpp>
# include <boost/interprocess/file_mapping.hpp>
# include <boost/interprocess/shared_memory_object.hpp>
# include <boost/interprocess/mapped_region.hpp>
# include <boost/interprocess/offset_ptr.hpp>
# include <boost/interprocess/managed_mapped_file.hpp>
# include <boost/interprocess/managed_external_buffer.hpp>
# include <boost/interprocess/allocators/allocator.hpp>
# include <boost/interprocess/containers/vector.hpp>
# include <boost/interprocess/mem_algo/simple_seq_fit.hpp>
# include <boost/interprocess/indexes/flat_map_index.hpp>
# include <sbmt/grammar/syntax_rule.hpp>
# include <gusc/lifetime.hpp>
# include <gusc/trie/basic_trie.hpp>
# include <gusc/trie/fixed_trie.hpp>
# include <boost/iterator/transform_iterator.hpp>
# include <boost/iterator/filter_iterator.hpp>
# include <distortion_model.hpp>
# include <rule_length_info.hpp>
# include <source_syntax/source_syntax.hpp>
# include <forest_reader.hpp>

/*
namespace boost
{
    inline void assertion_failed(char const * expr, char const * function, char const * file, long line)
    {
        throw std::runtime_error(expr + std::string(" ") + function + std::string(" ") +  file);
    }
}
*/


namespace xrsdb {


////////////////////////////////////////////////////////////////////////////////

struct header {
    sbmt::indexed_token_factory dict;
    std::map<sbmt::indexed_token,size_t> freq;
    std::map<sbmt::indexed_token,uint64_t> offsets;
    sbmt::feature_dictionary fdict;
    std::set<sbmt::indexed_token> knownset;

    template <class Archive>
    void serialize(Archive& ar, unsigned int version)
    {
        if (typeid(ar) == typeid(boost::archive::binary_iarchive)) {
            ar & BOOST_SERIALIZATION_NVP(freq);
            ar & BOOST_SERIALIZATION_NVP(dict);
        } else {
            ar & BOOST_SERIALIZATION_NVP(freq);
            ar & BOOST_SERIALIZATION_NVP(offsets);
            ar & BOOST_SERIALIZATION_NVP(dict);
        } if (version > 0) {
            ar & BOOST_SERIALIZATION_NVP(fdict);
        } if (version > 1) {
            ar & BOOST_SERIALIZATION_NVP(knownset);
        }
    }

    sbmt::indexed_token wildcard() { return dict.virtual_tag("0"); }
    void add_frequency(sbmt::fat_token word, size_t f = 1);
    void add_frequency(sbmt::indexed_token word, size_t f = 1);
    void add_frequencies(header const& other);
    void add_offset(sbmt::indexed_token word, uint64_t off);
    bool old_version() const { return offsets.empty(); }

    header& operator +=(header const& other)
    {
        add_frequencies(other);
        return *this;
    }

    size_t frequency(sbmt::fat_token const& f) const;
    uint64_t offset(sbmt::indexed_token const& f) const;
    header();
    ~header();
};
}

BOOST_CLASS_VERSION(xrsdb::header,2);

namespace xrsdb {

////////////////////////////////////////////////////////////////////////////////

namespace ip = boost::interprocess;

// typedef ip::managed_mapped_file mapped_file;
// typedef ip::basic_managed_mapped_file <
//           char
//         , ip::simple_seq_fit<ip::mutex_family, ip::offset_ptr<void> >
//         , ip::flat_map_index
//         >  mapped_file;
// typedef mapped_file::segment_manager segment_manager;
  typedef ip::basic_managed_external_buffer<char,ip::rbtree_best_fit<ip::null_mutex_family,ip::offset_ptr<void>, sizeof(char*)>,ip::iset_index>
        external_buffer_type;
typedef external_buffer_type::segment_manager segment_manager;

typedef ip::flat_map<
          sbmt::indexed_token
        , boost::tuple<boost::uint32_t,boost::uint32_t>
        , gusc::less
        , ip::allocator< 
            std::pair<
              sbmt::indexed_token
            , boost::tuple<boost::uint32_t,boost::uint32_t>
            >
          , segment_manager
          >
        > head_map;

typedef ip::flat_map<
          sbmt::indexed_token
        , boost::uint32_t
        , gusc::less
        , ip::allocator<std::pair<sbmt::indexed_token,boost::uint32_t>, segment_manager>
        > variable_head_map_member;

typedef gusc::varray< variable_head_map_member
                    , ip::allocator<variable_head_map_member,segment_manager> 
                    > variable_head_map;

typedef 
    sbmt::syntax_rule<
      sbmt::indexed_token
    , ip::allocator<sbmt::indexed_token,segment_manager>
    , gusc::create_plain
    > 
    fixed_rule;
typedef ip::flat_map<
          boost::uint32_t
        , float
        , gusc::less
        , ip::allocator<std::pair<boost::uint32_t,float>, segment_manager>
        > fixed_feature_vector
        ;
typedef ip::flat_map<
          boost::uint8_t
        , boost::uint8_t
        , gusc::less
        , ip::allocator< std::pair<boost::uint8_t,boost::uint8_t>, segment_manager>
        > rhs2var_map
        ;
typedef gusc::varray<boost::uint32_t, ip::allocator<boost::uint32_t,segment_manager> >
        sentid_varray;
typedef sbmt::lm_string< sbmt::indexed_token
                       , ip::allocator<sbmt::indexed_token,segment_manager>
                       > fixed_lm_string;
typedef gusc::varray< bool, ip::allocator<bool,segment_manager> > fixed_bool_varray;
typedef gusc::varray< rule_length::distribution_t
                    , ip::allocator<rule_length::distribution_t,segment_manager>
                    > fixed_rldist_varray;
typedef gusc::varray< sbmt::indexed_token
                    , ip::allocator<sbmt::indexed_token,segment_manager>
                    > fixed_token_varray;
typedef gusc::varray< boost::uint8_t
                    , ip::allocator<boost::uint8_t,segment_manager> 
                    > fixed_byte_varray;
template <class F, class G>
float dot(F const& f, G const& g)
{
    if (f.size() > g.size()) return dot(g,f);
    float retval = 0.;
    BOOST_FOREACH(typename F::value_type p, f) {
        typename G::const_iterator pos = g.find(p.first);
        if (pos != g.end()) {
            retval += p.second * pos->second;
        }
    }
    return retval;
}

typedef gusc::varray< boost::tuple<boost::uint8_t,boost::uint8_t>
                    , ip::allocator< boost::tuple<boost::uint8_t,boost::uint8_t>, segment_manager> 
                    > target_source_align_varray;


std::vector<sbmt::indexed_lm_token> 
make_leaflm_string( rhs2var_map const& mp
                  , sbmt::indexed_token_factory const& dict
                  , std::string const& s );

struct rule_application : boost::noncopyable {
private:
    struct isnum {
        bool operator()(feature const& f) const { return f.number; }
    };
    
    struct isvar {
        bool operator()(fixed_rule::rule_node const& rn) const
        {
            return rn.indexed();
        }
    };

    struct mapitem {
        fixed_rule::rule_node const* rule;
        mutable unsigned x;
        typedef std::pair<boost::uint8_t,boost::uint8_t> result_type;
        result_type operator()(fixed_rule::rule_node const& rn) const
        {
            return result_type(&rn - rule, x++);
        }
        mapitem(fixed_rule const* rule) : rule(&(*(rule->rhs_begin()))), x(0) {}
    };
    
    struct tform {
        typedef std::pair<boost::uint32_t,float> result_type;
        tform(sbmt::feature_dictionary* fdict)
        : fdict(fdict) {}
        result_type operator()(feature const& f) const 
        {
            return result_type(fdict->get_index(f.key),f.num_value);
        }
        sbmt::feature_dictionary* fdict;
    };
public:
    void swap(rule_application& other);
    fixed_rule rule;
    fixed_rule::lhs_preorder_iterator hwd;
    fixed_feature_vector costs;
    float cost;
    float heur;
    rhs2var_map rhs2var;
    fixed_bool_varray cross;
    rule_length::distribution_t rldist;
    fixed_rldist_varray vldist;
    fixed_lm_string lmstring;
    # ifdef XRSDB_LEAFLM
    fixed_lm_string leaflmstring;
    # endif // XRSDB_LEAFLM
    sbmt::indexed_token froot;
    fixed_token_varray fvars;
    # ifdef XRSDB_SENTIDS
    sentid_varray sentids;
    # endif // XRSDB_SENTIDS
    target_source_align_varray tgt_src_aligns;
    # ifdef XRSDB_HEADRULE
    head_map hwdm;
    head_map htgm;
    variable_head_map vhwdm;
    variable_head_map vhtgm;
    # endif // XRSDB_HEADRULE
    fixed_byte_varray headmarker;
    template <class A, class W>
    rule_application(rule_data const& rd, header& h, A& alloc, W const& w);
    void print(std::ostream& out, header& h) const;
};

typedef std::map<
          boost::int64_t
        , ip::offset_ptr<rule_application>
        , gusc::less
        > rule_application_map
        ;
        
void create_word_map( rule_application_map& mp
                    , boost::shared_array<char>& array
                    , std::istream& in
                    , exmp::forest const& frst
                    , sbmt::weight_vector const& weights
                    , header& h );

typedef boost::tuple<ip::offset_ptr<rule_application>,size_t> rule_application_array;
typedef boost::tuple<ip::offset_ptr<char>,size_t> compressed_signature_trie;

struct rule_application_array_adapter {
    typedef rule_application value_type;
    typedef ip::offset_ptr<rule_application> const_iterator;
    typedef ip::offset_ptr<rule_application> iterator;
    ip::offset_ptr<rule_application> ptr;
    size_t sz;
    const_iterator begin() const { return ptr; }
    const_iterator end() const 
    { 
        return ptr + sz; 
    }
    rule_application const& operator[](size_t x) const { return ptr[x]; }
    size_t size() const { return sz; }
    rule_application_array_adapter(rule_application_array const& ra)
    : ptr(ra.get<0>())
    , sz(ra.get<1>())
    {
        while (sz != 0 and ptr[sz-1].heur == std::numeric_limits<float>::infinity()) {
            --sz;
        }
    }
};

typedef ip::allocator<rule_application,segment_manager> rule_application_allocator;
typedef ip::allocator<char,segment_manager> char_allocator;

typedef gusc::basic_trie<
          sbmt::indexed_token
        , rule_application_array
        > subtrie_construct_t;

typedef gusc::fixed_trie<
          sbmt::indexed_token
        , rule_application_array
        , char_allocator
        , gusc::create_plain
        > signature_trie;

std::ostream& operator << (std::ostream& os, signature_trie const& st);
        
typedef gusc::basic_trie<
          boost::tuple<short,sbmt::indexed_token>
        , compressed_signature_trie
        > trie_construct_t;

typedef ip::allocator<signature_trie,segment_manager> signature_allocator;

typedef ip::offset_ptr<signature_trie> signature_trie_ptr;
typedef gusc::fixed_trie< 
          boost::tuple<short,sbmt::indexed_token>
        , compressed_signature_trie
        , char_allocator
        , gusc::create_plain
        > word_trie;

////////////////////////////////////////////////////////////////////////////////

class word_cluster;
class word_cluster_construct;

////////////////////////////////////////////////////////////////////////////////

boost::tuple<
  boost::filesystem::path
, std::string
> structure_from_token(sbmt::indexed_token tok);

bool cluster_exists( boost::filesystem::path const& p
                   , header const& h
                   , sbmt::indexed_token word );

////////////////////////////////////////////////////////////////////////////////

word_cluster
load_word_cluster( boost::filesystem::path const& p
                 , header const& h 
                 , sbmt::indexed_token tok);
                 
word_cluster
load_word_cluster( boost::filesystem::path const& p
                 , uint64_t offset );

//word_cluster
//load_word_cluster(boost::filesystem::path const& p);

////////////////////////////////////////////////////////////////////////////////

void
save_word_cluster( word_cluster const& wc
                 , std::ostream& p );

////////////////////////////////////////////////////////////////////////////////

void load_header( header& h
                , boost::filesystem::path const& p );

////////////////////////////////////////////////////////////////////////////////

void save_header( header const& h
                , boost::filesystem::path const& p );

////////////////////////////////////////////////////////////////////////////////

void create_empty_database( header const& h
                          , boost::filesystem::path const& rootdb );

////////////////////////////////////////////////////////////////////////////////

rule_application_array make_entry( external_buffer_type& subtrie_buffer
                                 , std::vector<rule_data>& rules
                                 , header& h
                                 , sbmt::weight_vector const& weights );
                                 
compressed_signature_trie 
make_sig_entry( external_buffer_type& subtrie_buffer
              , external_buffer_type& wordtrie_buffer
              , subtrie_construct_t& sdbc );
              
boost::tuple<boost::shared_array<char>,size_t>
create_word_trie(std::istream& in, sbmt::weight_vector const& weights, header& h);

struct weak_syntax_iterator 
: boost::iterator_facade<
    weak_syntax_iterator
  , sbmt::lm_token<sbmt::indexed_token> const
  , boost::forward_traversal_tag
  , sbmt::lm_token<sbmt::indexed_token> const >
{
    typedef sbmt::lm_token<sbmt::indexed_token> result_type;
    typedef 
        std::map<boost::tuple<sbmt::indexed_token,bool>,sbmt::indexed_token> 
        token_map;
    typedef 
        std::vector<boost::tuple<sbmt::indexed_token,fixed_rule::lhs_preorder_iterator> >
        stack_t;
        
    weak_syntax_iterator( fixed_rule::lhs_preorder_iterator itr
                        , fixed_rule::lhs_preorder_iterator end
                        , token_map const* inc
                        , rule_application const* rule );
    
    stack_t endpts;
    fixed_rule::lhs_preorder_iterator endpt(fixed_rule::tree_node const& nd);
    
    result_type dereference() const;
    
    void increment();
    
    bool equal(weak_syntax_iterator const& other) const;
    
    fixed_rule::lhs_preorder_iterator itr; 
    fixed_rule::lhs_preorder_iterator end;
    token_map const* inc;
    rule_application const* rule;
    
    void advance();
};

inline void swap(rule_application& ra1, rule_application& ra2) { ra1.swap(ra2); }

} // xrsdb

# endif //     XRSDB__FILESYSTEM_HPP
