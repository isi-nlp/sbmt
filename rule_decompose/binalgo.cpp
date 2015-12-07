# include "binalgo.hpp"


boost::regex relmstr("lm_string=\\{\\{\\{(.*?)\\}\\}\\}");
/*
  149654 - 127318 = 22336
  138170 - 127318 = 10852
  S-C(NP-C(x0:NP CC("and") NP(NPB(x1:NN) x2:PP)) x3:VP) -> x0 "AND" x3 x2 x1 "foo" ### align={{{[ #s=6 #t=5 0,0 1,1 2,4 3,3 4,2 5,1 ]}}} 
  VP(VBN("known") PP(IN("as") S-C(NP-C(NPB(NNP("sheikh") NNP("umar"))) VP(VBD("said") ,(",") PP(IN("of") NP-C(NP(NPB(NN("kidnapping"))) x0:CC SG(VP(x1:VBG x2:NP-C)) x3:, x4:NP)))))) -> "KNOWN_AS" "SHEIKH" "UMAR" "SAID," "OF_KIDNAPPING" x2 x0 x1 x3 x4 ### align={{{[ #s=10 #t=13 0,0 0,1 1,2 2,3 3,4 3,5 4,6 4,7 5,10 6,8 7,9 8,11 9,12 ]}}} 
 */
 
bool subder_lex_idx(subder_ptr const& sd, aligned_rule const& ar);

aligned_rule::aligned_rule( std::string const& line
                          , std::set<std::string> const& pointers
                          , bool empty_align )
{
    rule_data r = parse_xrs(line);
    construct(r,pointers,empty_align);
}

aligned_rule::aligned_rule( rule_data const& rule
                          , std::set<std::string> const& pointers
                          , bool empty_align )
{
    construct(rule,pointers,empty_align);
}


void aligned_rule::construct( rule_data const& r
                            , std::set<std::string> const& pointers
                            , bool empty_align )
{
    rule = sbmt::fatter_syntax_rule(r,sbmt::fat_tf);
    ptrdiff_t p = get_feature(r,"align");
    
    sbmt::alignment prealign;
    
    if (not empty_align) {
        if (p < ptrdiff_t(r.features.size())) {
            prealign = sbmt::alignment(r.features[p].str_value);
        } else {
            prealign = rule.default_alignment();
        }
    } else {
        prealign = rule.default_alignment(false);
    }
    std::vector<sbmt::fatter_syntax_rule::lhs_preorder_iterator> tarpos;
    sbmt::fatter_syntax_rule::lhs_preorder_iterator 
        itr = rule.lhs_begin(),
        end = rule.lhs_end();
    for (; itr != end; ++itr) {
        if (itr->is_leaf()) {
            tarpos.push_back(itr);
        }
    }
    
    for (size_t s = 0; s != prealign.n_src(); ++s) {
        BOOST_FOREACH(sbmt::alignment::index t, prealign.sa[s]) {
            align.insert(align_data::value_type(tarpos[t],rule.rhs_begin()+s));
        }
    }
    
    typedef boost::split_iterator<std::string::const_iterator> split_iterator;
    
    BOOST_FOREACH(feature const& ff, r.features) {
        if (pointers.find(ff.key) != pointers.end()) {
            pointer_map pm;
            split_iterator itr(ff.str_value, boost::first_finder(" ",boost::is_iequal()));
            split_iterator end;
            sbmt::fatter_syntax_rule::rhs_iterator ritr = rule.rhs_begin();
            sbmt::fatter_syntax_rule::rhs_iterator rend = rule.rhs_end();
            for (; ritr != rend; ++ritr) {
                if (ritr->indexed()) {
                    std::string n;
                    do {
                        n = boost::copy_range<std::string>(*itr);
                        ++itr;
                    } while (n == "");
                    pm.insert(std::make_pair(ritr,n));
                }
            }
            pointer_maps.insert(std::make_pair(ff.key,pm));
        }
    }
    
    features = r.features;
    id_map = sbmt::map_ids_rhs(rule);
    id_map_inv = sbmt::map_ids_lhs(rule);
}

void expand_source( subder_ptr& sd
                  , subder::source_position m
                  , subder::source_position M )
{
    assert(m < M);
    sd->source_begin = std::min(sd->source_begin,m);
    sd->source_end = std::max(sd->source_end,M);
    sd->binname = "";
}

void expand_source(subder_ptr& s, subder_ptr const& ss)
{
    s->source_begin = std::min(s->source_begin,ss->source_begin);
    s->source_end = std::max(s->source_end,ss->source_end);
    s->binname = "";
}

void expand_target( subder_ptr& sd
                  , subder::target_position m
                  , subder::target_position M )
{
    assert(m < M);
    sd->target_begin = std::min(sd->target_begin,m);
    sd->target_end = std::max(sd->target_end,M);
    sd->binname = "";
}

void expand_target(subder_ptr& s, subder_ptr const& ss)
{
    s->target_begin = std::min(s->target_begin,ss->target_begin);
    s->target_end = std::max(s->target_end,ss->target_end);
    s->binname = "";
}

