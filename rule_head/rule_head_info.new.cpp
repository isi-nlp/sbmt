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
# include <map>

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
typedef boost::tuple<word_t,double> dist_entry_t;
typedef gusc::shared_varray<dist_entry_t> dist_t;

struct dist {
    count_t c;
    count_t n;
    //sbmt::score_t backoff_wt;
    //sbmt::score_t first_order_wt;
    dist_t distribution;
    dist() : c(0), n(0){}
    void swap(dist& o)
    {
        std::swap(n,o.n);
        std::swap(c,o.c);
        distribution.swap(o.distribution);
    }
    
    template <class Range>
    dist(count_t c, count_t n, Range const& rng, bool check = true) 
      : c(c)
      , n(n)
      , distribution(boost::begin(rng), boost::end(rng))
      {
          if (check) {
              if (c < rng.size()) throw std::runtime_error("rule_head: c < table size");
              if (c > n) throw std::runtime_error("rule_head: c > n");
          }
      }
};

typedef sbmt::oa_hash_map<word_t,dist> word_table_t;
typedef sbmt::oa_hash_map< ruleid_t, boost::tuple< dist, std::vector<dist> > > 
        ruleid_table_t;
typedef sbmt::in_memory_token_storage dict_t;
typedef sbmt::oa_hash_map<sbmt::indexed_token,word_t> token_map_t;
typedef std::set<word_t> word_set_t;

typedef boost::tuple<ruleid_t,variable_t> index_t;
typedef small_array<2,index_t> hwidx_alt;
typedef small_array<2,word_t> hwidx_nt;
struct hwidx {
    hwidx_nt hwnt;
    std::vector<hwidx_alt> hwidx_alt_vec;
};

struct brf_dist {
    hwidx_nt nts;
    small_array<2,dist> dists;
};

typedef sbmt::oa_hash_map<sbmt::grammar_in_mem::rule_type, brf_dist> 
        brf_dist_map_t;

////////////////////////////////////////////////////////////////////////////////

static token_map_t token_map;
static dict_t table_dict;

word_t get_index(std::string const& str)
{
    return table_dict.get_index(str);
}

bool has_token(std::string const& str)
{
    return table_dict.has_token(str);
}

////////////////////////////////////////////////////////////////////////////////

struct ptables {
    brf_dist_map_t brf_dist_map;
    ruleid_table_t rule_primary;
    word_table_t nt_table;
    word_table_t sum_table;
    word_table_t predicted_table;
    dist word_unigram;
    int hwpos_id;
    int hwidx_id;
    int gt_prob_id;
};

struct search_op {
    template <class T>
    bool operator()(T const& a, T const& b) const
    {
        return boost::get<0>(a) < boost::get<0>(b);
    }
};

namespace {
search_op search_op_;
sbmt::score_t min_scr_(1e-200);
}

template <class Dist, class Key>
typename Dist::value_type
get_(Dist const& d, Key const& w)
{
    typedef typename Dist::value_type entry_t;
    typename Dist::const_iterator 
        dpos = std::lower_bound( d.begin()
                               , d.end()
                               , entry_t(w)
                               , search_op_
                               );
    
    if (dpos == d.end() or dpos->get<0>() != w) return entry_t(w);
    else return *dpos;
}

