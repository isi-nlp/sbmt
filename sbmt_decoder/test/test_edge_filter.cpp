#include <sbmt/search/edge_queue.hpp>
#include <sbmt/search/edge_filter.hpp>
#include <sbmt/edge/null_info.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <sbmt/sentence.hpp>
#include "grammar_examples.hpp"

//#include <boost/test/test_case_template.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <boost/logic/tribool.hpp>

/*
template class sbmt::cell_edge_filter<sbmt::edge<sbmt::null_info>,
                                      boost::logic::tribool>;
*/
/*
BOOST_AUTO_TEST_CASE(test_edge_queue)
{
    using namespace sbmt;
    
    typedef grammar_in_mem                             gram_type; 
    typedef edge<null_info>                            edge_type;
//    typedef concrete_edge_factory<edge_type,gram_type> tm_factory_type;
    typedef edge_factory<null_info_factory> tm_factory_type;
//    null_factory<null_info> nf;
    typedef edge_equivalence_pool<edge_type>           tm_equiv_pool_type;
    typedef edge_equivalence<edge_type>          edge_equiv_type;
       
    
    tm_factory_type ef;
    tm_equiv_pool_type epool;
    gram_type gram;
    
    init_grammar_marcu_1(gram);
    
    typedef edge_queue<edge_type> edge_queue_t;
    edge_queue<edge_type> pqueue1;
    edge_queue<edge_type> pqueue2;
    
    ///man i need to make some new examples:
    fat_sentence s = foreign_sentence("THESE 7PEOPLE INCLUDE COMINGFROM "
                                      "FRANCE AND RUSSIA p-DE ASTRO- -NAUTS .");
    
    fat_sentence::iterator itr = s.begin();
    fat_sentence::iterator end = s.end();
   
    int i=0;
    size_t sz = 0;
    /// initializing the chart with a sentence.
    for(; itr != end; ++itr,++i) {
        edge_type e = ef.create_edge(gram,*itr,span_t(i,i+1));
        /// not really how queue should be used...
        pqueue1.insert(epool, e);
        BOOST_CHECK_EQUAL(pqueue1.size(), ++sz);
        edge_queue_t::iterator pos = pqueue1.find(e);
        BOOST_CHECK(pos != pqueue1.end());
        pqueue2.insert(epool.create(e));
        
    }
    
    edge_equiv_type e1 = epool.create(
                             ef.create_edge(gram,"ASTRO-", span_t(8,9))
                         );
    edge_equiv_type e2 = epool.create(
                             ef.create_edge(gram,"-NAUTS", span_t(9,10))
                         );

    grammar_in_mem::token_factory_type tf = gram.dict();
    indexed_token astro  = tf.foreign_word("ASTRO-");
    indexed_token france = tf.foreign_word("FRANCE");
    indexed_token russia = tf.foreign_word("RUSSIA");
    indexed_token and_   = tf.foreign_word("AND");
    
    gram_type::rule_iterator rr = gram.binary_rules(astro).begin();
    
    gram_type::rule_range rrange= gram.binary_rules(astro);
    BOOST_CHECK(rrange.begin() != rrange.end()); 
    
    edge_equiv_type e3 = epool.create(
                          ef.create_edge(gram,*rr, e1, e2)
                      );
                      
    pqueue1.insert(epool, ef.create_edge(gram,*rr,e1,e2));
    pqueue2.insert(e3);
    
    BOOST_CHECK(e3.representative().score() < score_t(1.0));
    
    e1 = epool.create(
             ef.create_edge(gram,"FRANCE", span_t(4,5))
         );
    e2 = epool.create(
             ef.create_edge(gram,"AND", span_t(5,6))
         );    
    e3 = epool.create(
                  ef.create_edge(gram,"RUSSIA", span_t(6,7))
         );
         
    rr = gram.unary_rules(france).begin();
    
    edge_equiv_type e4 = epool.create(
                          ef.create_edge(gram,*rr, e1)
                      );
                      
    BOOST_CHECK(e4.representative().score() < score_t(1.0));
                      
    pqueue1.insert(epool, ef.create_edge(gram,*rr,e1));
    pqueue2.insert(e4);
    
    rr = gram.unary_rules(and_).begin();
    
    edge_equiv_type e5 = epool.create(
                          ef.create_edge(gram,*rr,e2)
                      );
    BOOST_CHECK(e5.representative().score() < score_t(1.0));
                      
    pqueue1.insert(epool, ef.create_edge(gram,*rr,e2));
    pqueue2.insert(e5);
    
    rr = gram.unary_rules(russia).begin();
    
    edge_equiv_type e6 = epool.create(
                          ef.create_edge(gram,*rr, e3)
                      );
                      
    BOOST_CHECK(e6.representative().score() < score_t(1.0));
                      
    pqueue1.insert(epool, ef.create_edge(gram,*rr,e3));
    pqueue2.insert(e6);
    
    edge_equiv_type e7 = epool.create(
                             ef.create_edge(gram,"COMINGFROM",span_t(3,4))
                         );
    indexed_token from = tf.foreign_word("COMINGFROM");
    indexed_token NNS  = tf.tag("NNS");
    indexed_token NNP  = tf.tag("NNP");
    indexed_token CC   = tf.tag("CC");
    indexed_token NP   = tf.tag("NP");

    
    BOOST_CHECK_EQUAL(pqueue1.size(),pqueue2.size());
    BOOST_CHECK(!pqueue1.empty() and !pqueue2.empty());
    
    score_t min(0.0);
    while (!pqueue1.empty()) {
        BOOST_CHECK_EQUAL(pqueue1.size(),pqueue2.size());
        std::size_t sz = pqueue1.size();
        BOOST_CHECK_EQUAL(pqueue2.empty(),false);
        edge_equiv_type eq1 = pqueue1.top();
        edge_equiv_type eq2 = pqueue2.top();
        BOOST_CHECK(eq1.representative() == eq2.representative());
        BOOST_CHECK(eq1.representative().score() >= min);
//        std::cout << "edge:" 
//                  << print(eq1.representative(),gram.dict()) 
//                  << "score:"
//                  << eq1.representative().score()
//                  << std::endl;
        min = eq1.representative().score();
        pqueue1.pop();
        pqueue2.pop();
        BOOST_CHECK_EQUAL(pqueue1.size(),sz - 1);
    }
    BOOST_CHECK(pqueue2.empty());
    BOOST_CHECK(min == score_t(1.0));
}*/
