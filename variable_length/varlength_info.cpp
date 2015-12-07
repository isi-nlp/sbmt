# include <sbmt/edge/any_info.hpp>
# include <sbmt/hash/oa_hashtable.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <boost/math/distributions/negative_binomial.hpp>
# include <boost/math/distributions/poisson.hpp>
# include <boost/math/distributions/exponential.hpp>
# include <boost/math/distributions/normal.hpp>
# include <string>
# include <sstream>
# include <gusc/generator/single_value_generator.hpp>
# include <gusc/sequence/any_sequence.hpp>
# include <gusc/generator/lazy_sequence.hpp>
# include <sbmt/logging.hpp>

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(vllog,"vlinfo",sbmt::root_domain);


template <class PDF>
class pdf_generator {
    
    int x;
    PDF pdf;
public:
    typedef double result_type;
    operator bool() const { return true; }
    explicit pdf_generator(PDF const& pdf) : x(0), pdf(pdf) {}
    double operator()() { return boost::math::pdf(pdf,x++); }
};

typedef gusc::any_sequence<double> distribution_t;

template <class PDF>
distribution_t make_table(PDF const& p)
{
    return distribution_t(gusc::make_lazy_sequence(pdf_generator<PDF>(p)));
}

typedef boost::tuple<int,int> hist_t;
typedef boost::tuple<double,double,double> moment_t;

int mode(std::vector<hist_t> const& hv)
{
    int md = 0;
    int freq = 0;

    BOOST_FOREACH(hist_t const& h, hv) {
        if (freq < boost::get<1>(h)) {
            boost::tie(md,freq) = h;
        }
    }
    return md;
}

moment_t moments(std::vector<hist_t> const& hv)
{
    int sz = 0;
    int sum = 0;
    int sumsq = 0;
    BOOST_FOREACH(hist_t const& h, hv) {
        sz += boost::get<1>(h);
        sum += boost::get<0>(h) * boost::get<1>(h); 
        sumsq += boost::get<0>(h) * boost::get<0>(h) * boost::get<1>(h); 
    }
    
    return moment_t(sz,sum,sumsq);
}

double size(moment_t const& m) 
{ 
    return m.get<0>(); 
}

double mean(moment_t const& m) 
{ 
    return m.get<1>() / size(m); 
}

double variance(moment_t const& m) 
{ 
    if (size(m) < 2.0) return 0.0;
    else return ( m.get<2>() - ((m.get<1>()*m.get<1>())/size(m)) ) / (size(m) - 1.0); 
}

typedef int info_type;

using namespace sbmt;
using namespace std;
using namespace boost;

ostream& operator << (ostream& out, distribution_t const& d)
{
    for (int x = 0; x != 40; ++x) {
        out << '(' << x << ' ' << d[x] << ')';
    }
    return out;
}

std::pair<std::string, distribution_t>
make_distribution(std::string const& line)
{
    stringstream sstr(line);
    string lbl;
    sstr >> lbl;

    vector<hist_t> v;
    
    copy( istream_iterator<hist_t>(sstr)
        , istream_iterator<hist_t>()
        , back_inserter(v)
        );
    
    //length starts at 1, but our distribution models start at 0, so offset
    BOOST_FOREACH(hist_t& h, v) {
        get<0>(h)--;
    }
    
    moment_t M = moments(v);
    double md = ::mode(v) + 0.5;
    double n = ::size(M);
    double mu = ::mean(M);
    double sigma2 = ::variance(M);
    
    distribution_t d;
    
    cerr << lbl << " n=" << n << " mu=" << mu << " sigma^2=" << sigma2 << " moments=" << M << ' ';
    
    if (mu == 0) {
        //nothing but zero counts.  use add-one smoothing:  0=n,1=1
        //and assume exponential
        double lambda = n + 1.0;
        cerr << "exponential(" << lambda << ") ";
        d = make_table(boost::math::exponential_distribution<>(lambda));
    } else if (sigma2 > mu) {
        // overdispersion: assume negative binomial, and use crummy method of
        // moments to estimate parameters
        if (sigma2 > md) {
            // method of moments estimation, using mode and variance as estimators
            // because getting the mode right always looks better than getting
            // the mean right. (and is probably more effective for assigning costs)
            double k = sqrt((md + 1)*(md + 1) - 4 * (md - sigma2)) - (md + 1.0);
            double p = 1.0 / (k + 1.0);
            double r = (md + k) / k;
            d = make_table(boost::math::negative_binomial_distribution<>(r,p));
        } else {
            double p = mu / sigma2;
            double r = (mu * mu) / (sigma2 - mu);
            cerr << "negbin(" << r << "," << p << ") ";
            d = make_table(boost::math::negative_binomial_distribution<>(r,p));
        }
/*    } else if (mu < sigma) {
        // id use negative binomial, but i want an accurate placement of the mode
        cerr << "gaussian(" << mu << "," << sqrt(sigma) << ") ";
        d = make_table(boost::math::normal_distribution<>(mu,sqrt(sigma)));
*/
    } else {
        // intentional fattening of the distribution
        cerr << "poisson(" << mu << ") ";
        d = make_table(boost::math::poisson_distribution<>(mu));
    }
    // poisson assumption
    cerr << d << '\n';
    return make_pair(lbl,d);
}

