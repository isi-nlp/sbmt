# include <mugu/t2s.hpp>

namespace sbmt { namespace t2s {

bool preterminal(indexed_syntax_rule::tree_node const& nd)
{
    return nd.children_begin() != nd.children_end() and
           is_lexical(nd.children_begin()->get_token()) and
           ++nd.children_begin() == nd.children_end();
}

size_t hash_value(internal_state const& s)
{
    return size_t(s.root);
}

bool operator == (internal_state const& s1, internal_state const& s2)
{
    return s1.root == s2.root;
}

void print(std::ostream& out, grammar_state const& state, in_memory_dictionary const& dict)
{
    if (state.virt) out << state.root;
    else out << print(state.root->get_token(),dict);
}

std::ostream& operator << (std::ostream& out, grammar_state const& state)
{
    in_memory_dictionary const* dict = get_dict(out);
    out << print(state,*dict);
    return out;
}

size_t hash_value(grammar_state const& s)
{
    size_t h; 
    if (s.virt) h = size_t(s.root);
    else h = hash_value(s.root->get_token());
    boost::hash_combine(h,s.virt);
    return h;
}

bool operator == (grammar_state const& s1, grammar_state const& s2)
{
    if (s1.virt) return s2.virt and (s1.root == s2.root);
    else return (not s2.virt) and (s1.root->get_token() == s2.root->get_token());
}

bool operator != (grammar_state const& s1, grammar_state const& s2)
{
    return !(s1 == s2);
}

bool operator == (indexed_token const& s1, grammar_state const& s2)
{
    return (s2.virt == false) and s1 == s2.root->get_token();
}

bool operator != (indexed_token const& s1, grammar_state const& s2)
{
    return !(s1 == s2);
}

bool operator == (grammar_state const& s1, indexed_token const& s2)
{
    return s2 == s1;
}

bool operator != (grammar_state const& s1, indexed_token const& s2)
{
    return !(s1 == s2);
}

bool operator > (rule const& r1, rule const& r2)
{
    return r1.score * r1.heur > r2.score * r2.heur;
}

hyperkey make_hyperkey( indexed_syntax_rule const& rule
                      , feature_vector const& fv
                      , indexed_syntax_rule::tree_node const& root
                      , rulepremap_t& rulemap
                      , virtmap_t& virtmap
                      , weight_vector const& wv );
                      
grammar_state find_state( indexed_syntax_rule const& r
                        , feature_vector const& fv
                        , indexed_syntax_rule::tree_node const& root
                        , rulepremap_t& rulemap
                        , virtmap_t& virtmap
                        , weight_vector const& wv )
{   
    grammar_state state;
    if (root.indexed() or is_lexical(root.get_token())) {
        state = grammar_state(root,false);
        return state;
    }
    hyperkey hkey = make_hyperkey(r,fv,root,rulemap,virtmap,wv);
    
    virtmap_t::iterator vpos = virtmap.find(hkey);
    if (vpos != virtmap.end()) {
        state = vpos->second;
    }
    //BOOST_FOREACH(ruledata_t const& rd, rulemap.equal_range(hkey)) {
    //    state = rd.second.get<0>();
    //    if (state.virt) break;
    //}
    if ((&root == r.lhs_root()) or (state.virt == false)) {
        state = grammar_state(root,&root != r.lhs_root());
        rulemap[hkey][state].push_back(rule(state,r,fv,geom(fv,wv)));
        //rulemap.insert(std::make_pair(hkey,boost::make_tuple(state,rule,fv)));
    }
    if (state.virt == true and vpos == virtmap.end()) {
        virtmap.insert(std::make_pair(hkey,state));
    }
    return state;
}

hyperkey make_real_hyperkey(indexed_syntax_rule::tree_node const& root)
{    
    size_t sz = 1;
    //assert(preterminal(root));
    BOOST_FOREACH(indexed_syntax_rule::tree_node const& unused, root.children()) ++sz;
    hyperkey hkey = hyperkey(sz);
    hkey[0] = grammar_state(root,false);
    sz = 1;
    BOOST_FOREACH(indexed_syntax_rule::tree_node const& c, root.children()) {
        hkey[sz] = grammar_state(c,false);
        ++sz;
    }
    //assert(hkey.size() == 2);
    return hkey;
}

void add_supps( indexed_syntax_rule const& syntax
              , indexed_syntax_rule::tree_node const& root
              , supp_rulemap_t& supp_rulemap
              , in_memory_dictionary& dict
              , feature_dictionary& fdict
              , weight_vector const& weights
              , size_t& rid
              )
{
    size_t sz = 1;
    BOOST_FOREACH(indexed_syntax_rule::tree_node const& unused, root.children()) ++sz;
    hyperkey hkey = hyperkey(sz);
    hkey[0] = grammar_state(root,false);
    sz = 1;
    if (hkey.size() == 1) return;
    BOOST_FOREACH(indexed_syntax_rule::tree_node const& c, root.children()) {
        hkey[sz] = grammar_state(c,false);
        if (hkey.size() > 1) {
            add_supps(syntax,c,supp_rulemap,dict,fdict,weights,rid);
        }
        ++sz;
    }
    supp_rulemap_t::iterator pos = supp_rulemap.find(hkey);
    if (pos == supp_rulemap.end()) {
        if (hkey.size() == 2 and is_lexical(hkey[1].root->get_token())) {
            std::stringstream rstr;
            rstr << print(hkey[0],dict) << "(\"" << print(hkey[1],dict) << "\") -> \"@UNKNOWN@\" ### target-unknown-rule=10^-1 id="<<++rid;
            rule_data rd = parse_xrs(rstr.str());
            size_t idx = fdict.get_index("target-unknown-rule");
            feature_vector fv;
            fv[idx] = 0.1;
            score_t ss = geom(fv,weights);
            indexed_syntax_rule tsyn(rd,dict);
            supp_rulemap.insert(std::make_pair(hkey,rule(hkey[0],tsyn,fv,ss)));
        } else {
            std::stringstream rstr;
            rstr << print(hkey[0],dict) << '(';
            for (sz = 1; sz != hkey.size(); ++sz) {
                if (sz > 1) rstr << ' ';
                rstr << 'x'<<sz-1<<':'<<print(hkey[sz],dict);
            }
            rstr << ')';
            rstr << " ->";
            for (sz = 1; sz != hkey.size(); ++sz) {
                rstr << 'x' << sz-1 << ' ';
            }
            rstr << "### target-glue-rule=10^-1 id="<<++rid;
            
            rule_data rd = parse_xrs(rstr.str());
            size_t idx = fdict.get_index("target-glue-rule");
            feature_vector fv;
            fv[idx] = 0.1;
            score_t ss = geom(fv,weights);
            indexed_syntax_rule tsyn(rd,dict);
            supp_rulemap.insert(std::make_pair(hkey,rule(hkey[0],tsyn,fv,ss)));
        }
    }
}
hyperkey make_hyperkey( indexed_syntax_rule const& rule
                      , feature_vector const& fv
                      , indexed_syntax_rule::tree_node const& root
                      , rulepremap_t& rulemap
                      , virtmap_t& virtmap
                      , weight_vector const& wv )
{
    size_t sz = 1;
    BOOST_FOREACH(indexed_syntax_rule::tree_node const& unused, root.children()) ++sz;
    hyperkey hkey = hyperkey(sz);
    hkey[0] = grammar_state(root,false);
    sz = 1;
    BOOST_FOREACH(indexed_syntax_rule::tree_node const& c, root.children()) {
        hkey[sz] = find_state(rule,fv,c,rulemap,virtmap,wv);
        ++sz;
    }
    return hkey;
}

/*
find_lexstate(indexed_syntax_rule)

internal_state add_lexstatemap( indexed_syntax_rule const& rule
                              , indexed_syntax_rule::tree_node const& root
                              , lexstatemap_t& lexstatemap )
{
    internal_state ret;
    if (root.indexed()) return;
    bool lex = true;
    
}
*/

void level_order_decompose( indexed_syntax_rule const& rule
                          , feature_vector const& fv
                          , rulepremap_t& rulemap
                          , virtmap_t& virtmap
                          , weight_vector const& wv )
{
    find_state(rule,fv,*rule.lhs_root(),rulemap,virtmap,wv);
}                          

/// forest

bool operator < (hyp const& h1, hyp const& h2)
{
    return h1.score() < h2.score();
}

bool operator > (hyp const& h1, hyp const& h2)
{
    return h1.score() > h2.score();
}


bool operator < (forest const& f1, forest const& f2)
{
    return f1.score() < f2.score();
}

bool operator > (forest const& f1, forest const& f2)
{
    return f1.score() > f2.score();
}



score_t hyp::transition_score() const 
{
    score_t scr = score();
    BOOST_FOREACH(forest const& f, children) scr /= f.score();
    return scr;
}

xplode_generator
tgt_xplode(hyp const& h, weight_vector const& weights);

xplode_generator
tgt_xplode_andnode(hyp const& h, weight_vector const& weights);

xplode_generator
tgt_xplode_ornode(forest f, weight_vector const& weights);

xhyp_generator
tgt_xforest_children(forest const& f, weight_vector const& weights);

xhyp_generator
tgt_xplode_hyps(hyp const& h, weight_vector const& weights);

xplode_generator
tgt_xplode_andnode(hyp const& h, weight_vector const& weights)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    assert(h.state().virt);
    feature_vector feats = h.scores(); //component_scores(e,gram,ef);

