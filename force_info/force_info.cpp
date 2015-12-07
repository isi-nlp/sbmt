# include <force_info/force_info.hpp>
/*
 
 load-grammar brf "../small.brf";
 set-info-option force force-constraint "77 reiterated its opposition to foreign military forces";
 lattice id="1" { 
   [0,1] "<foreign-sentence>"; 
   [1,2] "c1"; 
   [2,3] "c2";
   [3,4] "c3";
   [4,5] "c4";
   [5,6] "c5"; 
 };
 
 (41 (19 (12 1) (18 2 (13 3 (23 5)))))
 (41 (35 (12 1) 2 (13 3 (23 5))))
 (41 (15 (12 1) 2 (13 3 (23 5))))
 (41 (14 (12 1) 2 (13 3 (23 5))))
 
 */


////////////////////////////////////////////////////////////////////////////////


struct force_info_init {
    force_info_init()
    {
        sbmt::register_info_factory_constructor("force", sbmt::force_constructor());
        sbmt::register_rule_property_constructor("force","lm_string",sbmt::lm_string_constructor());
        sbmt::register_rule_property_constructor("force","etree_string",sbmt::lm_string_constructor());
    }
};

static force_info_init f;