typedef sbmt::oa_hash_map<string,distribution_t> distribution_map;

distribution_map read_distributions(std::istream& in)
{
    distribution_map ret;
    string line;
    while (getline(in,line)) {
        ret.insert(make_distribution(line));
    }
    return ret;
}

struct entry {
    score_t missing;
    typedef sbmt::oa_hash_map<size_t,score_t> map_type;
    map_type values;
};

typedef sbmt::oa_hash_map<string,entry> entries_map;

std::ostream& operator << (std::ostream& out, entry const& e)
{
    typedef sbmt::oa_hash_map<size_t,score_t>::value_type pair_t;
    BOOST_FOREACH(pair_t const& p, e.values) {
        out << p.first << '=' << sbmt::logmath::linear_scale << p.second << ' ';
    }
    return out << "\?\?\?=" << e.missing;
}

std::pair<string,entry> read_entry(string line)
{
    stringstream sstr(line);
    string lbl;
    sstr >> lbl;
    typedef tuple<int,int> hist_t;
    vector<hist_t> v;
    
    copy( istream_iterator<hist_t>(sstr)
        , istream_iterator<hist_t>()
        , back_inserter(v)
        );
    
    int total = 1;
    BOOST_FOREACH(hist_t h, v) {
        total += get<1>(h) + 1;
    }
    
    entry e;
    e.missing = 1.0/total;
    
    BOOST_FOREACH(hist_t h, v) {
        e.values[get<0>(h)] = double(get<1>(h) + 1) / double(total);
    }
    
    return std::make_pair(lbl,e);
}

sbmt::oa_hash_map<string,entry> read_entries(std::istream& in)
{
    sbmt::oa_hash_map<string,entry> ret;
    string line;
    while (getline(in,line)) {
        ret.insert(read_entry(line));
    }
    return ret;
}

struct read_vlfeat {
    typedef vector<string> result_type;
    
    template <class Dictionary>
    result_type operator()(Dictionary& dict, string const& str) const
    {
        result_type ret;
        stringstream sstr(str);
        copy( istream_iterator<string>(sstr)
            , istream_iterator<string>()
            , back_inserter(ret) );
        return ret;
    }
};

struct vlinfo_factory {
    typedef int info_type;
    typedef tuple<info_type,score_t,score_t> result_type;
    
    template <class Grammar>
    std::string hash_string(Grammar const& grammar, int x) const
    {
        return boost::lexical_cast<std::string>(x);
    }
    
    template <class Grammar>
    score_t
    rule_heuristic(Grammar const& gram, typename Grammar::rule_type rule) const
    {
        return 1.0;
    }
    
    template <class Grammar>
    bool
    scoreable_rule(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        return true;
    }
    
    typedef gusc::single_value_generator<result_type> result_generator;
    template <class Grammar, class Constituents>
    result_generator
    create_info( Grammar const& grammar
               , typename Grammar::rule_type rule
               , Constituents const& range ) const
    {
        score_t score;
        int len;
        tie(score,len) = create_info_data(grammar,rule,range);

        /// note: linear domain scores; exponential combination of weights
        score_t inside = score ^ weight;

        return make_tuple(len,inside,score_t(1.0));
    }
    
