# if ! defined(SBMT__EDGE__INFO_COMBINING_HPP)
# define       SBMT__EDGE__INFO_COMBINING_HPP

# include <sbmt/edge/edge.hpp>
# include <sbmt/edge/constituent.hpp>
# include <sbmt/edge/joined_info.hpp>
# include <sbmt/edge/composite_info.hpp>
# include <gusc/generator/product_heap_generator.hpp>
# include <gusc/generator/any_generator.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <gusc/generator/transform_generator.hpp>
# include <gusc/generator/lazy_sequence.hpp>
# include <gusc/iterator/iterator_from_generator.hpp>
# include <gusc/iterator/reverse.hpp>
# include <gusc/functional.hpp>
# include <boost/iterator/iterator_traits.hpp>
# include <boost/bind.hpp>
# include <boost/type_traits.hpp>
# include <boost/function_output_iterator.hpp>
# include <sbmt/edge/edge_info.hpp>
# include <sbmt/logging.hpp>
# include <sbmt/io/logging_macros.hpp>
# include <sbmt/feature/accumulator.hpp>

template <class V>
struct null_generator {
    typedef V result_type;
    operator bool() const {return false;}
    V operator()() { throw std::runtime_error("attempt to access empty null generator"); }
};

namespace sbmt {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(info_combine_d,"info-combine",root_domain);

template <class X>
struct unqualified_result_of {
    typedef typename boost::remove_cv<
                typename boost::remove_reference<
                    typename boost::remove_pointer<
                        typename boost::result_of<X>::type
                    >::type
                >::type
            >::type
            type;
};


template <class SubInfoExtractor>
struct edge_constituent {
    template <class X> struct result {};

    template <class Info, class S>
    struct result<edge_constituent<S>(edge<Info> const*)> {
    private:
        typedef typename unqualified_result_of<SubInfoExtractor(Info const*)>::type info_t;
    public:
        typedef constituent<info_t> type;
    };

    template <class Info>
    typename result<edge_constituent(edge<Info> const*)>::type
    operator()(edge<Info> const* e) const
    {
        typedef typename unqualified_result_of<SubInfoExtractor(Info const*)>::type info_t;
        //if (is_lexical(e.root())) return make_constituent(info_t(), e.root());
        //else
        return make_constituent(subinfo(&e->info()), e->root());
    }

    edge_constituent(SubInfoExtractor const& subinfo) : subinfo(subinfo) {}

private:
    SubInfoExtractor subinfo;
};

template <class SubInfoExtractor>
edge_constituent<SubInfoExtractor>
get_edge_constituent(SubInfoExtractor const& subinfo)
{
    return edge_constituent<SubInfoExtractor>(subinfo);
}


////////////////////////////////////////////////////////////////////////////////

struct get_info {
    template <class X> struct result {};

    template <class Info>
    struct result<get_info(Info const*)>
    {
        typedef Info const* type;
    };

