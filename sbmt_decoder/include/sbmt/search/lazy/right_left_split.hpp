# if ! defined(SBMT__SEARCH__LAZY__RIGHT_LEFT_SPLIT_HPP)
# define       SBMT__SEARCH__LAZY__RIGHT_LEFT_SPLIT_HPP

# include <sbmt/span.hpp>
# include <gusc/generator/peekable_generator_facade.hpp>
# include <gusc/generator/union_heap_generator.hpp>
# include <gusc/generator/finite_union_generator.hpp>
# include <gusc/generator/peek_compare.hpp>
# include <sbmt/edge/edge.hpp>
# include <boost/functional/hash.hpp>
# include <boost/foreach.hpp>
# include <sbmt/hash/hash_set.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/array.hpp>
# include <gusc/hash/boost_array.hpp>

namespace sbmt { namespace lazy {

////////////////////////////////////////////////////////////////////////////////

typedef boost::array<indexed_token,2> precube;
enum fixed {fixed_left, fixed_right};

inline precube make_precube(indexed_token x, indexed_token y)
{
    precube pc = {{ x, y }};
    return pc;
}

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleMap, class Cells>
class cell_ordered_precube_generator
: public
    gusc::peekable_generator_facade<
      cell_ordered_precube_generator<Grammar,RuleMap,Cells>
    , precube
    > 
{
public:
    typedef precube result_type;
    
    cell_ordered_precube_generator()
     : gram(NULL)
     , occmap(NULL) {}
    
    template <class Rules>
    cell_ordered_precube_generator( fixed position
                                  , indexed_token const& pc
                                  , Cells const& cells
                                  , Grammar const* gram
                                  , Rules const& range
                                  , stlext::hash_set<precube,boost::hash<precube> >* occmap )
      : fixed_position(position)
      , cell_itr(boost::begin(cells))
      , cell_end(boost::end(cells))
      , gram(gram)
      , rules(range) 
      , pc(pc)
      , occmap(occmap)
    {
        if (rules.empty()) cell_itr = cell_end;
        else set();
    }
      
private:
    fixed fixed_position;
    typename Cells::const_iterator cell_itr;
    typename Cells::const_iterator cell_end;
    Grammar const* gram;
    typename RuleMap::value_type rules;
    indexed_token pc;
    span_t varspan; 
    stlext::hash_set<precube,boost::hash<precube> >* occmap;
    precube curr;
    
    friend class gusc::generator_access;
    
    void pop()
    {
        ++cell_itr;
        set();
    }
    
    bool more() const { return cell_itr != cell_end; }
    
    precube const& peek() const { return curr; }
    
    void set()
    {
        while (more()) {
            indexed_token varroot = cell_itr->begin()->representative().root();
            curr = fixed_position == fixed_left 
                 ? make_precube(pc,varroot)
                 : make_precube(varroot,pc)
                 ;
            typename RuleMap::value_type::iterator pos = rules.find(curr);
            
            if (pos != rules.end()) {
                if (occmap->insert(curr).second) break;
            }
            ++cell_itr;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////
#if 0
template <class Grammar, class RuleMap, class Cells>
class rule_ordered_precube_generator
: public
    gusc::peekable_generator_facade<
      rule_ordered_precube_generator<Grammar,RuleMap,Cells>
    , precube
    > 
{
public:
    typedef precube result_type;
    
    rule_ordered_precube_generator()
     : gram(NULL)
     , occmap(NULL) {}
    
    template <class Rules>
    rule_ordered_precube_generator( fixed position
                                  , indexed_token const& pc
                                  , Cells const& cells
                                  , Grammar const* gram
                                  , Rules const& range
                                  , stlext::hash_set<precube,boost::hash<precube> >* occmap )
      : fixed_position(position)
      , cells(cells)
      , gram(gram)
      , rule_itr(boost::begin(range))
      , rule_end(boost::end(range)) 
      , pc(pc)
      , occmap(occmap)
    {
        set();
    }
      
private:
    fixed fixed_position;
    Cells cells;
    Grammar const* gram;
    typename RuleMap::value_type::const_iterator rule_itr;
    typename RuleMap::value_type::const_iterator rule_end;
    indexed_token pc;
    span_t varspan; 
    stlext::hash_set<precube,boost::hash<precube> >* occmap;
    precube curr;
    
    friend class gusc::generator_access;
    
    void pop()
    {
        ++rule_itr;
        set();
    }
    
    bool more() const { return rule_itr != rule_end; }
    
    precube const& peek() const { return curr; }
    
    void set()
    {
        while (more()) {
            indexed_token varroot = fixed_position == fixed_left 
                                  ? gram->rule_rhs((*rule_itr)[0],1)
                                  : gram->rule_rhs((*rule_itr)[0],0)
                                  ;
            typename Cells::iterator pos = cells.find(varroot);
            if (pos != cells.end()) {
                curr = fixed_position == fixed_left 
                     ? make_precube(pc,varroot)
                     : make_precube(varroot,pc)
                     ;
                if (occmap->insert(curr).second) break;
            }
            ++rule_itr;
        }
    }
};
#endif

template <class Grammar, class RuleMap, class Cells>
struct precube_to_cube {
    typedef typename Cells::value_type Edges;
    typedef typename RuleMap::value_type RulesSet;
    typedef typename RulesSet::value_type Rules;
    typedef typename Edges::value_type::edge_type Edge;
    
    fixed fixed_position;
    Grammar const* gram;
    concrete_edge_factory<Edge,Grammar>* ef;
    RuleMap const* rulemap;
    Cells left;
    Cells right;
    
    typedef cube_processor<Grammar,Rules,Edges> result_type;

    precube_to_cube()
     : gram(NULL)
     , ef(NULL)
     , rulemap(NULL) {}
     
    precube_to_cube( fixed position
                   , Grammar const* gram
                   , concrete_edge_factory<Edge,Grammar>* ef
                   , RuleMap const* rulemap
                   , Cells const& left
                   , Cells const& right )
    : fixed_position(position)
    , gram(gram)
    , ef(ef)
    , rulemap(rulemap)
    , left(left)
    , right(right) {}
    
    result_type operator()(precube const& pc)
    {
        indexed_token fix = fixed_position == fixed_left ? pc[0] : pc[1];
        assert(rulemap->find(fix) != rulemap->end());
        assert(rulemap->find(fix)->find(pc) != rulemap->find(fix)->end());
        Rules rules = *(rulemap->find(fix)->find(pc));
        assert(boost::begin(rules) != boost::end(rules));
        assert(gram->rule_rhs(rules[0],0) == pc[0]);
        assert(gram->rule_rhs(rules[0],1) == pc[1]);
        Edges le = *(left.find(pc[0]));
        assert(boost::begin(le) != boost::end(le));
        assert(le[0].representative().root() == pc[0]);
        Edges re = *(right.find(pc[1]));
        assert(boost::begin(re) != boost::end(re));
        assert(re[0].representative().root() == pc[1]);
        return result_type(*gram,*ef,rules,le,re);
    }
};

template <class Grammar, class RuleMap, class Cells>
class cube_generator_generator
: public 
    gusc::peekable_generator_facade<
      cube_generator_generator<Grammar,RuleMap,Cells>
    , gusc::iterator_from_generator< 
        gusc::transform_generator<
          cell_ordered_precube_generator<Grammar,RuleMap,Cells>
        , precube_to_cube<Grammar,RuleMap,Cells>
        >
      >
    >
{
    typedef cell_ordered_precube_generator<Grammar,RuleMap,Cells> pre_res_;
    typedef precube_to_cube<Grammar,RuleMap,Cells> transform_;
    typedef gusc::transform_generator<pre_res_, transform_> transform_gen_;
    typedef gusc::iterator_from_generator<transform_gen_> res_;
    typedef typename boost::range_value<Cells>::type cell_type;
    typedef typename boost::range_value<cell_type>::type edge_equiv_type;
    typedef typename edge_equiv_type::edge_type edge_type;
public:
    cube_generator_generator( fixed position
                            , Cells const& left
                            , Cells const& right
                            , Grammar const* gram
                            , RuleMap const* rulemap
                            , concrete_edge_factory<edge_type,Grammar>* ef
                            , stlext::hash_set<precube,boost::hash<precube> >* occmap 
                            )
    : gram(gram)
    , rulemap(rulemap)
    , fixed_position(position)
    , left(left)
    , right(right)
    , cell_itr(position == fixed_left ? boost::begin(left) : boost::begin(right))
    , cell_end(position == fixed_left ? boost::end(left) : boost::end(right))
    , ef(ef)
    , occmap(occmap) 
    {
        set();
    }
    
private:
    friend class gusc::generator_access;
    
    res_ const& peek() const { return curr; }
    void pop() { ++cell_itr; set(); }
    bool more() const { return cell_itr != cell_end; }
    
    void set()
    {
        while (more()) {
            indexed_token root = (*cell_itr)[0].representative().root();
            
            typename RuleMap::const_iterator 
                pos = rulemap->find(root);
            if (pos != rulemap->end()) {
                Cells const& varcells = fixed_position == fixed_left 
                                      ? right
                                      : left ;
                curr = res_( 
                         transform_gen_(
                           pre_res_( fixed_position
                                   , root
                                   , varcells
                                   , gram
                                   , *pos
                                   , occmap 
                                   )
                         , transform_(fixed_position,gram,ef,rulemap,left,right)
                         ) 
                       )
                       ;
                break;
            }
            ++cell_itr;
        }
    }
    
    Grammar const* gram;
    RuleMap const* rulemap;
    fixed fixed_position;
    Cells left;
    Cells right;
    typename Cells::const_iterator cell_itr;
    typename Cells::const_iterator cell_end;
    concrete_edge_factory<edge_type,Grammar>* ef;
    stlext::hash_set<precube,boost::hash<precube> >* occmap;
    res_ curr;
};

template <class Grammar, class RuleMap, class Cells, class Edge>
cube_generator_generator<Grammar,RuleMap,Cells>
generate_cube_generators( fixed position
                        , Cells const& left
                        , Cells const& right
                        , Grammar const& gram
                        , RuleMap const& rulemap
                        , concrete_edge_factory<Edge,Grammar>& ef
                        , stlext::hash_set<precube, boost::hash<precube> >& occmap 
                        )
{
    return cube_generator_generator<Grammar,RuleMap,Cells>( position
                                                          , left
                                                          , right
                                                          , &gram
                                                          , &rulemap
                                                          , &ef
                                                          , &occmap );
}

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleMap, class Cells>
class exhaustive_cube_split_processor
: public 
    gusc::peekable_generator_facade<
      exhaustive_cube_split_processor<Grammar,RuleMap,Cells>
    , typename split_processor_result<Grammar,RuleMap,Cells>::type
    >
{
    typedef Cells cell_sequence_type;
    typedef typename boost::range_value<Cells>::type cell_type;
    typedef typename boost::range_value<cell_type>::type edge_equiv_type;
    typedef typename edge_equiv_type::edge_type edge_type;
    typedef Grammar grammar_type;
    typedef RuleMap rule_map_type;
    typedef typename rule_map_type::value_type::value_type rule_sequence;
    typedef cube_processor<grammar_type,rule_sequence,cell_type> gen_type;
public:
    exhaustive_cube_split_processor( Cells const& left_cells
                                   , Cells const& right_cells
                                   , Grammar const& gram
                                   , RuleMap const& left_rulemap
                                   , concrete_edge_factory<edge_type,Grammar>& ef
                                   )
    {
        std::vector<gen_type> cubes;
        BOOST_FOREACH(cell_type const& lc, left_cells) {
            typename RuleMap::const_iterator 
                rms_pos = left_rulemap.find(cell_root(lc));
            if (rms_pos != left_rulemap.end()) {
                BOOST_FOREACH(rule_sequence const& rm, *rms_pos) {
                    typename Cells::const_iterator 
                        rc_pos = right_cells.find(gram.rule_rhs(rm[0],1));
                    if (rc_pos != right_cells.end()) {
                        cubes.push_back(gen_type(gram,ef,rm,lc,*rc_pos));
                    }
                }
            }
        }
        heap = gusc::generate_finite_union(cubes,lesser_edge_score<edge_type>());
    }
    
private:
    gusc::finite_union_generator< gen_type, lesser_edge_score<edge_type> > heap;
    friend class gusc::generator_access;
    void pop() { ++heap; }
    edge_type const& peek() const { return *heap; }
    bool more() const { return bool(heap); }
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleMap, class Cells>
class left_right_split_processor
: public 
    gusc::peekable_generator_facade<
      left_right_split_processor<Grammar,RuleMap,Cells>
    , typename split_processor_result<Grammar,RuleMap,Cells>::type
    >
{
    typedef Cells cell_sequence_type;
    typedef typename boost::range_value<Cells>::type cell_type;
    typedef typename boost::range_value<cell_type>::type edge_equiv_type;
    typedef typename edge_equiv_type::edge_type edge_type;
    typedef Grammar grammar_type;
    typedef RuleMap rule_map_type;
    typedef typename rule_map_type::value_type rule_sequence;
    
    typedef cube_processor<grammar_type,rule_sequence,cell_type> gen_type;
    typedef gusc::union_heap_generator<
              cube_generator_generator<Grammar,RuleMap,Cells>
            , gusc::peek_compare< lesser_edge_score<edge_type > >
            > cube_generator;

    gusc::union_heap_generator<      
      gusc::finite_union_generator<
        cube_generator
      , gusc::peek_compare< lesser_edge_score<edge_type> >
      >
    , lesser_edge_score<edge_type>
    > heap;
    boost::shared_ptr<stlext::hash_set<precube,boost::hash<precube> > > occmap;
public:
    left_right_split_processor( Cells const& left
                              , Cells const& right
                              , Grammar const& gram
                              , RuleMap const& left_rulemap
                              , RuleMap const& right_rulemap
                              , concrete_edge_factory<edge_type,Grammar>& ef
                              , size_t lookahead
                              )
      : occmap(new stlext::hash_set<precube,boost::hash<precube> >())
    {
        cube_generator 
            left_cubes( generate_cube_generators(fixed_left, left, right, gram, left_rulemap, ef, *occmap)
                      , lookahead
                      , gusc::peek_compare< lesser_edge_score<edge_type> >()
                      );
        cube_generator
            right_cubes( generate_cube_generators(fixed_right, left, right, gram, right_rulemap, ef, *occmap)
                       , lookahead
                       , gusc::peek_compare< lesser_edge_score<edge_type> >()
                       );
                          
        heap = gusc::generate_union_heap(
                 gusc::generate_finite_union(
                   cref_array(left_cubes,right_cubes)
                 , gusc::peek_compare< lesser_edge_score<edge_type> >()
                 )
               , lesser_edge_score<edge_type>()
               , lookahead
               );
    }
    
private:
    friend class gusc::generator_access;
    edge_type const& peek() const { return *heap; }
    void pop() { ++heap; }
    bool more() const { return bool(heap); }
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar, class RuleMap, class Edge>
class exhaustive_cube_split_factory {
    Grammar const* gram;
    RuleMap const* left_rulemap;
    concrete_edge_factory<Edge,Grammar>* ef;
public:
    template <class X> struct result {};

    template <class G, class RM, class E, class Cells>
    struct result<exhaustive_cube_split_factory<G,RM,E>(Cells,Cells)> {
        typedef exhaustive_cube_split_processor<G,RM,Cells> type;
    };
    
    template <class Cells>
    exhaustive_cube_split_processor<Grammar,RuleMap,Cells>
    operator()(Cells const& left, Cells const& right) const
    {
        typedef exhaustive_cube_split_processor<Grammar,RuleMap,Cells> 
                result_type;
                
        return result_type( left
                          , right
                          , *gram
                          , *left_rulemap
                          , *ef );
    }
    
    exhaustive_cube_split_factory( Grammar const& gram
                                 , RuleMap const& left_rulemap
                                 , concrete_edge_factory<Edge,Grammar>& ef
                                 )
    : gram(&gram)
    , left_rulemap(&left_rulemap)
    , ef(&ef) {}
};

template <class Grammar, class RuleMap, class Edge>
class left_right_split_factory {
    Grammar const* gram;
    RuleMap const* left_rulemap;
    RuleMap const* right_rulemap;
    concrete_edge_factory<Edge,Grammar>* ef;
    size_t lookahead;
public:
    template <class X> struct result {};
    
    template <class G, class RM, class E, class Cells>
    struct result<left_right_split_factory<G,RM,E>(Cells,Cells)> {
        typedef left_right_split_processor<G,RM,Cells> type;
    };
    
    template <class Cells>
    left_right_split_processor<Grammar,RuleMap,Cells>
    operator()(Cells const& left, Cells const& right) const
    {
        typedef left_right_split_processor<Grammar,RuleMap,Cells> result_type;
        return result_type( left
                          , right
                          , *gram
                          , *left_rulemap
                          , *right_rulemap
                          , *ef
                          , lookahead );
    }
    
    left_right_split_factory( Grammar const& gram
                            , RuleMap const& left_rulemap
                            , RuleMap const& right_rulemap
                            , concrete_edge_factory<Edge,Grammar>& ef
                            , size_t lookahead )
    : gram(&gram)
    , left_rulemap(&left_rulemap)
    , right_rulemap(&right_rulemap)
    , ef(&ef)
    , lookahead(lookahead) {}
};

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::lazy

# endif //     SBMT__SEARCH__LAZY__RIGHT_LEFT_SPLIT_HPP
