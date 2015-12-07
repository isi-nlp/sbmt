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

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(rh_log,"rule-head",sbmt::root_domain);

namespace rule_head {

  template <class Grammar> struct head_map {};
  template <class Grammar> struct variable_head_map {};

  struct state : sbmt::info_base<state> {
    sbmt::indexed_token tag;
    sbmt::indexed_token word;
    static bool ignore_state;

    state() {}

    state(sbmt::indexed_token tag, sbmt::indexed_token word)
      : tag(tag), word(word)
    {
      if (sbmt::is_lexical(tag) or sbmt::is_nonterminal(word)) {
        throw std::logic_error("tag/word reversed in rule_head::state constructor");
      }
    }

    bool equal_to(state const& other) const
    {
      return ignore_state || (tag == other.tag and word == other.word);
    }

    size_t hash_value() const
    {
      size_t x = 0;
      boost::hash_combine(x,tag);
      boost::hash_combine(x,word);
      return x;
    }
  };
bool state::ignore_state = false;

  BOOST_ENUM_VALUES(
                    head_type
                    , const char*
                    , (word)("word")
                    (tag)("tag")
                    );

  template <class M>
  void print_table(std::ostream& out, M& m)
  {
    sbmt::indexed_token t;
    typename M::mapped_type c;
    BOOST_FOREACH(boost::tie(t,c),m) {
      out << t << "=" << c << " ";
    }
  }

  template <class M>
  void print_tables(std::ostream& out, M& m)
  {
    BOOST_FOREACH(typename M::const_reference mm,m) {
      print_table(out,mm);
      out << " ||| ";
    }
  }

  typedef std::tr1::unordered_map<
    sbmt::indexed_token
    , boost::tuple<
        size_t
        , std::tr1::unordered_map<
            sbmt::indexed_token
            , size_t
            >
        >
  > backoff_map;

size_t read_backoff(std::istream& in, backoff_map& mp, sbmt::in_memory_dictionary const& dict, head_type type)
{
  std::string root, head;
  size_t count;
  std::set<std::string> vocab;
  while (in >> root >> head >> count) {
      sbmt::indexed_token roottok = root == "TOP" ? dict.toplevel_tag() : dict.tag(root) ;
      mp[roottok].get<1>()[type == head_type::tag ? dict.tag(head) : dict.native_word(head)] = count;
      mp[roottok].get<0>() += count;
      vocab.insert(head);
  }
  return vocab.size();
}

struct state_factory
{
  typedef state info_type;
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
    //return g.dict().label(i);
    return g.dict().label(i.tag) + "(" + g.dict().label(i.word) + ")";
  }
private:
  template <class Map>
  double mapvalue(Map const& mp, sbmt::indexed_token const& tok) const
  {
    typename Map::const_iterator p = mp.find(tok);
    return p == mp.end() ? 0 : p->second;
  }

  boost::tuple<size_t,size_t,size_t>
  backoff_values(backoff_map const& mp, sbmt::indexed_token root, sbmt::indexed_token head) const
  {
    size_t numer(0);
    size_t denom(0);
    size_t c(0);
    backoff_map::const_iterator p = mp.find(root);
    if (p != mp.end()) {
      denom = p->second.get<0>();
      c = p->second.get<1>().size();
      std::tr1::unordered_map<sbmt::indexed_token,size_t>::const_iterator pp = p->second.get<1>().find(head);
      if (pp != p->second.get<1>().end()) numer = pp->second;
    }
    return boost::make_tuple(numer,denom,c);
  }

  template <class Map>
  boost::tuple<boost::uint32_t,boost::uint32_t>
  smoothers(Map const& mp, sbmt::indexed_token const& tok) const
  {
    typename Map::const_iterator p = mp.find(tok);
    return p == mp.end() ? boost::tuple<boost::uint32_t,boost::uint32_t>(0,0) : p->second;
  }

  // 2-stage WB smoothing
  double prob(double numer, double denom, int c, double backoff_numer, double backoff_denom, int bc, double backoff_backoff) const
  {
    //std::cerr << numer << ' ' << denom << ' ' << c << ' ' << backoff_numer << ' ' << backoff_denom << ' ' << bc << ' ' << backoff_backoff;
    double wt1, wt2;

    if (denom == 0.0) {
      denom = 1.0;
      wt1 = 0.0;
    } else {
      wt1 = denom / (denom + c);
    }
    if (backoff_denom == 0.0) {
      backoff_denom = 1.0;
      wt2 = 0.0;
    } else {
      wt2 = backoff_denom / (backoff_denom + bc);
    }

    double ret = wt1 * ( numer / denom) + (1-wt1) * ( wt2 * (backoff_numer/backoff_denom) + (1-wt2)*backoff_backoff );
    //std::cerr << " -> " << ret;
    return ret;
  }

