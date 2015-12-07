# if ! defined(SBMT__SEARCH__LAZY__RULES_HPP)
# define       SBMT__SEARCH__LAZY__RULES_HPP

# include <sbmt/search/lazy/indexed_varray.hpp>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <boost/array.hpp>
# include <boost/range.hpp>
# include <boost/iterator/filter_iterator.hpp>
# include <boost/iterator/transform_iterator.hpp>
# include <gusc/hash/boost_array.hpp>
# include <gusc/generator/generator_from_iterator.hpp>
# include <gusc/generator/lazy_sequence.hpp>
# include <sbmt/search/lattice_reader.hpp>

namespace sbmt { namespace lazy {

enum rhs_side {rhs_left, rhs_right};

// the binary rules that have a common rhs.
// sorted by rule score
typedef gusc::shared_varray<grammar_in_mem::rule_type> ruleset;
typedef std::multimap<score_t, grammar_in_mem::rule_type, gusc::greater> preruleset;

struct get_unary_rhs {
    typedef indexed_token result_type;
    result_type operator()(ruleset const& rs) const
    {
        return operator()(rs[0]);
    }
    result_type operator()(preruleset const& rs) const
    {
        return operator()(rs.begin()->second);
    }
    result_type operator()(grammar_in_mem::rule_type r) const
    {
        result_type rhs = gram->rule_rhs(r,0);
        return rhs;
    }
    get_unary_rhs(grammar_in_mem const& gram) : gram(&gram) {}
    grammar_in_mem const* gram;
};

struct get_rhs {
    typedef boost::array<indexed_token,2> result_type;
    result_type operator()(ruleset const& rs) const
    {
        return operator()(rs[0]);
    }
    result_type operator()(preruleset const& rs) const
    {
        return operator()(rs.begin()->second);
    }
    result_type operator()(grammar_in_mem::rule_type r) const
    {
        result_type rhs = {{ gram->rule_rhs(r,0), gram->rule_rhs(r,1) }};
        return rhs;
    }
    get_rhs(grammar_in_mem const& gram) : gram(&gram) {}
    grammar_in_mem const* gram;
};

struct get_best_score {
    typedef score_t result_type;
    result_type operator()(preruleset const& rs) const
    {
        return rs.begin()->first;
    }
};

// the binary rules that have a common rhs[0] or rhs[1]. can be queried
// by full rhs.
typedef shared_indexed_varray<ruleset,get_rhs> rulesetset;

typedef boost::multi_index_container<
          preruleset
        , boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<get_rhs>
          , boost::multi_index::ordered_non_unique<get_best_score,gusc::greater>
          >
        > prerulesetset;

typedef shared_indexed_varray<ruleset,get_unary_rhs> unary_rulemap;
typedef boost::multi_index_container<
          preruleset
        , boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<get_unary_rhs>
          , boost::multi_index::ordered_non_unique<get_best_score,gusc::greater>
          >
        > unary_prerulemap;

struct get_partial_rhs {
    rhs_side side;
    typedef indexed_token result_type;
    indexed_token operator()(rulesetset const& rss) const
    {
        return operator()(rss[0][0]);
    }

    indexed_token operator()(grammar_in_mem::rule_type const& r) const
    {
        int s = side == rhs_left ? 0 : 1;
        return gram->rule_rhs(r,s);
    }

    indexed_token operator()(prerulesetset const& rss) const
    {
        return operator()(rss.begin()->begin()->second);
    }
    grammar_in_mem const* gram;