    xplode_generator gene;

    if (h.children.empty()) {
        gene = gusc::make_single_value_generator(
                xplode_t(feats,vector<xforest>(),geom(feats,weights))
               );
        assert(gene);
    } else {
        gene = gusc::generator_as_iterator(
                 gusc::generate_transform(
                   tgt_xplode(h,weights)
                 , boost::bind( mult_feats
                              , boost::cref(weights)
                              , feats
                              , boost::lambda::_1
                              )
                 )
               );
        assert(gene);
    }
    return gene;
}

xplode_generator
tgt_xplode_ornode(forest f, weight_vector const& weights)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    xplode_generator gene;

    if (f.state().virt) {
        gene = gusc::generate_union_heap(
                 gusc::generator_as_iterator(
                   gusc::generate_transform(
                     gusc::generate_from_range(f)
                   , boost::bind( tgt_xplode_andnode
                                , boost::lambda::_1
                                , boost::cref(weights)
                                )
                   )
                 )
               , &lesser_xplode_score
               );
        assert(gene);
    } else {
        vector<xforest> ff(1,xforest(forest_as_xforest(f,weights)));
        gene = gusc::make_single_value_generator(
                 xplode_t(feature_vector(),ff,ff[0].score())
               );
        assert(gene);
    }
    return gene;
}

