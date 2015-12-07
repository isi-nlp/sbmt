# ifndef RULE_DECOMPOSE__BINALGO_HPP
# define RULE_DECOMPOSE__BINALGO_HPP 1

# include <xrsparse/xrs.hpp>
# include <string>
# include <vector>
# include <iostream>
# include <deque>
# include <iterator>
# include <sbmt/grammar/alignment.hpp>
# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/grammar/lm_string.hpp>
# include <sbmt/hash/ref_array.hpp>
# include <graehl/shared/fileargs.hpp>
# include <graehl/shared/program_options.hpp>
# include <boost/bimap.hpp>
# include <boost/bimap/multiset_of.hpp>
# include <boost/tuple/tuple_comparison.hpp>
# include <boost/range.hpp>
# include <boost/program_options.hpp>
# include <boost/algorithm/string.hpp>
# include <boost/regex.hpp>

 
typedef std::map<std::string,std::string> virtmap;

struct aligned_rule {
    typedef boost::bimap<
              boost::bimaps::multiset_of<
                sbmt::fatter_syntax_rule::lhs_preorder_iterator
              >
            , boost::bimaps::multiset_of<
                sbmt::fatter_syntax_rule::rhs_iterator
              >
            > align_data;
    sbmt::fatter_syntax_rule rule;
    rule_data::feature_list features;
    std::map<int,int> id_map;
    std::map<int,int> id_map_inv;
    align_data align;
    
    typedef std::map<sbmt::fatter_syntax_rule::rhs_iterator,std::string> pointer_map;
    typedef std::map<std::string,pointer_map> pointer_map_set;
    
    pointer_map_set pointer_maps;
    
    aligned_rule() {}
    aligned_rule( rule_data const& line
                , std::set<std::string> const& pointers = std::set<std::string>()
                , bool empty_align=false );
    aligned_rule( std::string const& line
                , std::set<std::string> const& pointers = std::set<std::string>()
                , bool empty_align=false );
private:
    void construct( rule_data const& line
                  , std::set<std::string> const& pointers
                  , bool empty_align );
};

template <class T,class C>
void print_rec( std::basic_ostream<T,C>& out
              , sbmt::fatter_syntax_rule::tree_node const& n
              , std::map<int,int> const& idmap )
{
    sbmt::print_recurse<T,C,sbmt::fat_token>(out,n,idmap);
}

template <class T, class C>
void print_nd( std::basic_ostream<T,C>& out
             , sbmt::fatter_syntax_rule::rule_node const& n
             , sbmt::fatter_syntax_rule const& r
             , std::map<int,int> const& idmap )
{
    sbmt::print_node<T,C,sbmt::fat_token>(out,n,r,idmap);
}

struct subder;
struct ruletree;

typedef boost::shared_ptr<subder> subder_ptr;
struct subder {
    typedef sbmt::fatter_syntax_rule::lhs_preorder_iterator target_position;
    target_position target_begin, target_end;
    
    typedef sbmt::fatter_syntax_rule::rhs_iterator source_position;
    source_position source_begin, source_end;
    bool valid;
    int index;
    sbmt::fatter_syntax_rule::tree_node const* root;
    std::list<subder_ptr> children;
    std::vector<subder_ptr> rhs;
    bool varonly;
    bool lexunder;
    std::string binname;
    aligned_rule rule;
    subder(sbmt::fatter_syntax_rule const& r)
      : target_begin(r.lhs_end())
      , target_end(r.lhs_begin())
      , source_begin(r.rhs_end())
      , source_end(r.rhs_begin())
      , valid(false)
      , index(-1)
      , root(0)
      , varonly(false)
      , lexunder(false)
      {}
};


bool subder_has_rhs_lex( subder_ptr const& sd
                       , aligned_rule const& ar );

void expand_source( subder_ptr& sd
                  , subder::source_position m
                  , subder::source_position M );

void expand_source(subder_ptr& s, subder_ptr const& ss);

void expand_target( subder_ptr& sd
                  , subder::target_position m
                  , subder::target_position M );

void expand_target(subder_ptr& s, subder_ptr const& ss);

std::pair<subder::source_position,subder::source_position> 
source_range(subder_ptr const& sd);

std::pair<subder::target_position,subder::target_position> 
target_range(subder_ptr const& sd);

// true if represents index in composed rule.
bool subder_is_index(subder_ptr const& sd);

// true if represents lexical rhs item.
bool subder_is_lex(subder_ptr const& sd);

bool subder_internal(subder_ptr const& sd);

