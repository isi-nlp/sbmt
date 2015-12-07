# include <search/info_registry.hpp>
# include <sbmt/edge/ngram_constructor.hpp>
# include <cluster_info.hpp>
# include <nntm/nntm.hpp>
# include <force_info/force_info.hpp>
# include <rule_head/info_state.hpp>
# include <collins/info_state.hpp>
# include <neural_collins/info.hpp>
# include <srl/agreement/info.hpp>

namespace sbmt {
SBMT_SET_DOMAIN_LOGFILE(root_domain, "-2" );
SBMT_SET_DOMAIN_LOGGING_LEVEL(root_domain, info);
}

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(info_domain,"info-registry",sbmt::root_domain);


namespace ssyn {
    //using grammar_facade;
template <>
struct source_vars<xrsdb::search::grammar_facade> {
    typedef xrsdb::fixed_token_varray type;
};

}

namespace xrsdb { namespace {
    size_t lm_string_id = 0;
    size_t cross_id = 1;
    size_t rldist_id = 2;
    size_t vldist_id = 3;
    size_t taglm_string_id = 4;
    size_t froot_id = 5;
    size_t fvars_id = 6;
    size_t force_tree_id = 7;
    size_t force_string_id = 8;
    size_t leaflm_string_id = 9;
    size_t align_string_id = 10;
    size_t head_word_map_id = 11;
    size_t head_tag_map_id = 12;
    size_t variable_head_word_map_id = 13;
    size_t variable_head_tag_map_id = 14;
    size_t headmarker_id = 15;
} }

namespace xrsdb { namespace search {

template <>
struct rule_property_op<xrsdb::fixed_byte_varray> {
    typedef xrsdb::fixed_byte_varray const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->headmarker;
    }
};
#ifdef XRSDB_HEADRULE
template <>
struct rule_property_op<xrsdb::head_map> {
    typedef xrsdb::head_map const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        if (id == head_word_map_id) return r->hwdm;
        else if (id == head_tag_map_id) return r->htgm;
        else throw std::runtime_error("not-a-head-map");
    }
};

template <>
struct rule_property_op<xrsdb::variable_head_map> {
    typedef xrsdb::variable_head_map const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        if (id == variable_head_word_map_id) return r->vhwdm;
        else if (id == variable_head_tag_map_id) return r->vhtgm;
        else throw std::runtime_error("not-a-variable-head-map");
    }
};
#endif



template <>
struct rule_property_op<xrsdb::target_source_align_varray> {
    typedef xrsdb::target_source_align_varray const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->tgt_src_aligns;
    }
};
}}

template <>
struct align_data<xrsdb::search::grammar_facade> {
    typedef xrsdb::target_source_align_varray type;
    typedef type const& return_type;
    static return_type value( xrsdb::search::grammar_facade const& grammar
                            , xrsdb::search::grammar_facade::rule_type r
                            , size_t lmstrid )
    {
        return grammar.rule_property<type>(r,lmstrid);
    }
};

namespace xrsdb { namespace search {

using namespace sbmt;

template<>
struct rule_property_op<xrsdb::fixed_rule::lhs_preorder_iterator> {
    typedef xrsdb::fixed_rule::lhs_preorder_iterator result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->hwd;
    }
};

template <>
struct rule_property_op<rule_length::distribution_t> {
    typedef rule_length::distribution_t const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->rldist;
    }
};


template <>
struct rule_property_op<sbmt::indexed_token> {
    typedef sbmt::indexed_token const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->froot;
    }
};

template <>
struct rule_property_op<xrsdb::fixed_token_varray> {
    typedef xrsdb::fixed_token_varray const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->fvars;
    }
};

template <>
struct rule_property_op<xrsdb::fixed_rldist_varray> {
    typedef xrsdb::fixed_rldist_varray const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->vldist;
    }
};

template <>
struct rule_property_op<xrsdb::fixed_bool_varray> {
    typedef xrsdb::fixed_bool_varray const& result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        return r->cross;
    }
};

typedef std::map<boost::tuple<sbmt::indexed_token,bool>,sbmt::indexed_token> tokenmap;
std::map<boost::tuple<sbmt::indexed_token,bool>,sbmt::indexed_token> tokmap, alltagmap, notagmap;

