#include <sbmt/grammar/grammar_in_memory.hpp>
#include <sbmt/grammar/logging.hpp>
#include <sbmt/grammar/grammar.hpp>
#include "grammar_examples.hpp"
#include "test_util.hpp"
#include <sbmt/grammar/brf_file_reader.hpp>
#include <sbmt/grammar/lm_string.hpp>
using namespace sbmt;

struct id_sort {
    bool operator()(scored_syntax const& s1, scored_syntax const& s2) const
    {
        return s1.rule.id() < s2.rule.id();
    }
};

struct rule_sort {
    grammar_in_mem* g;
    rule_sort(grammar_in_mem& g) : g(&g) {}
    bool operator()( grammar_in_mem::rule_type r1
                   , grammar_in_mem::rule_type r2 ) const
    {
        if (g->rule_lhs(r1) != g->rule_lhs(r2)) 
            return label_less(g->rule_lhs(r1), g->rule_lhs(r2));
        else if (g->rule_rhs(r1,0) != g->rule_rhs(r2,0)) 
            return label_less(g->rule_rhs(r1,0), g->rule_rhs(r2,0));
        else if (g->rule_rhs(r1,1) != g->rule_rhs(r2,1)) 
            return label_less(g->rule_rhs(r1,1), g->rule_rhs(r2,1));
        else if (g->rule_lhs(r1).type() == tag_token) 
            return g->get_syntax(r1).id() < g->get_syntax(r2).id();
        else return false;
    }
    
    bool label_less(indexed_token t1, indexed_token t2) const
    {
        if (t1.type() != t2.type()) return t1.type() < t2.type();
        else return g->dict().label(t1) < g->dict().label(t2);
    }
};

void check_same_syntax_rules(grammar_in_mem &gram1, grammar_in_mem &gram2,double g2overg1,char const* desc)
{
    score_t eps=0.00001;
    
    typedef std::multiset<scored_syntax,id_sort> synset;
    
    grammar_in_mem::syntax_range r1 = gram1.all_syntax_rules();
    grammar_in_mem::syntax_range r2 = gram2.all_syntax_rules();
    
    synset synset1(r1.begin(),r1.end()), synset2(r2.begin(),r2.end());
    
    synset::iterator ritr1 = synset1.begin(), ritr2 = synset2.begin();
    //std::cout << logmath::log_scale;
    indexed_token_factory &tf1 = gram1.dict();
    indexed_token_factory &tf2 = gram2.dict();
    
    for (;ritr1 != synset1.end() and ritr2 != synset2.end(); ++ritr1, ++ritr2)
    {
        scored_syntax const& s1=*ritr1;
        scored_syntax const& s2=*ritr2;
        BOOST_CHECK_EQUAL(tf1.label(s1.rule.lhs_root_token()),tf2.label(s2.rule.lhs_root_token()));

        BOOST_CHECK_MESSAGE(
            s1.rule.id()==s2.rule.id()
           , desc << "\n\t"
                  << print(s1.rule,tf1) 
                  << " != " 
                  << print(s2.rule,tf2)
        );
// all scores (component and mixed) are now held in brf rules
//
//        LOGNUMBER_CHECK_EXP_CLOSE_MSG(
//            desc << "\n\tsyntax rule score for rules\n\t"
//                 << print(s1.rule,tf1) << "\n\t"
//                 << print(s2.rule,tf2) << "\n\t"
//          , pow(s1.score,g2overg1)
//          , s2.score
//          , eps
//        );
        
    }
    BOOST_CHECK(ritr2 == synset2.end());
    BOOST_CHECK(ritr1 == synset1.end());

 }

    

