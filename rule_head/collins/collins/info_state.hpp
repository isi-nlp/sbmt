# include <sbmt/token.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/edge/info_base.hpp>
# include <sbmt/edge/any_info.hpp>
# include <sbmt/logging.hpp>
# include <sbmt/span.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <string>
# include <boost/functional/hash.hpp>
# include <boost/regex.hpp>
# include <cfloat>
# include <collins/lm.hpp>

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(collins_log,"collins",sbmt::root_domain);

namespace collins {

template <class Grammar> struct headmarker {};

struct info : sbmt::info_base<info> {
    uint32_t word;
    uint32_t tag;
    bool equal_to(info const& other) const
    {
        return word == other.word and tag == other.tag;
    }
    size_t hash_value() const
    {
        size_t seed(0);
        boost::hash_combine(seed,word);
        boost::hash_combine(seed,tag);
        return seed;
    }
};

struct state_factory
{
    typedef info info_type;
    typedef boost::tuple<info_type,sbmt::score_t,sbmt::score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;

    template <class Grammar>
    bool scoreable_rule(Grammar const& g, typename Grammar::rule_type r) const
    {
        return true;
    }

    bool deterministic() const { return true; }

    template <class Grammar>
    sbmt::score_t rule_heuristic( Grammar const& g
                                , typename Grammar::rule_type r ) const
    {
        return 1.0;
    }

    template <class Grammar>
    std::string hash_string( Grammar const& g
                           , info_type i ) const
    {
        return "";
    }
private:
    template <class Rule, class Prop>
    boost::tuple<info_type,double,double,double,double> 
    score_info( Rule const& r
              , typename Rule::tree_node const& n 
              , Prop const& hposvec
              , info_type const* children ) const
    {
        boost::tuple<info_type,double,double,double,double> ret(info_type(),0.0,0.0);
        uint32_t lhs  = dict->find(n.get_token())->second;
        uint32_t hpos = hposvec[&n - r.lhs_begin()];
        uint32_t rhs[24];
        uint32_t* rhs_itr = rhs;
        size_t cn = 0;
        if (n.indexed()) {
            ret.get<0>() = children[n.index()];
            return ret;
        }
        else if (n.children_begin()->lexical()) {
            ret.get<0>().word = dict->find(n.children_begin()->get_token())->second;
            ret.get<0>().tag = dict->find(n.get_token())->second;
            return ret;
        } else {
            BOOST_FOREACH(typename Rule::tree_node const& cnd, n.children()) {
                ++cn;
                double rscr;
                double tscr;
                double wscr;
                double ascr;
                info_type head;
                boost::tie(head,rscr,tscr,wscr,ascr) = score_info(r,cnd,hposvec,children);
                if (cn <= 3) {
                    *rhs_itr++ = dict->find(cnd.get_token())->second;
                    *rhs_itr++ = head.tag;
                    *rhs_itr++ = head.word;
                }
                ret.get<1>() += rscr;
                ret.get<2>() += tscr;
                ret.get<3>() += wscr;
                ret.get<4>() += ascr;
                if (cn == hpos) ret.get<0>() = head; 
            }
        }
        if (cn <= 3 and hpos <= 3) {
            collins::idxrule sr(lhs,hpos,rhs,rhs_itr);
            SBMT_VERBOSE_STREAM(
              collins_log
            , collins::token_label(model.get()) <<
            "scoring:" <<  sr << " -> (" << model->word(ret.get<0>().tag) << ' ' << model->word(ret.get<0>().word)<< ")"
            );
            ret.get<1>() += model->logprob_rule(sr);
            ret.get<2>() += model->logprob_tag(sr);
            ret.get<3>() += model->logprob_word(sr);
            ret.get<4>() += model->logprob_amr(sr);
        }
        return ret;
    }
    
    template <class Grammar, class Range>
    boost::tuple<info_type,sbmt::score_t,sbmt::score_t,sbmt::score_t,sbmt::score_t>
    score_info( Grammar const& g
              , typename Grammar::rule_type r
              , Range children ) const
    {
        double res = 1.;
        SBMT_VERBOSE_STREAM(collins_log, sbmt::token_label(g.dict()) << g.get_syntax(r));
        typedef typename headmarker<Grammar>::type headmarker_type;
        headmarker_type const& hmark = g.template rule_property<headmarker_type>(r,headmarkerid);
        info_type cvec[256];
        info_type* citr = cvec;
        BOOST_FOREACH(sbmt::constituent<info_type> const& ci,children) {
            if (sbmt::is_nonterminal(ci.root())) *citr = *ci.info();
            else *citr = info_type();
            ++citr;
        }
        //std::cerr << r->rule << " children=" << citr - cvec << "\n";
        info_type head;
        double rlogprob;
        double tlogprob; 
        double wlogprob;
        double alogprob;
        boost::tie(head,rlogprob,tlogprob,wlogprob,alogprob) = score_info(r->rule,*r->rule.lhs_root(),hmark,cvec);
        return boost::make_tuple( head
                                , sbmt::score_t(rlogprob,sbmt::as_neglog10())
                                , sbmt::score_t(tlogprob,sbmt::as_neglog10())
                                , sbmt::score_t(wlogprob,sbmt::as_neglog10())
                                , sbmt::score_t(alogprob,sbmt::as_neglog10()) );
    }

public:
    template <class Grammar, class Range>
    result_generator
    create_info( Grammar const& g
               , typename Grammar::rule_type r
               , sbmt::span_t const&
               , Range children ) const
    {
        result_type res;
        sbmt::score_t rscr,tscr,wscr,ascr,heur;
        info_type in;
        boost::tie(in,rscr,tscr,wscr,ascr) = score_info(g,r,children);
        if (separate_features) {
            return boost::make_tuple(in,(rscr^rwt)*(tscr^twt)*(wscr^wwt),heur);
        } else {
            return boost::make_tuple(in,(rscr*tscr*wscr*ascr)^wt,heur);
        }
    }