struct hwpos {
    explicit hwpos(std::string const& str)
    {
        if (str[0] == '"') {
            wd = get_index(str.substr(1,str.size() - 2));
        } else {
            wd = -(boost::lexical_cast<word_t>(str) + 2);
        }
    }
    hwpos() : wd(get_index("@UNKNOWN")) {}
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

hwpos get_hwpos( sbmt::grammar_in_mem const& g
               , sbmt::grammar_in_mem::rule_type r
               , int hwpos_id )
{
    if (not g.rule_has_property(r,hwpos_id)) return hwpos();
    return g.rule_property<hwpos>(r,hwpos_id);
}

static double dmult = 1.0;

double wb_prob( count_t c
              , count_t n
              , double prob
              , double bkf
              , double mult )
{
    double cc = double(c)*mult;
    double nn = n;

    if (n == 0) return bkf;
    //if (counts > n) throw std::logic_error("rule-head:wb_prob: n < counts");
    if (n < c) throw std::logic_error("rule-head:wb_prob: n < c");
    return (nn/(nn + cc)) * prob + (cc/(nn + cc)) * bkf;
}

double
prob_headword_given_nt( ptables const& p
                      , word_t nt
                      , word_t headword 
                      )
{
    dist const* pdist;
    dist const* bpdist;
    word_table_t::const_iterator pos = p.nt_table.find(nt);
    if (pos != p.nt_table.end()) {
        pdist = &(pos->second);
        if (p.use_predicted) bpdist = &(p.predicted_table.find(nt)->second);
        else bpdist = &(p.word_unigram);
    } else {
        return 1.0; // unknown variable.  probably means glue....
    }
    
    double prob = get_(pdist->distribution,headword).get<1>();
    if (not p.use_secondary_backoffs) return std::max(prob,minscr);
    count_t c = pdist->c;
    count_t n = pdist->n;

    double bprob = get_(bpdist->distribution,headword).get<1>();
    count_t bc = bpdist->c;
    count_t bn = p.word_unigram.n;
    double bbprob = 1.0/double(bpdist->c + 1); 
    double retval = wb_prob(c,n,prob,wb_prob(bc,bn,bprob,bbprob,dmult),p.dmult);

    if (retval == 0.0)
        std::cerr << "wb_prob("<<c<<','<<n<<prob<<','<<"wb_prob("<<bc<<','<<bn<<','<<bprob<<','<<bbprob<<"))\n";
    return retval;
}

sbmt::score_t
prob_headword_given_rule_nt( ptables const& p
                           , sbmt::grammar_in_mem const& g
                           , dist const& brf_dist
                           , word_t nt
                           , word_t headword 
                           )
{
    double prob = get_(brf_dist.distribution,headword).get<1>();
    if (not p.use_backoffs) return std::max(p,minscr);
    count_t c = brf_dist.c;
    count_t n = brf_dist.n;
    double bkf = prob_headword_given_nt(p,nt,headword);
    double retval = wb_prob(c,n,prob,bkf,p.dmult);
    if (retval == 0.0)
        std::cerr << "wb_prob("<<c<<','<<n<<','<<prob<<','<<bkf<<")\n";
    return retval;
}

boost::tuple<word_t,variable_t>
get_rhs_var( sbmt::grammar_in_mem const& g
           , sbmt::grammar_in_mem::rule_type const& r
           , hwpos const& hp)
{
    sbmt::indexed_syntax_rule const& syn = g.get_syntax(r);
    variable_t lv = 0;
    variable_t rv = 0;
    unsigned rpos=-1;
    BOOST_FOREACH( sbmt::indexed_syntax_rule::tree_node const& lhs
                 , std::make_pair(syn.lhs_begin(), syn.lhs_end()) ) {
        if (lhs.indexed()) {
            if (lv == hp.index()) {
                rpos = lhs.index();
                break;
            }
            ++lv;
        }
    }
    if (lv != hp.index()) 
        throw std::runtime_error( "can not convert lhs idx=" + 
                                  boost::lexical_cast<std::string>(hp.index()) +
                                  " to rhs idx (lv=" +
                                  boost::lexical_cast<std::string>(lv) +
                                  " rpos=" +
                                  boost::lexical_cast<std::string>(rpos) +
                                  ")");

    unsigned rc = 0;
    BOOST_FOREACH( sbmt::indexed_syntax_rule::rule_node const& rhs
                 , std::make_pair(syn.rhs_begin(), syn.rhs_end()) ) {
        if (rhs.indexed()) {
            if (rc == rpos) {
                return boost::make_tuple( get_index(g.dict().label(syn.lhs_position(rhs)->get_token()))
                                        , rv );
            }
            ++rv;
        }
        ++rc;
    }
    
    throw std::runtime_error( "cannot convert lhs idx=" + 
                              boost::lexical_cast<std::string>(hp.index()) +
                              " to rhs idx");
}

sbmt::score_t
prob_headword_given_rule_nt( ptables const& p
                           , sbmt::grammar_in_mem const& g
                           , sbmt::grammar_in_mem::rule_type const& r
                           , word_t headword )
{
    ruleid_t id = g.get_syntax(r).id();
    hwpos hp = g.rule_property<hwpos>(r,p.hwpos_id);
    word_t nt;  variable_t v;
    boost::tie(nt,v) = get_rhs_var(g,r,hp);
    ruleid_table_t::const_iterator pos = p.rule_primary.find(id);
    if (pos == p.rule_primary.end()) return prob_headword_given_nt(p,nt,headword);
    else return prob_headword_given_rule_nt(p,g,pos->second.get<1>().at(v),nt,headword);
}

double prob_rule_given_head_nt_denominator( ptables const& p
                                          , sbmt::grammar_in_mem const& g
                                          , word_t headword
                                          , word_t nt
                                          )
{
    sum_table_t::const_iterator spos = p.sum_table.find(nt);
    // ie glue.  this should help to guarantee that for 
    // GLUE -> GLUE X, p(r | h, GLUE) = 1.0 * p(h | X) / 1.0
    // which closes off the subtree
    if (spos == p.sum_table.end()) return 1.0; 
    
    if (p.use_sum_table) {
        
    } else {
        
    }
}

boost::tuple<sbmt::score_t,sbmt::score_t>
prob_rule_given_head_nt( ptables const& p
                       , sbmt::grammar_in_mem const& g
                       , sbmt::grammar_in_mem::rule_type const& r
                       , word_t headword )
{
    if (g.rule_lhs(r).type() == sbmt::top_token) {
        hwpos hp = g.rule_property<hwpos>(r,p.hwpos_id);
        word_t nt;
        boost::tie(nt,boost::tuples::ignore) = get_rhs_var(g,r,hp);
        return boost::make_tuple(prob_headword_given_nt(p,nt,headword),1.0);
    }
    ruleid_t id = g.get_syntax(r).id();
    word_t nt = get_index(g.dict().label(g.rule_lhs(r)));
    ruleid_table_t::const_iterator pos = p.rule_primary.find(id);
    sbmt::score_t rprob = g.rule_scores(r)[p.gt_prob_id];
    hwpos hp = g.rule_property<hwpos>(r,p.hwpos_id);
    
    word_table_t::const_iterator spos = p.sum_table.find(nt);
    count_t c = 1, rn = 0;
    double rsum = 0.0;
    if (spos != p.sum_table.end()) {
        rn = spos->second.n;
        rsum = get_(spos->second.distribution,headword).get<1>();
        if (rsum == 0.0) {
            
            //rsum = get_(spos->second.distribution,get_index("<unk>")).get<1>();
            //std::cerr << "using unk for id="<< id << " headword=" << table_dict.get_token(headword) << " (" << rsum << ")\n"; 
            //if (rsum == 0.0) rsum = 1.0;
        }
    }
    
    if (pos != p.rule_primary.end() and rn > 0) { 
        count_t rcount = pos->second.get<0>().n;
        rprob = double(rcount)/double(rn);
    }
    
    sbmt::score_t hrnt;
    
    // unk word.  technically, p(r | nt, w) is undefined
    // we back-off to p(r | nt) rather than trust our calculation of p( unk | nt )
    if (rsum == 0) return boost::make_tuple(rprob,rprob);
    if (not hp.lexical()) hrnt = prob_headword_given_rule_nt(p,g,r,headword);
    sbmt::score_t hnt = rsum;
    
    sbmt::score_t scr;
    if (hp.lexical()) {
        if (hp.word() != headword) {
            throw std::logic_error("illegal rule/headword combination: " + table_dict.get_token(hp.word()) +
                                   " vs " + table_dict.get_token(headword) + "\n"
                                   "ruleid: " + boost::lexical_cast<std::string>(id) + "\n" );
        }
        scr = rprob / hnt;
    } else if (c > 0) {
        scr = (rprob * hrnt) / hnt;
        if (scr > 1.0 or scr == 0.0)
        std::cerr << sbmt::logmath::linear_scale << print(r,g)  << " id=" << id << " " << table_dict.get_token(headword) <<" rprob:" << rprob << " hrnt: " << hrnt 
                  << " numerator: " << rprob * hrnt << " denominator: " << hnt << '\n';
        
        
    }
    else scr = rprob;

    return boost::make_tuple(scr,rprob);
}

////////////////////////////////////////////////////////////////////////////////

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
        using boost::make_tuple;
        string word;
        string nt;
        ruleid_t id;
        count_t n;
        count_t c;
        double f;
        