void check_same_grammar(grammar_in_mem &gram1, grammar_in_mem &gram2, double g2overg1,char const* desc,bool check_h=true)
{
    check_same_syntax_rules(gram1,gram2,g2overg1,desc);
    //std::cout << "Comparing grammars:\n" << desc << "\n";

    score_t eps=0.00001;
    
    indexed_token_factory &tf1 = gram1.dict();
    indexed_token_factory &tf2 = gram2.dict();
    
    grammar_in_mem::rule_range r1 = gram1.all_rules();
    grammar_in_mem::rule_range r2 = gram2.all_rules();
    
    typedef std::multiset<grammar_in_mem::rule_type,rule_sort> ruleset;
    ruleset rs1(r1.begin(),r1.end(),rule_sort(gram1)),
            rs2(r2.begin(),r2.end(),rule_sort(gram2));
    
    ruleset::iterator ritr1 = rs1.begin();
    ruleset::iterator ritr2 = rs2.begin();
    
    //std::cerr << logmath::log_scale;
    
    for (;ritr1 != rs1.end() and ritr2 != rs2.end(); ++ritr1, ++ritr2)
    {
        BOOST_REQUIRE(ritr2 != rs2.end());
        indexed_token tok1 = gram1.rule_lhs(*ritr1);
        indexed_token tok2 = gram2.rule_lhs(*ritr2);
        
        BOOST_CHECK_EQUAL(tf1.label(tok1), tf2.label(tok2));
        BOOST_CHECK_EQUAL(tok1.type(), tok2.type());
        if (tok1.type() == tag_token)
            BOOST_CHECK_EQUAL(gram1.get_syntax_id(*ritr1)
                             ,gram2.get_syntax_id(*ritr2));
        LOGNUMBER_CHECK_EXP_CLOSE_MSG(
            desc << "\n\treal rule score for rules \n\t"
                 << print(*ritr1,gram1) << "\n\t"
                 << print(*ritr2,gram2) << "\n\t"
          , pow(gram1.rule_score(*ritr1),g2overg1)
          , gram2.rule_score(*ritr2)
          , eps 
        );

        if (check_h) {
        LOGNUMBER_CHECK_EXP_CLOSE_MSG(
            desc << "\n\treal rule heuristic for rules \n\t"
                 << print(*ritr1,gram1) << "\n\t"
                 << print(*ritr2,gram2) << "\n\t"
          , pow(gram1.rule_score_estimate(*ritr1),g2overg1)
          , gram2.rule_score_estimate(*ritr2)
          , eps 
        );
        }
        
        BOOST_CHECK_EQUAL( gram1.rule_property<bool>(*ritr1,lm_id)
                         , gram2.rule_property<bool>(*ritr2,lm_id) );
        BOOST_CHECK_EQUAL(
            fatten(gram1.rule_property<indexed_lm_string>(*ritr1,lm_string_id),gram1.dict())
          , fatten(gram2.rule_property<indexed_lm_string>(*ritr2,lm_string_id),gram2.dict())
        );
        
        
        char const* type="real";
        
        if (tok1.type() == virtual_tag_token) {
            type="virtual";
            LOGNUMBER_CHECK_EXP_CLOSE_MSG(
                desc << "\n\tvirtual rule score for rules \n\t"
                     << print(*ritr1,gram1) <<"\n\t"
                     << print(*ritr2,gram2) <<"\n\t"
              , pow(gram1.rule_score(*ritr1),g2overg1)
              , gram2.rule_score(*ritr2)
              , eps 
            );
        }
        if (check_h) {   
            LOGNUMBER_CHECK_EXP_CLOSE_MSG(
                desc << "\n\t" << type << " heuristics for rules \n\t"
                     << print(*ritr1,gram1) <<"\n\t"
                     << print(*ritr2,gram2) <<"\n\t"
              , pow(gram1.rule_score_estimate(*ritr1),g2overg1) 
              , gram2.rule_score_estimate(*ritr2)
              , eps 
            );
        }
        
        tok1 = gram1.rule_rhs(*ritr1,0);
        tok2 = gram2.rule_rhs(*ritr2,0);
        BOOST_CHECK_EQUAL(tf1.label(tok1), tf2.label(tok2));
        BOOST_CHECK_EQUAL( gram1.rule_rhs_size(*ritr1)
                         , gram2.rule_rhs_size(*ritr2) );
        
//        std::cerr << print(*ritr1, gram1) << std::endl;
//        std::cerr << print(*ritr2, gram2) << std::endl;
    }
    BOOST_CHECK(ritr2 == rs2.end());
    BOOST_CHECK(ritr1 == rs1.end());
}

