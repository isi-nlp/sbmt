# include <sbmt/hash/oa_hashtable.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/token.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <boost/tuple/tuple_comparison.hpp>
# include <iostream>
# include <sstream>
# include <string>
# include <sbmt/token/in_memory_token_storage.hpp>
# include <sbmt/edge/any_info.hpp>
# include <sbmt/logging.hpp>
# include <sbmt/io/log_auto_report.hpp>
# include <vector>
# include <boost/optional.hpp>
# include <boost/variant.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <gusc/sequence/any_sequence.hpp>
# include <gusc/generator/lazy_sequence.hpp>
# include <gusc/trie/basic_trie.hpp>
# include <graehl/shared/fileargs.hpp>
# include <tbb/task_scheduler_init.h>
# include <tbb/pipeline.h>
# include <boost/thread/mutex.hpp>
# include <boost/tokenizer.hpp>
# include <boost/token_iterator.hpp>
# include <set>

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(rh_log,"rule-head",sbmt::root_domain);

template <boost::uint8_t Max, class Type>
class small_array {
    boost::uint8_t sz;
    Type array[Max];
public:
    typedef Type* iterator;
    typedef Type const* const_iterator;
    typedef Type& reference;
    typedef Type const& const_reference;
    typedef std::ptrdiff_t difference_type;
    typedef std::size_t size_type;
    template <class I>
    small_array(I const& b, I const& e)
      : sz(std::distance(b,e)) { std::copy(b,e,&array[0]); }
    
    small_array() : sz(0) {}
    
    void push_back(Type const& t) { array[sz++] = t; }
    
    iterator begin() { return array; }
    iterator end() { return array + sz; }
    
    const_iterator begin() const { return array; }
    const_iterator end() const { return array + sz; }
    
    const_reference operator[](size_type x) const { return array[x]; }
    reference operator[](size_type x) { return array[x]; }
}; 

using namespace boost;
using sbmt::oa_hash_map;         
using std::istream;

typedef boost::int32_t ruleid_t;
typedef boost::int32_t word_t;
typedef boost::uint16_t variable_t;
typedef boost::uint32_t count_t;

typedef std::pair<ruleid_t,variable_t> key_type;
typedef boost::tuple<word_t,count_t> dist_entry_t;
typedef gusc::varray<dist_entry_t> dist_t;
struct dist {
    count_t c;
    count_t n;
    //sbmt::score_t backoff_wt;
    //sbmt::score_t first_order_wt;
    dist_t distribution;
    dist() {}
    void swap(dist& o)
    {
        std::swap(backoff_wt,o.backoff_wt);
        std::swap(first_order_wt,o.first_order_wt);
        distribution.swap(o.distribution);
    }
    
    template <class Range>
    dist(count_t c, count_t n, Range const& rng) 
      : c(c)
      , n(n)
      , distribution(boost::begin(rng), boost::end(rng))
      {}
};

typedef sbmt::oa_hash_map<word_t,dist_t> backoff_table_t;
typedef sbmt::oa_hash_map<key_type,dist> table_t;
typedef sbmt::in_memory_token_storage dict_t;
typedef sbmt::oa_hash_map<sbmt::indexed_token,word_t> token_map_t;
typedef std::set<word_t> word_set_t;

static token_map_t token_map;

static dict_t table_dict;

word_t get_index(std::string const& str)
{
    return table_dict.get_index(str);
//    std::pair<bool,dict_t::state> 
//        p = table_dict.insert(str.begin(),str.end(),table_dict_max);
//    if (p.first) ++table_dict_max;
//    return table_dict.value(p.second);
}


bool has_token(std::string const& str)
{
    return table_dict.has_token(str);
}

struct search_op {
    bool operator()(dist_entry_t const& a, dist_entry_t const& b) const
    {
        return boost::get<0>(a) < boost::get<0>(b);
    }
};

namespace {
search_op search_op_;
}

struct lex_table_input : tbb::filter {
    