xplode_generator
tgt_xplode(hyp const& h, weight_vector const& weights)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    xplode_generator ret;
    assert(not h.children.empty());

    ret = tgt_xplode_ornode(h.children[0],weights);
    assert(ret);
    for (size_t x = 1; x != h.children.size(); ++x) {
        typedef gusc::shared_lazy_sequence<xplode_generator> xplode_sequence;
        xplode_generator g = tgt_xplode_ornode(h.children[x],weights);
        assert(g);
        xplode_sequence v2(g);
        xplode_sequence v1(ret);
        ret = gusc::generate_product_heap(
                &mult_xplodes
              , &lesser_xplode_score
              , v1
              , v2
              );
    }
    return ret;
}


xhyp tgt_xhyp_from_xplode( weight_vector const& weights
                         , indexed_syntax_rule const& rule
                         , feature_vector const& scores
                         , xplode_t const& x )
{
    std::map<size_t,xforest> xfm;
    size_t idx = 0;
    BOOST_FOREACH(indexed_syntax_rule::tree_node const& nd,rule.lhs()) {
        if (nd.indexed()) {
            xfm.insert(std::make_pair(nd.index(),boost::get<1>(x).at(idx++)));
        }   
    }
    std::vector<xforest> reordered;
    BOOST_FOREACH(indexed_syntax_rule::rule_node const& nd, rule.rhs()) {
        if (nd.indexed()) reordered.push_back(xfm[rule.index(nd)]);
    }
    return xhyp( geom(scores, weights) * boost::get<2>(x)
               , rule
               , scores * boost::get<0>(x)
               , reordered
               );
}

xhyp_generator
tgt_xplode_hyps(hyp const& h, weight_vector const& weights)
{
    using std::vector;
    using boost::shared_ptr;
    using boost::get;

    indexed_syntax_rule syntax = h.syntax();
    feature_vector scores = h.scores();

    if (h.children.empty()) {
        xhyp xh(geom(scores, weights), syntax, scores, vector<xforest>());
        return gusc::make_single_value_generator(xh);
    } else {
        return gusc::generator_as_iterator(
                 gusc::generate_transform(
                   tgt_xplode(h,weights)
                 , boost::bind( tgt_xhyp_from_xplode
                              , boost::cref(weights)
                              , syntax
                              , scores
                              , boost::lambda::_1
                              )
                 )
               );
    }
}

