# include <sbmt/grammar/grammar_in_memory.hpp>
# include <sbmt/grammar/logging.hpp>
# include <boost/multi_index_container.hpp>
# include <boost/multi_index/hashed_index.hpp>
# include <sbmt/hash/oa_hashtable.hpp>
# include <graehl/shared/maybe_update_bound.hpp>
# include <map>
# include <boost/ref.hpp>
# include <boost/foreach.hpp>
# include <gusc/const_any.hpp>
# include <boost/thread/mutex.hpp>
# include <boost/tr1/unordered_map.hpp>
# include <boost/tr1/unordered_set.hpp>
// pe1950
# define GRAMMAR_PUSH_WEIGHTS 0
# define GRAMMAR_PUSH_WEIGHTS_NTHROOT 1
using namespace std;

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
namespace detail {

rule_info::rule_info( indexed_binary_rule const& rule
                    , score_map_type const& scores_
                    , texts_type const& texts_
                    , weight_vector const& weights
                    , score_t heuristic ) //for virtual rules
 : rule(rule)
 , syntax_id(NULL_SYNTAX_ID)
 , scores(scores_)
 , texts(texts_)
 , score(geom(scores,weights))
 , heuristic(heuristic)
 , hidden(false) {}

rule_info::rule_info( indexed_binary_rule const& rule
                    , syntax_id_type sid
                    , score_map_type const& scores_
                    , texts_type const& texts_
                    , weight_vector const& weights
                    , score_t heuristic ) // for real rules
 : rule(rule)
 , syntax_id(sid)
 , scores(scores_)
 , texts(texts_)
 , score(geom(scores,weights))
 , heuristic(heuristic)
 , hidden(false) {}

void rule_info::reweight_scores(weight_vector const& w)
{
    score = geom(scores,w);
}

void rule_info::construct_properties( property_constructors<> const& pc
                                    , indexed_token_factory& dict
                                    , feature_names_type const& fdict )
{
    if(rule.num_properties() == 0) {
        //std::cerr << "yes actually constructing\n";
        rule.properties = pc.construct_properties(dict,texts,fdict);
    }
}

rule_key
rule_info_key_extractor::operator()(rule_info const* info) const
{
        return rule_key( info->rule.rhs(0)
                       , info->rule.lhs().type() == top_token
                       , info->rule.rhs_size() == 1 );
}

rule_key2
rule_info_double_key_extractor::operator()(rule_info const* info) const
{

        return rule_key2( info->rule.rhs(0)
                        , info->rule.rhs_size() > 1 ? info->rule.rhs(1)
                                                    : indexed_token()
                        , info->rule.lhs().type() == top_token
                        , info->rule.rhs_size() == 1 );
}

indexed_token
rule_info_lhs_key_extractor::operator()(rule_info const* info) const
{
    return info->rule.lhs();
}


} // namespace detail

////////////////////////////////////////////////////////////////////////////////

template <class TF>
subgrammar_dict_markers dictionary_boundary(TF& tf)
{
    subgrammar_dict_markers m;
    m.tag_end = *tf.tags().end();
    m.virtual_end = *tf.virtual_tags().end();
    m.foreign_end = *tf.foreign_words().end();
    m.native_end = *tf.native_words().end();
    return m;
}

////////////////////////////////////////////////////////////////////////////////

static inline
detail::rule_key toplevel_unary_key(grammar_in_mem::token_type tok)
{
    return detail::rule_key(tok,true,true);
}

static inline
detail::rule_key unary_key(grammar_in_mem::token_type tok)
{
    return detail::rule_key(tok,false,true);
}

static inline
detail::rule_key toplevel_binary_key(grammar_in_mem::token_type tok)
{
    return detail::rule_key(tok,true,false);
}

static inline
detail::rule_key binary_key(grammar_in_mem::token_type tok)
{
    return detail::rule_key(tok,false,false);
}

static inline
detail::rule_key2 toplevel_binary_key( grammar_in_mem::token_type tok1
                                     , grammar_in_mem::token_type tok2)
{
    return detail::rule_key2(tok1,tok2,true,false);
}

static inline
detail::rule_key2 binary_key( grammar_in_mem::token_type tok1
                            , grammar_in_mem::token_type tok2 )
{
    return detail::rule_key2(tok1,tok2,false,false);
}

////////////////////////////////////////////////////////////////////////////////

namespace detail {

typedef rule_info const* CRP;
typedef std::tr1::unordered_map< indexed_token,CRP,boost::hash<indexed_token> > byroot_t;

struct virtual_rules
{
//    typedef rule_info *RP;

//    typedef std::pair<indexed_token,indexed_token> rhs;
    /* indexed_binary_rule::
       token_t lhs() const { return rule_top.lhs(); }
       token_t rhs(std::size_t x) const { return rule_top.rhs(x); }
    */
    //typedef oa_hash_map<indexed_token,CRP> byroot_t;

    byroot_t byroot;