    std::istream& infile;
    std::vector<std::string> line;
    size_t line_number;
    void* operator()(void*)
    {
        line_number = line_number % 10000;
        size_t ln = 0;
        for (; ln != 100; ++ln) {
            bool good = getline(infile,line[line_number + ln]);
            if (not good) {
                line[line_number + ln] = "";
                break;
            }
        }
        if (ln == 0) return 0;
        else {
            int retval = line_number;
            line_number += 100;
            return (&line[0]) + retval;
        }
    }
    lex_table_input(std::istream& in) 
      : tbb::filter(tbb::filter::serial)
      , infile(in)
      , line(10000)
      , line_number(0) {}
};

struct lex_table_procline : tbb::filter {
    void* operator()(void* lineptr)
    {
        using namespace std;
        string word;
        ruleid_t id;
        int n;
        int c;
        double f;
        typedef std::vector< std::pair<key_type,dist> > return_type;
        std::auto_ptr<return_type> tbl(new return_type());
        for (size_t offset = 0; offset != 100; ++offset) {
            variable_t idx = 0;
            if (*(static_cast<std::string*>(lineptr) + offset) == "") break;
            stringstream sstr(*(static_cast<std::string*>(lineptr) + offset));
            sstr >> id >> n;
            
            while (sstr) {
                sstr >> c;
                std::vector<dist_entry_t> entries;
                for (int x = 0; x != c; ++x) {
                    sstr >> word;
                    sstr >> f;
                    if (has_token(word)) {
                        word_t wid = get_index(word);
                        if (wordset.find(wid) != wordset.end()) {
                            entries.push_back(dist_entry_t(wid,count_t(wt)));
                        }
                    }
                }
                std::sort(entries.begin(),entries.end(), search_op_);
                tbl->push_back(make_pair(key_type(id,idx),dist(c,n,entries)));
                ++idx;
                sstr >> std::ws;
            }
        }
        return tbl.release();
    }
    
    lex_table_procline(word_set_t const& wordset) 
      : tbb::filter(tbb::filter::parallel)
      , wordset(wordset) {}
      
    word_set_t const& wordset;
};

struct lex_table_assemble : tbb::filter {
    table_t& tbl;
    lex_table_assemble(table_t& tbl) 
      : tbb::filter(tbb::filter::serial)
      , tbl(tbl) {}
    void* operator()(void* data) 
    {
        typedef std::pair<key_type,dist> datum_type;
        typedef std::vector<datum_type> data_type;
        std::auto_ptr<data_type> d(static_cast<data_type*>(data));
        BOOST_FOREACH(datum_type& dd, *d) {
            table_t::iterator pos = tbl.insert(datum_type(dd.first,dist())).first;
            const_cast<dist&>(pos->second).swap(dd.second);
        }
        return 0;
    }
};

table_t& read_lex_table_mt(istream& in, table_t& tbl, word_set_t const& wordset)
{
    sbmt::io::logging_stream& log_out = sbmt::io::registry_log(rh_log);
    sbmt::io::log_time_space_report 
        report(log_out,sbmt::io::lvl_terse,"read headword table: ");
    tbb::task_scheduler_init init;
    tbb::pipeline pipeline;
    lex_table_input input(in);
    lex_table_procline procline(wordset);
    lex_table_assemble assemble(tbl);
    pipeline.add_filter(input);
    pipeline.add_filter(procline);
    pipeline.add_filter(assemble);
    pipeline.run(100);
    return tbl;
}

backoff_table_t& 
read_backoff_table(istream& in, backoff_table_t& tbl)
{
    sbmt::io::logging_stream& log_out = sbmt::io::registry_log(rh_log);
    sbmt::io::log_time_space_report 
        report(log_out,sbmt::io::lvl_terse,"read headword backoff table: ");
    using namespace std;
    string wd;
    int n, c;
    double f;
    while (in) {
        in >> wd >> n >> c;
        word_t nonterminal = get_index(wd);
        std::vector<dist_entry_t> entries;
        for (int x = 0; x != c; ++x) {
            in >> wd;
            in >> f;
            double wt = double(f)/double(n);
            entries.push_back(dist_entry_t(get_index(wd),wt));
            
        }
        std::sort(entries.begin(),entries.end(),search_op_);
        tbl.insert(make_pair(nonterminal,dist_t(entries)));
        in >> std::ws;
    }
    return tbl;
}