    get_partial_rhs(grammar_in_mem const& gram, rhs_side side)
    : side(side)
    , gram(&gram) {}
};

//
typedef oa_hashtable<rulesetset,get_partial_rhs> rulemap;

typedef boost::multi_index_container<
          prerulesetset
        , boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<get_partial_rhs>
          >
        > prerulemap;

ruleset make_ruleset(preruleset const& prs);

rulesetset make_rulesetset(prerulesetset const& prss);

struct insert_preruleset {
    preruleset* prs;
    insert_preruleset(preruleset& prs) : prs(&prs) {}
    void operator()(prerulesetset& prss) const
    {
        prss.insert(*prs);
    }
};

struct insert_scored_rule {
    typedef std::pair<score_t,grammar_in_mem::rule_type> scored_rule;
    struct insert {
        scored_rule sr;
        insert(scored_rule const& sr) : sr(sr) {}
        void operator()(preruleset& prs) const
        {
            prs.insert(sr);
        }
    };
    insert inserter;
    prerulesetset::iterator pos;
    insert_scored_rule(prerulesetset::iterator pos, scored_rule const& sr)
    : inserter(sr)
    , pos(pos) {}
    void operator()(prerulesetset& prss) const
    {
        prss.modify(pos,inserter);
    }
};


template <class Edge>
rulemap make_rulemap( grammar_in_mem const& gram
                    , concrete_edge_factory<Edge,grammar_in_mem>& ef
                    , rhs_side side
                    , bool toplevel_rules = false )
{
    get_partial_rhs prhs(gram,side);
    get_rhs rhs(gram);
    prerulemap::ctor_args_list
      args(prerulemap::nth_index<0>::type::ctor_args(0,prhs));
    prerulemap prm(args);

    BOOST_FOREACH(grammar_in_mem::rule_type r, gram.all_rules()) { //FIXME: in debug, iterator_range::singular assertion (default init range).  how?  see grammary_in_memory::all_rules()
        if (gram.rule_rhs_size(r) != 2) continue;
        if ((gram.rule_lhs(r).type() == top_token) != toplevel_rules) continue;
        std::pair<score_t,grammar_in_mem::rule_type>
            input(ef.rule_heuristic(gram,r), r);
        indexed_token tok = prhs(r);
        boost::array<indexed_token,2> toks = rhs(r);
        prerulemap::iterator pos = prm.find(tok);
        if (pos == prm.end()) {
            prerulesetset::ctor_args_list
                args2( prerulesetset::nth_index<0>::type::ctor_args(0,rhs)
                     , prerulesetset::nth_index<1>::type::ctor_args()
                     );
            prerulesetset prss(args2);
            preruleset prs;
            prs.insert(input);
            prss.insert(prs);
            prm.insert(prss);
        } else {
            prerulesetset::iterator p = pos->find(toks);
            if (p == pos->end()) {
                preruleset prs;
                prs.insert(input);
                prm.modify(pos,insert_preruleset(prs));
            } else {
                prm.modify(pos,insert_scored_rule(p,input));
            }
        }
    }

    rulemap rm(0,prhs);

    BOOST_FOREACH(prerulesetset const& prss, prm)
    {
        rm.insert(make_rulesetset(prss));
    }

    return rm;
}

template <class Edge>
unary_rulemap
make_unary_rulemap( grammar_in_mem const& gram
                  , concrete_edge_factory<Edge,grammar_in_mem> const& ef )
{
    get_unary_rhs rhs(gram);

    unary_prerulemap::ctor_args_list
      args( unary_prerulemap::nth_index<0>::type::ctor_args(0,rhs)
          , unary_prerulemap::nth_index<1>::type::ctor_args()
          );
    unary_prerulemap prm(args);

    BOOST_FOREACH(grammar_in_mem::rule_type r, gram.all_rules()) {
        if (gram.rule_rhs_size(r) != 1) continue;
        if (gram.rule_lhs(r).type() == top_token) continue;
        if (is_lexical(gram.rule_rhs(r,0))) continue;

        std::pair<score_t,grammar_in_mem::rule_type>
            input(ef.rule_heuristic(gram,r), r);
        indexed_token tok = rhs(r);

        unary_prerulemap::iterator pos = prm.find(tok);
        if (pos == prm.end()) {
            preruleset prs;
            prs.insert(input);
            prm.insert(prs);
        } else {
            prm.modify(pos,insert_scored_rule::insert(input));
        }
    }

    gusc::shared_varray<ruleset> rv(prm.size());

    size_t x = 0;
    BOOST_FOREACH(preruleset const& prs, prm)
    {
        rv[x] = make_ruleset(prs);
        ++x;
    }

    return unary_rulemap(rv,rhs);
}

typedef std::map<indexed_token,grammar_in_mem::rule_type> lhs_map;

lhs_map make_lhs_map(grammar_in_mem const& g);

typedef boost::tuple<
          boost::tuple<indexed_token,int>
        , boost::tuple<indexed_token,int>
        > lex_distance;

typedef std::map<
          indexed_token
        , lex_distance
        > lex_distance_map;
        
boost::tuple<indexed_token,int>& ld_get(lex_distance& ld, int x);

boost::tuple<indexed_token,int> const& ld_get(lex_distance const& ld, int x);

lex_distance&
make_lex_distance_inside_map( grammar_in_mem const& g
                            , indexed_token lhs
                            , lhs_map const& lmap
                            , lex_distance_map& ldmap );

lex_distance_map
make_lex_distance_inside_map(grammar_in_mem const& g);

typedef std::map<indexed_token, std::set<int> > lex_outside_distances;
typedef std::map<indexed_token, lex_outside_distances> 
        single_lex_distance_outside_map;

typedef std::map< indexed_token
                , boost::tuple<lex_outside_distances,lex_outside_distances>
                > lex_distance_outside_map;
          
typedef std::multimap<
          indexed_token
        , grammar_in_mem::rule_type
        > rhs_map;
        
rhs_map make_rhs_map(grammar_in_mem const& g);
        
void extend_outside_map(lex_outside_distances& outmap, indexed_token tok, int d);

lex_outside_distances&
make_lex_distance_outside_map( grammar_in_mem const& g
                             , lex_distance_map const& limap
                             , rhs_map const& rmap
                             , indexed_token lhs
                             , single_lex_distance_outside_map& lomap
                             , int left_or_right 
                             );
                             
single_lex_distance_outside_map
make_lex_distance_outside_map( grammar_in_mem const& g
                             , lex_distance_map const& limap
                             , int left_or_right 
                             );

lex_distance_outside_map
make_lex_distance_outside_map( grammar_in_mem const& g
                             , lex_distance_map const& limap
                             );

bool rule_compatible( indexed_token tok
                    , lex_distance_outside_map const& ldom
                    , lex_outside_distances const& left
                    , lex_outside_distances const& right );
                    
struct rule_compat {
    rule_compat( grammar_in_mem const& gram
               , span_t spn
               , lex_distance_outside_map const& ldom
               , left_right_distance_map const& lrdm )
    : gram(&gram)
    , ldom(&ldom)
    , left(&lrdm.find(spn.left())->second.get<0>())
    , right(&lrdm.find(spn.right())->second.get<1>()) {}
    