struct subder_sort {
    typedef bool result_type;
    bool operator()(subder_ptr const& s1, subder_ptr const& s2) const
    {
        return s1->source_end < s2->source_end;
    }
};

subder_ptr new_subder(sbmt::fatter_syntax_rule const& r);

template <class Output>
boost::tuple<std::list<subder_ptr>::const_iterator,int,Output>
assemble_rhs( subder_ptr const& sd
            , aligned_rule const& ar
            , sbmt::fatter_syntax_rule::tree_node const* r
            , std::list<subder_ptr>::const_iterator si
            , int index
            , Output out )
{
    if (r->indexed()) {
        //std::cerr << ">>indexed "<<index<<":"<<r->get_token().label()<<"\n";
        subder_ptr s = new_subder(ar.rule);
        s->target_begin = r;
        s->target_end = r + 1;
        s->source_begin = ar.rule.rhs_begin() + r->index();
        s->source_end = s->source_begin + 1;
        s->index = index++;
        s->root = r;
        s->varonly = true;
        s->lexunder = false;
        *out = s;
        ++out;
    } else if (si != sd->children.end() and (*si)->root == r /*and ((*si)->source_begin <= (*si)->source_end)*/) {
        //std::cerr << ">>child "<<index<<":"<< r->get_token().label()<<"\n";
        subder_ptr s = *si;
        s->index = index++;
        *out = s;
        ++out;
        ++si;
    } else if (not r->lexical()) {
        //std::cerr << ">>recurse\n";
        sbmt::fatter_syntax_rule::lhs_children_iterator ci = r->children_begin(),
                                                     ce = r->children_end();

        for (; ci != ce; ++ci) {
            boost::tie(si,index,out) = assemble_rhs(sd,ar,&(*ci),si,index,out);
        }
        //std::cerr << ">>endrecurse\n";
    }
    return boost::make_tuple(si,index,out);
}

std::vector<subder_ptr> assemble_rhs(subder_ptr const& sd, aligned_rule const& ar);

void print_subder_align( std::ostream& out
                       , subder_ptr const& sd
                       , aligned_rule const& ar );

void print_subder_rhs( std::ostream& out
                     , subder_ptr const& sd
                     , aligned_rule const& ar );

boost::tuple<int,std::list<subder_ptr>::const_iterator>
print_subder_lhs( std::ostream& out
                , subder_ptr const& sd
                , sbmt::fatter_syntax_rule::tree_node const* r
                , int idx
                , std::list<subder_ptr>::const_iterator c
                );

void print_subder(std::ostream& out, subder_ptr const& sd, aligned_rule const& ar);
void print_subder_mr(std::ostream& out, subder_ptr const& sd, aligned_rule const& ar);

template <class Output>
Output subder_vars(aligned_rule const& ar, subder_ptr const& sd, Output out)
{
    std::vector<subder_ptr> const& rhs = sd->rhs;
    int idx = 0;

    print_subder(std::cerr,sd,ar);

    BOOST_FOREACH(subder_ptr const& r,rhs) {
        if (subder_is_index(r)) {
            *out = boost::make_tuple( 
                     ar.id_map_inv.find(r->root->index())->second
                   , idx++
                   , sd 
                   );
            ++out;
        } else if (not subder_is_lex(r)) {
            out = subder_vars(ar,r,out);
            ++idx;
        }
    }
    return out;
}

template <class Range>
bool leaf_nonlex(Range const& rng)
{
    BOOST_FOREACH(subder_ptr const& sd, rng) {
        if (not subder_is_index(sd)) {
            //std::cerr << "LEXICAL\n";
            return false;
        }
    }
    //std::cerr << "NONLEXICAL\n";
    return true;
}

subder_ptr get_subder( aligned_rule& ar
                     , sbmt::fatter_syntax_rule::tree_node const& n
                     , bool collapse_unary = false
                     , bool lift_leaf_nonlex = false
                     , bool forgetit = false
                     , bool leaf_or_nonlex = false
                     , bool epsilon_rules = false );

subder_ptr get_subder( aligned_rule& ar
                     , bool collapse_unary = false
                     , bool lift_leaf_nonlex = false
                     , bool forgetit = false
                     , bool leaf_or_nonlex = false
                     , bool epsilon_rules = false )
{
    return get_subder(ar,*ar.rule.lhs_root(), collapse_unary,lift_leaf_nonlex,forgetit,leaf_or_nonlex,epsilon_rules);
}

void print_subders( std::ostream& out
                  , subder_ptr const& sd
                  , aligned_rule const& ar
                  , int x = 0 );