    template <class Info>
    Info const* operator()(Info const* e) const
    {
        return e;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct first_info {
    template <class X> struct result {};

    template <class Info1, class Info2>
    struct result<first_info(joined_info<Info1,Info2> const*)>
    {
        typedef Info1 const* type;
    };

    template <class Info1, class Info2>
    Info1 const* operator()(joined_info<Info1,Info2> const* e) const
    {
        return &e->first_info();
    }
};

////////////////////////////////////////////////////////////////////////////////

struct second_info {
    template <class X> struct result {};

    template <class Info1, class Info2>
    struct result<second_info(joined_info<Info1,Info2> const*)>
    {
        typedef Info2 const* type;
    };

    template <class Info1, class Info2>
    Info2 const* operator()(joined_info<Info1,Info2> const* e) const
    {
        return &e->second_info();
    }
};

////////////////////////////////////////////////////////////////////////////////

class component_info {
public:
    typedef edge_info<any_info> const* result_type;
    result_type operator()(composite_info const* e) const
    {
        return &e->info(idx);
        //else return edge_info<any_info>(unscoreable_edge_info);
    }
    component_info(size_t idx) : idx(idx) {}
    size_t idx;
};

template <class C, class T> std::basic_ostream<C,T>&
operator << (std::basic_ostream<C,T>& os, component_info const& getinfo)
{
    return os << "component[" << getinfo.idx << "]";
}

template <class F, class G>
class unary_composition {
public:
    template <class X> struct result { typedef X type; };

    template <class X, class A, class B>
    struct result<unary_composition<A,B>(X)> {
        typedef typename boost::result_of<
                             A(typename boost::result_of<B(X)>::type)
                         >::type
                type;
    };

    unary_composition(F const& f, G const& g) : f(f), g(g) {}

    template <class X>
    typename result<unary_composition<F,G>(X)>::type
    operator()(X const& x) const { return f(g(x)); }
private:
    F f;
    G g;
};

template <class F, class G>
unary_composition<F,G> compose_unary(F const& f, G const& g)
{
    return unary_composition<F,G>(f,g);
}

////////////////////////////////////////////////////////////////////////////////

struct join_result {
    template <class X> struct result {};

    template <class Info1, class Info2>
    struct result<join_result( boost::tuple<Info1,score_t,score_t>
                             , boost::tuple<Info2,score_t,score_t>)>
    {
        typedef boost::tuple<joined_info<Info1,Info2>,score_t,score_t> type;
    };

    template <class Info1, class Info2>
    boost::tuple<joined_info<Info1,Info2>,score_t,score_t>
    operator()( boost::tuple<Info1,score_t,score_t> const& i1
              , boost::tuple<Info2,score_t,score_t> const& i2 ) const
    {
        return boost::make_tuple( joined_info<Info1,Info2>( boost::get<0>(i1)
                                                        , boost::get<0>(i2)
                                                        )
                                , boost::get<1>(i1) * boost::get<1>(i2)
                                , boost::get<2>(i1) * boost::get<2>(i2)
                                )
                                ;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct composite_accumulator {
private:
    typedef boost::tuple<edge_info<any_info>,score_t,score_t> arg_type;
public:
    typedef boost::tuple<composite_info,score_t,score_t> result_type;

    result_type operator()(result_type const& res, arg_type const& arg) const
    {
        size_t sz = boost::get<0>(res).size();
        result_type ret( composite_info(sz + 1, edge_info<any_info>())
                       , boost::get<1>(res) * boost::get<1>(arg)
                       , boost::get<2>(res) * boost::get<2>(arg) );

        for (size_t x = 0; x != sz; ++x) {
            boost::get<0>(ret).info(x) = boost::get<0>(res).info(x);
        }
        boost::get<0>(ret).info(sz) = boost::get<0>(arg);
        return ret;
    }

    result_type operator()(arg_type const& a) const
    {
        return result_type( composite_info(1,boost::get<0>(a))
                          , boost::get<1>(a)
                          , boost::get<2>(a) );

    }
};

////////////////////////////////////////////////////////////////////////////////

struct result_less {
    typedef bool result_type;

    template <class Info>
    bool operator()( boost::tuple<Info,score_t,score_t> const& i1
                   , boost::tuple<Info,score_t,score_t> const& i2 ) const
    {
        return boost::get<1>(i1) * boost::get<2>(i1) <
               boost::get<1>(i2) * boost::get<2>(i2);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class InfoFactory>
struct info_result_generator {
private:
    typedef typename InfoFactory::result_generator gen_t;
    typedef typename boost::result_of<gen_t()>::type res_t;
public:
    typedef gusc::any_generator<res_t> type;
    //typedef typename InfoFactory::result_generator type;
};

////////////////////////////////////////////////////////////////////////////////

template <class InfoFactory1, class InfoFactory2>
struct info_result_generator< joined_info_factory<InfoFactory1,InfoFactory2> > {
    typedef gusc::product_heap_generator<
        join_result
      , result_less
      , gusc::lazy_sequence<typename info_result_generator<InfoFactory1>::type>
      , gusc::lazy_sequence<typename info_result_generator<InfoFactory2>::type>
    > type;
};

////////////////////////////////////////////////////////////////////////////////

template < class CompositeInfo
         , class InfoFactory
         , class Grammar
         , class ConstituentOutputIterator
         , class InfoFromEdge >
ConstituentOutputIterator
scoreable_frontier_e( InfoFactory& info_factory
                  , Grammar& grammar
                  , edge<CompositeInfo> const& e
                  , ConstituentOutputIterator constituents_out
                  , InfoFromEdge getinfo
                  )
{
    typedef typename InfoFactory::info_type info_type;
    if (e.root().type() == foreign_token && !e.has_rule_id()) {
        return constituents_out;
    } else if (info_factory.scoreable_rule(grammar,grammar.rule(e.rule_id()))) {
        *constituents_out = constituent<info_type>(getinfo(&e.info()),e.root());
        ++constituents_out;
    } else {
        assert(e.has_first_child()); // if its not lexical, it better have a child
        constituents_out = scoreable_frontier_e( info_factory
                                               , grammar
                                               , e.first_child()
                                               , constituents_out
                                               , getinfo
                                               );
        if (e.has_second_child()) {
            constituents_out = scoreable_frontier_e( info_factory
                                                   , grammar
                                                   , e.second_child()
                                                   , constituents_out
                                                   , getinfo
                                                   );
        }
    }
    return constituents_out;
}

template < class InfoFactory
         , class Grammar
         , class EdgeRange
         , class ConstituentOutputIterator
         , class InfoFromEdge >
ConstituentOutputIterator
scoreable_frontier( InfoFactory& info_factory
                  , Grammar& grammar
                  , EdgeRange const& edges
                  , ConstituentOutputIterator constituents_out
                  , InfoFromEdge getinfo
                  )
{
    typedef typename boost::range_const_iterator<EdgeRange>::type iterator;
    typedef typename boost::iterator_value<iterator>::type edge_type;
    typedef typename edge_type::info_type info_type;

    iterator itr = boost::begin(edges), end = boost::end(edges);
    for (; itr != end; ++itr) {
        *constituents_out = scoreable_frontier_e<info_type>( info_factory
                                                , grammar
                                                , *itr
                                                , constituents_out
                                                , getinfo );
    }
    return constituents_out;
}

// eventually, i would like to use a generator library to make this, so as
// to not fool around with runtime stacks, and just interrupt the function stack
// with a yield statement...
template < class CompositeInfo
         , class SubInfoExtractor
         , class SatisfyCondition
         , class StopCondition = gusc::always_false
         >
struct frontier_generator
{
    typedef typename unqualified_result_of<SubInfoExtractor(CompositeInfo const*)>::type
            info_type;
    typedef edge<CompositeInfo> const* result_type;

    template <class Range>
    frontier_generator( Range const& range
                      , SubInfoExtractor const& getinfo
                      , SatisfyCondition const& satisfied
                      , StopCondition const& stop )
      : getinfo(getinfo)
      , satisfied(satisfied)
      , stop(stop)
    {
        typedef typename gusc::reverse_range_return<Range>::type RRange;
        RRange rrange = gusc::reverse_range(range);
        typename boost::range_iterator<RRange>::type itr = boost::begin(rrange),
                                                     end = boost::end(rrange);
        for (; itr != end; ++itr) {
            if (not stop(*itr)) {
                stk.push_back(&(*itr));
            }
        }
        find_frontier();
    }

    operator bool() const { return not stk.empty(); }

    result_type operator()()
    {
        result_type ret = stk.back();
        //std::cerr << "sfg::scoreable of type " << ret->root().type() << std::endl;
        stk.pop_back();
        find_frontier();
        return ret;
    }

private:
    typedef edge<CompositeInfo> const edge_t;
    SubInfoExtractor getinfo;
    std::vector< edge_t* > stk;
    SatisfyCondition satisfied;
    StopCondition stop;

    bool satisfied_edge(edge_t* pe) const
    {
        return !(is_lexical(pe->root()) && !pe->has_rule_id()) &&
                satisfied(*getinfo(&pe->info()));
    }

    void find_frontier()
    {
        while ((not stk.empty()) and ((not satisfied_edge(stk.back())) or stop(*stk.back()))) {
            edge_t* e = stk.back();
            stk.pop_back();
            if (stop(*e)) continue;
            // its a stack, which is why the second child has to be pushed
            // first, so the first child will be popped first
            if (e->has_second_child() and not stop(e->second_child())) {
                stk.push_back(&e->second_child());
            }
            if (e->has_first_child() and not stop(e->first_child())) {
                stk.push_back(&e->first_child());
            }
        }
    }
};

template < class Range
         , class SubInfoExtractor
         , class SatisfyCondition
         , class StopCondition >
frontier_generator<
    typename boost::range_value<Range>::type::info_type
  , SubInfoExtractor
  , SatisfyCondition
  , StopCondition
>
generate_frontier( Range const& range
                 , SubInfoExtractor const& getinfo
                 , SatisfyCondition const& satisfied
                 , StopCondition const& stop )
{
    return frontier_generator<
        typename boost::range_value<Range>::type::info_type
      , SubInfoExtractor
      , SatisfyCondition
      , StopCondition
    >(range,getinfo,satisfied,stop);
}

template <class CompositeInfo, class SubInfoExtractor, class SatisfyCondition>
struct constituents_generator
  : gusc::transform_generator<
      frontier_generator< 
        CompositeInfo
      , SubInfoExtractor
      , SatisfyCondition
      , gusc::always_false
      >
    , edge_constituent<SubInfoExtractor>
    > {
    typedef gusc::transform_generator<
              frontier_generator<
                CompositeInfo
              , SubInfoExtractor
              , SatisfyCondition
              , gusc::always_false
              >
            , edge_constituent<SubInfoExtractor>
            > base_type;
    template <class Range>
    constituents_generator( Range const& range
                          , SubInfoExtractor const& getinfo
                          , SatisfyCondition const& satisfied )
    : base_type( generate_frontier(range, getinfo, satisfied, gusc::always_false())
               , get_edge_constituent(getinfo) )
    {}

    constituents_generator(base_type const& b) : base_type(b) {}
};

template <class Range, class SubInfoExtractor, class SatisfyCondition>
constituents_generator<
    typename boost::range_value<Range>::type::info_type
  , SubInfoExtractor
  , SatisfyCondition
>
generate_constituents( Range const& range
                     , SubInfoExtractor const& getinfo
                     , SatisfyCondition const& satisfied )
{
    return constituents_generator<
             typename boost::range_value<Range>::type::info_type
           , SubInfoExtractor
           , SatisfyCondition
           >(range,getinfo,satisfied);
}

////////////////////////////////////////////////////////////////////////////////


template < class CompositeInfo
         , class SubInfoExtractor
         , class SatisfyCondition
         , class StopCondition >
frontier_generator<CompositeInfo,SubInfoExtractor,SatisfyCondition,StopCondition>
generate_children( edge<CompositeInfo> const& e
                 , SubInfoExtractor const& getinfo
                 , SatisfyCondition const& satisfied
                 , StopCondition const& stop )
{
   // assert(e.has_rule_id() and getinfo(&e.info())->scoreable());

    if (e.has_second_child()) {
        return generate_frontier( ref_array(e.first_child(), e.second_child())
                                , getinfo
                                , satisfied
                                , stop );
    } else if (e.has_first_child()) {
        return generate_frontier( ref_array(e.first_child())
                                , getinfo
                                , satisfied
                                , stop );
    } else {
        edge<CompositeInfo> const* pe = NULL;
        return generate_frontier( boost::make_iterator_range(pe,pe)
                                , getinfo
                                , satisfied
                                , stop );
    }
}

template <class CompositeInfo, class SubInfoExtractor, class SatisfyCondition>
constituents_generator<CompositeInfo,SubInfoExtractor,SatisfyCondition>
generate_constituents_e( edge<CompositeInfo> const& e
                       , SubInfoExtractor const& getinfo
                       , SatisfyCondition const& satisfied )
{
    return gusc::generate_transform( 
             generate_children(e,getinfo,satisfied,gusc::always_false())
           , get_edge_constituent(getinfo) 
           );
}

struct info_stateable {
    template <class Info>
    bool operator()(edge_info<Info> const& ei) const { return ei.stateable(); }
};

struct info_comparable {
    template <class Info>
    bool operator()(edge_info<Info> const& ei) const { return ei.comparable(); }
};

namespace {
    info_stateable stateable_;
    info_comparable comparable_;
}

template < class InfoFactory
         , class Grammar
         , class CompositeInfo
         , class SubInfoExtractor
         , class ScoreAccumulatorIterator
         , class HeurIterator
         , class StopCondition >
boost::tuple<ScoreAccumulatorIterator,HeurIterator>
component_scores( InfoFactory& info_factory
                , Grammar& grammar
                , edge<CompositeInfo> const& e
                , SubInfoExtractor const& getinfo
                , ScoreAccumulatorIterator scoresout
                , HeurIterator heurout
                , size_t num_component_scores
                , StopCondition const& stop )
{
    //std::cerr << print(e.root(),grammar) << "(\n" << std::flush;
    if (!(e.has_rule_id() and getinfo(&e.info())->stateable())) {
        return boost::make_tuple(scoresout,heurout);
    }
    frontier_generator<CompositeInfo,SubInfoExtractor,info_stateable,StopCondition>
        frontier = generate_children(e,getinfo,stateable_,stop);

    while (frontier) {
        edge<CompositeInfo> const* ce = frontier();
        //assert (not is_lexical(ce->root()));
        assert (getinfo(&ce->info())->stateable());
        assert (info_factory.scoreable_rule(grammar, grammar.rule(ce->rule_id())));
        ScoreAccumulatorIterator so = scoresout;
        HeurIterator ho = heurout;
        component_scores(info_factory,grammar,*ce,getinfo,so,ho,num_component_scores,stop);
    }

    //std::cerr << ")\n" << std::endl;
    return info_factory.component_scores(
               grammar
             , grammar.rule(e.rule_id())
             , e.span()
             , gusc::range_from_generator(generate_constituents_e(e,getinfo,stateable_))
             , *getinfo(&e.info())
             , scoresout
             , heurout );
    
    
}

////////////////////////////////////////////////////////////////////////////////

template < class InfoFactory
         , class Grammar
         , class CompositeInfo
         , class StopCondition >
void
component_scores( InfoFactory& info_factory
                , Grammar& grammar
                , edge<CompositeInfo> const& e
                , feature_vector& scores
                , feature_vector& heurs
                , StopCondition const& stop )
{
    multiply_accumulator<feature_vector> out(scores);
    replacement<feature_vector> hout(heurs);
    if (!(is_lexical(e.root()) && !e.has_rule_id()))
    component_scores( info_factory
                    , grammar
                    , e
                    , get_info()
                    , boost::make_function_output_iterator(out)
                    , boost::make_function_output_iterator(hout)
                    , 0
                    , stop
                    );
}

////////////////////////////////////////////////////////////////////////////////

template < class InfoFactory1
         , class InfoFactory2
         , class Grammar
         , class CompositeInfo
         , class SubInfoExtractor
         , class ScoreAccumulatorIterator
         , class HeurIterator
         , class StopCondition >
boost::tuple<ScoreAccumulatorIterator,HeurIterator>
component_scores( joined_info_factory<InfoFactory1,InfoFactory2>& info_factory
                , Grammar& grammar
                , edge<CompositeInfo> const& e
                , SubInfoExtractor const& getinfo
                , ScoreAccumulatorIterator scoresout
                , HeurIterator heurout
                , size_t ignore
                , StopCondition const& stop )
{
    ScoreAccumulatorIterator so = scoresout;
    HeurIterator ho = heurout;
    boost::tie(so,ho) =
         component_scores( info_factory.first_info_factory()
                         , grammar
                         , e
                         , compose_unary(first_info(),getinfo)
                         , so
                         , ho
                         , ignore
                         , stop );

    boost::tie(so,ho) =
         component_scores( info_factory.second_info_factory()
                         , grammar
                         , e
                         , compose_unary(second_info(),getinfo)
                         , so
                         , ho
                         , ignore
                         , stop );
    return boost::make_tuple(so,ho);
}

////////////////////////////////////////////////////////////////////////////////

template < class Grammar
         , class CompositeInfo
         , class SubInfoExtractor
         , class ScoreAccumulatorIterator
         , class HeurIterator
         , class StopCondition >
boost::tuple<ScoreAccumulatorIterator,HeurIterator>
component_scores( composite_info_factory& info_factory
                , Grammar& grammar
                , edge<CompositeInfo> const& e
                , SubInfoExtractor const& getinfo
                , ScoreAccumulatorIterator scoresout
                , HeurIterator heurout
                , size_t ignore
                , StopCondition const& stop )
{
    ScoreAccumulatorIterator so = scoresout;
    HeurIterator ho = heurout;
    for (size_t x = 0; x != info_factory.size(); ++x) {
        boost::tie(so,ho) =
             component_scores( info_factory.factory(x)
                             , grammar
                             , e
                             , compose_unary(component_info(x), getinfo)
                             , so
                             , ho
                             , ignore
                             , stop );
    }
    return boost::make_tuple(so,ho);
}

////////////////////////////////////////////////////////////////////////////////

template <class InfoFactory, class Grammar, class EdgeRange, class InfoFromEdge>
typename info_result_generator<InfoFactory>::type
create_info( InfoFactory& info_factory
           , Grammar& grammar
           , typename Grammar::rule_type rule
           , span_t const& span
           , EdgeRange const& edges
           , InfoFromEdge const& getinfo )
{
    typedef typename InfoFactory::info_type info_type;
    typedef boost::tuple<info_type,score_t,score_t> result_type;

    if (not info_factory.scoreable_rule(grammar,rule)) {
        info_type ret = info_factory.empty_info();
        //info_type ret = unscoreable_edge_info;
        //std::cerr << print(grammar.rule_lhs(rule),grammar) << " unscoreable\n";
        //assert(not ret.scoreable());
        return gusc::single_value_generator<result_type>(result_type(ret,1.0,1.0));
    }

    return info_factory.create_info( grammar
                                   , rule
                                   , span
                                   , gusc::range_from_generator(
                                       generate_constituents(edges,getinfo,stateable_)
                                     ) 
                                   );
}

template <class InfoFactory, class Grammar, class EdgeRange>
typename info_result_generator<InfoFactory>::type
create_info( InfoFactory& info_factory
           , Grammar& grammar
           , typename Grammar::rule_type rule
           , span_t const& span
           , EdgeRange const& edges )
{
    return create_info(info_factory,grammar,rule,span,edges,get_info());
}

////////////////////////////////////////////////////////////////////////////////

template < class InfoFactory1
         , class InfoFactory2
         , class Grammar
         , class EdgeRange
         , class ExtractInfo >
typename
info_result_generator< joined_info_factory<InfoFactory1,InfoFactory2> >::type
create_info( joined_info_factory<InfoFactory1,InfoFactory2>& info_f
           , Grammar& grammar
           , typename Grammar::rule_type rule
           , span_t const& span
           , EdgeRange const& edges
           , ExtractInfo const& getinfo )
{
    using namespace boost;
    typename info_result_generator<InfoFactory1>::type
        res1 = create_info( info_f.first_info_factory()
                          , grammar
                          , rule
                          , span
                          , edges
                          , compose_unary(first_info(), getinfo) );
    typename info_result_generator<InfoFactory2>::type
        res2 = create_info( info_f.second_info_factory()
                          , grammar
                          , rule
                          , span
                          , edges
                          , compose_unary(second_info(), getinfo) );

    return gusc::generate_product_heap(
               join_result()
             , result_less()
             , gusc::make_lazy_sequence(res1)
             , gusc::make_lazy_sequence(res2)
           );
}

////////////////////////////////////////////////////////////////////////////////

template <>
struct info_result_generator<composite_info_factory> {
    typedef gusc::any_generator< boost::tuple<composite_info,score_t,score_t> >
            type;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeRange, class ExtractInfo, class Grammar>
info_result_generator<composite_info_factory>::type
create_info_tail( composite_info_factory& info_f
                , Grammar& grammar
                , typename Grammar::rule_type rule
                , span_t const& span
                , EdgeRange const& edges
                , ExtractInfo const& getinfo
                , info_result_generator<composite_info_factory>::type gen
                , size_t so_far )
{
    typedef boost::tuple<composite_info,score_t,score_t> R;

    if (so_far + 1 == info_f.size()) return gen;
    size_t n = so_far + 1;
    info_result_generator< edge_info_factory<any_info_factory> >::type
        ci = create_info( info_f.factory(n)
                        , grammar
                        , rule
                        , span
                        , edges
                        , compose_unary(component_info(n), getinfo)
                        )
                        ;
    if (ci) {
        info_result_generator<composite_info_factory>::type
            ret = gusc::generate_product_heap(
                    composite_accumulator()
                  , result_less()
                  , gusc::make_lazy_sequence(gen)
                  , gusc::make_lazy_sequence(ci)
                  )
                  ;

        return create_info_tail(info_f, grammar, rule, span, edges, getinfo, ret, n);
    } else {
        //std::cerr << span << ": short-circuit "<<n<<"\n";
        return null_generator<R>();
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class EdgeRange, class ExtractInfo, class Grammar>
info_result_generator<composite_info_factory>::type
create_info( composite_info_factory& info_f
           , Grammar& grammar
           , typename Grammar::rule_type rule
           , span_t const& span
           , EdgeRange const& edges
           , ExtractInfo const& getinfo )
{
    typedef boost::tuple<composite_info,score_t,score_t> R;
    info_result_generator< edge_info_factory<any_info_factory> >::type
        ci = create_info( info_f.factory(0)
                        , grammar
                        , rule
                        , span
                        , edges
                        , compose_unary(component_info(0), getinfo)
                        )
                        ;
    if (ci) {
        info_result_generator<composite_info_factory>::type
                ret = gusc::generate_transform<composite_accumulator>(ci);
        return create_info_tail(info_f, grammar, rule, span, edges, getinfo, ret, 0);
    } else {
        //std::cerr << span << ": short-circuit "<<0<<"\n";
        return null_generator<R>();
    }
}

////////////////////////////////////////////////////////////////////////////////


struct info_tag {};
template <class X>
struct info_category {
    typedef info_tag type;
};

/*
template <class Info, class SubInfoExtractor>
bool edge_info_scoreable_equal( edge<Info> const& e1
                              , edge<Info> const& e2
                              , SubInfoExtractor const& getinfo
                              , info_tag const& sel )
{
    if (getinfo(&e1.info())->scoreable() != getinfo(&e2.info())->scoreable()) {
        return false;
    } else if (getinfo(&e1.info())->scoreable()) {
        return *getinfo(&e1.info()) == *getinfo(&e2.info());
    } else {
        return true;
    }
}

template <class Info>
bool edge_info_scoreable_equal( edge<Info> const& e1, edge<Info> const& e2 )
{
    return edge_info_scoreable_equal( e1
                                    , e2
                                    , get_info()
                                    , typename info_category<Info>::type()
                                    );
}
*/
template <class Info, class SubInfoExtractor>
bool edge_info_equal( edge<Info> const& e1
                    , edge<Info> const& e2
                    , SubInfoExtractor const& getinfo
                    , info_tag const& sel )
{
    if (getinfo(&e1.info())->comparable() != getinfo(&e2.info())->comparable()) {
        return false;
    }
    else if (getinfo(&e1.info())->comparable()) {
        return *getinfo(&e1.info()) == *getinfo(&e2.info());
    }
    else {
        constituents_generator<Info,SubInfoExtractor,info_comparable>
           frontier1(ref_array(e1),getinfo,comparable_),
           frontier2(ref_array(e2),getinfo,comparable_);
        while( frontier1 and frontier2 ) {
            if (frontier1() != frontier2()) return false;
        }
        return ((not frontier1) and (not frontier2));
    }
}

template <class Info>
bool edge_info_equal( edge<Info> const& e1, edge<Info> const& e2 )
{
    return edge_info_equal( e1
                          , e2
                          , get_info()
                          , typename info_category<Info>::type()
                          );
}

struct joined_info_tag : info_tag {};
template <class Info1,class Info2>
struct info_category< joined_info<Info1,Info2> > {
    typedef joined_info_tag type;
};
/*
template <class Info, class SubInfoExtractor>
bool edge_info_scoreable_equal( edge<Info> const& e1
                              , edge<Info> const& e2
                              , SubInfoExtractor getinfo
                              , joined_info_tag const& select )
{
    using namespace boost;

    typedef typename unqualified_result_of<SubInfoExtractor(Info const*)>::type
            subinfo_type;

    BOOST_STATIC_ASSERT((
        is_same<
            joined_info_tag
          , typename info_category<subinfo_type>::type
        >::value
    ));

    typedef typename subinfo_type::first_info_type info1_type;
    typedef typename subinfo_type::second_info_type info2_type;

    return edge_info_scoreable_equal( e1
                                    , e2
                                    , compose_unary(first_info(), getinfo)
                                    , typename info_category<info1_type>::type())
           and
           edge_info_scoreable_equal( e1
                                    , e2
                                    , compose_unary(second_info(), getinfo)
                                    , typename info_category<info2_type>::type());
}
*/
template <class Info, class SubInfoExtractor>
bool edge_info_equal( edge<Info> const& e1
                    , edge<Info> const& e2
                    , SubInfoExtractor getinfo
                    , joined_info_tag const& select )
{
    using namespace boost;

    typedef typename unqualified_result_of<SubInfoExtractor(Info const*)>::type
            subinfo_type;

    BOOST_STATIC_ASSERT((
        is_same<
            joined_info_tag
          , typename info_category<subinfo_type>::type
        >::value
    ));

    typedef typename subinfo_type::first_info_type info1_type;
    typedef typename subinfo_type::second_info_type info2_type;

    return edge_info_equal( e1
                          , e2
                          , compose_unary(first_info(), getinfo)
                          , typename info_category<info1_type>::type())
           and
           edge_info_equal( e1
                          , e2
                          , compose_unary(second_info(), getinfo)
                          , typename info_category<info2_type>::type());
}

struct composite_info_tag : info_tag {};

template <>
struct info_category<composite_info> {
    typedef composite_info_tag type;
};

/*
template <class Info, class SubInfoExtractor>
bool edge_info_scoreable_equal( edge<Info> const& e1
                              , edge<Info> const& e2
                              , SubInfoExtractor getinfo
                              , composite_info_tag const& select )
{
    using namespace boost;

    typedef typename unqualified_result_of<SubInfoExtractor(Info const*)>::type
            subinfo_type;

    BOOST_STATIC_ASSERT((
        is_same<
            composite_info_tag
          , typename info_category<subinfo_type>::type
        >::value
    ));
    if (getinfo(&e1.info())->size() != getinfo(&e2.info())->size()) return false;
    for (size_t x = 0; getinfo(&e1.info())->size() != x; ++x) {
        bool r = edge_info_scoreable_equal( e1
                                          , e2
                                          , compose_unary(component_info(x),getinfo)
                                          , info_category<edge_info<any_info> >::type()
                                          )
                                          ;
        if (not r) return false;
    }
    return true;
}
*/
template <class Info, class SubInfoExtractor>
bool edge_info_equal( edge<Info> const& e1
                    , edge<Info> const& e2
                    , SubInfoExtractor getinfo
                    , composite_info_tag const& select )
{
    using namespace boost;

    typedef typename unqualified_result_of<SubInfoExtractor(Info const*)>::type
            subinfo_type;

    BOOST_STATIC_ASSERT((
        is_same<
            composite_info_tag
          , typename info_category<subinfo_type>::type
        >::value
    ));
    if (getinfo(&e1.info())->size() != getinfo(&e2.info())->size()) return false;
    for (size_t x = 0; getinfo(&e1.info())->size() != x; ++x) {
        bool r = edge_info_equal( e1
                                , e2
                                , compose_unary(component_info(x),getinfo)
                                , info_category<edge_info<any_info> >::type()
                                )
                                ;
        if (not r) return false;
    }
    return true;
}

template <class Info, class SubInfoExtractor>
std::size_t edge_info_hash( edge<Info> const& e
                          , SubInfoExtractor const& getinfo
                          , info_tag const& select )
{
    using namespace boost;
    typedef typename unqualified_result_of<SubInfoExtractor(Info const*)>::type
            subinfo_type;

    BOOST_STATIC_ASSERT((
        is_same<
            typename info_category<subinfo_type>::type
          , info_tag
        >::value
    ));

    boost::hash<subinfo_type> infohash;
    if (getinfo(&e.info())->comparable()) {
        return infohash(*getinfo(&e.info()));
    } else {
        //std::cerr << "hashing type " << e.root().type() << std::endl;
        std::size_t ret = 0;

        constituents_generator<Info,SubInfoExtractor,info_comparable>
            frontier(ref_array(e),getinfo,comparable_);
        while (frontier) {
            boost::hash_combine(ret,frontier());
        }
        return ret;
    }
}

template <class Info>
std::size_t edge_info_hash(edge<Info> const& e)
{
    return edge_info_hash(e, get_info(), typename info_category<Info>::type());
}

template <class Info, class InfoExtractor>
std::size_t
edge_info_hash( edge<Info> const& e
              , InfoExtractor const& getinfo
              , joined_info_tag const& select )
{
    using namespace boost;
    typedef typename unqualified_result_of<InfoExtractor(Info const*)>::type
            subinfo_type;
    BOOST_STATIC_ASSERT((
           is_same<
                joined_info_tag
              , typename info_category<subinfo_type>::type
           >::value
    ));
    typedef typename subinfo_type::first_info_type info1_type;
    typedef typename subinfo_type::second_info_type info2_type;

    std::size_t ret = 0;
    first_info getinfo1;
    second_info getinfo2;
    hash_combine( ret
                , edge_info_hash( e
                                , compose_unary(getinfo1, getinfo)
                                , typename info_category<info1_type>::type()
                                )
                );
    hash_combine( ret
                , edge_info_hash( e
                                , compose_unary(getinfo2, getinfo)
                                , typename info_category<info2_type>::type()
                                )
                );
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

template <class Info, class InfoExtractor>
std::size_t
edge_info_hash( edge<Info> const& e
              , InfoExtractor const& getinfo
              , composite_info_tag const& select )
{
    using boost::hash_combine;
    size_t ret = 0;
    for (size_t x = 0; x != e.info().size(); ++x) {
        component_info ci(x);
        hash_combine( ret
                    , edge_info_hash( e
                                    , compose_unary(ci,getinfo)
                                    , info_category<edge_info<any_info> >::type()
                                    )
                    );
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

/*
template <class Info, class SubInfoExtractor>
std::size_t edge_info_scoreable_hash( edge<Info> const& e
                                    , SubInfoExtractor const& getinfo
                                    , info_tag const& select )
{
    using namespace boost;
    typedef typename unqualified_result_of<SubInfoExtractor(Info const*)>::type
            subinfo_type;

    BOOST_STATIC_ASSERT((
        is_same<
            typename info_category<subinfo_type>::type
          , info_tag
        >::value
    ));

    boost::hash<subinfo_type> infohash;
    if (getinfo(&e.info())->scoreable()) {
        return infohash(*getinfo(&e.info()));
    } else {
        return 0;
    }
}

template <class Info>
std::size_t edge_info_scoreable_hash(edge<Info> const& e)
{
    return edge_info_scoreable_hash(e, get_info(), typename info_category<Info>::type());
}

template <class Info, class InfoExtractor>
std::size_t
edge_info_scoreable_hash( edge<Info> const& e
                        , InfoExtractor const& getinfo
                        , joined_info_tag const& select )
{
    using namespace boost;
    typedef typename unqualified_result_of<InfoExtractor(Info const*)>::type
            subinfo_type;
    BOOST_STATIC_ASSERT((
           is_same<
                joined_info_tag
              , typename info_category<subinfo_type>::type
           >::value
    ));
    typedef typename subinfo_type::first_info_type info1_type;
    typedef typename subinfo_type::second_info_type info2_type;

    std::size_t ret = 0;
    first_info getinfo1;
    second_info getinfo2;
    hash_combine( ret
                , edge_info_scoreable_hash( e
                                          , compose_unary(getinfo1, getinfo)
                                          , typename info_category<info1_type>::type()
                                          )
                );
    hash_combine( ret
                , edge_info_scoreable_hash( e
                                          , compose_unary(getinfo2, getinfo)
                                          , typename info_category<info2_type>::type()
                                          )
                );
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

template <class Info, class InfoExtractor>
std::size_t
edge_info_scoreable_hash( edge<Info> const& e
                        , InfoExtractor const& getinfo
                        , composite_info_tag const& select )
{
    using boost::hash_combine;
    size_t ret = 0;
    for (size_t x = 0; x != e.info().size(); ++x) {
        component_info ci(x);
        hash_combine( ret
                    , edge_info_scoreable_hash( e
                                              , compose_unary(ci,getinfo)
                                              , info_category<edge_info<any_info> >::type()
                                              )
                    );
    }
    return ret;
}
*/

} // namespace sbmt

# endif //     SBMT__EDGE__INFO_COMBINING_HPP