        typedef boost::tuple< dist, std::vector<dist> > dists;
        typedef std::pair<ruleid_t,dists> datum_type;
        typedef std::vector<datum_type> return_type;
        std::auto_ptr<return_type> tbl(new return_type());
        for (size_t offset = 0; offset != 100; ++offset) {
            variable_t idx = 0;
            if (*(static_cast<std::string*>(lineptr) + offset) == "") break;
            stringstream sstr(*(static_cast<std::string*>(lineptr) + offset));
            sstr >> id >> nt >> n;
            std::vector<dist> dv;
            
            while (sstr) {
                std::vector<dist_entry_t> entries;
                sstr >> c;
                for (size_t x = 0; x != c; ++x) {
                    sstr >> word;
                    sstr >> f;
                    if (has_token(word)) {
                        word_t wid = get_index(word);
                        if (wordset.find(wid) != wordset.end()) {
                            entries.push_back(dist_entry_t(wid,f));
                        }
                    }
                }
                std::sort(entries.begin(),entries.end(), search_op_);
                //if (entries.size() != c) throw std::logic_error("rule-head: table variable empty");
                dv.push_back(dist(c,n,entries));
                //std::cerr << id << ' ' << n << ' ' << c << '\n';
                ++idx;
                sstr >> std::ws;
            }
            tbl->push_back(make_pair(
                   id
                 , make_tuple(dv.front(), vector<dist>(++dv.begin(), dv.end()))
                 ));
        }
        return tbl.release();
    }
    
    lex_table_procline(word_set_t const& wordset) 
      : tbb::filter(tbb::filter::parallel)
      , wordset(wordset) {}
      
    word_set_t const& wordset;
};

