# ifndef   NEURAL_COLLINS__INFO_HPP
# define   NEURAL_COLLINS__INFO_HPP

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

# include <tr1/unordered_set>

# define MAX_CONTEXT 32

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(neural_collins_log,"neural-collins",sbmt::root_domain);

namespace neural_collins {

template <class Grammar>
struct align_data {
    typedef std::vector< boost::tuple<boost::uint8_t,boost::uint8_t> > type;
    typedef type const& return_type;
    static return_type value( Grammar const& grammar
                            , typename Grammar::rule_type r
                            , size_t lmstrid )
    {
        return grammar.template rule_property<type>(r,lmstrid);
    }
};

struct compare_target {
    bool operator()(boost::tuple<boost::uint8_t,boost::uint8_t> b, boost::uint8_t c) const
    {
        return b.get<0>() < c;
    }
    bool operator()(boost::uint8_t c, boost::tuple<boost::uint8_t,boost::uint8_t> b) const
    {
        return c < b.get<0>();
    }
};

typedef std::map< int
                , std::map<sbmt::indexed_token,int>
                > pathmap;

struct info : sbmt::info_base<info> {
    typedef sbmt::indexed_token word_type;
    word_type headword;
    word_type headtag;
    int32_t   src_position;
    int32_t   leftsrcpos;
    int32_t   rightsrcpos;
    
    bool equal_to(info const& other) const
    {
        return other.headword == headword and
               other.headtag == headtag and
               other.src_position == src_position and
	       other.leftsrcpos == leftsrcpos and
	       other.rightsrcpos == rightsrcpos;
    }
    
    info() : headword(sbmt::as_top()), headtag(sbmt::as_top()), src_position(-1), leftsrcpos(-1), rightsrcpos(-1) {}
    
    info(word_type ht, word_type hw, int32_t sp)
    : headword(hw)
    , headtag(ht)
    , src_position(sp)
    , leftsrcpos(sp)
    , rightsrcpos(sp) {}
    
    size_t hash_value() const
    {
        size_t seed(0);
        boost::hash_combine(seed,headword);
        boost::hash_combine(seed,headtag);
        boost::hash_combine(seed,src_position);
	boost::hash_combine(seed,leftsrcpos);
	boost::hash_combine(seed,rightsrcpos);
        return seed;
    }
};

inline
std::ostream& operator << (std::ostream& os, info const& i)
{
    return os <<"("<<i.leftsrcpos<<")("<<i.headtag<<","<<i.headword<<","<<i.src_position<<")("<<i.rightsrcpos<<")";
}

struct deep_equal {
    template <class S, class T>
    bool operator()(T const& t, S const& s) const
    {
        typename boost::range_iterator<T const>::type titr = boost::begin(t);
        typename boost::range_iterator<T const>::type tend = boost::end(t);
        typename boost::range_iterator<T const>::type sitr = boost::begin(s);
        typename boost::range_iterator<T const>::type send = boost::end(s);
        for (; titr != tend or sitr != send; ++titr, ++sitr) if (*sitr != *titr) return false;
        return titr == tend and sitr == send;
    }
};

struct deep_hash {
    typedef size_t result_type;
    template <class T>
    size_t operator()(T const& t) const
    {
        return boost::hash_range(boost::begin(t),boost::end(t));
    }
};

typedef tr1::unordered_map< 
          boost::iterator_range<info const*>
        , int32_t
        , deep_hash
        , deep_equal > modeldict;

struct model_data {
    boost::shared_ptr<nplm_model> model;
    boost::shared_ptr<modeldict> inputdict;
    boost::shared_ptr<modeldict> outputdict;
    uint32_t inputunk;
    uint32_t inputsrcunk;
    uint32_t outputunk;
    uint32_t inputnull;
    uint32_t outputnull;
    uint32_t src_context;
    
    double lookup_ngram(uint32_t const* beg, uint32_t const* end) const
    {
        return model->lookup_ngram(beg,end-beg);
    }
    
    model_data(model_data const& o)
    : model(o.model)
    , inputdict(o.inputdict)
    , outputdict(o.outputdict)
    , inputunk(o.inputunk)
    , inputsrcunk(o.inputsrcunk)
    , outputunk(o.outputunk)
    , inputnull(o.inputnull)
    , outputnull(o.outputnull)
    , src_context(o.src_context) {}
    