void print_subders_mr( std::ostream& out
                       , subder_ptr const& sd
                       , aligned_rule const& ar );


void print_subder_vars( std::ostream& out
                      , subder_ptr const& sd
                      , aligned_rule const& ar);

typedef boost::tuple<subder::target_position,subder::target_position> 
        target_tuple;

bool intersect(target_tuple const& tr1, target_tuple const& tr2);

void union_range(std::vector<target_tuple>& rngset, target_tuple rng);

// NP-C-1(NPB-0(DT-1("the") x0:NN-2) x1:PP-0 x2:PP-0) -> x1 x0 x2 ### id=255086671 count=10^-219 align={{{[ #s=3 #t=4 0,2 1,1 2,3 ]}}} 
// V[PP-0_NN-2]["the"_1_0] -> NN-2 PP-0
// NP-C-1 -> V[PP-0_NN-2]["the"_1_0] PP-0 ### lm_string=
void union_range( std::vector<target_tuple>& rngset
                , std::pair<subder::target_position,subder::target_position> p);

struct always_true {
    bool operator()(subder_ptr const& s) const
    {
        return true;
    }
};

template <class OP>
boost::tuple<bool,subder_ptr> 
contig_block_( aligned_rule const& ar
             , sbmt::reference_array<subder_ptr,2> const& sds
             , bool target_unaligned_allowed
             , OP const& op )
{
    std::vector<target_tuple> tgtrng;
    subder_ptr s = new_subder(ar.rule);
    s->varonly = sds[0]->varonly and sds[1]->varonly;
    s->lexunder = (sds[0]->lexunder and sds[1]->lexunder) or
                  (sds[0]->lexunder and sds[1]->varonly) or
                  (sds[0]->varonly and sds[1]->lexunder);
    bool disjoint = (sds[0]->target_begin > sds[0]->target_end) or
                    (sds[1]->target_begin > sds[1]->target_end) or
                    (sds[0]->target_end <= sds[1]->target_begin) or
                    (sds[1]->target_end <= sds[0]->target_begin);
    if (not disjoint) return boost::make_tuple(false,s);
    
    BOOST_FOREACH(subder_ptr const& sd, sds) {
        expand_source(s,sd);
        if (std::distance(sd->source_begin,sd->source_end) > 1) {
            if (sd->target_begin < sd->target_end) {
                union_range(tgtrng,target_range(sd));
            }
        } else {
            if (sd->target_begin < sd->target_end) {
                union_range(tgtrng,target_range(sd));
            }
            BOOST_FOREACH( aligned_rule::align_data::right_map::value_type rp
                         , ar.align.right.equal_range(sd->source_begin) ) {
                union_range(tgtrng, std::make_pair(rp.second, rp.second + 1));
            } 
        }
    }

    if (tgtrng.empty()) return boost::make_tuple(true,s);
    std::sort(tgtrng.begin(),tgtrng.end());
    
    target_tuple tr = *tgtrng.begin();
    expand_target(s, tr.get<0>(), tr.get<1>());
    
    std::vector<target_tuple>::iterator ti = ++tgtrng.begin();
    for (; ti != tgtrng.end(); ++ti) {
        assert(ti->get<0>() > tr.get<1>());
        if (ti->get<0>() > tr.get<1>()) {
            if (target_unaligned_allowed == false) {
                return boost::make_tuple(false,s);
            }
            for (subder::target_position t = tr.get<1>(); t != ti->get<0>(); ++t) {
                if (ar.align.left.find(t) != ar.align.left.end()) {
                    return boost::make_tuple(false,s);
                }
            }
            s->varonly = false;
        }
        expand_target(s,ti->get<0>(),ti->get<1>());
        tr = *ti;
    }
    return boost::make_tuple(op(s),s);
}

template <class OP = always_true>
struct contig_block {
    aligned_rule const* par;
    bool target_unaligned_allowed;
    OP op;
    contig_block(aligned_rule const& ar, bool tua,OP const& op)
    : par(&ar)
    , target_unaligned_allowed(tua)
    , op(op) {}
    
    boost::tuple<bool,subder_ptr> 
    operator()(sbmt::reference_array<subder_ptr,2> const& sds) const
    {
        return contig_block_(*par,sds,target_unaligned_allowed,op);
    }
};

template <class O>
contig_block<O> make_contig_block(aligned_rule const& ar, bool tua, O const& o)
{
    return contig_block<O>(ar,tua,o);
}

contig_block<always_true> make_contig_block(aligned_rule const& ar, bool tua);

bool in_children(subder::target_position tpos, subder_ptr const& sd);

