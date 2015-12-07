#include <sbmt/chart/unordered_cell.hpp>
#include <sbmt/chart/ordered_cell.hpp>
#include <sbmt/edge/edge.hpp>
#include <sbmt/edge/null_info.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>

#include <boost/test/auto_unit_test.hpp>

#include "grammar_examples.hpp"
/*
BOOST_AUTO_TEST_CASE(test_cell)
{
    using namespace sbmt;
    
    typedef edge<null_info>                          tm_edge;
    typedef grammar_in_mem                           gram_type;
//    typedef concrete_edge_factory<tm_edge,gram_type> tm_edge_factory;
    typedef edge_factory<null_info_factory> tm_edge_factory;
    
    typedef edge_equivalence_pool<tm_edge>           tm_equiv_pool;
    tm_edge_factory tm_ef;
    tm_equiv_pool   tm_epool;
    
    gram_type g;
    init_grammar_marcu_1(g);

    indexed_token COMING=g.dict().foreign_word("COMINGFROM");
    //    grammar_in_mem::rule_range crules=g.unary_rules(COMING);
    
    tm_edge e = tm_ef.create_edge(COMING,span_t(1,2));
    
    detail::unordered_cell<tm_edge> ui(tm_epool, e); 
    ui.insert_edge(tm_epool, e);
    detail::unordered_cell<tm_edge>::range urange = ui.edges(); 
    
    BOOST_CHECK(ui.root() == COMING);
    BOOST_CHECK(ui.span() == span_t(1,2));
    
    
    detail::ordered_cell<tm_edge> oi(tm_epool, e);
    oi.insert_edge(tm_epool,e);
    detail::ordered_cell<tm_edge>::range orange = oi.edges(); 
    
    BOOST_CHECK(oi.root() == COMING);
    BOOST_CHECK(oi.span() == span_t(1,2));
}

template <class R>
inline size_t size_range(R const &r)
{
    size_t s=0;
    for (typename R::iterator i=r.begin();i!=r.end();++i)
        ++s;
    return s;
}

template <class E>
inline bool eq_score(E const& e1,E const & e2)
{
    return e1.inside_score() == e2.inside_score();
}


template <class R,class V>
inline bool range_contains_score(R const &r, V const &value)
{
    for (typename R::iterator i=r.begin();i!=r.end();++i)
        if (eq_score(*i,value))
            return true;
    return false;
}


BOOST_AUTO_TEST_CASE(test_equivalence)
{
    using namespace sbmt;
    typedef edge<null_info>                E;
    typedef edge_equivalence<E> Eq;
    edge_equivalence_pool<E> pool;
    
    indexed_token_factory tf;
    indexed_token f=tf.foreign_word("COMINGFROM");
    span_t s(1,2);
    
    E e;
    Eq eq;
    BOOST_CHECK(eq.is_none());

    unsigned max=2;
    Eq::max_edges_per_equivalence(max);
    BOOST_CHECK_EQUAL(Eq::max_edges_per_equivalence(),max);
    e.set_lexical(f,s,.1);
    eq=pool.create(e);
    BOOST_CHECK_EQUAL(size_range(eq.edges()),size_t(1));
    BOOST_CHECK(eq_score(*eq.edges().begin(),e));
    E e2;
    e2.set_lexical(f,s,.2);
    eq.insert_edge(e2);
    BOOST_CHECK_EQUAL(size_range(eq.edges()),max);
    BOOST_CHECK(eq_score(*eq.edges().begin(),e2));
    BOOST_CHECK(range_contains_score(eq.edges(),e));
    E e3;
    e3.set_lexical(f,s,.5);
    BOOST_CHECK(!range_contains_score(eq.edges(),e3));
    eq.insert_edge(e3);
    BOOST_CHECK_EQUAL(size_range(eq.edges()),max);    
    BOOST_CHECK(eq_score(*eq.edges().begin(),e3));
    BOOST_CHECK(!range_contains_score(eq.edges(),e));
    BOOST_CHECK(range_contains_score(eq.edges(),e2));
    eq.insert_edge(e);
    BOOST_CHECK(!range_contains_score(eq.edges(),e));

    E e4;
    e4.set_lexical(f,s,1);
    Eq eq2=pool.create(e4);
    eq2.insert_edge(e2);
    BOOST_CHECK(range_contains_score(eq2.edges(),e2));
    BOOST_CHECK(!range_contains_score(eq2.edges(),e3));
    BOOST_CHECK(range_contains_score(eq2.edges(),e4));
    
    eq.merge(eq2);
    BOOST_CHECK_EQUAL(size_range(eq.edges()),max);    
    BOOST_CHECK(!range_contains_score(eq.edges(),e));
    BOOST_CHECK(!range_contains_score(eq.edges(),e2));
    BOOST_CHECK(eq_score(*eq.edges().begin(),e4));
    BOOST_CHECK(range_contains_score(eq.edges(),e3));
}
*/