void set_prior(grammar_in_mem &g,double scale=1,double wt=1)
{
    std::stringstream sstr(grammar_nts);
    //std::cout << "\ngrammar-nts:\n"<< sstr.str() << std::endl;
    g.load_prior(sstr,1e-9,1,scale,wt);
    //g.prior.raise_pow(scale);
//    g.prior.set_prior(g.dict().tag("VP"),pow(p);
}


SBMT_SET_DOMAIN_LOGGING_LEVEL(grammar,pedantic);
SBMT_SET_DOMAIN_LOGFILE(grammar,"-2");


// tests the typical usage of push/popping grammars:
//  push a sequence of grammars that will stay loaded.
//  then push/pop a sequence of grammars (the per-sentence grammars)
//  at the end of the push/pop sequence, assert that the state of the grammar
//  is equivalent to some freshly loaded grammar.
template <class Chain>
void push_pop_chain(Chain initials, Chain push_pops, std::string final)
{
    
    using namespace boost;
    grammar_in_mem final_gram, chain_gram;
    fat_weight_vector sc;
    read(sc,"p:1.0,text-length:0.1");
    std::ifstream prior;
    prior.open(SBMT_TEST_DIR "/xgram/prior");
    final_gram.load_prior(prior,0.0005,100);
    prior.close();
    prior.open(SBMT_TEST_DIR "/xgram/prior");
    chain_gram.load_prior(prior,0.0005,100);
    prior.close();
    chain_gram.update_weights(sc,example_grammar_pc);
    std::stringstream msg;
    typename range_iterator<Chain>::type i = begin(initials), e = end(initials);
    msg << "init:[";
    if (i != e) {
        msg << *i;
        BOOST_CHECKPOINT(msg.str());
        std::ifstream f(i->c_str());
        brf_stream_reader b(f);
        chain_gram.load(b,sc,example_grammar_pc);
        ++i;
    } 
    for (; i != e; ++i) {
        msg << "+" << *i;
        BOOST_CHECKPOINT(msg.str());
        std::ifstream f(i->c_str());
        brf_stream_reader b(f);
        chain_gram.push(b);
    }
    msg << "]";
    i = begin(push_pops); e = end(push_pops);
    if (i != e) {
        msg << ";push:" << *i;
        BOOST_CHECKPOINT(msg.str());
        std::ifstream f(i->c_str());
        brf_stream_reader b(f);
        chain_gram.push(b);
        ++i;
    }
    for (;i != e; ++i) {
        msg << ";pop";
        BOOST_CHECKPOINT(msg.str());
        chain_gram.pop();
        msg << ";push:" << *i;
        BOOST_CHECKPOINT(msg.str());
        std::ifstream f(i->c_str());
        brf_stream_reader b(f);
        chain_gram.push(b);
    }
    msg << "; ---> "<< final;
    BOOST_CHECKPOINT(msg.str());
    std::ifstream f(final.c_str());
    brf_stream_reader b(f);
    final_gram.load(b,sc,example_grammar_pc);
    
    check_same_grammar(chain_gram,final_gram,1,msg.str().c_str());
}

BOOST_AUTO_TEST_CASE(test_push_pop_chain0)
{   
    using boost::make_iterator_range;
    using std::string;
    string inits[] = { "dummy" };
    string chain[] = { SBMT_TEST_DIR "/xgram/xgram.brf" };
    string final = SBMT_TEST_DIR "/xgram/xgram.brf";
    push_pop_chain( make_iterator_range(inits,inits) //empty
                  , make_iterator_range(chain)
                  , final
                  );
}

BOOST_AUTO_TEST_CASE(test_push_pop_chain1)
{   
    using boost::make_iterator_range;
    using std::string;
    string inits[] = { SBMT_TEST_DIR "/xgram/xgram.0m4.brf" };
    string chain[] = { SBMT_TEST_DIR "/xgram/xgram.1m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.2m4.brf"
                     };
    string final = SBMT_TEST_DIR "/xgram/xgram.02m4.brf";
    push_pop_chain( make_iterator_range(inits)
                  , make_iterator_range(chain)
                  , final
                  );
}

