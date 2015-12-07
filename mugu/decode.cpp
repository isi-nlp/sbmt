# include <mugu/t2s.hpp>
# include <boost/tr1/unordered_map.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/edge/null_info.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/search/impl/block_lattice_tree.tpp>
# include <sbmt/grammar/brf_archive_io.hpp>
# include <sbmt/search/lazy.hpp>
# include <sbmt/search/lazy/cky.hpp>
# include <sbmt/search/limit_syntax_length_filter.hpp>
# include <sbmt/search/filter_predicates.hpp>
# include <sbmt/search/edge_filter.hpp>
# include <string>
# include <fstream>

int main(int argc, char** argv)
{
    using namespace sbmt;
    using namespace sbmt::lazy;
    using namespace std;
    
    
    
    string lat = argv[1];
    string wts = argv[2];
    string global = argv[3];
    string psent = argv[4];
    
    fat_weight_vector weights = sbmt::t2s::weights_from_file(wts);
    std::cerr << "weights loaded\n";
    property_constructors<> pcons;
    grammar_in_mem gram;
    cerr << token_label(gram.dict());
    cerr << token_label(gram.dict());
    std::ifstream gifs(global.c_str());
    brf_fat_to_indexed_archive_reader grd(gifs);
    gram.load(grd,weights,pcons);
    
    std::cerr << "globals loaded\n";
    
    std::ifstream pifs(psent.c_str());
    brf_fat_to_indexed_archive_reader prd(pifs);
    gram.push(prd);
    std::cerr << "per-sent loaded\n";
    
    gusc::lattice_ast last;
    std::ifstream latifs(lat.c_str());
    latifs >> last;
    std::cerr << last << "\n";
    lattice_tree lattree = convert(gram,last);
    std::cerr << "lattice loaded\n";
    //std::cerr << lattree << "\n";
    span_t total_span = lattree.root().span();
    
    std::auto_ptr< edge_factory< edge_info_factory<null_info_factory> > > efactory(new edge_factory< edge_info_factory<null_info_factory> >());
    //boost::shared_ptr<t2s_edge_factory> efactory(new t2s_edge_factory(tgt,gram,total_span));
    typedef edge< edge_info<null_info> > edge_type;
    
    concrete_edge_factory<edge_type,grammar_in_mem> ecs(efactory);
    //std::cerr << "t2s loaded\n";
    
    exhaustive_top_addition<edge_type> tops(gram, ecs);
    
    first_experiment_cell_proc<edge_type>
        cell_proc( gram
                 , ecs
                 , lattree
                 , make_predicate_edge_filter<edge_type>(poplimit_histogram_predicate(10,10,10))
                 , make_predicate_edge_filter<edge_type>(histogram_predicate(0))
                 , 3 );
    
    limit_syntax_length_generator ckygen(40);
    
   
    //try {
        edge_equivalence<edge_type>::max_edges_per_equivalence(4);
        edge_equivalence<edge_type> top_equiv;
        bool retry_needed;
        boost::tie(
          top_equiv
        , retry_needed
        ) = block_cky( lattree
                     , ckygen
                     , cell_proc
                     , tops
                     , ecs
                     , 1 
                     , 1
                     , 4096
                     );
        std::cerr << "decoded\n";
        
        if (not retry_needed) {
            xforest xf(sbmt::make_equiv_as_xforest(top_equiv,gram,ecs));
            sbmt::t2s::xtree_generator gentree = sbmt::t2s::xtrees_from_xforest(xf);
            while (gentree) {
                sbmt::t2s::xtree_ptr tp = gentree();
                cout << sbmt::t2s::hyptree(tp,gram.dict()) << " -> \"fake\" ### id=1 "
                     << sbmt::t2s::nbest_features(sbmt::t2s::accum(tp),gram.feature_names())
                     << " deriv={{{" << tp << "}}}\n";
                break;
            }
        }

         /*
        output_results( opts
                      , top_equiv
                      , opts.grammar()
                      , efactory
                      , id
                      );
        
    } catch (sbmt::empty_chart const& e) {
        dummy_output_results(opts, id, opts.grammar(), efactory, e.what());
    } catch (std::exception const& e) {
        if (opts.exit_on_retry) throw;
        dummy_output_results( opts
                            , id
                            , opts.grammar()
                            , efactory
                            , e.what()
                            );
    } catch (...) {
        if (opts.exit_on_retry) throw;
        dummy_output_results( opts
                            , id
                            , opts.grammar()
                            , efactory
                            , "unknown error"
                            );
    }
    opts.grammar().erase_terminal_rules();
    */
    return 0;
}