    model_data( boost::shared_ptr<nplm_model> model
              , boost::shared_ptr<modeldict> inputdict
              , boost::shared_ptr<modeldict> outputdict
              , uint32_t src_context )
    : model(model)
    , inputdict(inputdict)
    , outputdict(outputdict)
    , inputunk(model ? model->lookup_input_word("<unk>") : 0)
    , inputsrcunk(model ? model->lookup_input_word("<source_unk>") : 0)
    , outputunk(model ? model->lookup_output_word("<unk>") : 0)
    , inputnull(model ? model->lookup_input_word("NULL") : 0)
    , outputnull(model ? model->lookup_output_word("NULL") : 0)
    , src_context(src_context)
    {}
};

template <class Grammar> struct headmarker {};

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
    typedef std::vector<info_type> affiliation_vector;
    template <class Grammar>
    affiliation_vector affiliation_prestring(Grammar& grammar, typename Grammar::rule_type r, info_type const* children) const
    {
        affiliation_vector ret;
        BOOST_FOREACH(typename Grammar::syntax_rule_type::tree_node const& nd, grammar.get_syntax(r).lhs()) {
            if (nd.lexical()) ret.push_back(info(nd.get_token(),nd.get_token(),-1));
            else if (nd.indexed()) ret.push_back(children[nd.index()]);
        }
        return ret;
    }
    
    template <class R>
    affiliation_vector affiliation_correspondence(R const& r, affiliation_vector const& af) const
    {
        affiliation_vector ret;
        int x = 0;
        BOOST_FOREACH(typename R::tree_node const& nd, r.lhs()) {
            if (nd.lexical() or nd.indexed()) {
                ret.push_back(af[x]);
                ++x;
            } else {
                ret.push_back(info());
            }
        }
        return ret;
    }
    
    template <class Gram>
    int
    affiliation_string( typename align_data<Gram>::return_type ts_align
                      , sbmt::span_t spn
                      , info const* children
                      , affiliation_vector::iterator beg
                      , affiliation_vector::iterator itr
                      , affiliation_vector::iterator end ) const
    {
        if (itr == end) return spn.right();
        affiliation_vector::iterator ret = itr;
        for (; itr != end; ++itr) {
            size_t tgt = itr - beg;
            if (is_lexical(itr->headtag)) { // a word, not a variable
                typename align_data<Gram>::type::const_iterator 
                    lower = std::lower_bound(ts_align.begin(),ts_align.end(),tgt,compare_target()),
                    upper = std::upper_bound(ts_align.begin(),ts_align.end(),tgt,compare_target());
                if (lower != upper) {
                    typename align_data<Gram>::type::const_iterator mid = lower + ((upper - lower) / 2);
                    size_t wd = mid->get<1>();
                    *itr = info(itr->headtag,itr->headword,children[wd].src_position);
                } else if (itr != beg) {
                    affiliation_vector::iterator bk = itr; --bk;
                    if (is_lexical(bk->headtag)) {
                        *itr = info(itr->headtag,itr->headword,bk->src_position);
                    }
                    else {
                        *itr = info(itr->headtag,itr->headtag,bk->rightsrcpos);
                    }
                } else {
                    *itr = info(itr->headtag,itr->headword,affiliation_string<Gram>(ts_align,spn,children,beg,++itr,end));
                    break;
                }
            }
        }
        if (is_lexical(ret->headtag)) return ret->src_position;
        else {
            return ret->leftsrcpos;
        }
    }
    
    uint32_t lookup_input(info const* b, info const* e, model_data const& m, bool source = false) const
    {
        modeldict::iterator pos = m.inputdict->find(boost::make_iterator_range(b,e));
        if (pos != m.inputdict->end()) return pos->second;
        else if (source) return m.inputsrcunk;
        else return m.inputunk; 
    }
    
    uint32_t lookup_output(info const* b, info const* e, model_data const& m) const
    {
        modeldict::iterator pos = m.outputdict->find(boost::make_iterator_range(b,e));
        if (pos != m.outputdict->end()) return pos->second;
        else return m.outputunk;
    }
    
    void lookup_input_string(info const* b, info const* e, model_data const& m) const
    {
        SBMT_VERBOSE_EXPR(
          neural_collins_log
        , uint32_t wd = lookup_input(b,e,m);
          BOOST_FOREACH(info x, std::make_pair(b,e)) continue_log(str) << x << ' ';
          continue_log(str) << " -> " << m.model->get_input_vocabulary().words().at(wd);
        );
    }