xhyp_generator
tgt_xforest_children(forest const& f, weight_vector const& weights)
{
    return
        gusc::generate_union_heap(
          gusc::generator_as_iterator(
            gusc::generate_transform(
              gusc::generate_from_range(f)
            , boost::bind( tgt_xplode_hyps
                         , boost::lambda::_1
                         , boost::cref(weights)
                         )
            )
          )
        , &lesser_xhyp_score
        );
}

/// \forest

template <class Children>
gusc::any_generator<gusc::varray<forest>, gusc::iterator_tag>
generate_children(Children const& children) 
{
    size_t x = 0;
    gusc::any_generator<gusc::varray<forest>, gusc::iterator_tag> gen;
    BOOST_FOREACH(gusc::shared_varray<forest> vf , children) {
        if (x == 0) {
            gen = gusc::make_peekable(gusc::generate_transform(
                    gusc::generate_from_range(
                      vf
                    )
                  , to_varray(boost::size(children))
                  ));
        } else {
            gen = gusc::generate_product_heap(
                    prodvec(x)
                  , lesser_children(x+1)
                  , gusc::make_lazy_sequence(gen)
                  , vf
                  )
                  ;
        }
        ++x;
    }
    return gen;
}

gusc::shared_varray<forest>
forests( indexed_syntax_rule const& syntax
       , indexed_syntax_rule::tree_node const& nd
       , feature_vector const& sourcetscores
       , score_t sourcetscore
       , std::tr1::unordered_map<size_t,gusc::shared_varray<forest> >& cmap
       , rulemap_t const& rulemap 
       , supp_rulemap_t const& supp_rulemap )
{   
    hyp_generator gen;
    hyperkey hkey = make_real_hyperkey(nd);
    rule supprule;
    hyp suphyp;
    //std::cerr << "forests: nd="<<nd.get_token() << " hkey=";
    //std::copy(hkey.begin(),hkey.end(),std::ostream_iterator<grammar_state>(std::cerr," "));
    //std::cerr << '\n';
    if (nd.indexed()) {
        return cmap[nd.index()];   
    } else if (preterminal(nd)) {
        assert(supp_rulemap.find(hkey) != supp_rulemap.end());
        supprule = supp_rulemap.find(hkey)->second;
        gusc::varray<forest> nochildren;
        rulemap_t::const_iterator pos = rulemap.find(hkey);
        
       
        if (pos != rulemap.end()) {
            //std::cerr << "forests: nd=" << nd.get_token() << " LEX ";
            //BOOST_FOREACH(gusc::shared_varray<rule> vf,pos->second) {
            //    std::cerr << "* " << vf.size();
            //}
            //std::cerr << "\n";
            gen = hyp_gen_gen(pos->second,syntax,nd,sourcetscores,sourcetscore,nochildren);
        }
        suphyp = hyp(supprule,syntax,nd,sourcetscores,sourcetscore,nochildren);
    } else {
        assert(supp_rulemap.find(hkey) != supp_rulemap.end());
        supprule = supp_rulemap.find(hkey)->second;
        std::vector< gusc::shared_varray<forest> > children;
        gusc::varray<forest> hypchildren(hkey.size() - 1);
        size_t cx = 0;
        BOOST_FOREACH(indexed_syntax_rule::tree_node const& cnd,nd.children()) {
            gusc::shared_varray<forest> ff = forests( syntax
                                                    , cnd
                                                    , sourcetscores
                                                    , sourcetscore
                                                    , cmap
                                                    , rulemap
                                                    , supp_rulemap );
            children.push_back(ff);
            bool found(false);
            BOOST_FOREACH(forest const& frst, ff) {
                if (frst.state() == hkey[cx+1]) {hypchildren[cx] = frst; found = true; break; }
            }
            assert(found);
            ++cx;
        }
        suphyp = hyp(supprule,syntax,nd,sourcetscores,sourcetscore,hypchildren);
        gen = hyps_from_children( children
                                , grammar_state(nd,false)
                                , rulemap
                                , syntax
                                , nd
                                , sourcetscores
                                , sourcetscore );
    }
    
    typedef std::tr1::unordered_map<grammar_state, std::vector<hyp>, boost::hash<grammar_state> > mp_t;
    mp_t mp;
    size_t x = 0;
    mp[suphyp.state()].push_back(suphyp);
    while (bool(gen) and (mp.size() < 20) and (x < 100)) {
        hyp h = gen();
        mp[h.state()].push_back(h);
        ++x;
        if (not h.state().virt) {
            assert(h.state() == nd.get_token());
        }
    }
    // add the supp:
    //hyperkey hkey = make_real_hyperkey(nd);

    typedef std::vector<forest> st_t;
    st_t st;
    BOOST_FOREACH(mp_t::value_type p, mp) {
        sort(p.second.begin(),p.second.end(),gusc::greater());
        st.push_back(forest(varray_as_forest(p.second)));
    }
    sort(st.begin(),st.end(),gusc::greater());
    //std::cerr << "forests: nd=" << nd.get_token() << ",mp.size"<<mp.size() << ",st.size="<<st.size()<<",x="<<x<<"\n";
    return gusc::shared_varray<forest>(st);
}

