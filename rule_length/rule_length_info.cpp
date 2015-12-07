# include <rule_length_info.hpp>

using namespace sbmt;

namespace rule_length {

boost::tuple<double,double> 
fattened_interpolation(moment_t const& moments, double lambda)
{
    double n = size(moments);
    double mu = mean(moments);
    double k1 = n / (n + lambda);
    double k2 = lambda / (n + lambda);
    double s2 = double(moments.get<2>())/n;
    double sigma2 = variance(moments);
    double sigma2hat = k1 * sigma2 + k2 * (sigma2 + s2);

    SBMT_VERBOSE_MSG(
      rl_log
    , "observed:(%s %s %s) new variance: ( %s * %s + %s * (%s + %s) ) -> %s"
    , size(moments) % mean(moments) % sigma2 %
      k1 % sigma2 % k2 % sigma2 % s2 % sigma2hat
    )
    ;

    return boost::make_tuple(mu,sqrt(sigma2hat));
}

distribution_t fattened_distribution(moment_t const& moments, double lambda)
{
    double mean, stdev;
    boost::tie(mean,stdev) = fattened_interpolation(moments, lambda);
    return make_table(boost::math::normal_distribution<float>(mean,stdev));
}

distribution_t preset_distribution(dist_param_t const& params)
{
    double mean, stdev;
    boost::tie(mean,stdev) = params;
    return make_table(boost::math::normal_distribution<float>(mean,stdev));
}
/*    
std::istream& operator>>(std::istream& in, set_prop& p)
{
    in >> std::boolalpha >> p.var;
    if (p.var) {
        SBMT_VERBOSE_STREAM(rl_log, "using feature vldist");
        register_rule_property_constructor("rule-length","vldist",make_rule_var_moments());
    } else {
        SBMT_VERBOSE_STREAM(rl_log, "using feature rldist");
        unregister_rule_property_constructor("rule-length","vldist");
        register_rule_property_constructor("rule-length","rldist",make_rule_moments());
    }
    return in;
}
*/

////////////////////////////////////////////////////////////////////////////////

struct rlinit {
    rlinit()
    {
        register_info_factory_constructor( "rule-length"
                                         , rlinfo_factory_constructor() );
        register_rule_property_constructor("rule-length","rldist",make_rule_moments());
                                         
    }
};

}

static rule_length::rlinit init; 
# if 0
int main(int argc, char** argv)
{
    distribution_map entries = read_distributions(cin);
    typedef distribution_map::value_type entry_t;
    
    BOOST_FOREACH(entry_t e, entries) {
        cout << e.first << '\t' << e.second << endl;
    }
    
    return 0;
}
# endif