namespace {
sbmt::score_t min_scr_(1e-200);
}

boost::tuple<bool,sbmt::score_t,bool> 
score_pos( int ruleid
         , int postag
         , int pos
         , int word
         , table_t const& tbl
         , table_t const& nltbl
         , backoff_table_t const& bkftbl
         )
{
    //std::cerr << "score_pos(" << ruleid 
    //          << ',' << table_dict.get_token(postag)
    //          << ',' << pos
    //          << ',' << (word >= 0 ? table_dict.get_token(word) : "NULL") << ")=";
    sbmt::score_t p = min_scr_; 
    sbmt::score_t b = min_scr_; 
    bool found = false;
    sbmt::score_t wp, wb;
    
    table_t::const_iterator ppos = tbl.find(std::make_pair(ruleid,pos));
    if (ppos == tbl.end()) { 
        ppos = nltbl.find(std::make_pair(ruleid,pos));
        if (ppos == nltbl.end()) {
            //std::cerr << "rule " << ruleid << " not found\n";
            return boost::make_tuple(false,1.0,true);
        }
    }
    dist_t::const_iterator 
        dpos = std::lower_bound( ppos->second.distribution.begin()
                               , ppos->second.distribution.end()
                               , dist_entry_t(word,sbmt::score_t())
                               , search_op_
                               );
    wb = ppos->second.backoff_wt;
    wp = ppos->second.first_order_wt;

    if (dpos != ppos->second.distribution.end() and dpos->get<0>() == word) {
        found = true;
        p = dpos->get<1>();
    }
    backoff_table_t::const_iterator bpos = bkftbl.find(postag);
    if (bpos != bkftbl.end()) {
        dpos = std::lower_bound( bpos->second.begin()
                               , bpos->second.end()
                               , dist_entry_t(word,sbmt::score_t())
                               , search_op_
                               );
        if (dpos != bpos->second.end() and dpos->get<0>() == word) { 
            found = true;
            b = dpos->get<1>();
        }
    }
    sbmt::score_t prob = 1.0;
    if (found) prob = wp*p + wb*b;
    //std::cerr << '(' << prob << ',' << found << ')' << '\n';
    return boost::make_tuple(true,prob,found);
}


typedef small_array<2,bool> hwdh;

struct read_hwdh {
    typedef hwdh result_type;
    result_type operator()( sbmt::in_memory_dictionary& dict
                          , std::string const& str ) const
    {
        std::stringstream sstr(str);
        result_type v;
        std::copy( std::istream_iterator<bool>(sstr)
                 , std::istream_iterator<bool>()
                 , std::back_inserter(v)
                 )
                 ;
        return v;
    }                 
};

typedef boost::tuple<ruleid_t,variable_t> index_t;
typedef small_array<2,index_t> hwidx_alt;
typedef small_array<2,word_t> hwidx_nt;
struct hwidx {
   hwidx_nt hwnt;
    std::vector<hwidx_alt> hwidx_alt_vec;
};

struct read_hwidx {
    typedef hwidx result_type;
    result_type operator()( sbmt::in_memory_dictionary& dict
                          , std::string const& str ) const
    {
        result_type v;
        boost::char_separator<char> sep("\t");
        typedef 
            boost::token_iterator_generator<boost::char_separator<char> >::type 
            iterator_t;
        iterator_t itr = 
            boost::make_token_iterator<std::string>(str.begin(),str.end(),sep);
        iterator_t end = 
            boost::make_token_iterator<std::string>(str.end(),str.end(),sep);
        if (itr != end) {    
            std::stringstream sstr(*itr);
            std::vector<std::string> svec;
            std::istream_iterator<std::string> sitr(sstr);
            std::istream_iterator<std::string> send;
            for (; sitr != send; ++sitr) {
                v.hwnt.push_back(get_index(*sitr));
            }
            ++itr;
        }
        for (; itr != end; ++itr) {
            std::stringstream sstr(*itr);
            sstr << boost::tuples::set_delimiter(',');
            small_array<2,index_t> sa;
            std::copy( std::istream_iterator<index_t>(sstr)
                     , std::istream_iterator<index_t>()
                     , std::back_inserter(sa)
                     )
                     ;
            v.hwidx_alt_vec.push_back(sa);
        }
        return v;
    }
};