weight_vector 
weights_from_file(std::string filename, feature_dictionary& dict)
{
    using namespace std;
    weight_vector wv;
    string line;
    std::ifstream ifs(filename.c_str());

    if (not ifs)
        throw std::runtime_error("weight file '" + filename + "' not opened successfully");
    while(getline(ifs,line)) {
        weight_vector w;
        read(w,line,dict);
        wv += w;
    }
    return wv;
}

fat_weight_vector 
weights_from_file(std::string filename)
{
    using namespace std;
    fat_weight_vector wv;
    string line;
    std::ifstream ifs(filename.c_str());

    if (not ifs)
        throw std::runtime_error("weight file '" + filename + "' not opened successfully");
    while(getline(ifs,line)) {
        fat_weight_vector w;
        read(w,line);
        wv += w;
    }
    return wv;
}

std::ostream& operator << (std::ostream& out, xtree_ptr const& t)
{
    if (not t->children.empty()) out << '(';
    out << t->root.rule().id();
    //out << logmath::neglog10_scale;
    //print(out,t->root.scores(),t->gram->feature_names());
    //out << '>';
    BOOST_FOREACH(xtree::children_type::value_type const& c, t->children) {
        out << ' ' << c.second;
    }
    if (not t->children.empty()) out << ')';
    return out;
}

xtree_children_generator 
generate_xtree_children(xhyp const& hyp, ornode_map& omap)
{
    xtree_children_generator ret;
    bool first = true;
    indexed_syntax_rule::rhs_iterator
        rhsitr = hyp.rule().rhs_begin(),
        rhsend = hyp.rule().rhs_end();
    size_t x = 0;
    for (; rhsitr != rhsend; ++rhsitr, ++x) {
        if (rhsitr->indexed()) {
            if (first) {
                xforest xf0 = hyp.child(x);
                ret = gusc::generator_as_iterator(
                        gusc::generate_transform(
                          gusc::generate_from_range(xtrees_from_xforest(xf0,omap))
                        , make_xtree_children()
                        )
                      );
                first = false;
            } else {
                typedef gusc::shared_lazy_sequence<xtree_children_generator> 
                        xtree_children_list;
                typedef gusc::shared_lazy_sequence<xtree_generator> 
                        xtree_list;
                xforest xfx = hyp.child(x);
                ret = gusc::generate_product_heap(
                        append_xtree_children()
                      , less_xtree_children_score()
                      , xtree_children_list(ret)
                      , xtrees_from_xforest(xfx,omap)
                      );
            }
        }
    }
    return ret; 
}


xtree_generator 
xtrees_from_xhyp(xhyp const& hyp, ornode_map& omap)
{
    if (hyp.num_children() == 0) {
        return gusc::make_single_value_generator(
                 xtree_ptr(new xtree(hyp,xtree_children()))
               );
    } else {
        return gusc::generator_as_iterator(
                 gusc::generate_transform(
                   generate_xtree_children(hyp,omap)
                 , make_xtree(hyp)
                 )
               );
    }
}

xtree_list 
xtrees_from_xforest(xforest const& forest, ornode_map& omap)
{
    ornode_map::iterator pos = omap.find(forest.id());
    if (pos != omap.end()) return pos->second;
    else {
        xtree_list lst(
         xtree_generator(
          gusc::generate_union_heap(
            gusc::generator_as_iterator(
              gusc::generate_transform(
                forest.begin()
              , make_xtree_generator_from_xhyp(omap)
              )
            )
          , less_xtree_score()
          )
         )
        );
        omap.insert(std::make_pair(forest.id(),lst));
        return lst;
    }
}

