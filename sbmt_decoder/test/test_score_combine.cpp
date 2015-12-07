#include <boost/test/auto_unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <sbmt/feature/feature_vector.hpp>
#include "test_util.hpp"

using namespace sbmt;
using namespace std;

template <class Feats>
void test_sc2(fat_feature_vector &sc2,Feats &feat_vec)
{
    BOOST_CHECK_EQUAL( feat_vec ^ sc2
                       , (score_t(0.5) ^ 2.0) * (score_t(0.5) ^ 0.5) );
    BOOST_CHECK_EQUAL(sc2["notfound"],0.0);
    BOOST_CHECK_EQUAL(sc2["scr"],2.0);
    BOOST_CHECK_EQUAL(sc2["scr'"],0.0);   
}


BOOST_AUTO_TEST_CASE(test_score_combiner)
{
    fat_weight_vector sc;
    sc["scr"] = 1.0;
    sc["ft"]  = 2.0;
    
    fat_feature_vector feat_vec;
    feat_vec["scr"] = score_t(0.5);
    feat_vec["ft"]  = score_t(0.5);
    
    BOOST_CHECK_EQUAL(geom(feat_vec,sc), score_t(0.5*0.25));
    
    sc["redundant"] = -4.0;
    BOOST_CHECK_EQUAL(geom(feat_vec,sc), score_t(0.5*0.25));
    
    read(sc, "scr:0.75,ft:0.5");
    BOOST_CHECK_EQUAL( geom(feat_vec,sc)
                     , (score_t(0.5) ^ 0.75) * (score_t(0.5) ^ 0.5) );
                     
    sc["redundant"] = -2.0;
    BOOST_CHECK_EQUAL( geom(feat_vec,sc)
                     , (score_t(0.5) ^ 0.75) * (score_t(0.5) ^ 0.5) );
                     
    LOGNUMBER_CHECK_CLOSE( geom(feat_vec,sc)
                         , (score_t(0.5) ^ 1.25)
                         , score_t(0.0000001) );
    
    LOGNUMBER_CHECK_CLOSE( geom(feat_vec,sc)
                         , score_t(0.4204482076)
                         , score_t(0.0000001) );
                         
    feat_vec["redundant"] = score_t(0.1);
    
    LOGNUMBER_CHECK_CLOSE( geom(feat_vec,sc)
                         , score_t(42.04482076)
                         , score_t(0.0000001) );
                         
    feat_vec["no-combine"] = score_t(0.1);
    LOGNUMBER_CHECK_CLOSE( geom(feat_vec,sc)
                         , score_t(42.04482076)
                         , score_t(0.0000001) );

/*
    std::string str2="scr:2.0,ft:0.5,mdinv:1.0";
    fat_weight_vector sc2;
    read(sc2,str2);

    test_sc2(sc2,feat_vec);
    

    std::istringstream istr2(str2);
    score_combiner sc3;
    istr2 >> sc3;
    test_sc2(sc3,feat_vec);

    sc3.reset();
    BOOST_CHECK_EQUAL(sc3.get_feature_weight("scr"),0.0);
    sc3=boost::lexical_cast<score_combiner>(str2);
    test_sc2(sc3,feat_vec);    
*/
    
}