    template <class Grammar, class Range, class Output, class HeurOutput>
    boost::tuple<Output,HeurOutput>
    component_scores( Grammar const& g
                    , typename Grammar::rule_type r
                    , sbmt::span_t const&
                    , Range children
                    , info_type i
                    , Output o
                    , HeurOutput ho ) const
    {
        sbmt::score_t rscr,tscr,wscr,ascr;
        info_type in;
        boost::tie(in,rscr,tscr,wscr,ascr) = score_info(g,r,children);
        if (separate_features) {
            *o = std::make_pair(rwtid,rscr); ++o;
            *o = std::make_pair(twtid,tscr); ++o;
            *o = std::make_pair(wwtid,wscr); ++o;
        } else {
            *o = std::make_pair(wtid,rscr*tscr*wscr); ++o;
        }
        //*o = std::make_pair(btid,bcr);
        //++o;
        return boost::make_tuple(o,ho);
    }

    template <class Grammar>
    state_factory( Grammar& g
                 , sbmt::property_map_type pmap
                 , tr1::unordered_map<sbmt::indexed_token,uint32_t> const* dict
                 , boost::shared_ptr<collins::model> model
                 , bool separate_features
                 )
    : headmarkerid(pmap["headmarker"])
    , wtid(g.feature_names().get_index("lex-cfg"))
    , rwtid(g.feature_names().get_index("lex-cfg-rule"))
    , twtid(g.feature_names().get_index("lex-cfg-tag"))
    , wwtid(g.feature_names().get_index("lex-cfg-word"))
    , awtid(g.feature_names().get_index("lex-cfg-amr"))
    , wt(g.get_weights()[wtid])
    , rwt(g.get_weights()[rwtid])
    , twt(g.get_weights()[twtid])
    , wwt(g.get_weights()[wwtid])
    , awt(g.get_weights()[awtid])
    , dict(dict)
    , model(model)
    , separate_features(separate_features)
    { }
    size_t headmarkerid;
    size_t wtid;
    size_t rwtid;
    size_t twtid;
    size_t wwtid;
    size_t awtid;
    float wt;
    float rwt;
    float twt;
    float wwt;
    float awt;
    tr1::unordered_map<sbmt::indexed_token,uint32_t> const* dict;
    boost::shared_ptr<collins::model> model;
    bool separate_features;
};

struct state_factory_constructor {
    tr1::unordered_map<sbmt::indexed_token,uint32_t> lmdict;
    std::string filename;
    boost::shared_ptr<collins::model> model;
    size_t tagmax;
    size_t targetmax;
    bool separate_features;
    double rule_wb_rate;
    double tag_wb_rate;
    double word_wb_rate;
    
    state_factory_constructor()
    : tagmax(0)
    , targetmax(0)
    , separate_features(false) 
    , rule_wb_rate(1.0)
    , tag_wb_rate(1.0) 
    , word_wb_rate(1.0) {}

    sbmt::options_map get_options()
    {
        sbmt::options_map opts("rule head distribution options");
        opts.add_option("collins-rule-wb-rate", sbmt::optvar(rule_wb_rate),"");
        opts.add_option("collins-tag-wb-rate", sbmt::optvar(tag_wb_rate),"");
        opts.add_option("collins-word-wb-rate", sbmt::optvar(word_wb_rate),"");
        opts.add_option( "collins-lex-model"
                       , sbmt::optvar(filename)
                       , "lexicalized cfg model from collins library"
                       );
        opts.add_option( "collins-separate-features"
                       , sbmt::optvar(separate_features)
                       , "allow separate tuing of lex-cfg-rule-head and lex-cfg-head-var"
                       );
        return opts;
    }

    bool set_option(std::string const& nm, std::string const& vl)
    {
        return false;
    }

    void init(sbmt::in_memory_dictionary& dict)
    {
        if ((not model) and (filename != "")) {
            SBMT_INFO_STREAM(collins_log,"loading " << filename);
            model.reset(new collins::model(filename,rule_wb_rate,tag_wb_rate,word_wb_rate,1.0));
        }
        if (model) {
            SBMT_INFO_STREAM(collins_log,"mapping dictionaries");
            BOOST_FOREACH(sbmt::indexed_token tok, dict.tags(tagmax)) {
                lmdict.insert(std::make_pair(tok,model->word(dict.label(tok))));
            }
            BOOST_FOREACH(sbmt::indexed_token tok, dict.native_words(targetmax)) {
                lmdict.insert(std::make_pair(tok,model->word(collins::at_replace(dict.label(tok)))));
            }
            lmdict.insert(std::make_pair(dict.toplevel_tag(),model->word("TOP")));
            tagmax = dict.tag_count();
            targetmax = dict.native_word_count();
            SBMT_INFO_STREAM(collins_log,"initialized" << filename);
        }
    }

    template <class Grammar>
    sbmt::any_type_info_factory<Grammar>
    construct(Grammar& grammar, sbmt::lattice_tree const& lat, sbmt::property_map_type pmap)
    {
        init(grammar.dict());
        return state_factory(grammar, pmap, &lmdict, model,separate_features);
    }
};

} // namespace collins