struct weak_syntax_constructor : sbmt::ngram_constructor {
    weak_syntax_constructor() : sbmt::ngram_constructor("taglm_string", "taglm-") {}
    template <class Grammar>
    sbmt::any_type_info_factory<Grammar>
    construct( Grammar& gram
             , sbmt::lattice_tree const& lat
             , sbmt::property_map_type const& pm
             )
    {
        sbmt::indexed_token_factory& d = gram.dict();
        tokmap[boost::make_tuple(d.tag("NP"),true)] = d.native_word("[NP]");
        tokmap[boost::make_tuple(d.tag("NP"),false)] = d.native_word("[/NP]");
        tokmap[boost::make_tuple(d.tag("NP-C"),true)] = d.native_word("[NP-C]");
        tokmap[boost::make_tuple(d.tag("NP-C"),false)] = d.native_word("[/NP-C]");
        tokmap[boost::make_tuple(d.tag("S"),true)] = d.native_word("[S]");
        tokmap[boost::make_tuple(d.tag("S"),false)] = d.native_word("[/S]");
        sbmt::any_type_info_factory<Grammar>
            fact = ((sbmt::ngram_constructor*)(this))->construct(gram,lat,pm);
        return fact;
    }
};


struct make_token {
    typedef lm_token<indexed_token> result_type;
    result_type operator()(fixed_rule::tree_node const& nd) const
    {
        if (nd.indexed()) return result_type(rule->rhs2var.find(nd.index())->second);
        else return result_type(nd.get_token());
    }
    rule_application const* rule;
    make_token(rule_application const* rule = 0) : rule(rule) {}
};

typedef std::map<boost::tuple<sbmt::indexed_token,bool>,sbmt::indexed_token> tokmap_t;

struct lmstring_adaptor {
    static std::set<indexed_token> unkset;
    struct leaf {
        bool operator()(fixed_rule::tree_node const& nd) const
        {
            return nd.is_leaf() and unkset.find(nd.get_token()) == unkset.end();
        }
    };

    lmstring_adaptor(rule_application const* rule, size_t id, tokmap_t const* tmap = 0) : rule(rule),tmap(tmap),id(id) {}
    rule_application const* rule;
    tokmap_t const* tmap;
    size_t id;
    typedef boost::transform_iterator<
              make_token 
            , boost::filter_iterator<
                leaf
              , fixed_rule::lhs_preorder_iterator
              >
            > const_iterator_impl;
    typedef boost::any_iterator<lm_token<indexed_token> const,boost::forward_traversal_tag,lm_token<indexed_token> const> const_iterator;
    typedef const_iterator iterator;
    size_t size() const 
    {  
        size_t sz = 0;
        const_iterator b = begin(), e = end();
        for(; b != e; ++b) ++sz;
        return sz;
    }
    const_iterator begin() const
    {
#ifdef XRSDB_LEAFLM
       if ((id == leaflm_string_id) and (rule->leaflmstring.size() != 1 or (not rule->leaflmstring.begin()->is_index()) or rule->leaflmstring.begin()->get_index() != 1)){
           return const_iterator(rule->leaflmstring.begin());
       } else 
#endif
      if (tmap) {
            return const_iterator(weak_syntax_iterator( rule->rule.lhs_begin()
                                                      , rule->rule.lhs_end()
                                                      , tmap
                                                      , rule));
       } else if (rule->lmstring.size() == 1 and rule->lmstring.begin()->is_index() and rule->lmstring.begin()->get_index() == 1)
            return const_iterator(boost::make_transform_iterator(
                     boost::make_filter_iterator(leaf(),rule->rule.lhs_begin(),rule->rule.lhs_end())
                   , make_token(rule)
                   ));
        else return const_iterator(rule->lmstring.begin());
    }
    const_iterator end() const
    {
#ifdef XRSDB_LEAFLM
        if ((id == leaflm_string_id) and (rule->leaflmstring.size() != 1 or (not rule->leaflmstring.begin()->is_index()) or rule->leaflmstring.begin()->get_index() != 1)){
               return const_iterator(rule->leaflmstring.end());
        } else
#endif 
        if (tmap) {
            return const_iterator(weak_syntax_iterator( rule->rule.lhs_end()
                                                      , rule->rule.lhs_end()
                                                      , tmap
                                                      , rule ));
        } else if (rule->lmstring.size() == 1 and rule->lmstring.begin()->is_index() and rule->lmstring.begin()->get_index() == 1)
            return const_iterator(boost::make_transform_iterator(
                     boost::make_filter_iterator(leaf(),rule->rule.lhs_end(),rule->rule.lhs_end())
                   , make_token(rule)
                   ));
        else return const_iterator(rule->lmstring.end());
    }
    bool is_identity() const 
    {
        const_iterator b = begin(), e = end();
        if (b == e) return false;
        if (b->is_token()) return false;
        ++b;
        if (b != e) return false;
        return true;
    }
};