std::pair<subder::source_position,subder::source_position> 
source_range(subder_ptr const& sd)
{
    if (sd->source_begin > sd->source_end) {
        return std::make_pair(sd->source_begin,sd->source_begin);
    } else {
        return std::make_pair(sd->source_begin,sd->source_end);
    }
}

std::pair<subder::target_position,subder::target_position> 
target_range(subder_ptr const& sd)
{
    if (sd->target_begin > sd->target_end) {
        return std::make_pair(sd->target_begin,sd->target_begin);
    } else {
        return std::make_pair(sd->target_begin,sd->target_end);
    }
}

// true if represents index in composed rule.
bool subder_is_index(subder_ptr const& sd) 
{ 
    return sd->root != 0 and sd->root->indexed(); 
}

// true if represents lexical rhs item.
bool subder_is_lex(subder_ptr const& sd) 
{ 
    return sd->root == 0 and 
           std::distance(sd->source_begin,sd->source_end) == 1; 
}

bool subder_internal(subder_ptr const& sd) 
{
    return sd->root != 0 and not sd->root->indexed();
}

subder_ptr new_subder(sbmt::fatter_syntax_rule const& r)
{
    return subder_ptr(new subder(r));
}
// PP(IN("of") NP-C(NPB(DT("the") NN("minutes")))) -> "des" "protokolls" ### align={{{[ #s=2 #t=3 0,0  1,2 ]}}}
std::vector<subder_ptr> assemble_rhs(subder_ptr const& sd, aligned_rule const& ar)
{
    std::multiset<subder_ptr,subder_sort> sset;

    //std::cerr << ">>begin\n";
    assemble_rhs( sd
                , ar
                , sd->root
                , sd->children.begin()
                , 0
                , std::inserter(sset,sset.end()) );
    //std::cerr << ">>end\n";
    //BOOST_FOREACH(subder_ptr const& s, sset) {
    //    std::cerr <<"<< "<<s->index<<":"<<s->source_begin->get_token().label();
    //}
    //std::cerr << '\n';
    std::vector<subder_ptr> svec;
    sbmt::fatter_syntax_rule::rhs_iterator ri = sd->source_begin;
    std::set<subder_ptr,subder_sort>::iterator si = sset.begin();

    if (sd->source_begin <= sd->source_end) for (; ri != sd->source_end; ) {
        if (si != sset.end() and ri == (*si)->source_begin) {
            svec.push_back(*si);
            ri = (*si)->source_end;
            //std::cerr <<" "<<(*si)->index<<":"<<(*si)->source_begin->get_token().label();
            ++si;
            
        } else if (si != sset.end() and (*si)->source_end < (*si)->source_begin) {
            svec.push_back(*si);
            ++si; 
        } else {
            //std::cerr << " \""<<ri->get_token().label()<<"\"";
            subder_ptr s = new_subder(ar.rule);
            s->source_begin = ri;
            ++ri;
            s->source_end = ri;
            s->lexunder = true;
            svec.push_back(s);
        }
    }
    while (si != sset.end() and (*si)->source_end < (*si)->source_begin) {
        svec.push_back(*si);
        ++si;
    }
    //std::cerr << '\n';
    assert(si == sset.end());
    return svec;
}

std::list<subder_ptr>::const_iterator
align_lhs_map( subder_ptr const& sd
             , sbmt::fatter_syntax_rule::tree_node const* r
             , std::list<subder_ptr>::const_iterator c
             , std::map<sbmt::fatter_syntax_rule::lhs_preorder_iterator,int>& lhs
             )
{
    if (r->lexical()) lhs.insert(std::make_pair(r,lhs.size()));
    else if (r->indexed()) lhs.insert(std::make_pair(r,lhs.size()));
    else if (c != sd->children.end() and (*c)->root == r) {
        lhs.insert(std::make_pair(r,lhs.size()));
        ++c;
    } else {
        sbmt::fatter_syntax_rule::lhs_children_iterator ci = r->children_begin(),
                                                        ce = r->children_end();
        for (; ci != ce; ++ci) {
            c = align_lhs_map(sd,&(*ci),c,lhs);
        }
    }
    return c;
}

void print_subder_align( std::ostream& out
                       , subder_ptr const& sd
                       , aligned_rule const& ar )
{
    std::map<sbmt::fatter_syntax_rule::lhs_preorder_iterator,int> lhs;
    align_lhs_map(sd,sd->root,sd->children.begin(),lhs);
    
    std::map<sbmt::fatter_syntax_rule::rhs_iterator,int> rhs;
    std::set< std::pair<int,int> > ais;
    
    int idx = 0;
    BOOST_FOREACH(subder_ptr const& s, sd->rhs) {
        if (subder_is_lex(s)) {
            BOOST_FOREACH( aligned_rule::align_data::right_map::value_type rp
                         , ar.align.right.equal_range(s->source_begin) ) {
                ais.insert(std::make_pair(idx,lhs[rp.second]));
            }
            rhs.insert(std::make_pair(s->source_begin,idx));
        }
        else {
            ais.insert(std::make_pair(idx,lhs[s->root]));
        }
        ++idx;
    }
    sbmt::fatter_syntax_rule::lhs_preorder_iterator lpos;
    int lidx;
    BOOST_FOREACH(boost::tie(lpos,lidx),lhs) {
        if (lpos->lexical()) {
            BOOST_FOREACH(aligned_rule::align_data::left_map::value_type lp,ar.align.left.equal_range(lpos))
            {
                ais.insert(std::make_pair(rhs[lp.second],lhs[lp.first]));
            }
        }
    }
    out << "align={{{[ #s="<<idx<<" #t="<<lhs.size(); 
    int s; int t;
    BOOST_FOREACH(boost::tie(s,t),ais) { out << " " << s<<","<<t; }
    out <<" ]}}}";
}