    bool operator()(grammar_in_mem::rule_type r) const
    {
        indexed_token lhs = gram->rule_lhs(r);
        return (not is_virtual_tag(lhs)) or 
               rule_compatible(lhs,*ldom,*left,*right);
    }
private:
    grammar_in_mem const* gram;
    lex_distance_outside_map const* ldom;
    lex_outside_distances const* left;
    lex_outside_distances const* right;
};

typedef gusc::shared_lazy_sequence< 
          gusc::generator_from_range<
            boost::iterator_range<
              boost::filter_iterator<
                rule_compat
              , ruleset::const_iterator
              > 
            >
          >
        > filtered_ruleset;

struct make_filtered_ruleset {
    typedef filtered_ruleset result_type;
    
    make_filtered_ruleset(rule_compat const& filter)
    : filter(filter) {}
    
    filtered_ruleset operator()(ruleset const& rs) const
    {
        return filtered_ruleset(
                 gusc::generate_from_range(
                   boost::make_iterator_range(
                     boost::make_filter_iterator(filter,rs.begin(),rs.end())
                   , boost::make_filter_iterator(filter,rs.end(),rs.end())
                   )
                 )
               );
    }
private:
    rule_compat filter;
};

struct not_empty {
    typedef bool result_type;
    template <class Range>
    bool operator()(Range const& rng) const
    {
        return boost::begin(rng) != boost::end(rng);
    }
};

struct filtered_rulesetset {
    typedef get_rhs::result_type arg_type;
    typedef boost::filter_iterator<
              not_empty
            , boost::transform_iterator<
                make_filtered_ruleset
              , rulesetset::const_iterator
              >
            > iterator;
    typedef iterator const_iterator;
    typedef iterator::value_type value_type;
    typedef iterator::reference reference;
    typedef reference const_reference;
    