std::ostream& operator << (std::ostream& out, lmstring_adaptor const& lmstr)
{
    bool first = true;
    BOOST_FOREACH(sbmt::lm_token<sbmt::indexed_token> lmtok, lmstr) {
        if (not first) out << ' ';
        out << '[' << lmtok << ']';
        first = false;
    }
    return out;
}

std::set<indexed_token> lmstring_adaptor::unkset;



void make_fullmap(sbmt::in_memory_dictionary& d)
{
    alltagmap[boost::make_tuple(d.toplevel_tag(),true)] = d.native_word("[TOP]");
    alltagmap[boost::make_tuple(d.toplevel_tag(),false)] = d.native_word("[/TOP]");
    BOOST_FOREACH(sbmt::indexed_token tok, d.tags()) {
        alltagmap[boost::make_tuple(tok,true)] = d.native_word("[" + d.label(tok) + "]");
        alltagmap[boost::make_tuple(tok,false)] = d.native_word("[/" + d.label(tok) + "]");
    }
    
    lmstring_adaptor::unkset.clear();
    BOOST_FOREACH(sbmt::indexed_token ut, d.native_words()) {
        if ("@UNKNOWN@" == d.label(ut).substr(0,9)) {
            lmstring_adaptor::unkset.insert(ut);
        }
    }
}

typedef std::vector< lm_token<sbmt::indexed_token> > lmvec;
lmvec& print_tagstring(fixed_rule::tree_node const& nd, tokenmap const& tokmap, rule_application const& r, lmvec& lmv)
{
  typedef lm_token<sbmt::indexed_token> result_type;
  if (nd.indexed()) {
    lmv.push_back(result_type(r.rhs2var.find(nd.index())->second));
  } else if (nd.lexical()) {
    lmv.push_back(result_type(nd.get_token()));
  } else {
    tokenmap::const_iterator pos = tokmap.find(boost::make_tuple(nd.get_token(),true));
    if (pos != tokmap.end()) lmv.push_back(pos->second);
    BOOST_FOREACH(fixed_rule::tree_node const& cnd, nd.children()) {
      print_tagstring(cnd,tokmap,r,lmv);
    }
    pos = tokmap.find(boost::make_tuple(nd.get_token(),false));
    if (pos != tokmap.end()) lmv.push_back(pos->second);
  }
  return lmv;  
}

template <>
struct rule_property_op<lmstring_adaptor> {
    typedef lmstring_adaptor result_type;
    result_type operator()(rule_application const* r, size_t id, grammar_facade const* g) const
    {
        if (id == lm_string_id) return lmstring_adaptor(r,lm_string_id);
        else if (id == taglm_string_id) return lmstring_adaptor(r,taglm_string_id,&tokmap);
        else if (id == leaflm_string_id) return lmstring_adaptor(r,leaflm_string_id);
        else if (id == force_tree_id) return lmstring_adaptor(r,force_tree_id,&alltagmap);
        else return lmstring_adaptor(r,0,&notagmap);
    }
};

}}

namespace sbmt {
template <>
struct ngram_rule_data<xrsdb::search::grammar_facade> {
    typedef xrsdb::search::lmstring_adaptor type;
    typedef type return_type;
    static return_type value( xrsdb::search::grammar_facade const& grammar
                            , xrsdb::search::grammar_facade::rule_type r
                            , size_t lmstrid )
    {
        return grammar.rule_property<type>(r,lmstrid);
    }
};

template <>
struct force_info_rule_property<xrsdb::search::grammar_facade> {
    typedef xrsdb::search::lmstring_adaptor type;
    typedef type value;
};
} // namespace sbmt

