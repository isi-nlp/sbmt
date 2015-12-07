# include <boost/coro/generator.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <boost/foreach.hpp>
# include <boost/bind.hpp>
# include <boost/tokenizer.hpp>
# include <boost/program_options.hpp>
# include <boost/regex.hpp>

# include <xrsparse/xrs.hpp>
# include <sbmt/grammar/syntax_rule.hpp>

# include <sstream>
# include <vector>
# include <string>


namespace coro = boost::coro;

typedef boost::tuple<std::string,std::string> keyval_t;
typedef std::pair<int,std::string> index_key_t;
typedef coro::generator<keyval_t> keyval_generator;
typedef coro::generator<std::string> str_generator;


std::string var(std::string const& str)
{
    /// \note  since -RRB- has nothing after the "-", it is not truncated.
    /// \note  since BAR variables are written @VAR-BAR, they are still 
    ///        recognizable as BAR variables after truncation
    static boost::regex suffix("(-\\w+)+$");
    
    return boost::regex_replace(str,suffix,"");
}

std::string var(sbmt::fat_syntax_rule::tree_node const& nd)
{
    return var(nd.get_token().label());
}

////////////////////////////////////////////////////////////////////////////////

std::string var_lenstr_generator(str_generator::self& gen, rule_data const& rd)
{
    std::string lenstr;
    BOOST_FOREACH(feature const& f, rd.features) {
        if (f.key == "vldist") {
            typedef boost::tokenizer< boost::char_separator<char> > toker;
            boost::char_separator<char> sep(",");
            toker tokens(f.str_value,sep);
            
            toker::iterator itr = tokens.begin(), end = tokens.end();
            for (; itr != end; ++itr) gen.yield(*itr);

            break;
        }
    }
    
    gen.exit();
    return std::string(); // dummy
}

str_generator var_lenstr(rule_data const & rd)
{
    return str_generator(boost::bind(var_lenstr_generator,_1,rd));
}

////////////////////////////////////////////////////////////////////////////////

std::string order_1_key( sbmt::fat_syntax_rule::tree_node const& parent
                       , sbmt::fat_syntax_rule::tree_node const& child )
{
    return var(parent) + "(" + var(child) + ")";
}

struct order_0_keys {
    void operator()( sbmt::fat_syntax_rule::tree_node const& root
                   , std::map<int,std::string>& ikeys)
    {
        using namespace sbmt;
        BOOST_FOREACH(fat_syntax_rule::tree_node const& nd, root.children()) {
            if (nd.indexed()) {
                ikeys[nd.index()] = var(nd);
            } else if (not nd.lexical()) {
                order_0_keys()(nd,ikeys);
            }
        }
    }
};

struct order_1_keys {
    void operator()( sbmt::fat_syntax_rule::tree_node const& root
                   , std::map<int,std::string>& ikeys)
    {
        using namespace sbmt;
        BOOST_FOREACH(fat_syntax_rule::tree_node const& nd, root.children()) {
            if (nd.indexed()) {
                ikeys[nd.index()] = order_1_key(root,nd);
            } else if (not nd.lexical()) {
                order_1_keys()(nd,ikeys);
            }
        }
    }
};

std::string order_2_key( sbmt::fat_syntax_rule::tree_node const& parent
                       , std::string left
                       , sbmt::fat_syntax_rule::tree_node const& child
                       , std::string right )
{
    return var(parent) + "(" + var(left) + "|" + var(child) + "|" + var(right) + ")";
}
////////////////////////////////////////////////////////////////////////////////

struct order_2_keys {
    void operator()( sbmt::fat_syntax_rule::tree_node const& root
                   , std::map<int,std::string>& ikeys)
    {
        using namespace boost;
        std::string left;
        std::string right;
        sbmt::fat_syntax_rule::lhs_children_iterator itr,end,pos;
        boost::tie(itr,end) = root.children();
           
        for (; itr != end; ++itr) {
            if (itr->indexed()) {
                pos = itr;
                ++pos;
                if (pos != end) right = pos->get_token().label();
                else right = "";
                ikeys[itr->index()] = order_2_key(root,left,*itr,right);
            } else if (not itr->lexical()) {
                order_2_keys()(*itr,ikeys);
            }
            left = itr->get_token().label();
        }
    }
};

template <class N>
std::string order_N_features_generator( str_generator::self& gen 
                                      , N nf
                                      , rule_data const& rd )
{
    std::map<int,std::string> ikeys;
    sbmt::fat_syntax_rule rule(rd,sbmt::fat_tf);
    nf(*rule.lhs_root(),ikeys);
    
    BOOST_FOREACH(index_key_t const& p, ikeys) {
        gen.yield(p.second);
    }
    gen.exit();
    return std::string(""); // will not reach.
}

template
std::string order_N_features_generator( str_generator::self& gen 
                                      , order_0_keys nf
                                      , rule_data const& rd );

template
std::string order_N_features_generator( str_generator::self& gen 
                                      , order_1_keys nf
                                      , rule_data const& rd );