  // WB smoothing
  double prob(double numer, double denom, int c, double backoff) const
  {
    //std::cerr << ' ' << numer << ' ' << denom << ' ' << c << ' ' << backoff;
    double wt;
    if (denom == 0.0) {
      denom = 1.0;
      wt = 0.0;
    } else {
      wt = denom / (denom + c);
    }
    double ret =  wt * (numer/denom) + (1 - wt) * backoff;
    //std::cerr << " -> " << ret;
    return ret;
  }

  template <class Grammar>
  void print_score_info( Grammar const& g, typename Grammar::rule_type r,
                         sbmt::indexed_token i, size_t hmapid, size_t vhmapid,
                         double p, double count ) const
  {
    ;
    typedef typename head_map<Grammar>::type head_map_type;
    head_map_type const& hmap = g.template rule_property<head_map_type>(r,hmapid);
    typedef typename variable_head_map<Grammar>::type variable_head_map_type;
    variable_head_map_type const& vhmap = g.template rule_property<variable_head_map_type>(r,vhmapid);
    std::cerr << "\n rule-head \n";
    std::cerr << sbmt::token_label(g.dict());
    std::cerr << '\n' << g.get_syntax(r) << " ### count=" << count << '\n';
    std::cerr << '\n' << "rule-denoms: ";
    print_table(std::cerr,hmap);
    std::cerr << '\n' << "var-numers: ";
    print_tables(std::cerr,vhmap);
    std::cerr << '\n';
    std::cerr << "head=" << i << '\n';
    std::cerr << "score=" << p << '\n';
  }

  template <class Grammar>
  void print_score_info( Grammar const& g, typename Grammar::rule_type r, 
                         sbmt::indexed_token i, size_t hmapid, size_t vhmapid,
                         sbmt::indexed_token ci, int xi, double p, double count) const
  {
    print_score_info(g,r,i,hmapid,vhmapid,p,count);
    std::cerr << "child head=" << ci << " map#=" << xi <<'\n';
  }