namespace rule_head {
template <> struct head_map<xrsdb::search::grammar_facade> {
    typedef xrsdb::head_map type;
};

template <> struct variable_head_map<xrsdb::search::grammar_facade> {
    typedef xrsdb::variable_head_map type;
};
}

namespace collins {
template <> struct headmarker<xrsdb::search::grammar_facade> {
    typedef xrsdb::fixed_byte_varray type;
};
}

namespace neural_collins {
template <> struct headmarker<xrsdb::search::grammar_facade> {
    typedef xrsdb::fixed_byte_varray type;
};
template <>
struct align_data<xrsdb::search::grammar_facade> {
    typedef xrsdb::target_source_align_varray type;
    typedef type const& return_type;
    static return_type value( xrsdb::search::grammar_facade const& grammar
                            , xrsdb::search::grammar_facade::rule_type r
                            , size_t lmstrid )
    {
        return grammar.rule_property<type>(r,lmstrid);
    }
};
}

namespace srl {
template <> struct headmarker<xrsdb::search::grammar_facade> {
    typedef xrsdb::fixed_byte_varray type;
};
template <>
struct align_data<xrsdb::search::grammar_facade> {
    typedef xrsdb::target_source_align_varray type;
    typedef type const& return_type;
    static return_type value( xrsdb::search::grammar_facade const& grammar
                            , xrsdb::search::grammar_facade::rule_type r
                            , size_t lmstrid )
    {
        return grammar.rule_property<type>(r,lmstrid);
    }
};
}

namespace rule_length {
template <>
struct var_distribution<xrsdb::search::grammar_facade> {
    typedef xrsdb::fixed_rldist_varray type;
};
}