void print_subder_rhs( std::ostream& out
                     , subder_ptr const& sd
                     , aligned_rule const& ar )
{
    
    std::vector<subder_ptr> const& rhs = sd->rhs;
    BOOST_FOREACH(subder_ptr const& s, rhs) {
        if (subder_is_lex(s)) {
            out << ' ' << '"' << s->source_begin->get_token().label() << '"';
        } else {
            out << ' ' << 'x' << s->index;
        }
    }
}

boost::tuple<int,std::list<subder_ptr>::const_iterator>
print_subder_lhs( std::ostream& out
                , subder_ptr const& sd
                , sbmt::fatter_syntax_rule::tree_node const* r
                , int idx
                , std::list<subder_ptr>::const_iterator c
                )
{
    if (r->lexical()) out << '"' << r->get_token() << '"';
    else if (r->indexed()) out << 'x' << idx++ << ':' << r->get_token();
    else if (c != sd->children.end() and (*c)->root == r) {
        out << 'x' << idx++ << ':' << r->get_token();
        ++c;
    } else {
        out << r->get_token() << '(';
        sbmt::fatter_syntax_rule::lhs_children_iterator ci = r->children_begin(),
                                                     ce = r->children_end();
        boost::tie(idx,c) = print_subder_lhs(out,sd,&(*ci),idx,c);
        ++ci;
        for (; ci != ce; ++ci) {
            out << ' ';
            boost::tie(idx,c) = print_subder_lhs(out,sd,&(*ci),idx,c);
        }
        out <<')';
    }
    return boost::make_tuple(idx,c);
}

void print_subder(std::ostream& out, subder_ptr const& sd, aligned_rule const& ar)
{
    print_subder_lhs(out,sd,sd->root,0,sd->children.begin());
    out << std::flush << " ->" << std::flush;
    print_subder_rhs(out,sd,ar);
    out << " ### ";
    print_subder_align(out,sd,ar);
    out << std::flush;
}

void print_subder_mr(std::ostream& out, subder_ptr const& sd, aligned_rule const& ar)
{
    print_subder_lhs(out,sd,sd->root,0,sd->children.begin());
    out << std::flush << " ->" << std::flush;
    print_subder_rhs(out,sd,ar);
    out << std::flush;
}

boost::tuple<bool,int,std::list<subder_ptr>::const_iterator>
subder_has_lhs_lex( subder_ptr const& sd
                  , sbmt::fatter_syntax_rule::tree_node const* r
                  , int idx
                  , std::list<subder_ptr>::const_iterator c )
{
    bool retval = false;
    if (r->lexical()) retval = true;
    else if (r->indexed()) retval = false;
    else if (c != sd->children.end() and (*c)->root == r) {
        retval = false;
        ++c;
    } else {
        bool rtv = false;
        sbmt::fatter_syntax_rule::lhs_children_iterator ci = r->children_begin(),
                                                        ce = r->children_end();
        for (; ci != ce; ++ci) {
            boost::tie(rtv,idx,c) = subder_has_lhs_lex(sd,&(*ci),idx,c);
            retval = retval or rtv;
        }
    }
    return boost::make_tuple(retval,idx,c);
}

boost::tuple<bool,int,std::list<subder_ptr>::const_iterator>
subder_has_lhs_idx( subder_ptr const& sd
                  , sbmt::fatter_syntax_rule::tree_node const* r
                  , int idx
                  , std::list<subder_ptr>::const_iterator c )
{
    bool retval = false;
    if (r->lexical()) retval = false;
    else if (r->indexed()) retval = true;
    else if (c != sd->children.end() and (*c)->root == r) {
        retval = true;
        ++c;
    } else {
        bool rtv = false;
        sbmt::fatter_syntax_rule::lhs_children_iterator ci = r->children_begin(),
                                                        ce = r->children_end();
        for (; ci != ce; ++ci) {
            boost::tie(rtv,idx,c) = subder_has_lhs_idx(sd,&(*ci),idx,c);
            retval = retval or rtv;
        }
    }
    return boost::make_tuple(retval,idx,c);
}

bool subder_has_lhs_lex_idx(subder_ptr const& sd)
{
    bool lex, idx;
    int i;
    std::list<subder_ptr>::const_iterator c;
    boost::tie(lex,i,c) = subder_has_lhs_lex(sd,sd->root,0,sd->children.begin());
    boost::tie(idx,i,c) = subder_has_lhs_idx(sd,sd->root,0,sd->children.begin());
    return lex and idx;
}