struct hwpos {
    explicit hwpos(std::string const& str)
    {
        if (str[0] == '"') {
            wd = get_index(str.substr(1,str.size() - 2));
        } else {
            wd = -(boost::lexical_cast<word_t>(str) + 2);
        }
    }
    hwpos() : wd(-1) {}
    bool lexical() const { return wd >= -1; }
    bool unknown() const { return wd == -1; }
    word_t index() const { return -(wd + 2); }
    word_t word() const { return wd; }
private:
    word_t wd;
};

bool operator==(hwpos const& a, hwpos const& b) { return a.word() == b.word(); }
bool operator!=(hwpos const& a, hwpos const& b) { return !(a == b); }
size_t hash_value(hwpos const& a) { return boost::hash<word_t>()(a.word()); }

struct read_hwpos {
    typedef hwpos result_type;
    result_type operator()( sbmt::in_memory_dictionary& dict
                          , std::string const& str ) const
    {
        return hwpos(str);
    }
};

struct named_table_t {
    table_t table;
    std::string name;
    //named_table_t() : table(100,0.75) {}
};

struct named_backoff_table_t {
    backoff_table_t table;
    std::string name;
    //named_backoff_table_t() : table(100,0.75) {}
};

std::ostream& operator << (std::ostream& os, named_backoff_table_t const& nt)
{
    return os << nt.name;
}

std::istream& operator >> (std::istream& is, named_backoff_table_t& nt)
{
    is >> nt.name;
    //std::cerr << "loading " << nt.name << std::endl;
    graehl::istream_arg infile;
    infile.set_none();
    infile.set(nt.name);
    nt.table.clear();
    read_backoff_table(infile.stream(),nt.table);
    //std::cerr << nt.name << " has " << nt.table.size() << " entries" << std::endl;
    return is;
}

hwpos get_hwpos( sbmt::grammar_in_mem const& g
               , sbmt::grammar_in_mem::rule_type r
               , int hwpos_id )
{
    if (not g.rule_has_property(r,hwpos_id)) return hwpos();
    return g.rule_property<hwpos>(r,hwpos_id);
}

struct rule_head_factory {
    typedef int info_type;
    typedef tuple<info_type,sbmt::score_t,sbmt::score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;
    typedef sbmt::grammar_in_mem grammar;
    
    bool scoreable_rule(grammar const& g, grammar::rule_type r) const
    {
        return true;
    }
    
    sbmt::score_t rule_heuristic(grammar const& g, grammar::rule_type r) const
    {
        return 1.0;
    }
    
    std::string hash_string(grammar const& g, info_type i) const
    {
        return boost::lexical_cast<std::string>(i);
    }
    
    template <class Range>
    info_type 
    create_info_(grammar const& g, grammar::rule_type r, Range children) const
    {
        int result = -1;
        hwdh const& hvec = g.rule_property<hwdh>(r,hwdh_id);
        int idx = 0;
        BOOST_FOREACH(typename boost::range_value<Range>::type const& rv, children) {
            int c = *rv.info();
            if (is_native_tag(rv.root())) { 
                if (hvec[idx]) result = c;
                ++idx;
            } else if (c >= 0) {
                result = c;
            }
        }
        return result;
    } 
    