    void add(CRP r)
    {
//        RP r=const_cast<RP>(cr);
        indexed_token const& root=r->rule.lhs();
        if (root.type()==virtual_tag_token) {
            //std::cerr << sbmt::print(r,gram) << '\n';
            //assert(byroot.find(root) == byroot.end());
            byroot.insert(std::make_pair(root,r));
        }
    }

    void improve_heuristic(score_t h,CRP r)
    {
        graehl::maybe_increase_max(const_cast<score_t&>(r->heuristic), h);
        improve_heuristic_recurse(h,r);
    }

    void improve_heuristic_recurse(score_t h, CRP rp)
    {
        indexed_binary_rule const& r=rp->rule;
        for (unsigned i=0;i<r.rhs_size();++i) {
            indexed_token c=r.rhs(i);
            if (c.type()==virtual_tag_token) {
                BOOST_FOREACH(byroot_t::value_type& vt, byroot.equal_range(c)) {
                    improve_heuristic(h,vt.second);
                }
                //improve_heuristic(h,byroot.at_throw(c));
            }
        }
    }

    void init_byroot(grammar_in_mem &gram)
    {
        byroot.clear();
        BOOST_FOREACH(grammar_in_mem::rule_info_multiset const& rim, rules) {
            BOOST_FOREACH(grammar_in_mem::rule_type r, rim) {
                add(r);
            }
        }
        //BOOST_FOREACH(grammar_in_mem::rule_type r, gram.all_rules()) add(r);
    }

    void rescore()
    {
        gram.set_constant_heuristic(as_zero());
        weight_vector heuristic_weights = gram.weights;

        BOOST_FOREACH(grammar_in_mem::rule_info_multiset const& rim, rules)
        BOOST_FOREACH(grammar_in_mem::rule_type rr, rim) {
        //BOOST_FOREACH(grammar_in_mem::rule_type rr, gram.all_rules()) {
            rule_info const& ri=const_cast<rule_info &>(*rr);
            if (ri.rule.lhs().type() == virtual_tag_token) {
                BOOST_FOREACH(feature_vector::value_type const& sp, ri.scores)
                {
                    ////////////////////////////////////////////////////////////
                    //
                    //  brf-level features will not be a part of heuristics
                    //  ie text-length when itg-binarizer binarizes text-length
                    //
                    ////////////////////////////////////////////////////////////
                    //cerr << "detected brf feature "
                    //     << gram.feature_names().get_token(sp.first)
                    //     << endl;
                    heuristic_weights[sp.first] = 0.0;
                }
            }
        }

        BOOST_FOREACH(grammar_in_mem::rule_info_multiset const& rim, rules)
        BOOST_FOREACH(grammar_in_mem::rule_type rr, rim) {
        //BOOST_FOREACH(grammar_in_mem::rule_type rr, gram.all_rules()) {
            rule_info & ri=const_cast<rule_info &>(*rr);
            indexed_binary_rule const&r=ri.rule;
            indexed_token const& root=r.lhs();

            if (root.type() != virtual_tag_token) {
                scored_syntax& ssyn=const_cast<scored_syntax&>(gram.get_scored_syntax(ri.syntax_id));
                ri.heuristic = gram.weighted_prior(root);
                ssyn.score_ = geom(ri.scores,heuristic_weights);
                improve_heuristic_recurse(ssyn.score_ * ri.heuristic, &ri);
                //ri.heuristic*=gram.tag_prior_bonus; // does not appear in normal heuristic scoring...
            }
        }
    }

    grammar_in_mem &gram;
    std::list<grammar_in_mem::rule_info_multiset>& rules;

    virtual_rules( grammar_in_mem &gram
                 , std::list<grammar_in_mem::rule_info_multiset>& rules )
      : gram(gram)
      , rules(rules)
    {
        init_byroot(gram);
    }

};

typedef std::pair<grammar_in_mem::rule_type,grammar_in_mem::rule_type>
        rule_pair;

typedef std::tr1::unordered_set<grammar_in_mem::rule_type> shared_rule_set;

typedef std::tr1::unordered_map<
          rule_pair
        , int
        , boost::hash<rule_pair>
        > dup_virt_rule_map;

typedef std::tr1::unordered_map<grammar_in_mem::rule_type,shared_rule_set> shared_rule_map;

typedef std::tr1::unordered_map<grammar_in_mem::rule_type,int> count_rule_map;

void count_bins( grammar_in_mem const& g
               , grammar_in_mem::rule_type s
               , grammar_in_mem::rule_type r
               , count_rule_map& cmap
               , shared_rule_map& smap
               , byroot_t const& br
               )
{
    if (is_virtual_tag(g.rule_lhs(r))) {
        ++cmap[r];
        smap[r].insert(s);
    }
    for (size_t x = 0; x != g.rule_rhs_size(r); ++x) {
        if (is_virtual_tag(g.rule_rhs(r,x))) {
            BOOST_FOREACH(byroot_t::value_type const& vt, br.equal_range(g.rule_rhs(r,x))) {
                count_bins(g,s,vt.second,cmap,smap,br);
            }
        }
    }
}



struct virtual_rule_weight_pusher
{
    byroot_t byroot;
    dup_virt_rule_map dvrm;
    shared_rule_map smap;