bool subder_has_rhs_lex_idx( subder_ptr const& sd
                           , aligned_rule const& ar )
{
    bool lex = false;
    bool idx = false;
    std::vector<subder_ptr> const& rhs = sd->rhs;
    BOOST_FOREACH(subder_ptr const& s, rhs) {
        if (subder_is_lex(s)) {
            lex = true;
        } else {
            idx = true;
        }
    }
    return lex and idx;
}

bool subder_has_rhs_lex( subder_ptr const& sd
                       , aligned_rule const& ar )
{
    bool lex = false;
    std::vector<subder_ptr> const& rhs = sd->rhs;
    BOOST_FOREACH(subder_ptr const& s, rhs) {
        if (subder_is_lex(s)) {
            lex = true;
            break;
        }
    }
    return lex;
}


bool subder_lex_idx(subder_ptr const& sd, aligned_rule const& ar)
{
    return subder_has_lhs_lex_idx(sd) or subder_has_rhs_lex_idx(sd,ar);
}



subder_ptr get_subder( aligned_rule& ar
                     , sbmt::fatter_syntax_rule::tree_node const& n
                     , bool collapse_unary
                     , bool lift_leaf_nonlex
                     , bool forgetit
                     , bool leaf_or_nonlex
                     , bool epsilon_rules
                     )
{
    bool oldforgetit = forgetit;
    bool repeat = false;
    subder_ptr s;
    do {
        repeat = false;
        s = new_subder(ar.rule);
        s->root = &n;

        BOOST_FOREACH(sbmt::fatter_syntax_rule::tree_node const& c, n.children()) {
            if (c.is_leaf()) {
                s->target_begin = std::min(s->target_begin,&c);
                s->target_end = std::max(s->target_end,(&c)+1);
                BOOST_FOREACH( aligned_rule::align_data::left_map::value_type rp
                             , ar.align.left.equal_range(&c) ) {
                    expand_source(s,rp.second,rp.second + 1);
                }
            } else {
                subder_ptr ss = get_subder(ar,c,collapse_unary,lift_leaf_nonlex,forgetit,leaf_or_nonlex);
                if (ss->valid) s->children.push_back(ss);
                else s->children.insert( s->children.end()
                                       , ss->children.begin()
                                       , ss->children.end() );
                expand_target(s,ss);
                expand_source(s,ss);
            }
        }
        
        if (s->source_begin >= s->source_end and epsilon_rules == false) {
            s->valid = false;
            return s;
        } 
        if (s->source_begin <= s->source_end) for ( sbmt::fatter_syntax_rule::rhs_iterator si = s->source_begin
            ; si != s->source_end
            ; ++si ) {
            BOOST_FOREACH( aligned_rule::align_data::right_map::value_type rp
                         , ar.align.right.equal_range(si) ) {
                if (rp.second < s->target_begin or rp.second >= s->target_end) {
                    s->valid = false;
                    return s;
                }
            }
        }
        
        if (s->root == ar.rule.lhs_root()) {
            s->source_begin = ar.rule.rhs_begin();
            s->source_end = ar.rule.rhs_end();
        }

        s->rhs = assemble_rhs(s,ar);
        
        if (leaf_or_nonlex and not forgetit) {
            // check and see if this rule is a mix of variables and lexical items. if it is,
            // pull the children rules into the parent.
            if (subder_lex_idx(s,ar)) {
                forgetit = true;
                //std::cerr << "RERUN: ";
                //print_subder(std::cerr,s,ar);
                //std::cerr << "\n";
                repeat = true;
            } else {
                //std::cerr << "GOOD: ";
                //print_subder(std::cerr,s,ar);
                //std::cerr << "\n";
            }
        }
    } while (repeat);
    
    forgetit = oldforgetit;
    int children = s->rhs.size();
    s->valid = (collapse_unary ? (children > 1) : true);
    if ((not s->valid) and 
        collapse_unary and 
        std::distance(s->source_begin,s->source_end) == 1) {
        for (subder::target_position tu = s->target_begin; tu != s->target_end; ++tu) {
            if (sbmt::is_lexical(tu->get_token())) {
                ar.align.insert(aligned_rule::align_data::value_type(tu,s->source_begin));   
            } 
        }
    }
    
    if (forgetit and s->root != ar.rule.lhs_root()) {
        s->valid = false;
        return s;
    }
    
    if (lift_leaf_nonlex and 
        leaf_nonlex(s->rhs) and 
        s->root != ar.rule.lhs_root()) {
        s->valid = false;
    }
    
    if (children == 1 and s->root == ar.rule.lhs_root() and collapse_unary) {
        s->valid = true;
        if (s->children.size() == 1) {
            subder_ptr sp = s->children.front();
            sp->root = &n;
            sp->valid = true;
            std::stringstream sstr;
            print_subder(sstr,sp,ar);
            //std::cerr << sstr.str() << '\n';
            sp->rule = aligned_rule(sstr.str());
            return sp;
        }
    }
    /*
    
    */
    if (s->valid) {
        std::stringstream sstr;
        print_subder(sstr,s,ar);
        //std::cerr << sstr.str() << '\n';
        s->rule = aligned_rule(sstr.str());
    }
    return s;
}