struct lex_table_assemble : tbb::filter {
    ruleid_table_t& tbl;
    lex_table_assemble(ruleid_table_t& tbl) 
      : tbb::filter(tbb::filter::serial)
      , tbl(tbl) {}
    void* operator()(void* data) 
    {
        typedef boost::tuple< dist, std::vector<dist> > dists;
        typedef std::pair<ruleid_t,dists> datum_type;
        typedef std::vector<datum_type> data_type;
        std::auto_ptr<data_type> d(static_cast<data_type*>(data));
        BOOST_FOREACH(datum_type& dd, *d) {
            ruleid_table_t::iterator pos = tbl.insert(datum_type(dd.first,dist())).first;
            const_cast<dist&>(pos->second.get<0>()).swap(dd.second.get<0>());
            const_cast<std::vector<dist>&>(pos->second.get<1>()).swap(dd.second.get<1>());
        }
        return 0;
    }
};

ruleid_table_t& read_lex_table_mt(istream& in, ruleid_table_t& tbl, word_set_t const& wordset)
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

word_table_t&
read_predicted_table(istream& in, word_table_t& tbl)
{
    sbmt::io::logging_stream& log_out = sbmt::io::registry_log(rh_log);
    sbmt::io::log_time_space_report 
        report(log_out,sbmt::io::lvl_terse,"read headword predicted table: ");
    using namespace std;
    string wd;
    count_t n, c;
    double f;
    while (in) {
        in >> wd >> c;
        n = c;
        word_t nonterminal = get_index(wd);
        std::vector<dist_entry_t> entries;
        for (size_t x = 0; x != c; ++x) {
            in >> wd;
            in >> f;
            entries.push_back(dist_entry_t(get_index(wd),double(f)));
        }
        std::sort(entries.begin(),entries.end(),search_op_);
        tbl.insert(make_pair(nonterminal,dist(c,n,entries)));
        in >> std::ws;
    }
    return tbl;
}