    void setup_pushing()
    {
        BOOST_FOREACH(grammar_in_mem::rule_info_multiset const& rim, rules) {
            BOOST_FOREACH(grammar_in_mem::rule_type r, rim) {
                if (is_native_tag(gram.rule_lhs(r))) {
                    count_rule_map cm;
                    count_bins(gram,r,r,cm,smap,byroot);
                    BOOST_FOREACH(count_rule_map::value_type vt, cm) {
                        if (vt.second > 1) dvrm[std::make_pair(r,vt.first)] = vt.second;
                    }
                }
            }
        }
    }
    # if GRAMMAR_PUSH_WEIGHTS_NTHROOT
    void push_score(grammar_in_mem::rule_type r)
    {
        rule_info& ri = const_cast<rule_info&>(*r);
        score_t score = 0.0;
        BOOST_FOREACH(grammar_in_mem::rule_type synr,smap[r]) {
            int c = 1;
            dup_virt_rule_map::iterator dpos = dvrm.find(std::make_pair(synr,r));
            if (dpos != dvrm.end()) c = dpos->second;
            int f = gram.get_syntax(synr).rhs_size() - 1;
            double n = 1.0/double(f*c);
            score = std::max(score,pow(gram.get_scored_syntax(synr).score_,n));
        }
        ri.score = ri.score * score;
        BOOST_FOREACH(grammar_in_mem::rule_type synr,smap[r]) {
            int c = 1;
            dup_virt_rule_map::iterator dpos = dvrm.find(std::make_pair(synr,r));
            if (dpos != dvrm.end()) c = dpos->second;
            //score_t& s = const_cast<score_t&>(gram.get_scored_syntax(synr).score_);
            //s = s/(pow(score,double(c)));
            rule_info& si = const_cast<rule_info&>(*synr);
            si.score = si.score/(pow(score,double(c)));
        }
    }
    # else
    void push_score(grammar_in_mem::rule_type r)
    {
        rule_info& ri = const_cast<rule_info&>(*r);
        score_t score = 0.0;
        BOOST_FOREACH(grammar_in_mem::rule_type synr,smap[r]) {
            int c = 1;
            dup_virt_rule_map::iterator dpos = dvrm.find(std::make_pair(synr,r));
            if (dpos != dvrm.end()) c = dpos->second;
            double n = 1.0/double(c);
            score = std::max(score,pow(gram.get_scored_syntax(synr).score_,n));
        }
        ri.score = ri.score * score;
        BOOST_FOREACH(grammar_in_mem::rule_type synr,smap[r]) {
            int c = 1;
            dup_virt_rule_map::iterator dpos = dvrm.find(std::make_pair(synr,r));
            if (dpos != dvrm.end()) c = dpos->second;
            score_t& s = const_cast<score_t&>(gram.get_scored_syntax(synr).score_);
            s = s/(pow(score,double(c)));
            rule_info& si = const_cast<rule_info&>(*synr);
            si.score = si.score/(pow(score,double(c)));
        }
    }
    # endif // GRAMMAR_PUSH_WEIGHTS_NTHROOT
    void push_score_recurse(grammar_in_mem::rule_type r, shared_rule_set& srs)
    {
        if (srs.find(r) != srs.end()) return;
        for (size_t x = 0; x != gram.rule_rhs_size(r); ++x) {
            indexed_token rhs = gram.rule_rhs(r,x);
            if (is_virtual_tag(rhs)) {
                BOOST_FOREACH(byroot_t::value_type& vt,byroot.equal_range(rhs)) {
                    push_score_recurse(vt.second,srs);
                }
            }
        }
        push_score(r);
        srs.insert(r);
    }

    void add(CRP r)
    {
        indexed_token const& root=r->rule.lhs();
        if (root.type()==virtual_tag_token) {

            byroot.insert(std::make_pair(root,r));
        }
    }

    void improve_heuristic(score_t h,CRP r)
    {
        graehl::maybe_increase_max(const_cast<score_t&>(r->heuristic), h);
        improve_heuristic_recurse(h,r);
    }

    void improve_heuristic_recurse(score_t h, CRP rp)
    {
        indexed_binary_rule const& r=rp->rule;
        for (unsigned i=0;i<r.rhs_size();++i) {
            indexed_token c=r.rhs(i);
            if (c.type()==virtual_tag_token) {
                BOOST_FOREACH(byroot_t::value_type& vt, byroot.equal_range(c)) {
                    improve_heuristic(h,vt.second);
                }
                //improve_heuristic(h,byroot.at_throw(c));
            }
        }
    }

    void setup_byroot()
    {
        byroot.clear();
        BOOST_FOREACH(grammar_in_mem::rule_info_multiset const& rim, rules) {
            BOOST_FOREACH(grammar_in_mem::rule_type r, rim) {
                add(r);
            }
        }
        //BOOST_FOREACH(grammar_in_mem::rule_type r, gram.all_rules()) add(r);
    }