template
std::string order_N_features_generator( str_generator::self& gen 
                                      , order_2_keys nf
                                      , rule_data const& rd );
;

template <class N>
str_generator order_N_features(N nf, rule_data const& rd)
{
    return str_generator( bind( order_N_features_generator<N>
                              , _1
                              , nf
                              , rd )
                        );
}

str_generator order_0_features(rule_data const& rd)
{
    return order_N_features(order_0_keys(), rd);
}

str_generator order_1_features(rule_data const& rd)
{
    return order_N_features(order_1_keys(), rd);
}

str_generator order_2_features(rule_data const& rd)
{
    return order_N_features(order_2_keys(), rd);
}

            
template <class N>
keyval_t order_N_keyvals_generator( keyval_generator::self& gen
                                  , N nf
                                  , rule_data const& rd )
{
    str_generator ikeys = order_N_features(nf,rd);
    str_generator varlen = var_lenstr(rd);
    
    while (ikeys && varlen) {
        gen.yield(keyval_t(ikeys(),varlen()));
    } if (varlen or ikeys) {
        throw std::runtime_error(
                  "size mismatch between variables in rule and vlhist feature"
              );
    }
    gen.exit();
    return keyval_t();
}

template
keyval_t order_N_keyvals_generator( keyval_generator::self& gen
                                  , order_0_keys nf
                                  , rule_data const& rd );


template
keyval_t order_N_keyvals_generator( keyval_generator::self& gen
                                  , order_2_keys nf
                                  , rule_data const& rd );

template
keyval_t order_N_keyvals_generator( keyval_generator::self& gen
                                  , order_1_keys nf
                                  , rule_data const& rd );

keyval_generator order_0_keyvals(rule_data const & rd)
{
    return keyval_generator( bind( order_N_keyvals_generator<order_0_keys>
                                 , _1
                                 , order_0_keys()
                                 , rd
                                 )
                           );
}

keyval_generator order_1_keyvals(rule_data const& rd)
{
    return keyval_generator( bind( order_N_keyvals_generator<order_1_keys>
                                 , _1
                                 , order_1_keys()
                                 , rd
                                 )
                           );
}

keyval_generator order_2_keyvals(rule_data const& rd)
{
    return keyval_generator( bind( order_N_keyvals_generator<order_2_keys>
                                 , _1
                                 , order_2_keys()
                                 , rd
                                 )
                           );
}

////////////////////////////////////////////////////////////////////////////////

struct options {
    int order;
    bool feature_string;
};

options parse_options(int argc, char** argv)
{
    bool help = false;
    namespace po = boost::program_options;
    options opts;
    po::options_description desc;
    desc.add_options()
        ( "help,h"
        , po::bool_switch(&help)->default_value(false)
        , "display options"
        )
        ( "order,i"
        , po::value<int>(&opts.order)->default_value(0)
        , "order of classifying which distribution a variable belongs to.\n"
          "  0 - \tclassified by non-terminal.\n"
          "  1 - \tclassified by non-terminal and parent non-terminal.\n"
          "  2 - \tclassified by non-terminal, and parent and immediate sibling non-terminals.\n"
        )
        ( "feature-string,f"
        , po::bool_switch(&opts.feature_string)->default_value(false)
        , "create rule feature-string"
        )
        ;
    try {
        po::variables_map vm;
        store(parse_command_line(argc,argv,desc), vm);
        notify(vm);
        if (help or opts.order < 0 or opts.order > 2) {
            throw 0;
        } 
    } catch(...) {
        std::cout << desc << '\n';
        exit(1);
    }
    return opts;
}


int main(int argc, char** argv) 
{
    options opts = parse_options(argc,argv);
    std::string line;
    // print keyvals as 
    // key \t value
    std::cout << boost::tuples::set_open('\0') 
              << boost::tuples::set_delimiter('\t') 
              << boost::tuples::set_close('\0');
    
    keyval_generator gen;
    str_generator sgen;
    while (getline(std::cin,line)) {
        rule_data rd = parse_xrs(line);
        switch (opts.order) {
            case 0:
                if (not opts.feature_string) gen = order_0_keyvals(rd);
                else sgen = order_0_features(rd);
                break;
            case 1:
                if (not opts.feature_string) gen = order_1_keyvals(rd);
                else sgen = order_1_features(rd);
                break;
            case 2:
                if (not opts.feature_string) gen = order_2_keyvals(rd);
                else sgen = order_2_features(rd);
                break;
        }
        
        if (opts.feature_string) {
	  std::cout << rd.id << '\t' << "vlfeat" << opts.order << "={{{ ";
            while (sgen) std::cout << *sgen++ << ' ';
            std::cout << "}}}\n";
        } else {
            while (gen) {
                std::cout << boost::get<0>(*gen) 
                          << '\t' 
                          << boost::get<1>(*gen) 
                          << '\n';
                ++gen;
            }
        }
    }
    return 0;
}