    template <class Range>
    boost::tuple<bool,sbmt::score_t,int>
    create_scores_(hwidx_nt const& ntvec, hwidx_alt const& hvec, Range children, bool print=false) const
    {
        int num_unk = 0;
        sbmt::score_t scr = 1.0;
        int idx = 0;
        bool ascore = true;
        typename boost::range_iterator<Range>::type citr, cend;
        citr = boost::begin(children);
        cend = boost::end(children);
        BOOST_FOREACH(word_t nt, ntvec) {
            if (nt != get_index("--")) { 
                int id, pos;
                bool found;
                bool ascr;
                sbmt::score_t s;
                boost::tie(id,pos) = hvec[idx];
                boost::tie(ascr,s,found) = score_pos( id
                                                    , nt
                                                    , pos
                                                    , *citr->info()
                                                    , *table
                                                    , *nonlex_table
                                                    , *backoff_table
                                                    )
                                                    ;
                if (not found) ++num_unk;
                else scr *= s;
                ascore = ascore && ascr;
                ++idx;
                if (false)        
                std::cerr << boost::format("\t\tp( %s | %s, %s ) = (%s,%s,%s)\n") 
                         % table_dict.get_token(*citr->info()) 
                         % id 
                         % pos 
                         % ascr 
                         % s 
                         % found;
            }
            ++citr;
        }
       // std::cerr << boost::format("\tscore (%s,%s,%s)\n") % ascore % scr % num_unk;
        return boost::make_tuple(ascore,scr,num_unk);
    }
    
    template <class Range>
    boost::tuple<sbmt::score_t,int>
    create_scores_(grammar const& g, grammar::rule_type r, Range children,bool print=false) const
    {
        //std::cerr << "calc max score over alts\n";
        sbmt::score_t scr = 0.0;
        int numunk = 0;
        bool scored = false;
        if (g.rule_has_property(r,hwidx_id)) {
            hwidx const& hvec = g.rule_property<hwidx>(r,hwidx_id);
            BOOST_FOREACH(hwidx_alt const& ha, hvec.hwidx_alt_vec) {
                bool ascr; sbmt::score_t s; int n;
                boost::tie(ascr,s,n) = create_scores_(hvec.hwnt,ha,children,print);
                if (ascr) {
                    scr = std::max(scr,s);
                    numunk = std::max(numunk,n); // n should always be the same.
                    if (numunk != n) throw std::runtime_error("shared rules should agree about rule-head-unk count");
                    scored = true;
                }
            }
        }
        if (not scored) scr = 1.0;
        if (false) std::cerr << boost::format("max-score: (%s,%s)\n") % scr % numunk;
        return boost::make_tuple(scr,numunk);
    }
    
    template <class Range>
    result_generator
    create_info( grammar const& g
               , grammar::rule_type r
               , sbmt::span_t const&
               , Range children ) const
    {
        int info = create_info_(g,r,children);
        if (info < 0 and g.is_complete_rule(r)) {
            hwpos h = get_hwpos(g,r,hwpos_id);
            if (h.lexical()) {
                info = h.word();
            }
        }
        sbmt::score_t scr;
        int num_unk;
        
        boost::tie(scr,num_unk) = create_scores_(g,r,children);
        sbmt::score_t fscr = pow(scr,wt) * pow(indicator,unk_wt*num_unk);
        //std::cerr << boost::format("create_info: (%s,%s,%s)\n") % (info >= 0 ? table_dict.get_token(info) : "NONE") % scr % num_unk; 
        return boost::make_tuple( info
                                , fscr
                                , sbmt::as_one() );
    }
    
    template <class Range, class Output>
    Output component_scores( grammar const& g
                           , grammar::rule_type r
                           , sbmt::span_t const&
                           , Range children
                           , info_type i
                           , Output o ) const
    {
        sbmt::score_t scr;
        int num_unk;
        boost::tie(scr,num_unk) = create_scores_(g,r,children,true);
        *o = std::make_pair(wt_id,scr); ++o;
        *o = std::make_pair(unk_wt_id,pow(indicator,num_unk)); ++o;
        return o;
    }
    