void print_subders( std::ostream& out
                  , subder_ptr const& sd
                  , aligned_rule const& ar
                  , int x )
{
    for (int y = 0; y != x; ++y) out << "  ";
    print_subder(out,sd,ar);
    out << std::endl;
    BOOST_FOREACH(subder_ptr const& ci, sd->children) {
        print_subders(out,ci,ar,x+1);
    }
}

void print_subders_mr( std::ostream& out
                       , subder_ptr const& sd
                       , aligned_rule const& ar )
{

  print_subder_mr(out,sd,ar);
  out << "\t" << ar.rule.id();
  out << std::endl;
  BOOST_FOREACH(subder_ptr const& ci, sd->children) {
    print_subders_mr(out,ci,ar);
  }
}


void print_subder_vars( std::ostream& out
                      , subder_ptr const& sd
                      , aligned_rule const& ar)
{
    typedef boost::tuple<int,int,subder_ptr> vtype;
    std::set<vtype> sds;
    subder_vars(ar,sd,std::inserter(sds,sds.end()));
    BOOST_FOREACH(vtype const& s, sds) {
        std::cout << boost::get<0>(s) << ":" << boost::get<1>(s) << ":";
        print_subder(out,boost::get<2>(s),ar);
        std::cout << std::endl;
    }
}

bool intersect(target_tuple const& tr1, target_tuple const& tr2)
{
    if (tr1.get<0>() > tr2.get<0>()) return intersect(tr2,tr1);
    return (tr1.get<1>() >= tr2.get<0>());
}

void union_range(std::vector<target_tuple>& rngset, target_tuple rng)
{
    bool go = true;
    while (go) {
        go = false;
        std::vector<target_tuple>::iterator prng = rngset.begin();
        for (; prng != rngset.end(); ++prng) {
            if (intersect(*prng,rng)) {
                rng.get<0>() = std::min(rng.get<0>(),prng->get<0>());
                rng.get<1>() = std::max(rng.get<1>(),prng->get<1>());
                std::swap(*prng,rngset.back());
                rngset.pop_back();
                go = true;
                break;
            }
        }
    }
    rngset.push_back(rng);
} 
// NP-C-1(NPB-0(DT-1("the") x0:NN-2) x1:PP-0 x2:PP-0) -> x1 x0 x2 ### id=255086671 count=10^-219 align={{{[ #s=3 #t=4 0,2 1,1 2,3 ]}}} 
// V[PP-0_NN-2]["the"_1_0] -> NN-2 PP-0
// NP-C-1 -> V[PP-0_NN-2]["the"_1_0] PP-0 ### lm_string=
void union_range( std::vector<target_tuple>& rngset
                , std::pair<subder::target_position,subder::target_position> p)
{
    union_range(rngset,boost::make_tuple(p.first,p.second));
}

contig_block<always_true> make_contig_block(aligned_rule const& ar, bool tua)
{
    return contig_block<always_true>(ar,tua,always_true());
}

bool in_children(subder::target_position tpos, subder_ptr const& sd)
{
    BOOST_FOREACH(subder_ptr const& s, sd->children) {
        if ((tpos >= s->target_begin) and (tpos < s->target_end)) return true;
    }
    return false;
}

bool beginning_of_children(subder::target_position tpos, subder_ptr const& sd)
{
    BOOST_FOREACH(subder_ptr const& s, sd->children) {
        if (tpos == s->target_begin) return true;
    }
    return false;
}

void print_bin_features( std::ostream& out
                       , subder_ptr const& sd
                       , aligned_rule const& ar )
{
    int num_source = 0;
    int num_target = 0;
    int nrm[2] = {0,1};
    int inv[2] = {1,0};
    int* idxptr = nrm;
    subder::target_position tst = sd->target_begin;
    BOOST_FOREACH(subder_ptr const& sc, sd->children) {
        if (not subder_is_lex(sc)) {
            if (tst > sc->target_begin) idxptr = inv;
            tst = sc->target_begin;
        }
    }
    BOOST_FOREACH(subder_ptr const& sc, sd->children) {
        if (subder_is_lex(sc)) ++num_source;
    }
    
    if (num_source > 0) out << "foreign-length=" << num_source << ' ';

    out << "lm_string={{{";
    bool first = true;
    subder::target_position tpos = sd->root == ar.rule.lhs_root() 
                                 ? ar.rule.lhs_begin() 
                                 : sd->target_begin;
    subder::target_position tend = sd->root == ar.rule.lhs_root() 
                                 ? ar.rule.lhs_end() 
                                 : sd->target_end;
    if (tpos < tend) for (; tpos != tend; ++tpos) {
        bool lic = tpos->lexical() and not in_children(tpos,sd);
        bool bic = beginning_of_children(tpos,sd);
        if (lic or bic) {
            if (not first) out << ' ';
            first = false;
        }
        if (lic) {
            ++num_target;
            out << '"' << tpos->get_token().label() << '"';
        } else if (bic) {
            out << *idxptr;
            ++idxptr;
        }
    }
    out << "}}}";
    if (num_target > 0) out << ' '<< "text-length="<<num_target;
    
    BOOST_FOREACH(aligned_rule::pointer_map_set::value_type const& pm, ar.pointer_maps) {
        std::stringstream sstr;
        bool first = true;
        BOOST_FOREACH(subder_ptr const& sc, sd->children) {
            if (subder_is_index(sc)) {
                if (not first) sstr << ' ';
                first = false;
                sstr << pm.second.find(sc->source_begin)->second;
            }
        }
        if (sstr.str() != "") {
            out << ' ' << pm.first << "={{{" << sstr.str() << "}}}";
        }
    }
}