xtree_generator xtrees_from_xforest(xforest const& forest)
{
    return gusc::generator_as_iterator(toplevel_generator(forest));
}

void accum(xtree_ptr const& t,sbmt::feature_vector& f)
{
    f *= t->root.scores();
    BOOST_FOREACH(xtree::children_type::value_type const& c, t->children) {
        accum(c.second,f);
    }
}

sbmt::feature_vector accum(xtree_ptr const& t)
{
    sbmt::feature_vector f;
    accum(t,f);
    return f;
}

std::string 
hyptree(xtree_ptr const& t,sbmt::indexed_syntax_rule::tree_node const& n, in_memory_dictionary& dict)
{
    using namespace sbmt;
    if (n.indexed()) {
        xtree_ptr ct = t->children[n.index()];
        assert(n.get_token() == ct->root.rule().lhs_root()->get_token());
        return hyptree(ct,*(ct->root.rule().lhs_root()),dict);
    } else if (n.lexical()) {
        return "\"" + dict.label(n.get_token()) + "\"";
    } else {
        bool first = true;//n.children_begin()->lexical();
        std::stringstream sstr;
        sstr << dict.label(n.get_token()) << '(';
        BOOST_FOREACH(indexed_syntax_rule::tree_node const& c, n.children()) {
            if (not first) sstr << ' ';
            first = false;
            sstr << hyptree(t,c,dict);
        }
        sstr << ')';
        return sstr.str();
    }
}

std::string 
hyptree(xtree_ptr const& t, in_memory_dictionary& dict)
{
    return hyptree(t,*(t->root.rule().lhs_root()),dict);
}

std::string nbest_features(sbmt::feature_vector const& f, feature_dictionary& dict)
{
    std::stringstream sstr;
    //sstr << sbmt::logmath::neglog10_scale;
    bool first = true;
    BOOST_FOREACH(sbmt::feature_vector::const_reference v, f) {
        if (not first) sstr << ' ';
        sstr << dict.get_token(v.first) << '=' << v.second;
        first = false;
    }
    return sstr.str();
}

feature_vector features(rule_data const& rd, feature_dictionary& fdict)
{
    feature_vector fv;
    BOOST_FOREACH(feature const& f, rd.features)
    {
        if (f.key != "id") {
            std::stringstream sstr(f.str_value);
            score_t scrr;
            sstr >> scrr;
            if (scrr != 1.0 and scrr != 0.0) {
                fv[fdict.get_index(f.key)] = scrr;
                //sm.insert(std::make_pair(dict.get_index(f.key),score_t(f.num_value,as_neglog10())));
            }
        }
    }
    return fv;
}

rulemap_t read_grammar( std::istream& in
                            , in_memory_dictionary& dict
                            , feature_dictionary& fdict
                            , weight_vector const& wv )
{
    int x = 0;
    rulepremap_t rulemap;
    virtmap_t virtmap;
    rulemap_t rmap;
    std::string line;
    
    while (getline(in,line)) {
        //cerr << "processing " << line << '\n';
        rule_data rd = parse_xrs(line);
        indexed_syntax_rule rule(rd,dict);
        feature_vector fv = features(rd,fdict);
        //std::cerr << print(fv,fdict) << '\n';
        level_order_decompose(rule,fv,rulemap,virtmap,wv);
        ++x;
        //if (x % 100000 == 0) std::cerr << x << '\n';
    }
    BOOST_FOREACH( rulepremap_t::value_type& rrd, rulemap )  {
        std::vector< gusc::shared_varray<rule> > rls;
        BOOST_FOREACH( rulepremap_t::value_type::second_type::value_type& rd_, rrd.second) {
            sort(rd_.second.begin(),rd_.second.end(),gusc::greater());
            rls.push_back(gusc::shared_varray<rule>(rd_.second));
            std::vector<rule>().swap(rd_.second);
        }
        sort(rls.begin(),rls.end(),first_greater());
        rmap.insert(std::make_pair(rrd.first,rulemap_entry(rls)));
    }
    return rmap;
}

}} // sbmt::t2s