    void lookup_output_string(info const* b, info const*e, model_data const& m) const
    {
        SBMT_VERBOSE_EXPR(
          neural_collins_log
        , uint32_t wd= lookup_output(b,e,m);
          BOOST_FOREACH(info x, std::make_pair(b,e)) continue_log(str) << x << ' ';
          continue_log(str) << " -> " << m.model->get_output_vocabulary().words().at(wd);
        );
    }

    void log_lookup(uint32_t const* b, uint32_t const* e, model_data const& m) const
    {
        SBMT_VERBOSE_EXPR(
          neural_collins_log
        , uint32_t const* ce = e - 1;
          continue_log(str) << "collins-neural lookup: ";
          for (uint32_t const* cx = b; cx != ce; ++cx) {
              continue_log(str) << m.model->get_input_vocabulary().words().at(*cx) << ' ';
          }
          continue_log(str) << m.model->get_output_vocabulary().words().at(*ce);
        );
    }
    
    //fill src context
    void fillsrc(int spn, uint32_t* beg, uint32_t* end, model_data const& md) const
    {
        if (spn < 0) {
            for (uint32_t* itr = beg; itr != end; ++itr) {
                info i;
                *itr = md.inputnull;
            }
            return;
        }
        int F = end - beg;
        uint32_t* m = beg + (F/2);
        pathmap::const_iterator pos = pmap.find(spn);
        for (; m != end; ++m) {
            if (pos != pmap.end()) {
                sbmt::indexed_token src = pos->second.begin()->first;
                info i(src,src,0);
                *m = lookup_input(&i,&i+1,md,true);
                pos = pmap.find(pos->second.begin()->second);
            } else {
                *m = lookup_input(&send,&send+1,md,true);
            }
        }
        m = beg + ((F/2)-1);
        pos = revpmap.find(spn);
        for (; m != beg -1; --m) {
            if (pos != revpmap.end()) {
                sbmt::indexed_token src = pos->second.begin()->first;
                info i(src,src,0);
                *m = lookup_input(&i,&i+1,md,true);
                pos = revpmap.find(pos->second.begin()->second);
            } else {
                *m = lookup_input(&sstart,&sstart+1,md,true);
            }
        }
    }
    
    
    
    
    template <class Grammar, class Rule, class Prop>
    boost::tuple<info_type,double,double,double> 
    score_info( Grammar const& g
              , Rule const& r
              , typename Rule::tree_node const& n 
              , Prop const& hposvec
              , info_type const* children
              , affiliation_vector const& af ) const
    {
        boost::tuple<info_type,double,double,double> ret(info_type(),0.0,0.0,0.0);
        info lhs(n.get_token(),n.get_token(),0);
        uint32_t hpos = hposvec[&n - r.lhs_begin()];
        info_type rhs[16];
        info_type rhsh[16];
        
        info_type* rhs_itr = rhs;
        info_type* rhsh_itr = rhsh;
        size_t cn = 0;
        if (n.indexed()) {
            ret.get<0>() = children[n.index()];
            //return ret;
        }
        else if (n.children_begin()->lexical()) { // preterminal case
            int p = (&n + 1) - r.lhs_begin();
            ret.get<0>() = info_type(n.get_token(),n.children_begin()->get_token(),af.at(p).src_position);
            //return ret;
        } else {
            info dep;
            BOOST_FOREACH(typename Rule::tree_node const& cnd,n.children()) {
                ++cn;
                double rlscr;
                double hvscr;
                double tvscr;
                info_type hw;
                boost::tie(hw,rlscr,hvscr,tvscr) = score_info(g,r,cnd,hposvec,children,af);
                *rhs_itr = info(cnd.get_token(),cnd.get_token(),hpos);
                ++rhs_itr;
                *rhsh_itr = hw;
                ++rhsh_itr;
                //*rhs_itr = hw;
                //++rhs_itr;

                ret.get<1>() += rlscr;
                ret.get<2>() += hvscr;
                ret.get<3>() += tvscr;
                if (cn == hpos) ret.get<0>() = hw; 
                else dep = hw;
            }
            info_type hw = ret.get<0>();
            info_type t(hw.headtag,hw.headtag,0), w(hw.headword,hw.headword,0);
            info_type dt, dw;
            //if (dep.src_position != -1) {
                dt = info(dep.headtag,dep.headtag,0);
                dw = info(dep.headword,dep.headword,0);
            //}
            info i1; info i2(g.dict().toplevel_tag(),g.dict().toplevel_tag(),-1);
	    assert(i1 == i2);

            
            uint32_t scratch[MAX_CONTEXT];
            
            // construct rule-score
            uint32_t* scratch_itr = scratch;
            *scratch_itr++ = lookup_input(&lhs,&lhs + 1,rhmodel);
            *scratch_itr++ = lookup_input(&t,&t+1,rhmodel);
            *scratch_itr++ = lookup_input(&w,&w+1,rhmodel);
            fillsrc(dep.src_position,scratch_itr,scratch_itr + rhmodel.src_context,rhmodel);
            scratch_itr += rhmodel.src_context;
            *scratch_itr++ = lookup_output(rhs,rhs_itr,rhmodel);
            ret.get<1>() += rhmodel.lookup_ngram(scratch,scratch_itr) / 5.0;
            SBMT_VERBOSE_EXPR(
              neural_collins_log
            , SBMT_EXPR_LOG << sbmt::token_label(g.dict()) << r << '\n';
              SBMT_EXPR_LOG << lhs << " ->";
              BOOST_FOREACH(info rc, std::make_pair(rhs,rhs_itr)) SBMT_EXPR_LOG << " " << rc;
              SBMT_EXPR_LOG << " ###";
              BOOST_FOREACH(info rc, std::make_pair(rhsh,rhsh_itr)) SBMT_EXPR_LOG << " " << rc;
              SBMT_EXPR_LOG << " hpos=" << hpos << " info=" << ret.get<0>();
            ); 
            log_lookup(scratch,scratch_itr,rhmodel);
            if (dep != info()) {
            //construct tag score:
            scratch_itr = scratch;
            *scratch_itr++ = lookup_input(&lhs,&lhs + 1,tvmodel);
            *scratch_itr++ = lookup_input(&t,&t+1,tvmodel);
            *scratch_itr++ = lookup_input(&w,&w+1,tvmodel);
            fillsrc(dep.src_position,scratch_itr,scratch_itr + tvmodel.src_context,tvmodel);
            scratch_itr += tvmodel.src_context;
            *scratch_itr++ = lookup_input(rhs,rhs_itr,tvmodel);
            *scratch_itr++ = lookup_output(&dt,&dt+1,tvmodel);
            ret.get<2>() += tvmodel.lookup_ngram(scratch,scratch_itr) / 1.0;
            log_lookup(scratch,scratch_itr,tvmodel);
            
            //construct word score
            scratch_itr = scratch;
            *scratch_itr++ = lookup_input(&lhs,&lhs + 1,hvmodel);
            *scratch_itr++ = lookup_input(&t,&t+1,hvmodel);
            *scratch_itr++ = lookup_input(&w,&w+1,hvmodel);
            fillsrc(dep.src_position,scratch_itr,scratch_itr + hvmodel.src_context,hvmodel);
            scratch_itr += hvmodel.src_context;
            *scratch_itr++ = lookup_input(rhs,rhs_itr,hvmodel);
            info dt1515 = info(dt.headtag,dt.headword,1515);
            *scratch_itr++ = lookup_input(&dt1515,&dt1515+1,hvmodel);
            *scratch_itr++ = lookup_output(&dw,&dw+1,hvmodel);
            lookup_input_string(&dt1515,&dt1515+1,rhmodel);
            lookup_input_string(&dt1515,&dt1515+1,tvmodel);
            lookup_input_string(&dt1515,&dt1515+1,hvmodel);
            SBMT_VERBOSE_STREAM(neural_collins_log,"dt1515: " << dt1515 << ' ' << "dw: " << dw << "\n");
            ret.get<3>() += hvmodel.lookup_ngram(scratch,scratch_itr) / 5.0 ;
            log_lookup(scratch,scratch_itr,hvmodel);
            }
            //std::cerr << "\n";
            //return ret;
        }
        if (hvmodel.src_context == 0 and tvmodel.src_context == 0 and rhmodel.src_context == 0) {
            ret.get<0>().leftsrcpos = -1;
            ret.get<0>().rightsrcpos = -1;
            ret.get<0>().src_position = -1;
        }
        return ret;
        
    }
    