std::string const& binname(subder_ptr const& sd, aligned_rule const& ar)
{
    if (sd->binname != "") return sd->binname;
    std::stringstream out;
    bool first = true;
    int minidx = 1000;

    if (std::distance(sd->source_begin,sd->source_end) > 1) out << "V[";
    for ( subder::source_position spos = sd->source_begin
        ; spos != sd->source_end
        ; ++spos ) {
        if (not first) out << '_';
        if (spos->indexed()) {
            out << spos->get_token().label();
            
            if (std::distance(sd->source_begin,sd->source_end) > 1)
            BOOST_FOREACH(aligned_rule::pointer_map_set::value_type const& vt, ar.pointer_maps) {
                out << '{' << vt.second.find(spos)->second<< '}';
            }
            minidx = std::min( minidx
                             , ar.id_map_inv.find(ar.rule.index(*spos))->second 
                             );
        } else { 
            out << '"' << spos->get_token().label() << '"'; 
        }
        first = false;
    }
    if (std::distance(sd->source_begin,sd->source_end) > 1) {
        first = true;
        out << "][";
        if(sd->target_begin < sd->target_end) {
            for ( subder::target_position tpos = sd->target_begin
                ; tpos != sd->target_end
                ; ++tpos ) {
                if (tpos->indexed() or tpos->lexical()) {
                    if (not first) out << '_';
                    first = false;
                }
                if (tpos->indexed()) {
                    out << (ar.id_map_inv.find(tpos->index())->second - minidx);
                } else if (tpos->lexical()) {
                    out << '"'<< tpos->get_token().label() << '"';
                }
            }
        }
        out << "]";
    }
    sd->binname = out.str();
    return sd->binname;
}

void print_toplevel( std::ostream& out
                   , aligned_rule const& ar
                   , std::pair<std::string,std::string> const& vl )
{
    out << "X: " << ar.rule << '\n';
    out << "V: " << vl.first << " -> " << vl.second;
    out << " id="<< ar.rule.id();
    BOOST_FOREACH(feature f, ar.features) {
        if (    f.key != "text-length" 
            and f.key != "foreign-length" 
            and f.key != "lm_string" 
            and f.key != "id" 
            and f.key != "" 
            and ar.pointer_maps.find(f.key) == ar.pointer_maps.end() ) {
            out << ' ' << f;
        }
    }
    out << '\n';
}

void generate_bin_rules( std::ostream& out
                       , subder_ptr const& sd
                       , aligned_rule const& ar
                       , std::map<std::string,std::string>& m
                       , bool top )
{
    if (sd->children.empty()) return;
    std::stringstream rhsout;
    std::string lhs;
    if (ar.rule.lhs_root() == sd->root) {
        assert(top);
        lhs = ar.rule.lhs_root()->get_token().label();
    } else {
        assert(not top);
        lhs = binname(sd,ar);
    }
    if (top or m.find(lhs) == m.end()) {
        bool first = true;
        BOOST_FOREACH(subder_ptr const& s, sd->children) {
            if (not first) rhsout << ' ';
            first = false;
            rhsout << binname(s,ar);
        }
        rhsout << " ### ";
        print_bin_features(rhsout,sd,ar);
        if (top) {
            print_toplevel(out,ar,std::make_pair(lhs,rhsout.str()));
        } else {
            m.insert(std::make_pair(lhs,rhsout.str()));
        }
    
        BOOST_FOREACH(subder_ptr const& s, sd->children) {
            generate_bin_rules(out,s,ar,m,false);
        }
    }
}

sbmt::fat_lm_string 
bin_lm_string(subder_ptr const& sd, aligned_rule const& ar)
{
    std::stringstream sstr;
    boost::smatch m;
    print_bin_features(sstr,sd,ar);
    boost::regex_search(sstr.str(),m,relmstr);
    return sbmt::fat_lm_string(m.str(1),sbmt::fat_tf);
}

sbmt::fat_lm_string
xrs_lm_string(aligned_rule const& ar)
{
    std::stringstream sstr;
    sbmt::fatter_syntax_rule::lhs_preorder_iterator litr = ar.rule.lhs_begin();
    for (; litr != ar.rule.lhs_end(); ++litr) {
        if (is_lexical(litr->get_token())) sstr << '"'<< litr->get_token().label() << '"' << ' ';
        else if (litr->indexed()) sstr << ar.id_map_inv.find(litr->index())->second << ' ';
    }
    return sbmt::fat_lm_string(sstr.str(),sbmt::fat_tf);
}

