# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/token.hpp>
# include <iostream>
# include <sstream>
# include <boost/tuple/tuple.hpp>
# include <boost/tr1/unordered_map.hpp>
# include <gusc/generator.hpp>
# include <gusc/iterator.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/search/lazy/indexed_varray.hpp>
# include <sbmt/feature/feature_vector.hpp>
# include <sbmt/forest/implicit_xrs_forest.hpp>

# include <mugu/t2s.hpp>

using namespace std;
using namespace sbmt;
using namespace sbmt::t2s;

int main(int argc, char** argv)
{   
    dictdata dd;
    //in_memory_dictionary dict;
    //feature_dictionary fdict;
    
    dd.weights = weights_from_file(argv[1],dd.feature_names());

    cout << token_label(dd.dict());
    cerr << token_label(dd.dict());
    
    std::ifstream ifs(argv[2]);
    rulemap_t rulemap = read_grammar( ifs
                                          , dd.dict()
                                          , dd.feature_names()
                                          , dd.weights );
    supp_rulemap_t supp_rulemap;

   /*
    BOOST_FOREACH( rulemap_t::value_type const& rrd, rulemap)  {
        BOOST_FOREACH( rulemap_entry::value_type const& rd_, rrd.second) {
            BOOST_FOREACH(rule const& rd, rd_){
                copy(rrd.first.begin(),rrd.first.end(),ostream_iterator<grammar_state>(cout," "));
                cout << "|||" << print(rd.state,dd.dict()) << "|||" << rd.score;
                if (not rd.state.virt) cout << "|||" << print(rd.syntax,dd.dict()) << " " << nbest_features(rd.scores,dd.feature_names());
                cout << '\n';
            }
        }
    }
    */


    std::string line;
    getline(cin,line);
    rule_data sourcerule = parse_xrs(line);
    feature_vector sfv = features(sourcerule,dd.feature_names());
    score_t sscr = geom(sfv,dd.weights);
    indexed_syntax_rule source(sourcerule,dd.dict());
    
    size_t rid = 412000000000;
    add_supps( source
             , *source.lhs_root()
             , supp_rulemap
             , dd.dict()
             , dd.feature_names()
             , dd.weights
             , rid );

/*
    cout << "SUPP\n";
    BOOST_FOREACH( supp_rulemap_t::value_type const& rrd, supp_rulemap)  {
        copy(rrd.first.begin(),rrd.first.end(),ostream_iterator<grammar_state>(cout," "));
        cout << "|||" << print(rrd.second.state,dd.dict());
        if (not rrd.second.state.virt) cout << "|||" << print(rrd.second.syntax,dd.dict());
        cout << '\n';
    }
*/

    std::tr1::unordered_map<size_t,gusc::shared_varray<forest> > empty;
    gusc::shared_varray<forest> 
        ff = forests(source,*source.lhs_root(),sfv,sscr,empty,rulemap,supp_rulemap);
    if (ff.size()) {
/*
        BOOST_FOREACH(forest fff, ff) {
            BOOST_FOREACH(hyp h,fff) {
                std::cout << h.score() << ' ' << print(h.scores(),dd.feature_names()) << '\n';
            }
        }
*/
        xforest xf(forest_as_xforest(ff[0],dd.weights));
/*
        print_id_forest(cout,xf,dd);
        cout << "\n\n";
        print_target_string_forest(cout,xf,dd);
        cout << "\n\n";
        print_source_string_forest(cout,xf,dd);
        cout << "\n";
*/
        xtree_generator gentree = xtrees_from_xforest(xf);
        while (gentree) {
            xtree_ptr tp = gentree();
            cout << xf.score() << '\t' << tp->scr << '\t'
                 << sbmt::t2s::hyptree(tp,dd.dict()) << " -> \"fake\" ### id=1 "
                 << sbmt::t2s::nbest_features(sbmt::t2s::accum(tp),dd.feature_names()) 
                 << " deriv={{{" << tp << "}}}\n";
            break;
        }
    }

    return 0;
}