namespace xrsdb { namespace search {
    
info_registry_type info_registry;
sbmt::property_map_type pmap;

sbmt::property_map_type& get_property_map()
{
    return pmap;
}

bool info_property(grammar_facade::rule_type r, size_t id)
{
    if (id == lm_string_id) return true;
    else if (id == taglm_string_id) return true;
    else if (id == cross_id) return not r->cross.empty();
    else if (id == vldist_id) return not r->vldist.empty();
    else if (id == rldist_id) return r->rldist.mean() >= 0;
    else if (id == froot_id or id == fvars_id) return r->froot.type() == sbmt::tag_token;
    else if (id == force_tree_id) return true;
    else if (id == force_string_id) return true;
    else if (id == leaflm_string_id) return true;
    else throw std::runtime_error("not-a-property-id");
}

struct initer {
initer()
{
    any_type_info_factory_constructor<grammar_facade> ng = ngram_constructor();
    info_registry.insert(std::make_pair("ngram",ng));
#ifdef XRSDB_LEAFLM    
    any_type_info_factory_constructor<grammar_facade> llm = ngram_constructor("leaflm_string","leaflm-");
    info_registry.insert(std::make_pair("leaflm",llm));
#endif
    any_type_info_factory_constructor<grammar_facade> sg = weak_syntax_constructor();
    info_registry.insert(std::make_pair("taglm",sg));
    
    any_type_info_factory_constructor<grammar_facade> dt = distortion_constructor<fixed_bool_varray>();
    info_registry.insert(std::make_pair("distortion",dt));
    
    any_type_info_factory_constructor<grammar_facade> src = ssyn::constructor("ssyn","froot","fvars");
    info_registry.insert(std::make_pair("ssyn",src));
#ifdef XRSDB_HEADRULE    
    any_type_info_factory_constructor<grammar_facade> rh = rule_head::state_factory_constructor();
    info_registry.insert(std::make_pair("rule-head",rh));
#endif
    any_type_info_factory_constructor<grammar_facade> rl = rule_length::rlinfo_factory_constructor();
    info_registry.insert(std::make_pair("rule-length",rl));
    
    any_type_info_factory_constructor<grammar_facade> fc = sbmt::force_constructor();
    info_registry.insert(std::make_pair("force",fc));
    
    any_type_info_factory_constructor<grammar_facade> cc = sbmt::cluster_constructor();
    info_registry.insert(std::make_pair("cluster",cc));
    
    any_type_info_factory_constructor<grammar_facade> nt = nntm_factory_constructor();
    info_registry.insert(std::make_pair("nntm",nt));
    
    //any_type_info_factory_constructor<grammar_facade> nd = nndm_factory_constructor();
    //info_registry.insert(std::make_pair("nndm",nd));
    
    //any_type_info_factory_constructor<grammar_facade> nf = nnfm_factory_constructor();
    //info_registry.insert(std::make_pair("nnfm",nf));
    
    any_type_info_factory_constructor<grammar_facade> nco = neural_collins::state_factory_constructor();
    info_registry.insert(std::make_pair("neural-collins",nco));
    
    any_type_info_factory_constructor<grammar_facade> co = collins::state_factory_constructor();
    info_registry.insert(std::make_pair("collins",co));
    
    any_type_info_factory_constructor<grammar_facade> srl = srl::info_constructor();
    info_registry.insert(std::make_pair("srl",srl));
    
    pmap["lm_string"] = lm_string_id;
    pmap["leaflm_string"] = leaflm_string_id;
    pmap["taglm_string"] = taglm_string_id;
    pmap["cross"] = cross_id;
    pmap["rldist"] = rldist_id;
    pmap["vldist"] = vldist_id;
    pmap["froot"] = froot_id;
    pmap["fvars"] = fvars_id;
    pmap["etree_string"] = force_tree_id;
    pmap["estring"] = force_string_id;
    pmap["align"] = align_string_id;
    pmap["head_word_map"] = head_word_map_id;
    pmap["variable_head_word_map"] = variable_head_word_map_id;
    pmap["head_tag_map"] = head_tag_map_id;
    pmap["variable_head_tag_map"] = variable_head_tag_map_id;
    pmap["headmarker"] = headmarker_id;
}
};

initer in;

boost::program_options::options_description get_info_options()
{
    boost::program_options::options_description opts(":: info-type options :");
    info_registry_type::iterator itr = info_registry.begin(), 
                                 end = info_registry.end();
    for (; itr != end; ++itr) {
        namespace po = boost::program_options;
        sbmt::options_map omap = itr->second.get_options();
        po::options_description o(omap.title() + " [" + itr->first + "]");
        for (options_map::iterator i = omap.begin(); i != omap.end(); ++i) {
            o.add_options()
              ( i->first.c_str()
              , po::value<std::string>()->notifier(i->second.opt)
              , i->second.description.c_str()
              )
              ;
        }
        opts.add(o);
    }
    return opts;
}

void set_info_option(std::string info, std::string key, std::string val)
{
    SBMT_VERBOSE_STREAM(info_domain,"setting " << info << ':' << key << " = " << val);
    info_registry.find(info)->second.set_option(key,val);
}

void tee_info_option(std::string info, std::string key, std::string val)
{
    set_info_option(info,key,val);
    std::cout << "set-info-option " << info << '  '<< '"' << key << '"' << ' ' << '"' << val << "\" ;\n" << std::endl;
}

typedef any_type_info_factory<grammar_facade> any_xinfo_factory;

std::vector<std::string> info_names(std::string info_names_str)
{
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(",");
    tokenizer tokens(info_names_str, sep);
    std::vector<std::string> retval;
    std::copy(tokens.begin(),tokens.end(),std::back_inserter(retval));
    return retval;
}

gusc::shared_varray<any_xinfo_factory> 
get_info_factories( std::string info_names_str
                  , grammar_facade& gram
                  , lattice_tree& ltree 
                  , property_map_type& pmap )
{
    std::vector<std::string> keys = info_names(info_names_str);
    std::vector<any_xinfo_factory> facts;
    BOOST_FOREACH(std::string key, keys) {
        SBMT_VERBOSE_STREAM(info_domain, "construct info: " << key << "...");
        facts.push_back(info_registry.find(key)->second.construct(gram,ltree,pmap));
        SBMT_VERBOSE_STREAM(info_domain, "info: " << key << " done");
    }
    return gusc::shared_varray<any_xinfo_factory>(facts.begin(),facts.end());
}

void init_info_factories(sbmt::indexed_token_factory& dict)
{
    typedef std::pair< 
              std::string const
            , sbmt::any_type_info_factory_constructor<grammar_facade> 
            > pairt;
    SBMT_VERBOSE_STREAM(info_domain, "initializing infos...");
    for (info_registry_type::iterator pt = info_registry.begin(); pt != info_registry.end(); ++pt) { 
      SBMT_VERBOSE_STREAM(info_domain, "initializing " << pt->first);
      pt->second.init(dict); 
    }
}
}} // namespace xrsdb::search