  template <class Grammar, class Range>
  sbmt::score_t score_info( Grammar const& g
                            , typename Grammar::rule_type r
                            , Range children
                            , info_type i ) const
  {
    double wres = 1.0;
    double tres = 1.0;
    // TODO: only do the find once
    // if no rprob this is an artificial rule, so no rhprob
    if (r->costs.find(rprobid) == r->costs.end())
      return 1.0;
    double rprob = sbmt::score_t(r->costs.find(rprobid)->second,sbmt::as_neglog10()).linear();
    typedef
      typename Grammar::syntax_rule_type::lhs_preorder_iterator
      position_t;
    position_t pos = g.template rule_property<position_t>(r,hwposid);

    typedef typename head_map<Grammar>::type head_map_type;
    head_map_type const& hwmap = g.template rule_property<head_map_type>(r,hwmapid);
    head_map_type const& htmap = g.template rule_property<head_map_type>(r,htmapid);
    typedef typename variable_head_map<Grammar>::type variable_head_map_type;
    variable_head_map_type const& vhwmap = g.template rule_property<variable_head_map_type>(r,vhwmapid);
    variable_head_map_type const& vhtmap = g.template rule_property<variable_head_map_type>(r,vhtmapid);
    // if there is no distribution for the rule or it has no black box head weight, dont score
    if (hwmap.size() > 0 || rprob_lambda == 1.0) {
      double count = r->costs.find(countid)->second;
      double hwdenom, hwunique, htdenom, htunique;
      boost::tie(hwunique,hwdenom) = smoothers(hwmap,i.word);
      boost::tie(htunique,htdenom) = smoothers(htmap,i.tag);
      double hwnumer = count;
      double htnumer = count;
      int idx = 0;
      int vidx = 0;
      BOOST_FOREACH(sbmt::constituent<info_type> c, children) {
        if (sbmt::is_nonterminal(c.root())) {
          if (pos->indexed() and pos->index() == idx) {
            hwnumer = mapvalue(vhwmap[vidx],i.word);
            htnumer = mapvalue(vhtmap[vidx],i.tag);
          } else {
            double cwnumer = mapvalue(vhwmap[vidx],(*c.info()).word);
            double ctnumer = mapvalue(vhtmap[vidx],(*c.info()).tag);
            double cdenom = count;
            size_t bwnumer,bwdenom,bwc;
            size_t btnumer,btdenom,btc;
            boost::tie(bwnumer,bwdenom,bwc) = backoff_values(*wbackoffs,c.root(),
                                                             (*c.info()).word);
            boost::tie(btnumer,btdenom,btc) = backoff_values(*tbackoffs,c.root(),
                                                             (*c.info()).tag);
            double wresp = prob(cwnumer,cdenom,vhwmap[vidx].size(),bwnumer,
                                bwdenom,bwc,wuniform_backoff);
            double tresp = prob(ctnumer,cdenom,vhtmap[vidx].size(),btnumer,
                                btdenom,btc,tuniform_backoff);
            if ( wresp > 1.0 or wresp <= DBL_MIN ) 
              print_score_info(g, r, i.word, hwmapid, vhwmapid, (*c.info()).word, 
                               vidx, wresp, count);
            if ( tresp > 1.0 or tresp <= DBL_MIN ) 
              print_score_info(g, r, i.tag, htmapid, vhtmapid, (*c.info()).tag, 
                               vidx, tresp, count);
            wres *= wresp;
            tres *= tresp;
          }
          ++vidx;
        }
        ++idx;
      }
      double wresp = prob(hwnumer,hwdenom,hwunique,rprob);
      double tresp = prob(htnumer,htdenom,htunique,rprob);
      if (wresp > 1.0 or wresp<= DBL_MIN) 
        print_score_info(g, r, i.word, hwmapid, vhwmapid, wresp, count);
      if (tresp > 1.0 or tresp<= DBL_MIN) 
        print_score_info(g, r, i.tag, htmapid, vhtmapid, tresp, count);
      wres *= wresp;
      tres *= tresp;
      if (wres > 1.0 or wres <= DBL_MIN) {
        std::cerr << "*** wres underflow " << wres << " ***\n";
        print_score_info(g, r, i.word, hwmapid, vhwmapid, wresp, count);
      }
      if (tres > 1.0 or tres <= DBL_MIN) {
        std::cerr << "*** tres underflow " << tres << " ***\n";
        print_score_info(g, r, i.tag, htmapid, vhtmapid, tresp, count);
      }
    }
    
    // p(head|TOP)
    sbmt::indexed_token root = g.get_syntax(r).lhs_root()->get_token();
    if (root.type() == sbmt::top_token) {
        size_t bwnumer,bwdenom,bwc;
        size_t btnumer,btdenom,btc;
        boost::tie(bwnumer,bwdenom,bwc) = backoff_values(*wbackoffs,root,i.word);
        boost::tie(btnumer,btdenom,btc) = backoff_values(*tbackoffs,root,i.tag);
        wres *= prob(bwnumer,bwdenom,bwc,wuniform_backoff);
        tres *= prob(btnumer,btdenom,btc,tuniform_backoff);
    }
    
    double res =  (rprob_lambda*rprob) +
                  (wmodel_lambda*wres) +
                  (tmodel_lambda*tres);
    return sbmt::score_t(-std::log10(res),sbmt::as_neglog10());
  }

  sbmt::indexed_token atrepl(sbmt::indexed_token tok) const
  {
    tr1::unordered_map<sbmt::indexed_token,sbmt::indexed_token>::const_iterator
      pos = atmap->find(tok);
    return pos == atmap->end() ? tok : pos->second;
  }

  template <class Grammar, class Range>
  info_type
  create_info( Grammar const& g
               , typename Grammar::rule_type r
               , Range children ) const
  {
    typedef
      typename Grammar::syntax_rule_type::lhs_preorder_iterator
      position_t;
    position_t pos = g.template rule_property<position_t>(r,hwposid);

    if (not pos->indexed()) {
      return info_type(atrepl((pos - 1)->get_token()), 
                       atrepl(pos->get_token()));
    } else {
      int idx = 0;
      BOOST_FOREACH(sbmt::constituent<info_type> c, children) {
        if (pos->index() == idx) {
          if (pos->get_token() != c.root()) {
            throw std::logic_error("rule_head::state_factory::create_info rule/children mismatch");
          }
          return *c.info();
        }
        ++idx;
      }
      throw std::logic_error("rule_head::state_factory::create_info did not find head position");
    }
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
    res.get<0>() = create_info(g,r,children);
    res.get<1>() = score_info(g,r,children,res.get<0>()) ^ wt;
    return res;
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
    *o = std::make_pair(wtid,score_info(g,r,children,i));
    ++o;
    return boost::make_tuple(o,ho);
  }

  template <class Grammar>