    template <class Grammar, class Range>
    boost::tuple<info_type,sbmt::score_t,sbmt::score_t,sbmt::score_t>
    score_info( Grammar const& g
              , typename Grammar::rule_type r
              , sbmt::span_t const& spn
              , Range children ) const
    {
        sbmt::indexed_token leftword,rightword;
        int leftsrcpos, rightsrcpos;
        double res = 1.;
        typedef typename headmarker<Grammar>::type headmarker_type;
        headmarker_type const& hmark = g.template rule_property<headmarker_type>(r,headmarkerid);
        typename align_data<Grammar>::return_type alstr = align_data<Grammar>::value(g,r,alignstrid);
        info_type cvec[256];
        info_type* citr = cvec;
        BOOST_FOREACH(sbmt::constituent<info_type> const& ci,children) {
            if (sbmt::is_nonterminal(ci.root())) *citr = *ci.info();
            else *citr = info_type(ci.root(),ci.root(),ci.span().left());
            ++citr;
        }
        affiliation_vector af = affiliation_prestring(g,r,cvec);
        if (hvmodel.src_context or tvmodel.src_context or rhmodel.src_context) {
	    affiliation_string<Grammar>(alstr,spn,cvec,af.begin(),af.begin(),af.end());
        }
        leftsrcpos = af.front().leftsrcpos;
        rightsrcpos = af.back().rightsrcpos;
        af = affiliation_correspondence(r->rule,af);
        // insert mapping of source words here
        
        //std::cerr << r->rule << " children=" << citr - cvec << "\n";
        info_type hw;
        double rhlogprob; 
        double hvlogprob;
        double tvlogprob;
        boost::tie(hw,rhlogprob,tvlogprob,hvlogprob) = score_info(g,r->rule,*r->rule.lhs_root(),hmark,cvec,af);
        hw.leftsrcpos = leftsrcpos; hw.rightsrcpos = rightsrcpos;
        return boost::make_tuple( hw
                                , sbmt::score_t(rhlogprob,sbmt::as_log10())
                                , sbmt::score_t(tvlogprob,sbmt::as_log10())
                                , sbmt::score_t(hvlogprob,sbmt::as_log10())
                                );
    }

public:
    template <class Grammar, class Range>
    result_generator
    create_info( Grammar const& g
               , typename Grammar::rule_type r
               , sbmt::span_t const& spn
               , Range children ) const
    {
        result_type res;
        sbmt::score_t rhscr,hvscr,tvscr,heur;
        info_type in;
        boost::tie(in,rhscr,tvscr,hvscr) = score_info(g,r,spn,children);
        if (separate_features) {
            return boost::make_tuple(in,(rhscr^rh)*(hvscr^hv)*(tvscr^tv),heur);
        } else {
            return boost::make_tuple(in,(rhscr*hvscr*tvscr)^wt,heur);
        }
    }