    void rescore()
    {
        gram.set_constant_heuristic(as_zero());
        weight_vector heuristic_weights = gram.weights;

        BOOST_FOREACH(grammar_in_mem::rule_info_multiset const& rim, rules)
        BOOST_FOREACH(grammar_in_mem::rule_type rr, rim) {
        //BOOST_FOREACH(grammar_in_mem::rule_type rr, gram.all_rules()) {
            rule_info const& ri=const_cast<rule_info &>(*rr);
            if (ri.rule.lhs().type() == virtual_tag_token) {
                BOOST_FOREACH(feature_vector::value_type const& sp, ri.scores)
                {
                    ////////////////////////////////////////////////////////////
                    //
                    //  brf-level features will not be a part of heuristics
                    //  ie text-length when itg-binarizer binarizes text-length
                    //
                    ////////////////////////////////////////////////////////////
                    //cerr << "detected brf feature "
                    //     << gram.feature_names().get_token(sp.first)
                    //     << endl;
                    heuristic_weights[sp.first] = 0.0;
                }
            }
        }

        BOOST_FOREACH(grammar_in_mem::rule_info_multiset const& rim, rules)
        BOOST_FOREACH(grammar_in_mem::rule_type rr, rim) {
            rule_info & ri=const_cast<rule_info &>(*rr);
            indexed_binary_rule const&r=ri.rule;
            indexed_token const& root=r.lhs();

            // the only heuristic now is the prior.  scores will be pushed
            if (root.type() != virtual_tag_token) {
                scored_syntax& ssyn=const_cast<scored_syntax&>(gram.get_scored_syntax(ri.syntax_id));
                ri.heuristic = gram.weighted_prior(root);
                ssyn.score_ = geom(ri.scores,heuristic_weights);
                improve_heuristic_recurse(ri.heuristic, &ri);
                //ri.heuristic*=gram.tag_prior_bonus; // does not appear in normal heuristic scoring...
            }
        }

        shared_rule_set srs;
        BOOST_FOREACH(grammar_in_mem::rule_info_multiset const& rim, rules)
        BOOST_FOREACH(grammar_in_mem::rule_type rr, rim) {
            if (is_virtual_tag(gram.rule_lhs(rr))) push_score_recurse(rr,srs);
        }
    }

    grammar_in_mem &gram;
    std::list<grammar_in_mem::rule_info_multiset>& rules;

    virtual_rule_weight_pusher( grammar_in_mem &gram
                              , std::list<grammar_in_mem::rule_info_multiset>& rules )
      : gram(gram)
      , rules(rules)
    {
        setup_byroot();
        setup_pushing();
    }

};

////////////////////////////////////////////////////////////////////////////////

class syntax_rule_action
{
 public:
    syntax_rule_action( grammar_in_mem* gram
                      , bool keep_texts = true
                      , bool keep_align = true
                      , bool keep_headmarker=true )
        : fdict(gram->feature_names())
        , gram(gram)
        , weights(gram->weights)
        , keep_texts(keep_texts)
        , keep_align(keep_align)
        , subid(gram->subgrammar_dict_max.size())
        , keep_headmarker(keep_headmarker)
        , mtx(new boost::mutex())
    {}

    void operator()( indexed_syntax_rule const& rule
                   , texts_map_type const& texts )
    {
       // std::cerr << "waddup?\n";
        scored_syntax_ptr s(new scored_syntax( rule
                                             , texts_map_type()
                                             , gram->feature_names().get_index("align")
                                             , keep_texts
                                             , keep_align
                                             , keep_headmarker
                                             )
                           );
        {
            boost::mutex::scoped_lock lk(*mtx);
            gram->syntax_rules.back().insert(s);
            gram->min_syntax_id = std::min(gram->min_syntax_id, rule.id());
        }
    }

    void finish()
    {
    }
 private:
    feature_names_type &fdict;
    grammar_in_mem* gram;
    weight_vector const& weights;
    bool keep_texts,keep_align;
    boost::uint8_t subid;
    bool keep_headmarker;
    boost::shared_ptr<boost::mutex> mtx;
};

////////////////////////////////////////////////////////////////////////////////

class binary_rule_action
{
 public:
    binary_rule_action(grammar_in_mem* gram, bool keep_texts, bool keep_headmarker)
        : gram(gram)
        , subid(gram->subgrammar_dict_max.size())
        , keep_texts(keep_texts)
        , keep_headmarker(keep_headmarker)
        , headmarker_id(gram->feature_names().get_index("headmarker"))
        , mtx(new boost::mutex())
    {
        property_constructors<>::iterator
            i = gram->prop_constructors.begin(),
            e = gram->prop_constructors.end();
        for (; i != e; ++i) prop_names.insert(gram->feature_names().get_index(i->first));
    }