sbmt::fat_lm_string 
xrs_lm_string_from_bin(subder_ptr const& sd, aligned_rule const& ar)
{
    if (subder_is_index(sd)) return sbmt::fat_lm_string("0",sbmt::fat_tf);

    std::stringstream sstr;
    std::vector<sbmt::fat_lm_string> children;
    BOOST_FOREACH(subder_ptr ss, sd->children) {
        if (not subder_is_lex(ss)) children.push_back(xrs_lm_string_from_bin(ss,ar));
    }
    if (children.empty()) return bin_lm_string(sd,ar);
    BOOST_FOREACH(sbmt::fat_lm_token tok, bin_lm_string(sd,ar)) {
        if (tok.is_index()) {
            int offset = tok.get_index() == 0 ? 0 : (children.at(0).n_variables());
            BOOST_FOREACH(sbmt::fat_lm_token t, children.at(tok.get_index())) {
                if (t.is_index()) { 
                    sstr << (t.get_index() + offset) << ' ';
                } else {
                    sstr << '"' << t.get_token().label() << '"' << ' ';
                }
            }
        } else {
            sstr << '"' << tok.get_token().label() << '"' << ' ';
        }
    }
    return sbmt::fat_lm_string(sstr.str(),sbmt::fat_tf);
}

subder_ptr from_wd(aligned_rule const& ar, subder::source_position sp) 
{
    subder_ptr sd = new_subder(ar.rule);
    sd->source_begin = sp;
    sd->source_end = sp + 1;
    sd->lexunder = true;
    return sd;
}

subder_ptr from_idx(aligned_rule const& ar, subder::source_position sp)
{
    subder_ptr sd = new_subder(ar.rule);
    sd->source_begin = sp;
    sd->source_end = sp + 1;
    sd->varonly = true;
    return sd;
}

subder_ptr from_src(aligned_rule const& ar, subder::source_position sp)
{
    if (sp->indexed()) return from_idx(ar,sp);
    else return from_wd(ar,sp);
}

subder_ptr binarize_unaligned( aligned_rule const& ar
                             , subder_ptr const& sd )
{
    std::vector<subder_ptr> final;
    for ( subder::source_position spos = ar.rule.rhs_begin()
        ; spos != sd->source_begin
        ; ++spos ) {
        final.push_back(from_wd(ar,spos));
    }
    subder::target_position root = sd->root;
    sd->root = 0;

    final.push_back(sd);
    if (sd->source_begin < sd->source_end) for ( subder::source_position spos = sd->source_end
        ; spos != ar.rule.rhs_end()
        ; ++spos ) {
        final.push_back(from_wd(ar,spos));
    }
    if (final.size() > 1) {
        subder_ptr s = make_binarize_rhs(make_contig_block(ar,true))(final).front();
        //setroot(s,root);
        s->root = root;
        return s;
    } else { 
        //setroot(sd,root);
        sd->root = root;
        return sd; 
    }
}

void push_unaligned( aligned_rule const& ar
                   , subder_ptr const& sd
                   , std::deque<subder_ptr>& rhs )
{
    std::vector<subder_ptr> final;
    for ( subder::source_position spos = ar.rule.rhs_begin()
        ; spos != sd->source_begin
        ; ++spos ) final.push_back(from_wd(ar,spos));

    for ( std::vector<subder_ptr>::reverse_iterator ri = final.rbegin()
        ; ri != final.rend()
        ; ++ri ) rhs.push_front(*ri);

    if (sd->source_begin < sd->source_end) for ( subder::source_position spos = sd->source_end
        ; spos != ar.rule.rhs_end()
        ; ++spos ) rhs.push_back(from_wd(ar,spos));

}

std::deque<subder_ptr>
binarize_step(aligned_rule const& ar, std::deque<subder_ptr> rhs, virtmap& virts)
{
    rhs = binarize_subregions(
            rhs
          , varonly_or_lexunder()
          , make_binarize_rhs_memo_recurse(
              ar
            , make_contig_block(ar,true)
            , virts
            )
          );
    
    rhs = binarize_subregions(
            rhs
          , lexunder()
          , make_binarize_rhs_memo_2stage( 
              ar
            , make_contig_block(ar,true,lexunder())
            , virts
            )
          );
          
    rhs = binarize_subregions(
            rhs
          , varonly_or_lexunder()
          , make_binarize_rhs_memo_2stage(
              ar
            , make_contig_block(ar,true,lexunder())
            , virts
            )
          );
    
    rhs = binarize_subregions(
            rhs
          , varonly_or_lexunder()
          , make_binarize_rhs_memo_2stage(
              ar
            , make_contig_block(ar,true,varonly_or_lexunder())
            , virts
            )
          );

    rhs = make_binarize_rhs_memo_2stage(ar,make_contig_block(ar,true),virts)(rhs);
    return rhs;
}