    filtered_rulesetset( rulesetset const& rs
                       , rule_compat const& filter ) 
    : rs(rs)
    , filter(filter) {}
    
    iterator find(arg_type const& tok) const 
    {   
        rulesetset::const_iterator pos = rs.find(tok);
        if (pos != rs.end()) {
            if (not not_empty()(make_filtered_ruleset(filter)(*pos))) pos = rs.end();
        }
        return make(pos); 
    }
    iterator begin() const { return make(rs.begin()); }
    iterator end() const { return make(rs.end()); }
    //reference operator[](arg_type const& tok) const { return make(rs[tok]); }
    //reference operator[](size_t tok) const { return make(rs[tok]); }
    bool empty() const { return begin() == end(); }
private:
    iterator make(rulesetset::const_iterator itr) const
    {
        return boost::make_filter_iterator(
                 not_empty()
               , boost::make_transform_iterator(itr,make_filtered_ruleset(filter))
               , boost::make_transform_iterator(rs.end(),make_filtered_ruleset(filter))
               );
    }
    /*
    value_type make(ruleset const& r) const
    {
        return make_filtered_ruleset(filter)(r);
    }*/
    
    rulesetset rs;
    rule_compat filter;
};

struct make_filtered_rulesetset {
    typedef filtered_rulesetset result_type;
    make_filtered_rulesetset(rule_compat const& filter)
    : filter(filter) {}
    filtered_rulesetset operator()(rulesetset const& rss) const
    {
        return filtered_rulesetset(rss,filter);
    }
private:
    rule_compat filter;
};

struct filtered_rulemap {
    typedef boost::filter_iterator<
              not_empty
            , boost::transform_iterator<
                make_filtered_rulesetset
              , rulemap::const_iterator
              > 
            > iterator;
    typedef iterator const_iterator;
    typedef iterator::value_type value_type;
    typedef iterator::reference reference;
    typedef reference const_reference;
    
    filtered_rulemap( rulemap const& rm
                    , grammar_in_mem const& gram
                    , span_t spn
                    , lex_distance_outside_map const& ldom
                    , left_right_distance_map const& lrdm ) 
    : rm(&rm)
    , filter(gram,spn,ldom,lrdm) {}
    
    iterator find(indexed_token tok) const 
    { 
        rulemap::const_iterator pos = rm->find(tok);
        if (pos != rm->end()) {
            if (not not_empty()(make_filtered_rulesetset(filter)(*pos))) pos = rm->end();
        }
        return make(pos);
    }
    iterator begin() const { return make(rm->begin()); }
    iterator end() const { return make(rm->end()); }
private:
    iterator make(rulemap::const_iterator itr) const
    {
        return boost::make_filter_iterator(
                 not_empty()
               , boost::make_transform_iterator(itr,make_filtered_rulesetset(filter))
               , boost::make_transform_iterator(rm->end(),make_filtered_rulesetset(filter))
               );
    }
    rulemap const* rm;
    rule_compat filter;
};

} } // namespace sbmt::lazy

# endif //     SBMT__SEARCH__LAZY__RULES_HPP
