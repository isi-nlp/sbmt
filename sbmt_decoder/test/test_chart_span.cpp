#include <sbmt/chart/unordered_cell.hpp>
#include <sbmt/chart/ordered_cell.hpp>
#include <sbmt/chart/ordered_span.hpp>
#include <sbmt/chart/unordered_span.hpp>
#include <sbmt/edge/edge.hpp>
#include <sbmt/edge/null_info.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>

#include <boost/test/auto_unit_test.hpp>

#include "grammar_examples.hpp"

//FIXME: /nfs/topaz/graehl/dev/sbmt/trunk/sbmt_decoder/test/test_chart_span.cpp:22: error: no matching function for call to 'sbmt::concrete_edge_factory<sbmt::edge<sbmt::null_info>, sbmt::grammar_in_mem>::concrete_edge_factory()'
/*
BOOST_AUTO_TEST_CASE(test_chart_span)
{
    using namespace sbmt;
    
    typedef grammar_in_mem                           gram_type;
    typedef edge<edge_info<null_info> >              tm_edge;
    typedef concrete_edge_factory<tm_edge,gram_type> tm_edge_factory;
    typedef edge_equivalence_pool<tm_edge>           tm_equiv_pool;
//    null_info_factory tm_ef;
    edge_factory<edge_info_factory<null_info_factory> > tm_ef;
        
    tm_equiv_pool   tm_epool;
    
    gram_type g;
    init_grammar_marcu_1(g);
    
    tm_edge e (tm_ef.create_edge(g,"COMINGFROM",span_t(1,2)));
    
    detail::unordered_cell<tm_edge> ui(tm_epool, e); 
    ui.insert_edge(tm_epool, e);
    detail::unordered_cell<tm_edge>::range urange = ui.edges(); 
    
    BOOST_CHECK(ui.root() == g.dict().foreign_word("COMINGFROM"));
    BOOST_CHECK(ui.span() == span_t(1,2));
    
    
    detail::ordered_cell<tm_edge> oi(tm_epool, e);
    oi.insert_edge(tm_epool,e);
    detail::ordered_cell<tm_edge>::range orange = oi.edges(); 
    
    BOOST_CHECK(oi.root() == g.dict().foreign_word("COMINGFROM"));
    BOOST_CHECK(oi.span() == span_t(1,2));
    
    typedef detail::ordered_span< detail::ordered_cell<tm_edge> > oo_span_t;
    oo_span_t oos(tm_epool,e);
    
    oos.insert_edge(tm_epool,e);
    //BOOST_CHECK(oos.begin()->span() == span_t(1,2));
    
    typedef detail::unordered_span< detail::ordered_cell<tm_edge> > uo_span_t;
    uo_span_t uos(tm_epool,e);
    
    uos.insert_edge(tm_epool,e);
    //BOOST_CHECK(uos.begin()->span() == span_t(1,2));

}*/