word_table_t& 
read_backoff_table(istream& in, word_table_t& tbl)
{
    sbmt::io::logging_stream& log_out = sbmt::io::registry_log(rh_log);
    sbmt::io::log_time_space_report 
        report(log_out,sbmt::io::lvl_terse,"read headword backoff table: ");
    using namespace std;
    string wd;
    count_t n, c, f;
    while (in) {
        in >> wd >> n >> c;
        word_t nonterminal = get_index(wd);
        std::vector<dist_entry_t> entries;
        for (size_t x = 0; x != c; ++x) {
            in >> wd;
            in >> f;
            entries.push_back(dist_entry_t(get_index(wd),double(f)/double(n)));
            
        }
        std::sort(entries.begin(),entries.end(),search_op_);
        tbl.insert(make_pair(nonterminal,dist(c,n,entries)));
        in >> std::ws;
    }
    return tbl;
}

dist create_unigram_table(word_table_t const& tbl)
{
    std::set<dist_entry_t,search_op> counts;
    count_t n = 0;
    BOOST_FOREACH(word_table_t::value_type const& v,tbl) {
        n += v.second.n;
        BOOST_FOREACH(dist_entry_t const& d, v.second.distribution) {
            std::set<dist_entry_t,search_op>::iterator pos = counts.find(d);
            if (pos != counts.end()) {
                const_cast<double&>(pos->get<1>()) += d.get<1>();
            } else {
                counts.insert(d);
            }
        }
    }
    return dist(counts.size(),n,counts);
}

