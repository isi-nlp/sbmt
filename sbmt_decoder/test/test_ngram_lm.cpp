#include <boost/test/auto_unit_test.hpp>
#include <boost/ref.hpp>
#include <sbmt/logmath.hpp>
#include <sbmt/ngram/LWNgramLM.hpp>
#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <map>
#include <set>


#include "test_util.hpp"

#include <graehl/shared/test.hpp>

using namespace sbmt;
using namespace sbmt::logmath;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE(test_ngram_options)
{
    using namespace graehl;
    using namespace std;
    
    ngram_options opt;
    stringstream s;
    string s1="[@u]",s2="[c]",s3="[@o]";
    BOOST_CHECK(test_extract_insert(s1,opt));
    BOOST_CHECK(test_extract_insert(s2,opt));
    BOOST_CHECK(test_extract_insert(s3,opt));
}

BOOST_AUTO_TEST_CASE(test_dynamic_ngram_lm)
{
}


BOOST_AUTO_TEST_CASE(test_ngram_lm)
{
    BOOST_CHECKPOINT("reading ngram");
    LWNgramLM lm;
    lm.read(SBMT_TEST_DIR "/test.lw");

    
    typedef unsigned W; //LWNgramLM::lm_id_type

    W c[10];
    ptrdiff_t people,coming,from,France,end=0;
    
    c[(people=end++)]=lm.id("people");
    c[(coming=end++)]=lm.id("coming");
    c[(from=end++)]=lm.id("from");
    c[(France=end++)]=lm.id("France");

    BOOST_CHECK_EQUAL(lm.word(c[from]),"from");
    W *upto=c+1;
    
    score_t zero=as_zero();
    using namespace sbmt::io;
    
//    logfile_registry::instance().set_logfile(ngram,"-");
//    logfile_registry::instance().set_logging_level(ngram,lvl_debug);    
    
    BOOST_CHECK_EQUAL(lm.open_prob(c,1).log10(),-1);
    BOOST_CHECK_EQUAL(lm.find_bow(c+people,upto+people).log10(),-16);
    BOOST_CHECK_EQUAL(lm.find_bow(c+people,upto+from),zero);
    BOOST_CHECK_EQUAL(lm.open_prob(c+coming,upto+from).log10(),-1);
    BOOST_CHECK_EQUAL(lm.find_bow(c+coming,upto+from).log10(),-11);
    BOOST_CHECK_EQUAL(lm.longest_prefix(c+people,c+end)-c,coming+1);
    BOOST_CHECK_EQUAL(lm.longest_prefix(c+coming,upto+from)-c,from+1);

    BOOST_CHECK(!lm.find_prob(c+coming,c+end).is_zero());
    
//    BOOST_CHECK_EQUAL(lm.longest_suffix(c+people,c+end)-c,from); // note: coming from france is in lm, but suffix/prefix return only up to maxorder-1 (i.e. things with backoffs)
//    BOOST_CHECK_EQUAL(lm.longest_suffix(c+coming,upto+from)-c,coming);
    
    {
        std::stringstream s;
        lm.print(s,c+people,c+end);
        BOOST_CHECK_EQUAL(s.str(),"\"people\",\"coming\",\"from\",\"France\"");
    }
    {
        std::stringstream s;
        lm.print(s,c+people,end);
        BOOST_CHECK_EQUAL(s.str(),"\"people\",\"coming\",\"from\",\"France\"");
//        BOOST_CHECK_EQUAL(s.str(),"NOT.");
    }
        
    score_t tolerance=1e-10;
    
}

////////////////////////////////////////////////////////////////////////////////