BOOST_AUTO_TEST_CASE(test_push_pop_chain2)
{   
    using boost::make_iterator_range;
    using std::string;
    string inits[] = { SBMT_TEST_DIR "/xgram/xgram.0m4.brf" };
    string chain[] = { SBMT_TEST_DIR "/xgram/xgram.1m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.2m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.3m4.brf" 
                     };
    string final = SBMT_TEST_DIR "/xgram/xgram.03m4.brf";
    push_pop_chain( make_iterator_range(inits)
                  , make_iterator_range(chain)
                  , final
                  );
}

BOOST_AUTO_TEST_CASE(test_push_pop_chain3)
{   
    using boost::make_iterator_range;
    using std::string;
    string inits[] = { SBMT_TEST_DIR "/xgram/xgram.0m4.brf" };
    string chain[] = { SBMT_TEST_DIR "/xgram/xgram.3m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.2m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.1m4.brf" 
                     };
    string final = SBMT_TEST_DIR "/xgram/xgram.01m4.brf";
    push_pop_chain( make_iterator_range(inits)
                  , make_iterator_range(chain)
                  , final
                  );
}

BOOST_AUTO_TEST_CASE(test_push_pop_chain4)
{   
    using boost::make_iterator_range;
    using std::string;
    string inits[] = { SBMT_TEST_DIR "/xgram/xgram.0m4.brf" };
    string chain[] = { SBMT_TEST_DIR "/xgram/xgram.2m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.3m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.1m4.brf" 
                     };
    string final = SBMT_TEST_DIR "/xgram/xgram.01m4.brf";
    push_pop_chain( make_iterator_range(inits)
                  , make_iterator_range(chain)
                  , final
                  );
}

BOOST_AUTO_TEST_CASE(test_push_pop_chain5)
{   
    using boost::make_iterator_range;
    using std::string;
    string inits[] = { SBMT_TEST_DIR "/xgram/xgram.0m4.brf" };
    string chain[] = { SBMT_TEST_DIR "/xgram/xgram.2m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.3m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.2m4.brf" 
                     };
    string final = SBMT_TEST_DIR "/xgram/xgram.02m4.brf";
    push_pop_chain( make_iterator_range(inits)
                  , make_iterator_range(chain)
                  , final
                  );
}

BOOST_AUTO_TEST_CASE(test_push_pop_chain6)
{   
    using boost::make_iterator_range;
    using std::string;
    string inits[] = { SBMT_TEST_DIR "/xgram/xgram.0m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.1m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.2m4.brf"
                     , SBMT_TEST_DIR "/xgram/xgram.3m4.brf" 
                     };
    string chain[] = { "dummy" };
    string final = SBMT_TEST_DIR "/xgram/xgram.brf";
    push_pop_chain( make_iterator_range(inits)
                  , make_iterator_range(chain,chain) //empty
                  , final
                  );
}


BOOST_AUTO_TEST_CASE(test_archive_integrity)
{
    
    grammar_in_mem gram1, gram2;
    double bp=1;
    set_prior(gram1,bp);
    //set_prior(gram2,bp);
    gram2.load_prior(.1);    
    double wa=.5,wb=.7,wc=2,wd=-.1,wt=1;
    
    init_sc::init(example_grammar_sc,wa,wb,wc,wd,wt);
    init_grammar_marcu_staggard_wts_from_archive(gram1);
    init_grammar_marcu_staggard_wts(gram2);


    check_same_grammar(gram1,gram2,1,"1",false);

    grammar_in_mem gram3,gram4;
    double port=.5;
    
    set_prior(gram3,bp*port,1/port);
    //gram3.weight_tag_prior=1/port;
    init_grammar_marcu_staggard_wts_from_archive(gram3);
    check_same_grammar(gram1,gram3,1,"weight_prior",true);

    double r=10;
    set_prior(gram4,bp*r);
    init_sc::init(example_grammar_sc,r*wa,r*wb,r*wc,r*wd,r*wt);
    init_grammar_marcu_staggard_wts_from_archive(gram4);
    
    check_same_grammar(gram1,gram4,r,"scaled",true);

    
    fat_weight_vector sc;
    init_sc::init(sc,wa,wb,wc,wd,wt);
    gram4.load_prior(.1);
    gram4.update_weights(sc,example_grammar_pc);

    check_same_grammar(gram2,gram4,1,"recompute",true);
}