    template <class Grammar, class Range, class Output, class HeurOutput>
    boost::tuple<Output,HeurOutput>
    component_scores( Grammar const& g
                    , typename Grammar::rule_type r
                    , sbmt::span_t const& spn
                    , Range children
                    , info_type i
                    , Output o
                    , HeurOutput ho ) const
    {
        sbmt::score_t rhscr,tvscr,hvscr;
        info_type in;
        boost::tie(in,rhscr,tvscr,hvscr) = score_info(g,r,spn,children);
        if (separate_features) {
            *o = std::make_pair(rhid,rhscr);
            ++o;
            *o = std::make_pair(hvid,hvscr);
            ++o;
            *o = std::make_pair(tvid,tvscr);
            ++o;
        } else {
            *o = std::make_pair(wtid,rhscr*hvscr*tvscr);
            ++o;
        }
        //*o = std::make_pair(btid,bcr);
        //++o;
        return boost::make_tuple(o,ho);
    }

    template <class Grammar>
    state_factory( Grammar& g
                 , sbmt::property_map_type pmap
                 , sbmt::lattice_tree const& ltree
                 , bool separate_features
                 , model_data rhmodel
                 , model_data hvmodel
                 , model_data tvmodel
                 )
    : headmarkerid(pmap["headmarker"])
    , wtid(g.feature_names().get_index("neural-collins"))
    , rhid(g.feature_names().get_index("neural-collins-gen-rule"))
    , hvid(g.feature_names().get_index("neural-collins-gen-headword"))
    , tvid(g.feature_names().get_index("neural-collins-gen-headtag"))
    , wt(g.get_weights()[wtid])
    , rh(g.get_weights()[rhid])
    , hv(g.get_weights()[hvid])
    , tv(g.get_weights()[tvid])
    , alignstrid(pmap["align"])
    , separate_features(separate_features)
    , rhmodel(rhmodel)
    , hvmodel(hvmodel)
    , tvmodel(tvmodel)
    , sstart(g.dict().foreign_word("<foreign-sentence>"),g.dict().foreign_word("<foreign-sentence>"),0)
    , send(g.dict().foreign_word("</foreign-sentence>"),g.dict().foreign_word("</foreign-sentence>"),0)
    { init(ltree.root(), g.dict()); }
    
