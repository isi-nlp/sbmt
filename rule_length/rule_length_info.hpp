#ifndef     RULE_LENGTH__RULE_LENGTH_INFO
#define     RULE_LENGTH__RULE_LENGTH_INFO

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
# include <boost/tokenizer.hpp>
# include <boost/token_iterator.hpp>
# include <boost/function_output_iterator.hpp>

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(rl_log,"rule-length",sbmt::root_domain);

namespace rule_length {


const sbmt::score_t scorefloor = 1e-20;

typedef boost::uint64_t hist_size_t;
typedef boost::tuple<hist_size_t,hist_size_t,hist_size_t> moment_t;
typedef boost::tuple<double,double> dist_param_t;

////////////////////////////////////////////////////////////////////////////////

template <class PDF>
class log_pdf {
    PDF pdf;
public:
    typedef sbmt::score_t result_type;
    operator bool() const { return true; }
    explicit log_pdf(PDF const& pdf) : pdf(pdf) {}
    log_pdf(){}
    sbmt::score_t operator()(int x) const 
    { 
        float floor = 1e-20;
        return std::max(floor,boost::math::pdf(pdf,x)); 
    }
    float mean() const { return pdf.mean(); }
    float scale() const { return pdf.scale(); }
};

template <class PDF> 
std::ostream& operator << (std::ostream& os, log_pdf<PDF> const& pdf)
{
    return os << '(' << pdf.mean() << ',' << pdf.scale() << ')';
}

template <class PDF>
class log_trap_pdf {
    PDF pdf;
public:
    typedef sbmt::score_t result_type;
    explicit log_trap_pdf(PDF const& pdf) : pdf(pdf) {}
    sbmt::score_t operator()(int x) const
    { 
        double floor = 1e-20;
        double sum = 0;
        double m = boost::math::pdf(pdf, x - 0.5);
        for (int d = -5; d != 5; ++d) {
            double M = boost::math::pdf(pdf, x + 0.1 * (d + 1));
            sum += ((m + M) / 2.0) * 0.1;
            m = M;
        }
        
        SBMT_VERBOSE_MSG(rl_log,"sum(pdf,[%s,%s]) = %s",(x-0.5) % (x+0.5) % sum);
        return std::max(floor,std::min(sum,1.0));
    }
};



////////////////////////////////////////////////////////////////////////////////

typedef log_pdf< boost::math::normal_distribution<float> > distribution_t;

template <class Grammar>
struct var_distribution {
    typedef std::vector<distribution_t> type;
};

template <class PDF>
distribution_t make_table(PDF const& p)
{
    return distribution_t(log_pdf<PDF>(p));
}

////////////////////////////////////////////////////////////////////////////////


inline hist_size_t size(moment_t const& m) 
{ 
    return m.get<0>(); 
}

inline double mean(moment_t const& m) 
{ 
    return double(m.get<1>()) / size(m); 
}

inline double variance(moment_t const& m) 
{ 
    double floor = 1e-12; //1e-3;
    if (size(m) < 2.0) return floor;
    
    double v = double(m.get<2>() * m.get<0>() - m.get<1>() * m.get<1>()) /
               double(m.get<0>() * (m.get<0>() - 1));
    return std::max(floor,v);
}

////////////////////////////////////////////////////////////////////////////////

boost::tuple<double,double> 
fattened_interpolation(moment_t const& moments, double lambda);

distribution_t fattened_distribution(moment_t const& moments, double lambda);
distribution_t preset_distribution(dist_param_t const& params);

////////////////////////////////////////////////////////////////////////////////

namespace {
struct variable_t {};
variable_t variable;
struct terminal_t {};
terminal_t terminal;

void suppress_variable_terminal_warnings(terminal_t t = terminal, variable_t v = variable) {}
}

struct len_info {
    int var;
    int total;
    