  state_factory(  Grammar& g
                 , sbmt::property_map_type pmap
                 , tr1::unordered_map<sbmt::indexed_token,sbmt::indexed_token> const* atmap
                 , backoff_map const* wbackoffs
                 , backoff_map const* tbackoffs
                 , size_t wvocabsize
                 , size_t tvocabsize
                 , double rprob_lambda
                 , double wmodel_lambda
                 )
    : hwposid(pmap["hwpos"])
    , htmapid(pmap["head_tag_map"])
    , hwmapid(pmap["head_word_map"])
    , vhtmapid(pmap["variable_head_tag_map"])
    , vhwmapid(pmap["variable_head_word_map"])
    , wtid(g.feature_names().get_index("rhprob"))
    , wt(g.get_weights()[wtid])
    , countid(g.feature_names().get_index("rawcount"))
    , rprobid(g.feature_names().get_index("rprob"))
    , atmap(atmap)
    , wbackoffs(wbackoffs)
    , tbackoffs(tbackoffs)
    , wuniform_backoff(1.0/double(wvocabsize))
    , tuniform_backoff(1.0/double(tvocabsize))
    , rprob_lambda(rprob_lambda)
    , wmodel_lambda(wmodel_lambda)
    , tmodel_lambda(1.0-rprob_lambda-wmodel_lambda)
  {  }

  size_t hwposid;
  size_t htmapid;
  size_t hwmapid;
  size_t vhtmapid;
  size_t vhwmapid;
  size_t wtid;
  float wt;
  size_t countid;
  size_t rprobid;
  tr1::unordered_map<sbmt::indexed_token,sbmt::indexed_token> const* atmap;
  backoff_map const* wbackoffs;
  backoff_map const* tbackoffs;
  double wuniform_backoff;
  double tuniform_backoff;
  double rprob_lambda;
  double wmodel_lambda;
  double tmodel_lambda;
};

struct state_factory_constructor {
  std::string word_backoff_table_filename, tag_backoff_table_filename;
  bool ignore_state;
  tr1::unordered_map<sbmt::indexed_token,sbmt::indexed_token> atmap;
  backoff_map wbackoffs;
  backoff_map tbackoffs;
  size_t tagmax;
  size_t targetmax;
  size_t wvocabsize;
  size_t tvocabsize;
  double rprob_lambda;
  double wmodel_lambda;

  state_factory_constructor()
    : tagmax(0)
    , targetmax(0)
    , wvocabsize(0)
    , tvocabsize(0)
    , rprob_lambda(0)
    , wmodel_lambda(1.0)
    , ignore_state(false)
  {  }

  sbmt::options_map get_options()
  {
    sbmt::options_map opts("rule head distribution options");
    opts.add_option("rule-head-rprob-lambda", sbmt::optvar(rprob_lambda), "black box weight given to nonterminal model");
    opts.add_option("rule-head-word-lambda", sbmt::optvar(wmodel_lambda), "black box weight given to word model");
    opts.add_option("rule-head-word-backoff-table",sbmt::optvar(word_backoff_table_filename), "p(head|root) table");
    opts.add_option("rule-head-tag-backoff-table",sbmt::optvar(tag_backoff_table_filename), "p(head|root) table");
    opts.add_option("rule-head-ignore-state",sbmt::optvar(ignore_state), 
                    "set to true to ignore state" "default=false");
    return opts;
  }

  bool set_option(std::string const& nm, std::string const& vl)
  {
    return false;
  }

  void init(sbmt::in_memory_dictionary& dict)
  {
    boost::regex digits("\\d+");
    BOOST_FOREACH(sbmt::indexed_token tok, dict.tags(tagmax)) {
      sbmt::indexed_token attok = dict.tag(boost::regex_replace(dict.label(tok),digits,"@"));
      if (tok != attok) atmap.insert(std::make_pair(tok,attok));
    }
    BOOST_FOREACH(sbmt::indexed_token tok, dict.native_words(targetmax)) {
      sbmt::indexed_token attok = dict.native_word(boost::regex_replace(dict.label(tok),digits,"@"));
      if (tok != attok) atmap.insert(std::make_pair(tok,attok));
    }
    tagmax = dict.tag_count();
    targetmax = dict.native_word_count();
    if (wvocabsize == 0) {
      std::ifstream in(word_backoff_table_filename.c_str());
      wvocabsize = read_backoff(in,wbackoffs,dict,head_type::word);
    }
    if (tvocabsize == 0) {
      std::ifstream in(tag_backoff_table_filename.c_str());
      tvocabsize = read_backoff(in,tbackoffs,dict,head_type::tag);
    }

  }

  template <class Grammar>
  sbmt::any_type_info_factory<Grammar>
  construct(Grammar& grammar, sbmt::lattice_tree const& lat, sbmt::property_map_type pmap)
  {
    init(grammar.dict());
    state::ignore_state = ignore_state;
    return state_factory(grammar, pmap, &atmap, &wbackoffs, &tbackoffs, wvocabsize, tvocabsize, rprob_lambda, wmodel_lambda);
  }
};

} // namespace rule_head