/*
boost::tuple<bool,subder_ptr>
binarize_non_composed(aligned_rule const& ar)
{
    std::deque<subder_ptr> rhs;
    for (subder::source_position sp = ar.rule.rhs_begin(); sp != ar.rule.rhs_end(); ++sp) {
        rhs.push_back(from_src(ar,sp));
    }
    
    if ( ar.rule.rhs_size() == 1 and 
         rhs.size() == 1 and 
         std::distance(rhs.front()->source_begin, rhs.front()->source_end) == 1
       )  {
         subder_ptr s(new subder(*sd));
         s->children.push_back(rhs.front());
         return boost::make_tuple(true,s);
    }
    
    rhs = binarize_step(ar,rhs);
    
    subder_ptr s = rhs.front();
    s->root = ar.rule.lhs_root();
    //the following ensures we grab english unaligned
    s->target_begin = ar.rule.lhs_begin();
    s->target_end = ar.rule.lhs_end();

    return boost::make_tuple(rhs.size() == 1,s);
}
*/

boost::tuple<bool,subder_ptr,aligned_rule>
binarize_robust( std::string line
               , bool nocompose
               , std::set<std::string> const& pointer_features
               , virtmap& virts )
{
    aligned_rule ar(line, pointer_features);
    bool lift_nonlex = false;
    for (subder::source_position sp = ar.rule.rhs_begin(); sp != ar.rule.rhs_end(); ++sp) {
        if (not sp->indexed()) {
            lift_nonlex = true;
            break;
        }
    }

    subder_ptr sd = get_subder(ar,*ar.rule.lhs_root(),true,lift_nonlex,nocompose);

    bool b;
    subder_ptr sb;
    boost::tie(b,sb) = binarize(ar,sd,virts);
    

    if (not b) {
        ar = aligned_rule(line,pointer_features,true);
        subder_ptr sd = get_subder(ar,*ar.rule.lhs_root(),true,lift_nonlex,true);
        boost::tie(b,sb) = binarize(ar,sd,virts);
    }
    
    return boost::make_tuple(b,sb,ar);
}

boost::tuple<bool,subder_ptr> 
binarize(aligned_rule const& ar, subder_ptr const& sd, virtmap& virts)
{
    std::vector<subder_ptr> const& rhs_pre_pre = sd->rhs;
    
    // first priority is to binarize in terms of composition of rules
    std::deque<subder_ptr> rhs;
    BOOST_FOREACH(subder_ptr const& sc, rhs_pre_pre) {
        if (subder_internal(sc)) {
            bool b;
            subder_ptr s;
            boost::tie(b,s) = binarize(ar,sc,virts);
            if (not b) return boost::make_tuple(b,s);
            rhs.push_back(s);
        } else {
            rhs.push_back(sc);
        }
    }

    // the unary rule specialization...
    if ( ar.rule.rhs_size() == 1 and 
         rhs.size() == 1 and 
         std::distance( rhs.front()->source_begin
                      , rhs.front()->source_end ) == 1 and
         sd->root == ar.rule.lhs_root() )  {
         subder_ptr s(new subder(*sd));
         s->children.push_back(rhs.front());
         return boost::make_tuple(true,s);
    }
    
    if (sd->root == ar.rule.lhs_root()) push_unaligned(ar,sd,rhs);
    // 130807 144679 148127 179705 212748 227093 
//    BOOST_FOREACH(subder_ptr const& sc, rhs) {
//        if (sc->varonly and not subder_is_index(sc)) sc->varonly = false;
//        if (sc->lexunder and not subder_is_lex(sc)) sc->lexunder = false;
//    }
// X(X("a") X("b") x0:X x1:X X(x2:X X("c") X("d") x3:X) x4:X x5:X X("e") X("f")) -> "A" "B" x0 x1 x2 "C" "D" x3 x4 x5 "E" "F" ### id=1 align={{{[ #s=12 #t=12 0,0 1,1 2,2 3,3 4,4 5,5 6,6 7,7 8,8 9,9 10,10 11,11 ]}}}    
    // second priority is to binarize in terms of non-virtual non-terminals
    
    rhs = binarize_step(ar,rhs,virts);
    
    subder_ptr s = rhs.front();
    //setroot(s,sd->root);
    s->root = sd->root;
    //the following ensures we grab english unaligned, which changes potential label name...
    s->target_begin = sd->target_begin;
    s->target_end = sd->target_end;
    s->binname = "";
    
    //if (s->root == ar.rule.lhs_root()) s = binarize_unaligned(ar,s);
    return boost::make_tuple(rhs.size() == 1,s);
}

virtmap memotable_in(std::istream& in)
{
    virtmap v;
    std::string m;
    while (in >> m) {
        v.insert(std::make_pair(m,""));
    }
    return v;
}

std::ostream& memotable_out(std::ostream& out, virtmap const& v)
{
    BOOST_FOREACH(virtmap::value_type const& vv, v) {
        out << vv.first << '\n';
    }
    return out;
}

bool print_lm_str_cmp(subder_ptr sb, aligned_rule const& ar) {
    virtmap v;
    std::cerr << "error binarizing:\n"
              << ar.rule << '\n'
              << xrs_lm_string_from_bin(sb,ar) << " <-- from-brf -- from-xrs --> " << xrs_lm_string(ar) << '\n';
    generate_bin_rules(std::cerr,sb,ar,v);
    BOOST_FOREACH(virtmap::value_type const& vr, v) {
        if (vr.second != "") {
            std::cerr << "V: "<< vr.first << " -> " << vr.second << '\n';
        }
    }
    return false;
}