    template <class Grammar, class Constituents, class ScoreOutputIterator>
    ScoreOutputIterator
    component_scores( Grammar const& grammar
                    , typename Grammar::rule_type rule
                    , Constituents const& range
                    , info_type result
                    , ScoreOutputIterator scores_out ) const
    {
        score_t score;
        tie(score,tuples::ignore) = create_info_data(grammar,rule,range);
        *scores_out = std::make_pair(weight_id,score); ++scores_out;
        return scores_out;
    }
    
    score_t scr(int len, string const& lbl) const
    {
        entries_map::const_iterator pentry = entries.find(lbl);
        if (pentry == entries.end()) return 1.0;
        
        entry::map_type::const_iterator
            pos = pentry->second.values.find(len);
            
        return pos != pentry->second.values.end() ? pos->second 
                                                  : pentry->second.missing;
    }
    
    template <class Grammar, class Constituents>
    tuple<score_t,int>
    create_info_data( Grammar const& grammar
                    , typename Grammar::rule_type rule
                    , Constituents const& range ) const
    {
        typedef vector<string> feat_type;
        feat_type const* feat = NULL;
        if (grammar.rule_has_property(rule, feature_id)) {
            feat = &grammar.template rule_property<feat_type>(rule, feature_id);
        } else {
            
        }
        typename boost::range_iterator<Constituents>::type citr = begin(range), 
                                                           cend = end(range);
        int len = 0;
        score_t score = 1.0;

        if (is_lexical(grammar.rule_lhs(rule))) {
            return make_tuple(score,1);
        }

        size_t idx = 0;
        for (; citr != cend; ++citr) {
            indexed_token rhs_token = citr->root();

            int clen = *(citr->info());
            len += clen;
            if (is_native_tag(rhs_token) and feat) {
                score *= scr(clen,feat->at(idx));
                ++idx;
            }
        }
        
        SBMT_VERBOSE_MSG(vllog, "create_info_data -> %s", make_tuple(score,len));

        return make_tuple(score,len);
    }
    
    vlinfo_factory( entries_map const& entries
                  , double weight
                  , size_t weight_id
                  , size_t feature_id )
      : entries(entries)
      , weight(weight)
      , weight_id(weight_id)
      , feature_id(feature_id) {}
    
    entries_map entries;
    double weight;
    size_t weight_id;
    size_t feature_id;
};

struct load_table {
    entries_map map;
    string fname;
};

ostream& operator << (ostream& out, load_table const& tbl)
{
    return out << tbl.fname;
}

istream& operator >> (istream& in, load_table& tbl)
{
    string str;
    in >> str;
    clog << "reading file "<< str << endl;
    ifstream file(str.c_str());
    tbl.map = read_entries(file);
    tbl.fname = str;
    clog << str << " has " << tbl.map.size() << " entries" << endl;
    return in;
}

struct vlinfo_factory_constructor {
    load_table tbl;
    
    options_map get_options() {
    options_map opts("vlinfo options");
    opts.add_option( "vlinfo-table"
                   , optvar(tbl)
                   , "file containing vl counts."
                   );  
      
      return opts;
    }
    
    bool set_option(string key, string value) const { return false; }
    
    template <class Grammar>
    vlinfo_factory construct( Grammar& grammar
                            , lattice_tree const& lattice
                            , property_map_type pmap ) const
    {
        size_t weight_id = grammar.feature_names().get_index("vl");
        double weight = grammar.get_weights()[weight_id];
        size_t feature_id = pmap["vlfeat1"];
        
        return vlinfo_factory(tbl.map,weight,weight_id,feature_id);
    }
};

struct vlinit {
    vlinit()
    {
        register_info_factory_constructor("vlinfo", vlinfo_factory_constructor());
        register_rule_property_constructor("vlinfo","vlfeat1",read_vlfeat());
    }
};

static vlinit init;

//# if 0
int main(int argc, char** argv)
{
    distribution_map entries = read_distributions(cin);
    typedef distribution_map::value_type entry_t;
    
    BOOST_FOREACH(entry_t e, entries) {
        cout << e.first << '\t' << e.second << endl;
    }
    
    return 0;
}
//# endif