    len_info& operator+=(len_info const& o)
    {
        var += o.var;
        total += o.total;
        return *this;
    }
    len_info() : var(0), total(0) {}
    len_info(variable_t const&, int n);
    len_info(terminal_t const&, int n);
    len_info& operator++(void) { ++total; return *this; }
};

len_info::len_info(variable_t const&, int n) : var(n), total(n) {}
len_info::len_info(terminal_t const&, int n) : var(0), total(n) {}

inline std::ostream& operator << (std::ostream& out, len_info const& l)
{
    return out << '(' << l.var << ',' << l.total << ')';
}

inline bool operator == (len_info const& l1, len_info const& l2)
{
    return l1.var == l2.var and l1.total == l2.total;
}

inline bool operator != (len_info const& l1, len_info const& l2)
{
    return !(l1 == l2);
}

inline size_t hash_value(len_info const& l)
{
    size_t x = 0;
    boost::hash_combine(x,l.var);
    boost::hash_combine(x,l.total);
    return x;
}

////////////////////////////////////////////////////////////////////////////////

struct rlinfo_factory
: sbmt::info_factory_new_component_scores<rlinfo_factory>
{
    struct accum {
        double rule_weight;
        double var_weight;
        size_t rule_id;
        size_t var_id;
        sbmt::score_t* total;
        void operator()(std::pair<size_t,sbmt::score_t> const& p) const
        {
            if (p.first == rule_id) *total *= p.second ^ rule_weight;
            else if (p.first == var_id) *total *= p.second ^ var_weight;
            else throw std::runtime_error("invalid weight id in accumulator");
        } 
    };
    
    typedef len_info info_type;
    typedef boost::tuple<info_type,sbmt::score_t,sbmt::score_t> result_type;
    
    template <class Grammar>
    std::string hash_string(Grammar const& grammar, info_type x) const
    {
        return boost::lexical_cast<std::string>(x);
    }
    
    template <class Grammar>
    sbmt::score_t
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
               , sbmt::span_t const& span
               , Constituents const& range ) const
    {
        sbmt::score_t total;
        accum acc;
        acc.total = &total;
        acc.rule_weight = rule_weight;
        acc.var_weight = var_weight;
        acc.rule_id = rule_weight_id;
        acc.var_id = var_weight_id;
        info_type info;
        boost::tie(info,boost::tuples::ignore) = 
            create_info_( grammar
                        , rule
                        , span
                        , range
                        , boost::make_function_output_iterator(acc)
                        );

        return boost::make_tuple(info,total,sbmt::score_t(1.0));
    }
    
    template <class Grammar, class Constituents, class ScoreOutputIterator>
    ScoreOutputIterator
    component_scores_old( Grammar const& grammar
                        , typename Grammar::rule_type rule
                        , sbmt::span_t const& span
                        , Constituents const& range
                        , info_type result
                        , ScoreOutputIterator scores_out ) const
    {
        //sbmt::io::scoped_domain_settings scp(rl_log,sbmt::io::lvl_verbose);
        ScoreOutputIterator ret;
        boost::tie(boost::tuples::ignore,ret) = create_info_(grammar,rule,span,range,scores_out);
        return ret;
    }
    
    template <class Grammar, class Accum>
    Accum scr_var( info_type const& info
                 , int var
                 , Grammar const& grammar
                 , typename Grammar::rule_type rule
                 , Accum accum ) const
    {
        typedef typename var_distribution<Grammar>::type var_distribution_t;
        sbmt::score_t scr = 1.0;
        if (grammar.rule_has_property(rule, var_feature_id)) {
            distribution_t const& 
                d = grammar.template rule_property<var_distribution_t>(rule,var_feature_id)[var];
            scr = d(info.var);
            *accum = std::make_pair(var_weight_id,scr);
            ++accum;
        }
        return accum;
    }
    
    template <class Grammar>
    sbmt::score_t scr_rule( int len
                          , Grammar const& grammar
                          , typename Grammar::rule_type rule
                          , sbmt::span_t const& span ) const
    {
        using namespace sbmt;
        score_t scr = 1.0;
        if (grammar.rule_has_property(rule, rule_feature_id)) {
            typename Grammar::syntax_rule_type const& synrule = grammar.get_syntax(rule);
            typename Grammar::syntax_rule_type::rhs_iterator ritr = synrule.rhs_begin(), rend = synrule.rhs_end();
            for (; ritr != rend; ++ritr) {
                if (is_lexical(ritr->get_token())) --len;
            }

            distribution_t const& d = grammar.template rule_property<distribution_t>(rule,rule_feature_id);
            scr = d(len);
            if (scorefloor >= scr) {
                SBMT_VERBOSE_MSG( rl_log
                             , "rule %s scores under floor for len %s (%s %s)"
                             , print(rule,grammar) % len % d.mean() % d.scale()
                             )
                             ;
            }
//            std::cerr << "** " << synrule.id()
//                               << "[" << span.right() - span.left() << "]"
//                               << "[" << len << "]=" << scr.neglog10() << '\n';
        } 
        return scr;
    }
    
    template <class Grammar, class Constituents,class ScoreAccum>
    boost::tuple<info_type,ScoreAccum>
    create_info_( Grammar const& grammar
                , typename Grammar::rule_type rule
                , sbmt::span_t const& span
                , Constituents const& range
                , ScoreAccum accum
                , bool print = false ) const
    {
        using namespace sbmt;
        info_type ret;
        int idx = 0;
        if (is_lexical(grammar.rule_lhs(rule))) { return boost::make_tuple(ret,accum); }
        typename boost::range_iterator<Constituents>::type 
            citr = boost::begin(range),
            cend = boost::end(range);
        for (size_t ridx = 0; ridx != grammar.rule_rhs_size(rule); ++ridx) {
            while ((citr != cend) and is_lexical(citr->root())) ++citr;
            indexed_token rhs_token = grammar.rule_rhs(rule,ridx);
            if (is_lexical(rhs_token)) { ++ret; }
            else {
                ret += *(citr->info());
                if (use_var_score and is_native_tag(rhs_token)) {
                    accum = scr_var( *citr->info()
                                   , idx
                                   , grammar
                                   , rule
                                   , accum );
                    ++idx;
                }
                ++citr;
            }
        }

        if (is_native_tag(grammar.rule_lhs(rule))) {
            ret = info_type(variable,ret.total);
            if (use_rule_score) {
  	        *accum = std::make_pair( rule_weight_id
				       , scr_rule(ret.var,grammar,rule,span)
				       );
                ++accum;
	    }
        }
        
        return boost::make_tuple(ret,accum);
    }
    