bool beginning_of_children(subder::target_position tpos, subder_ptr const& sd);

void print_bin_features( std::ostream& out
                       , subder_ptr const& sd
                       , aligned_rule const& ar );

std::string const& binname(subder_ptr const& sd, aligned_rule const& ar);

void print_toplevel( std::ostream& out
                   , aligned_rule const& ar
                   , std::pair<std::string,std::string> const& vl );

void generate_bin_rules( std::ostream& out
                       , subder_ptr const& sd
                       , aligned_rule const& ar
                       , std::map<std::string,std::string>& m
                       , bool top = true );

sbmt::fat_lm_string 
bin_lm_string(subder_ptr const& sd, aligned_rule const& ar);

sbmt::fat_lm_string
xrs_lm_string(aligned_rule const& ar);

sbmt::fat_lm_string 
xrs_lm_string_from_bin(subder_ptr const& sd, aligned_rule const& ar);

template <class Range,class COMBOP>
std::deque<subder_ptr>&
binarize_rhs_memo_recurse_( aligned_rule const& ar
                          , Range rhs
                          , COMBOP const& combop
                          , std::map< 
                              boost::tuple<
                                typename boost::range_iterator<Range>::type
                              , typename boost::range_iterator<Range>::type
                              >
                            , std::deque<subder_ptr>
                            > & chart 
                          , virtmap& virts
                          )
{
    typename std::map< 
          boost::tuple<
            typename boost::range_iterator<Range>::type
          , typename boost::range_iterator<Range>::type
          >
        , std::deque<subder_ptr>
        >::iterator pos;

    typename boost::range_iterator<Range>::type 
        begin = boost::begin(rhs),
        end = boost::end(rhs),
        curr;
    pos = chart.find(boost::make_tuple(begin,end));
    if (pos != chart.end()) return pos->second;
    std::deque<subder_ptr> d;
    //subder_ptr sd;
    assert(begin != end);
    if (std::distance(begin,end) == 1) {
        return chart.insert(std::make_pair(boost::make_tuple(begin,end),std::deque<subder_ptr>(begin,end))).first->second;
    }
    curr = begin; ++curr;    
    for (; curr != end; ++curr) {
        std::deque<subder_ptr>& l = binarize_rhs_memo_recurse_( ar
                                                              , std::make_pair(begin,curr)
                                                              , combop 
                                                              , chart
                                                              , virts );
        std::deque<subder_ptr>& r = binarize_rhs_memo_recurse_( ar
                                                              , std::make_pair(curr,end)
                                                              , combop
                                                              , chart
                                                              , virts );
        assert(not l.empty());
        assert(not r.empty());
        if (l.size() == 1 and r.size() == 1) {
            subder_ptr s; bool b;
            boost::tie(b,s) = combop(sbmt::ref_array(l.front(),r.front()));
            if (b) {
                
                if (virts.find(binname(s,ar)) != virts.end()) {
                    s->children.push_back(l.front());
                    s->children.push_back(r.front());
                    return chart.insert(std::make_pair(boost::make_tuple(begin,end),std::deque<subder_ptr>(1,s))).first->second;
                }
            } 
        }
        if (d.empty() or (l.size() + r.size()) < d.size()) {
            d = l;
            d.insert(d.end(),r.begin(),r.end());
        }  
    }
    return chart.insert(std::make_pair(boost::make_tuple(begin,end),d)).first->second;
}

template <class CombineOp>
struct binarize_rhs_memo_recurse {
    binarize_rhs_memo_recurse(aligned_rule const& ar, CombineOp const& c, virtmap& virts)
    : par(&ar)
    , combine(c)
    , virts(&virts) {}
    template <class Range>
    std::deque<subder_ptr> operator()(Range rhs) const
    {
        std::map< 
              boost::tuple<
                typename boost::range_iterator<Range>::type
              , typename boost::range_iterator<Range>::type
              >
            , std::deque<subder_ptr>
            > chart;
        return binarize_rhs_memo_recurse_(*par,rhs,combine,chart,*virts);
    }
private:
    aligned_rule const* par;
    CombineOp combine;
    virtmap* virts;
};

template <class C>
binarize_rhs_memo_recurse<C>
make_binarize_rhs_memo_recurse(aligned_rule const& ar, C const& c,virtmap& virts)
{
    return binarize_rhs_memo_recurse<C>(ar,c,virts);
}