    void
    init( sbmt::lattice_tree::node const& lnode
        , sbmt::in_memory_dictionary& dict )
    {
        if (lnode.is_internal()) {
            BOOST_FOREACH(sbmt::lattice_tree::node lchld, lnode.children()) {
                init(lchld,dict);
            }
        } else {
            revpmap[lnode.lat_edge().span.right()][lnode.lat_edge().source] = lnode.lat_edge().span.left();
            pmap[lnode.lat_edge().span.left()][lnode.lat_edge().source] = lnode.lat_edge().span.right();
        }
    }
    size_t headmarkerid;
    size_t wtid;
    size_t rhid;
    size_t hvid;
    size_t tvid;
    size_t alignstrid;
    float wt;
    float rh;
    float hv;
    float tv;
    bool separate_features;
    model_data rhmodel;
    model_data hvmodel;
    model_data tvmodel;
    pathmap pmap;
    pathmap revpmap;
    info sstart;
    info send;
};

boost::tuple<bool,std::vector<info> >
read_wacky(sbmt::in_memory_dictionary& dict, std::string tok)
{
    boost::tuple<bool,std::vector<info> > ret;
    ret.get<0>() = false;
    std::vector<std::string> vs;
    boost::split(vs,tok,boost::is_any_of("_"));
    if (vs.size() > 1) {
        if (vs.back() == "1515") {
            ret.get<0>() = true;
            if (vs[0] == "NULL") ret.get<1>().push_back(info(sbmt::indexed_token(sbmt::as_top()),sbmt::indexed_token(sbmt::as_top()),1515));
            else ret.get<1>().push_back(info(dict.tag(vs[0]),dict.tag(vs[0]),1515));
            //std::cerr << "wacky: " << ret.get<1>()[0] << " -> " << tok << "\n";
        }
        else {
            try { 
                int h = boost::lexical_cast<int>(vs[0]);
                for (size_t x = 1; x != vs.size(); ++x) {
                    if (vs[x] != "NULL") {
                        ret.get<0>() = true;
                        ret.get<1>().push_back(info(dict.tag(vs[x]),dict.tag(vs[x]),h));
                    } else {
                        break;
                    }
                }
            } catch (boost::bad_lexical_cast const& e) {
                ret.get<0>() = false;
            }
        }
    } else if (vs[0] == "NULL") {
        ret.get<0>() = true;
        ret.get<1>().push_back(info());
    }
    return ret;
}

struct state_factory_constructor {
    typedef tr1::unordered_set< std::vector<info>, deep_hash, deep_equal > tok_set;
    tok_set toks;
    
    std::string filename;
    boost::shared_ptr<nplm_model> rlmodel;
    boost::shared_ptr<modeldict> rlmodeldict;
    boost::shared_ptr<modeldict> rlmodeloutdict;
    
    boost::shared_ptr<nplm_model> hvmodel;
    boost::shared_ptr<modeldict> hvmodeldict;
    boost::shared_ptr<modeldict> hvmodeloutdict;
    
    boost::shared_ptr<nplm_model> tvmodel;
    boost::shared_ptr<modeldict> tvmodeldict;
    boost::shared_ptr<modeldict> tvmodeloutdict;
    