    rlinfo_factory( double rule_weight
                  , double var_weight
                  , size_t rule_weight_id
                  , size_t var_weight_id
                  , size_t rule_feature_id
                  , size_t var_feature_id
		  , bool use_rule
		  , bool use_var )
      : rule_weight(rule_weight)
      , var_weight(var_weight)
      , rule_weight_id(rule_weight_id)
      , var_weight_id(var_weight_id)
      , rule_feature_id(rule_feature_id)
      , var_feature_id(var_feature_id)
      , use_var_score(use_var) 
      , use_rule_score(use_rule) {}
    
    double rule_weight;
    double var_weight;
    size_t rule_weight_id;
    size_t var_weight_id;
    size_t rule_feature_id;
    size_t var_feature_id;
    bool use_var_score;
    bool use_rule_score;
};

////////////////////////////////////////////////////////////////////////////////


static double coeff = 10.0;
static bool use_var = false;
static bool use_rule = true;

struct rlinfo_factory_constructor {    
    sbmt::options_map get_options() 
    {
        sbmt::options_map opts("rule-length info options"); 
        opts.add_option( "rule-length-per-variable"
                       , sbmt::optvar(use_var)
                       , "should info use variable vldist distributions "
                         "default=false"
                       )
                       ;
        opts.add_option( "rule-length-rule-score"
		       , sbmt::optvar(use_rule)
		       , "should info use rule distributions "
                         "default=true"
		       )
	               ;    
        opts.add_option( "rule-length-smoothing-coefficient"
                       , sbmt::optvar(coeff)
                       , "the larger the coeffecient is, "
                         "the fatter the distribution curves. default=10.0"
                       )
                       ;
        return opts;
    }
    void init(sbmt::in_memory_dictionary& dict) {}
    bool set_option(std::string key, std::string value) const { return false; }
    
    template <class Grammar>
    rlinfo_factory construct( Grammar& grammar
                            , sbmt::lattice_tree const& lattice
                            , sbmt::property_map_type pmap ) const
    {
        size_t rule_weight_id = grammar.feature_names().get_index("rule-length");
        double rule_weight = grammar.get_weights()[rule_weight_id];
        size_t rule_feature_id = pmap["rldist"];
        size_t var_weight_id = grammar.feature_names().get_index("var-length");
        double var_weight = grammar.get_weights()[var_weight_id];
        size_t var_feature_id = pmap["vldist"];
        return rlinfo_factory( rule_weight, var_weight
                             , rule_weight_id, var_weight_id
                             , rule_feature_id, var_feature_id
                             , use_rule, use_var );
    }
};

////////////////////////////////////////////////////////////////////////////////

struct make_rule_var_moments {
    typedef std::vector<distribution_t> result_type;
    template <class Dict>
    result_type operator()(Dict& dict, std::string const& s) const
    {
        result_type v;
        std::stringstream sstr(s);
        sstr << boost::tuples::set_delimiter(',');
        moment_t m;
        while(sstr >> m) {
            v.push_back(fattened_distribution(m,coeff));
        }
        return v;
    }
};

struct make_rule_var_params {
    typedef std::vector<distribution_t> result_type;
    template <class Dict>
    result_type operator()(Dict& dict, std::string const& s) const
    {
        result_type v;
        std::stringstream sstr(s);
        sstr << boost::tuples::set_delimiter(',');
        dist_param_t m;
        while(sstr >> m) {
            v.push_back(preset_distribution(m));
        }
        return v;
    }
};

struct make_rule_moments {
    typedef distribution_t result_type;
    template <class Dict>
    result_type operator()(Dict& dict, std::string const& s) const
    {
        std::stringstream sstr(s);
        sstr << boost::tuples::set_delimiter(',');
        moment_t m;
        sstr >> m;
        SBMT_VERBOSE_MSG( rl_log
                        , "rldist={{{%s}}} -> N=%s mean=%s variance=%s"
                        , s % size(m) % mean(m) % variance(m)
                        )
                        ;
        return fattened_distribution(m,coeff);
    }
};

struct make_rule_params {
    typedef distribution_t result_type;
    template <class Dict>
    result_type operator()(Dict& dict, std::string const& s) const
    {
        std::stringstream sstr(s);
        sstr << boost::tuples::set_delimiter(',');
        dist_param_t m;
        sstr >> m;
        return preset_distribution(m);
    }
};

}

#endif  //  RULE_LENGTH__RULE_LENGTH_INFO