    void operator()( binary_rule<indexed_token> const& rtop
                   , score_map_type scores
                   , texts_map_type const& properties_
                   , vector<syntax_id_type> const& syntax_id )
    {
        //std::cerr << "bwaddup?\n";
        // we only need the properties that match the contents
        // of prop_constructors
        texts_map_type properties;
        texts_map_type::const_iterator
            pri = properties_.begin(),
            pre = properties_.end();
        //std::cerr << "*** binary_rule_action: " << sbmt::print(rtop,gram->dict());
        //if (syntax_id.size() == 1) std::cerr << "id=" << syntax_id.front();
        //std::cerr << "\n";

        for (;pri != pre; ++pri) {
            //std::cerr <<"*** insert-property: " << gram->feature_names().get_token(pri->first) << "\n";
            if ((prop_names.find(pri->first) != prop_names.end())
                or keep_texts
                or (keep_headmarker and pri->first == headmarker_id)) {
                properties.insert(*pri);
                //std::cerr << "prop " << gram->feature_names().get_token(pri->first) << " = " << pri->second << '\n';
            }
        }

        indexed_binary_rule ibr = indexed_binary_rule(rtop.topology());

        if (rtop.lhs().type() == virtual_tag_token) {
            // FIXME: can insert throw?  then auto_ptr?

            rule_info* r = new rule_info( ibr
                                        , scores
                                        , properties
                                        , gram->weights
                                        , as_one()
                                        );

            bool found = false;
            std::list<grammar_in_mem::rule_info_multiset>::iterator
                gitr = gram->rules.begin(),
                gend = --(gram->rules.end());
            for (; gitr != gend; ++gitr) {
                BOOST_FOREACH( detail::rule_info const* pr
                             , gitr->get<grammar_in_mem::double_key>().equal_range(binary_key(rtop.rhs(0),rtop.rhs(1)))
                             ) {
                    if (gram->rule_lhs(pr) == rtop.topology().lhs()) {
                        delete r;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            if (not found) {
                boost::mutex::scoped_lock lk(*mtx);
                gram->rules.back().insert(r);
            }
        } else { // real rule (has single syntax_id)
            assert(syntax_id.size() == 1);
            syntax_id_type id=syntax_id.front();

            rule_info* r = new rule_info( ibr
                                        , id
                                        , scores
                                        , properties
                                        , gram->weights
                                        , gram->weighted_prior(rtop.lhs())
                                        );
            {
                boost::mutex::scoped_lock lk(*mtx);
                gram->rules.back().insert(r);
            }
        }
    }

    void finish()
    {

    }
 private:
    grammar_in_mem* gram;
    boost::uint8_t subid;
    bool keep_texts;
    bool keep_headmarker;
    feature_id_type headmarker_id;
    std::set<boost::uint32_t> prop_names;
    boost::shared_ptr<boost::mutex> mtx;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace detail

////////////////////////////////////////////////////////////////////////////////

grammar_in_mem::token_type grammar_in_mem::rule_lhs(rule_type r) const
{ return r->rule.lhs(); }

bool grammar_in_mem::is_complete_rule(rule_type r) const
{ return is_native_tag(rule_lhs(r)); }

grammar_in_mem::binary_rule_type const&
grammar_in_mem::binary_rule(rule_type r) const
{ return r->rule; }

std::size_t grammar_in_mem::rule_rhs_size(rule_type r) const
{ return r->rule.rhs_size(); }

grammar_in_mem::token_type
grammar_in_mem::rule_rhs(rule_type r, std::size_t index) const
{ return r->rule.rhs(index); }

bool grammar_in_mem::rule_has_property(rule_type r, size_t idx) const
{ return r->rule.has_property(idx); }

score_t grammar_in_mem::rule_score_estimate(rule_type r) const
{ return r->heuristic; }

score_t grammar_in_mem::rule_score(rule_type r) const
{ return r->score; }

feature_vector const& grammar_in_mem::rule_scores(rule_type r) const
{ return r->scores; }

texts_type grammar_in_mem::rule_text(rule_type r) const
{ return r->texts; }

bool grammar_in_mem::equal_rhs2(rule_type r1,rule_type r2) const
{
    indexed_binary_rule const& b1=r1->rule;
    indexed_binary_rule const& b2=r2->rule;
    assert(b1.rhs_size()==2 && b2.rhs_size()==2);
    return  b1.rhs(0)==b2.rhs(0) && b2.rhs(1)==b2.rhs(1);
}

bool grammar_in_mem::not_hidden::operator()(detail::rule_info const* ri) const
{
    return (not ri->hidden) and (ri->heuristic > 0);
}

void
grammar_in_mem::load_prior(std::istream& in, score_t floor, double smooth, double pow, double wt)
{
    std::stringstream sstr;
    std::string line;
    while (getline(in,line)) sstr << line << std::endl;

    weight_tag_prior = wt;
    prior_string = sstr.str();
    prior_floor = floor;
    prior_pow   = pow;
    prior_smooth = smooth;
    recompute_heuristics();
}

void grammar_in_mem::load_prior(score_t floor, double wt)
{
    weight_tag_prior = wt;
    prior_string = "";
    prior_floor = floor;
    prior_smooth = 1.0;
    prior_pow    = 1.0;
    recompute_heuristics();
}

void
grammar_in_mem::swap_impl(grammar_in_mem &o)
{
    using std::swap;
    rules.swap(o.rules);
    tokens.swap(o.tokens);
    feature_dict.swap(o.feature_dict);
    syntax_rules.swap(o.syntax_rules);
    weights.swap(o.weights);
    swap(weight_virtual_completion,o.weight_virtual_completion);
    swap(weight_tag_prior,o.weight_tag_prior);
    swap(prior,o.prior);
    swap(prior_string,o.prior_string);
    std::swap(prior_floor,o.prior_floor);
    swap(prior_smooth,o.prior_smooth);
    swap(prior_pow,o.prior_pow);
}

struct begin_end_extractor2 {
    typedef std::pair<
      grammar_in_mem::rule_info_multiset::index<grammar_in_mem::double_key>::type::iterator
    , grammar_in_mem::rule_info_multiset::index<grammar_in_mem::double_key>::type::iterator
    > result_type;
    result_type operator()(grammar_in_mem::rule_info_multiset const& rm) const
    {
        return result_type( rm.get<grammar_in_mem::double_key>().begin()
                          , rm.get<grammar_in_mem::double_key>().end() );
    }
};

template <class Key>
struct equal_range_extractor2 {
    typedef std::pair<
      grammar_in_mem::rule_info_multiset::index<grammar_in_mem::double_key>::type::iterator
    , grammar_in_mem::rule_info_multiset::index<grammar_in_mem::double_key>::type::iterator
    > result_type;
    result_type operator()(grammar_in_mem::rule_info_multiset const& rm) const
    {
        return rm.get<grammar_in_mem::double_key>().equal_range(k);
    }
    equal_range_extractor2(Key const& k) : k(k) {}
    Key k;
};

template <class Key>
equal_range_extractor2<Key> make_equal_range_extractor2(Key const& k)
{
    return equal_range_extractor2<Key>(k);
}

////////////////////////////////////////////////////////////////////////////////

grammar_in_mem::rule_range grammar_in_mem::all_rules() const
{
    rule_iterator_ b(rules,gusc::begin_end_extractor());
    rule_iterator_ e(rules,gusc::begin_end_extractor(),false);
    return rule_range( boost::make_filter_iterator<not_hidden>(b,e)
                     , boost::make_filter_iterator<not_hidden>(e,e) );
}

////////////////////////////////////////////////////////////////////////////////

grammar_in_mem::rule_range2 grammar_in_mem::all_rules2() const
{
    rule_iterator2_ b(rules,begin_end_extractor2());
    rule_iterator2_ e(rules,begin_end_extractor2(),false);
    return rule_range2( boost::make_filter_iterator<not_hidden>(b,e)
                      , boost::make_filter_iterator<not_hidden>(e,e) );
}

////////////////////////////////////////////////////////////////////////////////

grammar_in_mem::rule_range
grammar_in_mem::unary_rules(grammar_in_mem::token_type rhs) const
{
    rule_iterator_ b(rules,gusc::make_equal_range_extractor(unary_key(rhs)));
    rule_iterator_ e(rules,gusc::make_equal_range_extractor(unary_key(rhs)),false);
    return rule_range( boost::make_filter_iterator<not_hidden>(b,e)
                     , boost::make_filter_iterator<not_hidden>(e,e) );
}

////////////////////////////////////////////////////////////////////////////////

grammar_in_mem::rule_range
grammar_in_mem::binary_rules(grammar_in_mem::token_type rhs0) const
{
    rule_iterator_ b(rules,gusc::make_equal_range_extractor(binary_key(rhs0)));
    rule_iterator_ e(rules,gusc::make_equal_range_extractor(binary_key(rhs0)),false);
    return rule_range( boost::make_filter_iterator<not_hidden>(b,e)
                     , boost::make_filter_iterator<not_hidden>(e,e)
                     );
}

grammar_in_mem::rule_range2
grammar_in_mem::binary_rules(token_type rhs0, token_type rhs1) const
{
    rule_iterator2_ b(rules,make_equal_range_extractor2(binary_key(rhs0,rhs1)));
    rule_iterator2_ e(rules,make_equal_range_extractor2(binary_key(rhs0,rhs1)),false);
    return rule_range2( boost::make_filter_iterator<not_hidden>(b,e)
                      , boost::make_filter_iterator<not_hidden>(e,e)
                      );
}

////////////////////////////////////////////////////////////////////////////////

grammar_in_mem::rule_range
grammar_in_mem::toplevel_unary_rules(grammar_in_mem::token_type rhs) const
{
    rule_iterator_ b(rules,gusc::make_equal_range_extractor(toplevel_unary_key(rhs)));
    rule_iterator_ e(rules,gusc::make_equal_range_extractor(toplevel_unary_key(rhs)),false);

    return rule_range( boost::make_filter_iterator<not_hidden>(b,e)
                     , boost::make_filter_iterator<not_hidden>(e,e)
                     );
}

////////////////////////////////////////////////////////////////////////////////

grammar_in_mem::rule_range
grammar_in_mem::toplevel_binary_rules(grammar_in_mem::token_type rhs0) const
{
    rule_iterator_ b(rules,gusc::make_equal_range_extractor(toplevel_binary_key(rhs0)));
    rule_iterator_ e(rules,gusc::make_equal_range_extractor(toplevel_binary_key(rhs0)),false);
    return rule_range( boost::make_filter_iterator<not_hidden>(b,e)
                     , boost::make_filter_iterator<not_hidden>(e,e)
                     );
}

grammar_in_mem::rule_range2
grammar_in_mem::toplevel_binary_rules(token_type rhs0, token_type rhs1) const
{
    rule_iterator2_ b(rules,make_equal_range_extractor2(toplevel_binary_key(rhs0,rhs1)));
    rule_iterator2_ e(rules,make_equal_range_extractor2(toplevel_binary_key(rhs0,rhs1)),false);
    return rule_range2( boost::make_filter_iterator<not_hidden>(b,e)
                      , boost::make_filter_iterator<not_hidden>(e,e)
                      );
}

////////////////////////////////////////////////////////////////////////////////

grammar_in_mem::grammar_in_mem()
    : weight_tag_prior(1)
    , weight_virtual_completion(1)
    , tag_prior_bonus(as_one())
    , prior_floor(1e-9)
    , prior_smooth(1.0)
    , prior_pow(1.0)
    , prior_string("")
    , min_syntax_id(0)

{}

////////////////////////////////////////////////////////////////////////////////

template<class Iterator>
void delete_rules(Iterator gitr, Iterator gend) {
    for (; gitr != gend; ++gitr) {
        grammar_in_mem::rule_info_multiset::iterator itr = gitr->begin(),
                                                     end = gitr->end();
        for (; itr != end; ++itr) {
            delete const_cast<detail::rule_info*>(*itr);
        }
    }
}

void
grammar_in_mem::load( brf_reader& reader
                    , fat_weight_vector const& combine
                    , property_constructors<dict_t> const& pc
                    , bool keep_texts
                    , bool keep_align
                    , bool keep_headmarker)
{
    min_syntax_id = 0;
    delete_rules(rules.begin(),rules.end());

    subgrammar_dict_max.clear();
    rules.clear();
    syntax_rules.clear();
    tokens.reset_storage(); // wipes out tag-prior map!  even if it wasnt here
                            // loading an archive or text-archive would wipe it.
                            // we want to clear virt,foreign,native words
                            // regardless...
                            // maybe tag-prior needs to be moved out of grammar
                            // and into null-info-factory?
    feature_dict.reset();
    update_weights(combine,pc);
    push(reader, keep_texts, keep_align, keep_headmarker);
}

void
grammar_in_mem::push( brf_reader& reader
                    , bool keep_texts
                    , bool keep_align
                    , bool keep_headmarker )
{
    rules.push_back(rule_info_multiset());
    syntax_rules.push_back(scored_syntax_set());
    detail::syntax_rule_action sa(this,keep_texts,keep_align,keep_headmarker);
    detail::binary_rule_action ba(this,keep_texts,keep_headmarker);
    reader.set_handlers( sa
                       , ba
                       , tokens
                       , feature_dict );
    SBMT_VERBOSE_STREAM(grammar,"begin read keep_texts=" << keep_texts << " keep_align="<< keep_align << " keep_headmarker=" << keep_headmarker << "");
    reader.read();
    sa.finish();
    ba.finish();
    SBMT_VERBOSE_STREAM(grammar,"begin heuristic");
    recompute_heuristics();
    SBMT_VERBOSE_STREAM(grammar,"begin properties");
    rule_info_multiset::iterator b = rules.back().begin();
    rule_info_multiset::iterator e = rules.back().end();
    feature_id_type headmarker_id = feature_names().get_index("headmarker");
    for (; b != e; ++b) {
        //std::cerr << "construct properties for " << print(*b,*this) << "\n";
        const_cast<detail::rule_info*>(*b)->construct_properties(prop_constructors,tokens,feature_dict);
        if (not keep_texts) {
            if (not keep_headmarker) {
                texts_type().swap(const_cast<detail::rule_info*>(*b)->texts);
            } else {
                texts_type only_hm;
                text_feature_vector_byid::const_iterator pos = (*b)->texts.begin(), end_ = (*b)->texts.end();
                for(; pos != end_; ++pos) if (pos->first == headmarker_id) break;
                if (pos != end_) {
                    only_hm.insert(*pos);
                }
                only_hm.swap(const_cast<detail::rule_info*>(*b)->texts);
            }
        }
    }
    subgrammar_dict_max.push_back(dictionary_boundary(tokens));
}

void
grammar_in_mem::pop()
{
    rule_info_multiset::iterator bitr = rules.back().begin(), bend = rules.back().end();
    for (; bitr != bend; ++bitr) {
            detail::rule_info* ri = const_cast<detail::rule_info*>(*bitr);
            delete ri;
    }
    rules.pop_back();
    syntax_rules.pop_back();

    subgrammar_dict_markers m = subgrammar_dict_max.back();
    tokens.shrink(m.foreign_end, m.native_end, m.tag_end, m.virtual_end);
    subgrammar_dict_max.pop_back();
    recompute_heuristics(); // need to undo the heuristic updates
}

void grammar_in_mem::
update_weights(fat_weight_vector const& comb, property_constructors<dict_t> const& pc)
{
    weights = index(comb,feature_dict);
    prop_constructors = pc;
    recompute_scores();
}

size_t grammar_in_mem::size() const
{
    size_t sz = 0;
    BOOST_FOREACH(rule_info_multiset const& rms, rules) {
        sz += rms.size();
    }
    return sz;
}

grammar_in_mem::~grammar_in_mem()
{
    delete_rules(rules.begin(), rules.end());
}

grammar_in_mem::rule_type
grammar_in_mem::insert_scored_syntax( indexed_syntax_rule ri
                                    , score_map_type const& scores
                                    , texts_map_type const& texts )
{
    indexed_token lhs = ri.lhs_root()->get_token();
    indexed_token rhs = ri.rhs_begin()->get_token();
    std::stringstream brstr;
    brstr << print(lhs,tokens) << ":VL: ";
    print_lhs(brstr,ri,tokens);
    brstr << "-> \"HACK\" ### id=" << ri.id();
    brstr << " virtual_label=no complete_subtree=yes lm=yes lm_string={{{";
    indexed_syntax_rule::lhs_preorder_iterator lhsi = ri.lhs_begin(), lhse = ri.lhs_end();
    for (;lhsi != lhse; ++lhsi)
        if (lhsi->lexical())
            brstr << " \"" << print(lhsi->get_token(),tokens) << "\"";
    brstr << "}}}";
    ns_RuleReader::Rule BR(brstr.str());
    (*BR.getRHSLexicalItems())[0] = tokens.label(rhs);
    indexed_binary_rule br(BR,tokens,prop_constructors);

    ri.set_id(min_syntax_id - 1);
    scored_syntax_ptr ssyn(new scored_syntax(ri,texts,feature_names().get_index("align")));

    detail::rule_info* rl = new detail::rule_info( br
                                                 , min_syntax_id - 1
                                                 , scores
                                                 , texts
                                                 , weights
                                                 , weighted_prior(*ssyn)
                                                 );
    ssyn->score_ = rl->score;
    syntax_rules.back().insert(ssyn);
    rl->hidden = true;
    subgrammar_dict_max.back() = dictionary_boundary(tokens);
    rules.back().insert(rl);
    --min_syntax_id;
    return rl;
}

grammar_in_mem::rule_type
grammar_in_mem::insert_terminal_rule( indexed_token tok
                                    , score_map_type const& svec
                                    , texts_map_type const& text_feats
                                    )
{
    rule_topology<indexed_token> rtop(tok,tok);
    std::vector<gusc::const_any>
        props = prop_constructors.construct_properties( tokens
                                                      , boost::make_iterator_range(text_feats)
                                                      , feature_dict
                                                      );

    indexed_binary_rule br(rtop,props);

    detail::rule_info* rl = new detail::rule_info( br
                                                 , svec
                                                 , text_feats
                                                 , weights
                                                 , 1
                                                 );
    rl->hidden = true;
    subgrammar_dict_max.back() = dictionary_boundary(tokens);
    rules.back().insert(rl);
    return rl;
}

void
grammar_in_mem::erase_terminal_rules()
{
    rule_info_multiset::iterator 
        itr = rules.back().begin(), 
        end = rules.back().end();
    for (; itr != end;) {
        rule_info_multiset::iterator pos = itr;
        ++itr;
        if (is_foreign((*pos)->rule.lhs())) {
            detail::rule_info* rl = const_cast<detail::rule_info*>(*pos);
            delete rl;
            rules.back().erase(pos);
        }
    }
}

void grammar_in_mem::recompute_heuristics()
{
    prior.set_string(prior_string,dict(),prior_floor,prior_smooth);
    prior.raise_pow(prior_pow);
    # if GRAMMAR_PUSH_WEIGHTS
    SBMT_VERBOSE_STREAM(grammar,"pushing weights");
    BOOST_FOREACH(rule_info_multiset const& g, rules) {
        BOOST_FOREACH(detail::rule_info const* r, g) {
            const_cast<detail::rule_info *>(r)->reweight_scores(weights);
        }
    }
    detail::virtual_rule_weight_pusher r(*this,rules);
    # else
    detail::virtual_rules r(*this,rules);
    # endif
    r.rescore();
}

void grammar_in_mem::recompute_scores()
{
    BOOST_FOREACH(rule_info_multiset const& g, rules) {
        BOOST_FOREACH(detail::rule_info const* r, g) {
            const_cast<detail::rule_info *>(r)->reweight_scores(weights);
        }
    }
    recompute_heuristics();
}

void grammar_in_mem::set_constant_heuristic(score_t h)
{
    BOOST_FOREACH(rule_info_multiset const& g, rules) {
        BOOST_FOREACH(detail::rule_info const* r, g) {
            const_cast<detail::rule_info *>(r)->heuristic.set(h);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