    size_t tagmax;
    size_t targetmax;
    size_t sourcemax;
    bool separate_features;
    uint32_t rule_source_context;
    uint32_t tag_source_context;
    uint32_t word_source_context;
    bool rlnormalize;
    bool tvnormalize;
    bool hvnormalize;
    state_factory_constructor()
    : tagmax(0)
    , targetmax(0)
    , sourcemax(0)
    , separate_features(true)
    , rule_source_context(11)
    , tag_source_context(11)
    , word_source_context(0)
    , rlnormalize(true)
    , tvnormalize(true)
    , hvnormalize(false) {}

    sbmt::options_map get_options()
    {
        sbmt::options_map opts("neural-collins options");
        opts.add_option( "neural-collins-model"
                       , sbmt::optvar(filename)
                       , "lexicalized cfg model from collins library"
                       );
        opts.add_option( "neural-collins-separate-features"
                       , sbmt::optvar(separate_features)
                       , "allow separate tuing of lex-cfg-rule-head and lex-cfg-head-var"
                       );
        opts.add_option( "neural-collins-rule-source-context"
                       , sbmt::optvar(rule_source_context)
                       , "number of source words conditioning rule prediction model"
                       );
        opts.add_option("neural-collins-rule-model-normalize"
                       , sbmt::optvar(rlnormalize)
                       , "normalize scores"
                       );               
        opts.add_option( "neural-collins-tag-source-context"
                       , sbmt::optvar(tag_source_context)
                       , "number of source words conditioning tag prediction model"
                       );
        opts.add_option("neural-collins-tag-model-normalize"
                       , sbmt::optvar(tvnormalize)
                       , "normalize scores"
                       );
        opts.add_option( "neural-collins-word-source-context"
                       , sbmt::optvar(word_source_context)
                       , "number of source words conditioning word prediction model"
                       );
        opts.add_option("neural-collins-word-model-normalize"
                       , sbmt::optvar(hvnormalize)
                       , "normalize scores"
                       );
        return opts;
    }

    bool set_option(std::string const& nm, std::string const& vl)
    {
        return false;
    }
    
    int lookup_input_word( boost::shared_ptr<nplm_model> m
                         , std::string const& lbl 
                         , int unk )
    {
        int ret = m->lookup_input_word(lbl);
        if (ret != unk) return ret;
        else {
            if (lbl == "<foreign-sentence>") {
                ret = m->lookup_input_word("<source_s>");
                if (ret != unk) return ret;
                else return m->lookup_input_word("<s>");
            } else if (lbl == "</foreign-sentence>") {
                ret = m->lookup_input_word("</source_s>");
                if (ret != unk) return ret;
                else return m->lookup_input_word("</s>");
            }
            else return ret;
        }
    }
    
    boost::iterator_range<info const*> make_info_token(sbmt::indexed_token it)
    {
        std::vector<info> v(1);
        v[0] = info(it,it,0);
        tok_set::iterator pos = toks.insert(v).first;
        return boost::iterator_range<info const*>(&(pos->front()),&(pos->front())+1);
    }
    
    boost::iterator_range<info const*> make_info_token(std::vector<info> const& v)
    {
        tok_set::iterator pos = toks.insert(v).first;
        return boost::iterator_range<info const*>(&(pos->front()),&(pos->front())+pos->size());
    }
    
    void initwackies( nplm::vocabulary const& v
                    , boost::shared_ptr<modeldict>& md
                    , sbmt::in_memory_dictionary& dict )
    {
        md.reset(new modeldict());
        size_t x = 0;
        BOOST_FOREACH(std::string tok,v.words()) {
            bool keep;
            std::vector<info> v;
            boost::tie(keep,v) = read_wacky(dict,tok);
            if (keep) md->insert(std::make_pair(make_info_token(v),x));
            ++x;
        }
    }
    