    rule_head_factory( sbmt::grammar_in_mem& g
                     , sbmt::property_map_type pmap
                     , table_t& table
                     , table_t& nonlex_table
                     , backoff_table_t& backoff_table )
    : hwdh_id(pmap["hwdh"])
    , hwidx_id(pmap["hwidx"])
    , hwpos_id(pmap["hwpos"])
    , indicator(0.1)
    , wt_id(g.feature_names().get_index("rule-head"))
    , unk_wt_id(g.feature_names().get_index("rule-head-unk"))
    , wt(g.get_weights()[wt_id])
    , unk_wt(g.get_weights()[unk_wt_id])
    , table(&table)
    , nonlex_table(&nonlex_table)
    , backoff_table(&backoff_table) 
    {
        //std::cerr << "backoff has " << this->backoff_table->size() << " entries" << std::endl;
        //std::cerr << "first-order has " << this->table->size() << " entries" << std::endl;
    }
    
    int hwdh_id;
    int hwidx_id;
    int hwpos_id;
    sbmt::score_t indicator;
    boost::uint32_t wt_id;
    boost::uint32_t unk_wt_id;
    double wt;
    double unk_wt;
    
    table_t* table;
    table_t* nonlex_table;
    backoff_table_t* backoff_table;
};

void read(named_table_t& n, word_set_t const& wordset)
{
    n.table.clear();
    graehl::istream_arg tblfile(n.name.c_str());
    read_lex_table_mt(*tblfile,n.table,wordset);
}

struct rule_head_constructor {
    sbmt::options_map get_options()
    {
        sbmt::options_map opts("rule head distribution options");
        opts.add_option( "rule-head-nonlex-table"
                       , sbmt::optvar(nonlex.name)
                       , "headword distribution table for nonlex rules"
                       )
                       ;
        opts.add_option( "rule-head-backoff-table"
                       , sbmt::optvar(backoff)
                       , "backoff distribution table"
                       )
                       ;
        return opts;
    }
    bool set_option(std::string const& nm, std::string const& vl)
    {
        if (nm == "rule-head-lex-table") {
            full.name = vl;
            return true;
            //std::cerr << nonlex.name << " has " << nonlex.table.size() << " entries" << std::endl;
        } else { return false; }
    }
    sbmt::any_info_factory construct( sbmt::grammar_in_mem& grammar
                                    , sbmt::lattice_tree const& lat
                                    , sbmt::property_map_type pmap )
    {
        //std::cerr << backoff.name << " has " << backoff.table.size() << " entries" << std::endl;
        token_map.clear();
        word_set_t word_set;
        int hwpos_id = pmap["hwpos"];
        BOOST_FOREACH(sbmt::grammar_in_mem::rule_type r, grammar.all_rules()) {
            if (grammar.is_complete_rule(r)) {
                hwpos h = get_hwpos(grammar,r,hwpos_id);
                if (h.lexical()) word_set.insert(h.word());
                sbmt::indexed_token lhs = grammar.rule_lhs(r);
                int lhsid = get_index(grammar.dict().label(lhs));
                token_map.insert(std::make_pair(lhs,lhsid));
            }
        }
        
        read(full,word_set);
        read(nonlex,word_set);
        
        return rule_head_factory( grammar
                                , pmap
                                , full.table
                                , nonlex.table
                                , backoff.table );
    }
                                    
    named_table_t nonlex;
    named_backoff_table_t backoff;
    named_table_t full;
};

struct rhinit {
    rhinit()
    {
        using namespace sbmt;
        register_info_factory_constructor( "rule-head"
                                         , rule_head_constructor()
                                         );
        register_rule_property_constructor( "rule-head"
                                          , "hwidx"
                                          , read_hwidx() 
                                          );
        register_rule_property_constructor( "rule-head"
                                          , "hwdh"
                                          , read_hwdh()
                                          );
        register_rule_property_constructor( "rule-head"
                                          , "hwpos"
                                          , read_hwpos()
                                          );
    }
};

rhinit init;
/*
int main() 
{
    dict_t dict;
    table_t tbl; 
    read_table(std::cin, tbl, dict);
    return 0;
}
*/