word_table_t& 
read_sum_table(istream& in, word_table_t& tbl)
{
    sbmt::io::logging_stream& log_out = sbmt::io::registry_log(rh_log);
    sbmt::io::log_time_space_report 
        report(log_out,sbmt::io::lvl_terse,"read headword sum table: ");
    using namespace std;
    string wd;
    count_t c, n;
    double f;
    while (in) {
        in >> wd >> n >> c;
        //std::cerr << wd << ' ' << n << ' ' << c << '\n';
        word_t nonterminal = get_index(wd);
        std::vector<dist_entry_t> entries;
        for (size_t x = 0; x != c; ++x) {
            in >> wd >> f;
            entries.push_back(dist_entry_t(get_index(wd),f));
            
        }
        std::sort(entries.begin(),entries.end(),search_op_);
        tbl.insert(make_pair(nonterminal,dist(c,n,entries,false)));
        in >> std::ws;
    }
    std::cerr << "sum table has " << tbl.size() << " lines\n";
    return tbl;
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

void make_brf_dist_map(ptables& p, sbmt::grammar_in_mem const& g)
{
    using namespace sbmt;
    BOOST_FOREACH(grammar_in_mem::rule_type r, g.all_rules()) {
        if (g.rule_has_property(r,p.hwidx_id)) {
            hwidx const& h = g.rule_property<hwidx>(r,p.hwidx_id);
            typedef std::set<dist_entry_t,search_op> dist_entries;
            dist_entries dsets[2];
            count_t c[2] = {0,0};
            count_t n[2] = {0,0};
            size_t idx = 0;
            bool found = false;
            BOOST_FOREACH(word_t w, h.hwnt) {
                if (get_index("--") != w) {
                    BOOST_FOREACH(hwidx_alt const& ha,h.hwidx_alt_vec) {
                        ruleid_t id;
                        variable_t v;
                        boost::tie(id,v) = ha[idx];
                        ruleid_table_t::iterator pos = p.rule_primary.find(id);
                        if (pos != p.rule_primary.end()) {
                            found = true;
                            count_t c_ = pos->second.get<1>().at(v).c;
                            count_t n_ = pos->second.get<1>().at(v).n;
                            
                            // this is not true (true for n, not for c).  its an upper-bound for c
                            c[idx] += c_;
                            n[idx] += n_;
                            
                            dist_t const& distribution = pos->second.get<1>().at(v).distribution;
                            BOOST_FOREACH(dist_entry_t dentry, distribution) {
                                dist_entries::iterator spos = dsets[idx].find(dentry);
                                if (spos != dsets[idx].end()) {
                                    const_cast<double&>(spos->get<1>()) += n_ * dentry.get<1>();
                                } else {
                                    dsets[idx].insert(dentry);
                                }
                            }
                        }
                    }
                    BOOST_FOREACH(dist_entries::value_type const& d, dsets[idx]) {
                        const_cast<double&>(d.get<1>()) /= n[idx];
                    }
                    ++idx;
                }
            }
            if (true) {
                brf_dist bdist;
                bdist.nts = h.hwnt;
                for (size_t i = 0; i != idx; ++i) {
                    
                    bdist.dists.push_back(dist(c[i],n[i],dsets[i]));
                }
                p.brf_dist_map.insert(std::make_pair(r,bdist));
            }
        }
    }
}

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

struct head_info {
    bool is_head;
    word_t word;
    head_info() : is_head(false), word(-1) {}
};

bool operator == ( head_info const& h1, head_info const& h2)
{
    return (h1.is_head == h2.is_head) and (h1.word == h2.word);
}

bool operator != ( head_info const& h1, head_info const& h2)
{
    return not (h1 == h2);
}

std::size_t hash_value(head_info const& hi)
{
    size_t x = 0;
    boost::hash_combine(x,hi.is_head);
    boost::hash_combine(x,hi.word);
    return x;
}

struct rule_head_factory {
    typedef head_info info_type;
    typedef tuple<info_type,sbmt::score_t,sbmt::score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;
    typedef sbmt::grammar_in_mem grammar;
    
    bool scoreable_rule(grammar const& g, grammar::rule_type r) const
    {
        return true;
    }
    
    sbmt::score_t rule_heuristic( sbmt::grammar_in_mem const& g
                                , sbmt::grammar_in_mem::rule_type r ) const
    {
        return 1.0;
    }
    
    std::string hash_string( sbmt::grammar_in_mem const& g
                           , info_type i ) const
    {

        return boost::lexical_cast<std::string>(i.is_head) + "," + 
               boost::lexical_cast<std::string>(i.word);
    }
    
    template <class Range>
    info_type 
    create_info_( sbmt::grammar_in_mem const& g
                , sbmt::grammar_in_mem::rule_type r
                , Range children ) const
    {
        info_type result; // { false, -1}
        hwdh const& hvec = g.rule_property<hwdh>(r,hwdh_id);
        int idx = 0;

        BOOST_FOREACH(typename boost::range_value<Range>::type const& rv, children) {
            info_type c = *rv.info();
            if (is_native_tag(rv.root())) { 
                if (hvec[idx]) {
                    if (result.is_head) throw std::logic_error("rule-head: two heads in one rule");
                    result.is_head = true; result.word = c.word;
                } else if (not result.is_head) {
                    result.is_head = false; result.word = c.word;
                } // result is already holding a headword
                ++idx;
            } else if (c.word >= 0) {
                if (not result.is_head) {
                    result = c;
                } else if (c.is_head) {
                    throw std::logic_error("rule-head: two children are heads");
                }
            }
        }
        
        if (g.is_complete_rule(r)) {
            result.is_head = false;
        }
        
        return result;
    } 
    
    template <class Range>
    boost::tuple<sbmt::score_t,sbmt::score_t,sbmt::score_t>
    create_scores_(grammar const& g, grammar::rule_type r, Range children,info_type hw, bool print=false) const
    {
        sbmt::score_t rh_scr = 1.0;
        sbmt::score_t hr_scr = 1.0;
        sbmt::score_t bk_scr = 1.0;
        
        if (g.is_complete_rule(r)) {
            boost::tie(rh_scr,bk_scr) = prob_rule_given_head_nt(*p,g,r,hw.word);
            if (print) {
                std::cerr << "\np( "
                          << sbmt::print(r,g)
                          << " | "
                          << table_dict.get_token(hw.word)
                          << " ) = ("
                          << rh_scr 
                          << ","
                          << bk_scr
                          << ")\n";
            }
            //hr_scr = prob_head_given_nt(*p,g,r,hw);
        }
        brf_dist_map_t::const_iterator pos = (*p).brf_dist_map.find(r);
        hwdh const& hvec = g.rule_property<hwdh>(r,hwdh_id);
        
        if (pos != (*p).brf_dist_map.end()) {
            size_t idx = 0;
            size_t hvidx = 0;
            typename boost::range_iterator<Range>::type citr = boost::begin(children);
            BOOST_FOREACH(word_t w, pos->second.nts) {
                if (w != get_index("--")) {
                    word_t chr = citr->info()->word;
                    bool score_it = false;
                    if (is_native_tag(citr->root())) {
                        score_it = not hvec[hvidx];
                        ++hvidx;
                    } else {
                        score_it = not citr->info()->is_head;
                    }
                    if (chr == -1) throw std::logic_error("rule-head: headword unset");
                    if (score_it) {
                        sbmt::score_t hr_scr_i = prob_headword_given_rule_nt(*p,g,pos->second.dists[idx],w,chr);
                        hr_scr *= hr_scr_i;
                        if (print) {
                        std::cerr << "\np( "  
                                  << table_dict.get_token(chr) 
                                  << " | "
                                  << sbmt::print(r,g)
                                  << " , "
                                  << table_dict.get_token(w)
                                  << " ) = "
                                  << hr_scr_i
                                  << "\n";
                        }
                    }
                    ++idx;
                }
                ++citr;
            }
        }
        
        return boost::make_tuple(rh_scr,hr_scr,bk_scr);
    }
    
    template <class Range>
    result_generator
    create_info( sbmt::grammar_in_mem const& g
               , sbmt::grammar_in_mem::rule_type r
               , sbmt::span_t const&
               , Range children ) const
    {
        info_type info = create_info_(g,r,children);
        if ((info.word < 0 or not info.is_head) and g.is_complete_rule(r)) {
            hwpos h = get_hwpos(g,r,hwpos_id);
            if (h.lexical()) {
                info.word = h.word();
            }
            if (info.word < 0) throw std::logic_error("rule-head: complete rule headword not set");
        }
        sbmt::score_t rh_scr,hr_scr,bk_scr;
        
        boost::tie(rh_scr,hr_scr,bk_scr) = create_scores_(g,r,children,info);
        sbmt::score_t fscr = pow(rh_scr,rh_wt) 
                           * pow(hr_scr,hr_wt) 
                           * pow(bk_scr,bk_wt)
                           * pow(rh_scr * hr_scr,lx_wt);
 
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
        sbmt::score_t rh_scr, hr_scr, bk_scr;
        boost::tie(rh_scr,hr_scr,bk_scr) = create_scores_(g,r,children,i,true);
        *o = std::make_pair(rh_wt_id,rh_scr); ++o;
        *o = std::make_pair(hr_wt_id,hr_scr); ++o;
        *o = std::make_pair(bk_wt_id,bk_scr); ++o;
        *o = std::make_pair(lx_wt_id,rh_scr * hr_scr); ++o;
        return o;
    }
    
    rule_head_factory( sbmt::grammar_in_mem& g
                     , sbmt::property_map_type pmap
                     , ptables& p )
    : hwdh_id(pmap["hwdh"])
    , hwpos_id(pmap["hwpos"])
    , rh_wt_id(g.feature_names().get_index("rule-head"))
    , hr_wt_id(g.feature_names().get_index("head-rule"))
    , bk_wt_id(g.feature_names().get_index("bkf-rh-prob"))
    , lx_wt_id(g.feature_names().get_index("lex-prob"))
    , rh_wt(g.get_weights()[rh_wt_id])
    , hr_wt(g.get_weights()[hr_wt_id])
    , bk_wt(g.get_weights()[bk_wt_id])
    , lx_wt(g.get_weights()[lx_wt_id])
    , p(&p) {}
    
    int hwdh_id;
    int hwpos_id;
    boost::uint32_t rh_wt_id;
    boost::uint32_t hr_wt_id;
    boost::uint32_t bk_wt_id;
    boost::uint32_t lx_wt_id;
    double rh_wt;
    double hr_wt;
    double bk_wt;
    double lx_wt;
    ptables* p;
};

void read(ruleid_table_t& table, std::string const& name, word_set_t const& wordset)
{
    graehl::istream_arg tblfile(name.c_str());
    read_lex_table_mt(*tblfile,table,wordset);
}

struct rule_head_constructor {
    bool setup;
    rule_head_constructor() : setup(false) {}
    sbmt::options_map get_options()
    {
        sbmt::options_map opts("rule head distribution options");
        opts.add_option( "rule-head-nonlex-table"
                       , sbmt::optvar(nonlex)
                       , "headword distribution table for nonlex rules"
                       )
                       ;
        opts.add_option( "rule-head-backoff-table"
                       , sbmt::optvar(backoff)
                       , "backoff distribution table"
                       )
                       ;
        opts.add_option( "rule-head-predicted-table"
                       , sbmt::optvar(predicted)
                       , "backoff distribution table"
                       )
                       ;
        opts.add_option( "rule-head-sum-table"
                       , sbmt::optvar(sum)
                       , "backoff distribution table"
                       )
                       ;
        opts.add_option( "rule-head-diversity-multiplier"
                       , sbmt::optvar(dmult)
                       , "witten-bell diversity multiplier"
                       )
                       ;
        return opts;
    }
    bool set_option(std::string const& nm, std::string const& vl)
    {
        if (nm == "rule-head-lex-table") {
            lex = vl;
            return true;
        } else { return false; }
    }
    sbmt::any_info_factory construct( sbmt::grammar_in_mem& grammar
                                    , sbmt::lattice_tree const& lat
                                    , sbmt::property_map_type pmap )
    {
        
        //std::cerr << backoff.name << " has " << backoff.table.size() << " entries" << std::endl;
        //token_map.clear();
        word_set_t word_set;
        int hwpos_id = pmap["hwpos"];
        get_index("@UNKNOWN");
        BOOST_FOREACH(sbmt::grammar_in_mem::rule_type r, grammar.all_rules()) {
            if (grammar.is_complete_rule(r)) {
                hwpos h = get_hwpos(grammar,r,hwpos_id);
                if (h.lexical()) word_set.insert(h.word());
                sbmt::indexed_token lhs = grammar.rule_lhs(r);
                int lhsid = get_index(grammar.dict().label(lhs));
                token_map.insert(std::make_pair(lhs,lhsid));
            }
        }
        
        tables.rule_primary.clear();
        read(tables.rule_primary,nonlex,word_set);
        read(tables.rule_primary,lex,word_set);
        tables.hwpos_id = hwpos_id;
        if (not setup) {
            tables.nt_table.clear();
            tables.sum_table.clear();
            graehl::istream_arg bkf(backoff.c_str());
            graehl::istream_arg pdt(predicted.c_str());
            read_backoff_table(*bkf,tables.nt_table);
            tables.word_unigram = create_unigram_table(tables.nt_table);
            read_predicted_table(*pdt,tables.predicted_table);
            graehl::istream_arg sm(sum.c_str());
            read_sum_table(*sm,tables.sum_table);
            setup = true;
        }
        
        tables.hwidx_id = pmap["hwidx"];
        tables.gt_prob_id = grammar.feature_names().get_index("gt_prob");
        tables.brf_dist_map.clear();
        make_brf_dist_map(tables,grammar);
        
        return rule_head_factory( grammar
                                , pmap
                                , tables );
    }
    ptables tables;
    std::string lex;
    std::string nonlex;
    std::string backoff;
    std::string predicted;
    std::string sum;
};

struct rhinit {
    rhinit()
    {
        using namespace sbmt;
        register_info_factory_constructor( "rule-head"
                                         , rule_head_constructor()
                                         );
        register_rule_property_constructor( "rule-head"
                                          , "hwdh"
                                          , read_hwdh()
                                          );
        register_rule_property_constructor( "rule-head"
                                          , "hwidx"
                                          , read_hwidx()
                                          );
        register_rule_property_constructor( "rule-head"
                                          , "hwpos"
                                          , read_hwpos()
                                          );

    }
};

rhinit init;