    void initmodel( boost::shared_ptr<nplm_model> m
                  , boost::shared_ptr<modeldict> md
                  , boost::shared_ptr<modeldict> omd
                  , sbmt::in_memory_dictionary& dict )
    {
        int unk = m->lookup_input_word("<unk>");
        BOOST_FOREACH(sbmt::indexed_token tok, dict.native_words(targetmax)) {
            int inp = lookup_input_word(m,dict.label(tok),unk);
            md->insert(std::make_pair(make_info_token(tok),inp));
            int oup = m->lookup_output_word(dict.label(tok));
            omd->insert(std::make_pair(make_info_token(tok),oup));

        } 
        {
            sbmt::indexed_token tok = dict.toplevel_tag();
            int inp = lookup_input_word(m,dict.label(tok),unk);
            md->insert(std::make_pair(make_info_token(tok),inp));
            int oup = m->lookup_output_word(dict.label(tok));
            omd->insert(std::make_pair(make_info_token(tok),oup));
        }
        BOOST_FOREACH(sbmt::indexed_token tok, dict.tags(tagmax)) {
            int inp = lookup_input_word(m,dict.label(tok),unk);
            md->insert(std::make_pair(make_info_token(tok),inp));
            int oup = m->lookup_output_word(dict.label(tok));
            omd->insert(std::make_pair(make_info_token(tok),oup));
        }
        BOOST_FOREACH(sbmt::indexed_token tok, dict.foreign_words(sourcemax)) {
            md->insert(std::make_pair(make_info_token(tok),lookup_input_word(m,dict.label(tok),unk)));
        }
        //std::cerr << sbmt::token_label(dict) << "neural-collins input map:\n";
        //for (modeldict::iterator itr = md->begin(); itr != md->end(); ++itr) {
        //    std::cerr << "[";
        //    BOOST_FOREACH(info ix, itr->first) std::cerr << " " << "(" << ix.headword << "," << ix.src_position << ")";
        //    std::cerr << " ] -> " << m->get_input_vocabulary().words().at(itr->second) << "\n";
        //}
        //for (modeldict::iterator itr = omd->begin(); itr != omd->end(); ++itr) {
        //    std::cerr << "[";
        //    BOOST_FOREACH(info ix, itr->first) std::cerr << " " << "(" << ix.headword << "," << ix.src_position << ")";
        //    std::cerr << " ] -> " << m->get_output_vocabulary().words().at(itr->second) << "\n";
        //}
    }

    void init(sbmt::in_memory_dictionary& dict)
    {
        std::cerr << sbmt::token_label(dict);
        if ((not rlmodel) and (filename != "")) {
            SBMT_INFO_STREAM(collins_log,"loading " << filename);
            std::ifstream ifs(filename.c_str());
            std::string name;
            ifs >> name;
            SBMT_INFO_STREAM(collins_log,"rule predictor: "<< name << " normalize: " << rlnormalize);
            rlmodel.reset(new nplm_model(name,'@',rlnormalize));
            ifs >> name;
            SBMT_INFO_STREAM(collins_log,"tag predictor: "<< name << " normalize: " << tvnormalize);
            tvmodel.reset(new nplm_model(name,'@',tvnormalize));
            ifs >> name;
            SBMT_INFO_STREAM(collins_log,"word predictor: "<< name << " normalize: " << hvnormalize);
            hvmodel.reset(new nplm_model(name,'@',hvnormalize));

            initwackies(rlmodel->get_input_vocabulary(),rlmodeldict,dict);
            initwackies(rlmodel->get_output_vocabulary(),rlmodeloutdict,dict);
            initwackies(hvmodel->get_input_vocabulary(),hvmodeldict,dict);
            initwackies(hvmodel->get_output_vocabulary(),hvmodeloutdict,dict);
            initwackies(tvmodel->get_input_vocabulary(),tvmodeldict,dict);
            initwackies(tvmodel->get_output_vocabulary(),tvmodeloutdict,dict);
        }
        if (rlmodel) {
            initmodel(rlmodel,rlmodeldict,rlmodeloutdict,dict);
            initmodel(hvmodel,hvmodeldict,hvmodeloutdict,dict);
            initmodel(tvmodel,tvmodeldict,tvmodeloutdict,dict);
            tagmax = dict.tag_count();
            targetmax = dict.native_word_count();
            sourcemax = dict.foreign_word_count();
        }
    }

    template <class Grammar>
    sbmt::any_type_info_factory<Grammar>
    construct(Grammar& grammar, sbmt::lattice_tree const& lat, sbmt::property_map_type pmap)
    {
        init(grammar.dict());
        return state_factory( grammar
                            , pmap
                            , lat
                            , separate_features
                            , model_data(rlmodel,rlmodeldict,rlmodeloutdict,rule_source_context)
                            , model_data(hvmodel,hvmodeldict,hvmodeloutdict,word_source_context)
                            , model_data(tvmodel,tvmodeldict,tvmodeloutdict,tag_source_context) );
    }
};

} // namespace collins





# endif // NEURAL_COLLINS__INFO_HPP