template <class CombineOp> 
struct binarize_rhs {
    binarize_rhs(CombineOp const& combine) : combine(combine) {}
    template <class Range> 
    std::deque<subder_ptr> operator()(Range const& rhs) const
    {
        std::deque<subder_ptr> stk;
        std::reverse_iterator<typename boost::range_iterator<Range const>::type>
            ritr(boost::end(rhs)), rend(boost::begin(rhs));

        while (ritr != rend) {
            stk.push_front(*ritr); ++ritr;
            while (stk.size() > 1) {
                subder_ptr s;
                bool go;
                boost::tie(go,s) = combine(sbmt::ref_array(stk[0],stk[1]));
                if (go) {
                    s->children.push_back(stk[0]);
                    s->children.push_back(stk[1]);
                    stk.pop_front();
                    stk.pop_front();
                    stk.push_front(s);
                } else {
                    break;
                }
            }
        }
        return stk;
    }
private:
    CombineOp combine;
};

template <class C>
binarize_rhs<C> make_binarize_rhs(C const& c)
{
    return binarize_rhs<C>(c);
}

template <class CombineOp> 
struct binarize_rhs_memo_2stage {
    binarize_rhs_memo_2stage( aligned_rule const& ar
                            , CombineOp const& combine 
                            , virtmap& virt )
    : par(&ar)
    , combine(combine)
    , pvirt(&virt) {}
    
    template <class Range>
    std::deque<subder_ptr> operator()(Range const& rhs) const
    {
        std::deque<subder_ptr> 
            sdv = make_binarize_rhs_memo_recurse(*par,make_contig_block(*par,true),*pvirt)(rhs);
        if (sdv.size() > 1) {
            sdv = make_binarize_rhs(combine)(sdv);
        }
        return sdv;
    }
private:
    aligned_rule const* par;
    CombineOp combine;
    virtmap* pvirt;
};

template <class CombineOp>
binarize_rhs_memo_2stage<CombineOp>
make_binarize_rhs_memo_2stage(aligned_rule const& ar, CombineOp const& combine,virtmap& virts)
{
    return binarize_rhs_memo_2stage<CombineOp>(ar,combine,virts);
}

subder_ptr from_wd(aligned_rule const& ar, subder::source_position sp);

subder_ptr from_idx(aligned_rule const& ar, subder::source_position sp);

subder_ptr from_src(aligned_rule const& ar, subder::source_position sp);

subder_ptr binarize_unaligned( aligned_rule const& ar
                             , subder_ptr const& sd );

void push_unaligned( aligned_rule const& ar
                   , subder_ptr const& sd
                   , std::deque<subder_ptr>& rhs );

template <class Range, class OP, class BINOP>
std::deque<subder_ptr>
binarize_subregions( Range const& rng
                   , OP const& op
                   , BINOP const& binop )
{
    typename boost::range_iterator<Range const>::type beg = boost::begin(rng),
                                                      end = boost::end(rng),
                                                      cur;
    std::deque<subder_ptr> rhs;
    int count = 0;
    for (cur = beg; cur != end; ++cur) {
        if (op(*cur)) {
            if (count == 0) beg = cur;
            ++count;
        } else {
            if (count >= 1) {
                std::deque<subder_ptr> sdv = binop(std::make_pair(beg,cur));
                rhs.insert(rhs.end(),sdv.begin(),sdv.end());
            }
            count = 0;
            beg = cur;
            rhs.push_back(*cur);
        }
    }
    if (count >= 1) {
        std::deque<subder_ptr> sdv = binop(std::make_pair(beg,cur));
        rhs.insert(rhs.end(), sdv.begin(), sdv.end());
    }
    return rhs;
}

struct varonly_or_lexunder {
    bool operator()(subder_ptr const& sd) const
    {
        return sd->varonly or sd->lexunder;
    }
};

struct lexunder {
    bool operator()(subder_ptr const& sd) const
    {
        return sd->lexunder;
    }
};

struct varonly {
    bool operator()(subder_ptr const& sd) const
    {
        return sd->varonly;
    }
};

std::deque<subder_ptr>
binarize_step(aligned_rule const& ar, std::deque<subder_ptr> rhs, virtmap& virts);

boost::tuple<bool,subder_ptr> 
binarize(aligned_rule const& ar, subder_ptr const& sd,virtmap& virts);

virtmap memotable_in(std::istream& in);

std::ostream& memotable_out(std::ostream& out, virtmap const& v);

bool print_lm_str_cmp(subder_ptr sb, aligned_rule const& ar);

boost::tuple<bool,subder_ptr,aligned_rule>
binarize_robust( std::string line
               , bool nocompose
               , std::set<std::string> const& pointer_features
               , virtmap& virts );

# endif // ifndef RULE_DECOMPOSE__BINALGO_HPP
