# include <sbmt/edge/sentence_info.hpp> // for span_string
# include <sbmt/search/force_sentence_filter.hpp> // for conversion to span_string
# include <sbmt/edge/any_info.hpp>
# include <sbmt/grammar/rule_feature_constructors.hpp>
# include <sbmt/grammar/tree_tags.hpp>
# include <sbmt/edge/constituent.hpp>
# include <boost/range.hpp>
# include <boost/enum.hpp>
# include <boost/foreach.hpp>
# include <boost/iterator/transform_iterator.hpp>
# include <boost/tr1/unordered_map.hpp>
# include <boost/thread/shared_mutex.hpp>
# include <boost/thread/locks.hpp>

namespace sbmt {

struct make_span_strings {
    typedef std::list<sbmt::span_string> result_type;
    
    sbmt::substring_hash_match<sbmt::indexed_token> match;
    
    template <class Constraint>
    make_span_strings(Constraint const& constraint)
      : match(constraint.begin(),constraint.end()){}
      
    template <class Dictionary>
    result_type operator()(Dictionary& dict, std::string const& str)
    {
        sbmt::indexed_lm_string lmstr(str,dict);
        result_type res;
        sbmt::span_strings_from_lm_string(lmstr,match,std::back_inserter(res));
        //BOOST_FOREACH(sbmt::span_string const& spnstr, res) {
        //    std::cerr << "span string: " << str << " -> " << spnstr << '\n';
        //}
        return res;
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class Iterator>
Iterator ith_variable(std::size_t ith, Iterator itr, Iterator const& end)
{
    std::size_t x = 0;
    for (; itr != end; ++itr) {
        if (is_lexical(itr->root())) continue;
        else {
            if (x == ith) break;
            ++x;
        }
    }
    return itr;
}

template <class ItrT> std::pair<sbmt::span_t,bool>
join_span_range_(sbmt::span_string const& sstr, ItrT fbegin, ItrT fend)
{
    using namespace std;
    using namespace sbmt;
    
    /*
        std::cerr << sstr << " ||| ";
        for (ItrT fi = fbegin; fi != fend; ++fi) {
            std::cerr << *(fi->info()) << ' ';
        }
        std::cerr << '\n';
    }
    */

    pair<span_t,bool> retval(span_t(0,0), true);
    span_index_t spn_left  = 0;
    span_index_t spn_right = 0;
    span_string::iterator itr = sstr.begin(),
                          end = sstr.end();
    bool initialized = false;
    bool zero_interaction = false;
    
    for (; itr != end; ++itr) {
        if (!initialized) {
            if (itr->is_index()) {
                ItrT pos = ith_variable(itr->get_index(),fbegin,fend);
                assert(pos != fend);
                if (length(*(pos->info())) == 0) {
                    zero_interaction = true;
                    continue;
                }
                spn_left = pos->info()->left();
                spn_right = pos->info()->right();
            } else {
                span_t const& ss = itr->get_span();
                spn_left = ss.left();
                spn_right = ss.right();
            }
            initialized = true;
        } else {
            if (itr->is_index()) {
                ItrT pos = ith_variable(itr->get_index(),fbegin,fend);
                
                assert(pos != fend);
                if (length(*(pos->info())) == 0) {
                    zero_interaction = true;
                    continue;
                }
                if (pos->info()->left() != spn_right) {
                    retval.second = false;
                    break;
                }
                spn_right = pos->info()->right();
            } else {
                span_t const& ss = itr->get_span();
                if (ss.left() != spn_right) {
                    retval.second = false;
                    break;
                }
                spn_right = ss.right();
            }
        }
    }
    retval.first = span_t(spn_left,spn_right);
    //if (zero_interaction and not retval.second) {
    //    std::cerr << "*** force-info: zero interaction with " << sstr << " --> " << retval.first <<','<<retval.second << '\n';
    //}
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class Constituents, class Syn>
class coverage_generator 
: public gusc::peekable_generator_facade<
    coverage_generator<Constituents,Syn>
  , boost::tuple<sbmt::span_t,sbmt::score_t,sbmt::score_t>
  >
{
public:
    coverage_generator(Syn const* rule, std::list<sbmt::span_string> const& lst, Constituents rng, bool toplevel, sbmt::span_t total)
     : rule(rule), itr(lst.begin()), end(lst.end()), rng(boost::begin(rng),boost::end(rng)), toplevel(toplevel), total(total) 
    {
        pop();
    }
    
    struct get_span {
        typedef sbmt::span_t const& result_type;
 
        result_type operator()(sbmt::constituent<sbmt::span_t> const& c) const
        {
            return *c.info();
        }
    };
private:
    friend class gusc::generator_access;
    Syn const* rule;
    std::list<sbmt::span_string>::const_iterator itr, end;
    std::vector<typename boost::range_value<Constituents>::type> rng;

    boost::tuple<sbmt::span_t,sbmt::score_t,sbmt::score_t> current;
    bool valid;
    bool toplevel;
    sbmt::span_t total;
    bool more() const { return valid; }
    void pop() 
    {
        valid = false;
        using namespace sbmt;
        while (itr != end) {
            std::pair<span_t,bool> 
                res = join_span_range_(*itr, boost::begin(rng), boost::end(rng));
            if (res.second and ((not toplevel) or res.first == total)) {
              current = boost::make_tuple(res.first,1.0,sbmt::score_t(10*(res.first.left()-res.first.right()),sbmt::as_neglog10()));
                /*
                std::cerr << "coverage: " << res.first << " <<< ";
                BOOST_FOREACH(typename boost::range_value<Constituents>::type vvv, rng) std::cerr << (*(vvv.info())) << ' ';
                std::cerr << " + { " << *itr << " }" << *rule << "\n";
                */
                valid = true;
                ++itr;
                break;
            }
            ++itr;
        }
    }
    
    boost::tuple<sbmt::span_t,sbmt::score_t,sbmt::score_t> const& peek() const 
    { 
        return current; 
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
struct force_info_rule_property {
    typedef sbmt::indexed_lm_string type;
    typedef type const& value;
};

template <class Grammar>
struct force_info_factory 
  : sbmt::info_factory_new_component_scores< force_info_factory<Grammar>, false > 
{
    typedef sbmt::span_t info_type;
    typedef gusc::any_generator< boost::tuple<sbmt::span_t,sbmt::score_t,sbmt::score_t> > 
            result_generator;
    
    typedef std::list<sbmt::span_string> feature_type;
    
    std::size_t force_string;
    
    mutable sbmt::substring_hash_match<sbmt::indexed_token>* match;
    
    typedef std::tr1::unordered_map<std::size_t,feature_type, boost::hash<std::size_t> > featmap_type;
    mutable boost::shared_ptr<
         featmap_type
    > featmap;
    
    mutable boost::shared_ptr<boost::shared_mutex> mtx;
    
    sbmt::span_t total;
    
    force_info_factory(std::size_t force_string, sbmt::substring_hash_match<sbmt::indexed_token>& match, sbmt::span_t total) 
      : force_string(force_string)
      , match(&match)
      , featmap(new featmap_type())
      , mtx(new boost::shared_mutex())
      , total(total) {}
    
    feature_type make(Grammar const& grammar, typename Grammar::rule_type rule) const
    {
        feature_type res;
        typedef typename force_info_rule_property<Grammar>::type rule_prop_type;
        span_strings_from_lm_string( 
            grammar.template rule_property<rule_prop_type>(rule,force_string)
          , *match
          , std::back_inserter(res)
        );
        return res;    
    }
    
    feature_type const& findmake(Grammar const& grammar, typename Grammar::rule_type rule) const
    {
        size_t id = grammar.get_syntax(rule).id();
        {
            boost::shared_lock<boost::shared_mutex> readlock(*mtx);
            typename featmap_type::iterator pos = featmap->find(id);
            if (pos != featmap->end()) {
                //std::cerr << "force cache: " << print(rule,grammar) << " :";
                //BOOST_FOREACH(sbmt::span_string const& sss, pos->second) std::cerr << sss << " , ";
                //std::cerr << '\n';
                return pos->second;
            }
        }
        {
            boost::upgrade_lock<boost::shared_mutex> mightwritelock(*mtx);
            typename featmap_type::iterator pos = featmap->find(id);
            if (pos != featmap->end())  {
                //std::cerr << "force cache2: " << print(rule,grammar) << " :";
                //BOOST_FOREACH(sbmt::span_string const& sss, pos->second) std::cerr << sss << " , ";
                //std::cerr << '\n';
                return pos->second;
            }
            else {
                boost::upgrade_to_unique_lock<boost::shared_mutex> writelock(mightwritelock);
                feature_type res;
                typedef typename force_info_rule_property<Grammar>::type rule_prop_type;
                span_strings_from_lm_string( 
                    grammar.template rule_property<rule_prop_type>(rule,force_string)
                  , *match
                  , std::back_inserter(res)
                );
                //std::cerr << "force: " << print(rule,grammar) << " :"; 
                //BOOST_FOREACH(sbmt::span_string const& sss, res) std::cerr << sss << " , ";
                //std::cerr << '\n';
                bool ignore;
                boost::tie(pos,ignore) = featmap->insert(std::make_pair(id,res));
                //std::cerr << "force v2: " << print(rule,grammar) << " :";
                //BOOST_FOREACH(sbmt::span_string const& sss, pos->second) std::cerr << sss << " , ";
                //std::cerr << '\n';
                return pos->second;
            }
        }
    }
    
    template <class Constituents>
    result_generator create_info( Grammar const& grammar
                                , typename Grammar::rule_type rule
                                , sbmt::span_t const&
                                , Constituents const& chldrn ) const
    {
        bool toplevel = grammar.rule_lhs(rule).type() == sbmt::top_token;
        coverage_generator<Constituents,typename Grammar::syntax_rule_type> c(&grammar.get_syntax(rule),findmake(grammar,rule),chldrn,toplevel,total);
        //std::cerr << "more? "<< bool(c) << "\n";
        return c;
    }
    
    template <class OutputIterator, class Constituents>
    OutputIterator
    component_scores_old( Grammar& iterator
                        , typename Grammar::rule_type rule
                        , sbmt::span_t const&
                        , Constituents const& children
                        , sbmt::span_t const& result
                        , OutputIterator out ) const
    {
        return out;
    }
     
    std::string hash_string( Grammar const& grammar
                           , sbmt::span_t const & val ) const
    {
        std::stringstream sstr;
        sstr << val;
        return sstr.str();
    }
    
    bool scoreable_rule( Grammar const& grammar
                       , typename Grammar::rule_type rule ) const
    {
        return grammar.rule_has_property(rule,force_string);
    }
    
    sbmt::score_t rule_heuristic( Grammar const& grammar
                                , typename Grammar::rule_type rule ) const
    {
        if (not scoreable_rule(grammar,rule)) return 1.0;
        else return make(grammar,rule).empty() ? 0.0 : 1.0;
    }
};

struct force_constructor {
    typedef std::list<sbmt::span_string> feature_type;
    BOOST_ENUM_VALUES(
      constraint_type
    , const char*
    , (sentence)("sentence")
      (lisp_etree)("lisp_etree")
      (treebank_etree)("treebank_etree")
    );
    sbmt::options_map get_options()
    {
        sbmt::options_map opts("force decoding:");
        opts.add_option( "force-constraint-type"
                       , sbmt::optvar(type)
                       , "constraint type: sentence | lisp_etree | treebank_etree"
                       );
        
        return opts;
    }
    
    force_constructor() : type(constraint_type::sentence) {}
    
    sbmt::substring_hash_match<sbmt::indexed_token> match;
    std::string constraint;
    constraint_type type;
    
    bool set_option(std::string key, std::string value)
    {   
        using namespace sbmt;
        if (key == "force-constraint") {
            constraint = value;
            return true;
        } else {
            return false;
        }
    }
    void init(in_memory_dictionary& dict) {}
    template <class Grammar>
    force_info_factory<Grammar> 
    construct(Grammar& grammar, sbmt::lattice_tree const&, sbmt::property_map_type pmap)
    {
        using namespace sbmt;
        
        std::vector<indexed_token> c;
        if (type == constraint_type::lisp_etree) {
            c = lisp_tree_tags(constraint,grammar.dict());
        } else if (type == constraint_type::treebank_etree) {
            c = tree_tags(constraint,grammar.dict());
        } else {
            indexed_sentence sent = native_sentence(constraint,grammar.dict());
            c = std::vector<indexed_token>(sent.begin(),sent.end());
        }
        
        match = sbmt::substring_hash_match<indexed_token>(c.begin(),c.end());
        
        std::size_t force_string = type == constraint_type::sentence 
                                 ? pmap["estring"]
                                 : pmap["etree_string"]
                                 ;
        return force_info_factory<Grammar>(force_string,match,sbmt::span_t(0,c.size()));
    }
};

} // namespace sbmt
