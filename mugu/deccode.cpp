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

typedef sbmt::indexed_token source_state;
typedef boost::tuple<source_state,sbmt::t2s::grammar_state> state; 
typedef std::tr1::unordered_map<
          source_state
        , gusc::shared_varray<sbmt::t2s::forest>
        , boost::hash<source_state>
        > chart_span_entry;
typedef std::tr1::unordered_map<
          sbmt::span_t
        , chart_span_entry
        , boost::hash<sbmt::span_t>
        > chart;
typedef std::tr1::unordered_map<
          sbmt::t2s::grammar_state
        , std::vector<sbmt::t2s::hyp>
        , boost::hash<sbmt::t2s::grammar_state>
        > prechart_source_state_entry;
typedef std::tr1::unordered_map<
          source_state
        , prechart_source_state_entry
        , boost::hash<source_state>
        > prechart_span_entry;
typedef 
      std::tr1::unordered_map<
        sbmt::span_t
      , prechart_span_entry
      , boost::hash<sbmt::span_t>
      > prechart;
      
chart initial_chart(sbmt::span_t const& total_span)
{
    chart ret;
    /// initialize each spans entry to prevent thread conflicts
    for (sbmt::span_index_t x = total_span.left(); x != total_span.right(); ++x) {
        for (sbmt::span_index_t y = x + 1; y <= total_span.right(); ++y) {
            ret[sbmt::span_t(x,y)] = chart_span_entry();
        }
    }
    return ret;
}

prechart initial_prechart(sbmt::span_t const& total_span)
{
    prechart ret;
    /// initialize each spans entry to prevent thread conflicts
    for (sbmt::span_index_t x = total_span.left(); x != total_span.right(); ++x) {
        for (sbmt::span_index_t y = x + 1; y <= total_span.right(); ++y) {
            ret[sbmt::span_t(x,y)] = prechart_span_entry();
        }
    }
    return ret;
}

struct make_t2s;

class t2s_edge_factory
{
public:
    typedef sbmt::edge_factory<
              sbmt::edge_info_factory<
                sbmt::null_info_factory
              >
            > edge_factory_type;
    typedef edge_factory_type::edge_type edge_type;
    typedef edge_factory_type::edge_equiv_type edge_equiv_type;
    typedef sbmt::grammar_in_mem grammar_type;
    typedef sbmt::grammar_in_mem::rule_type rule_type;

    friend struct make_t2s;
    sbmt::t2s::rulemap_t rulemap;
    sbmt::t2s::supp_rulemap_t suppmap;
    edge_factory_type sfactory;
    chart chrt;
    prechart prechrt;

    t2s_edge_factory( std::string rulefile
                    , sbmt::grammar_in_mem& gram
                    , sbmt::span_t totalspan )
    {
        chrt = initial_chart(totalspan);
        prechrt = initial_prechart(totalspan);
        std::ifstream ifs(rulefile.c_str());
        rulemap = sbmt::t2s::read_grammar( ifs
                                         , gram.dict()
                                         , gram.feature_names()
                                         , gram.weights );
        size_t rid = 415000000000;
        BOOST_FOREACH(boost::shared_ptr<sbmt::scored_syntax> syn, gram.all_syntax_rules()) {
            sbmt::t2s::add_supps( syn->rule
                                , *(syn->rule.lhs_root())
                                , suppmap
                                , gram.dict()
                                , gram.feature_names()
                                , gram.weights
                                , rid );
        }
    }


    sbmt::edge_stats stats() const
    { return sfactory.stats(); }
    
    void set_cells() { sfactory.set_cells(); }
    
    
    /*
     typedef std::tr1::unordered_map<
               source_state
             , gusc::shared_varray<sbmt::t2s::forest>
             , boost::hash<source_state>
             > chart_span_entry;
     typedef std::tr1::unordered_map<
               sbmt::span_t
             , chart_span_entry
             , boost::hash<sbmt::span_t>
             > chart;
     
     typedef std::tr1::unordered_map<
               sbmt::t2s::grammar_state
             , std::vector<sbmt::t2s::hyp>
             , boost::hash<sbmt::t2s::grammar_state>
             > prechart_source_state_entry;
     typedef std::tr1::unordered_map<
               source_state
             , prechart_source_state_entry
             , boost::hash<source_state>
             > prechart_span_entry;
     typedef 
           std::tr1::unordered_map<
             sbmt::span_t
           , prechart_span_entry
           , boost::hash<sbmt::span_t>
           > prechart;
     */
    void finish(sbmt::span_t spn) 
    {
        std::cerr << "finish "<< spn << "\n";
        chart_span_entry& cse = chrt[spn];
        
        BOOST_FOREACH(chart_span_entry::value_type& v, cse) {
            BOOST_FOREACH(sbmt::t2s::forest f, v.second) {
                BOOST_FOREACH(sbmt::t2s::hyp h,f) {
                    prechrt[spn][v.first][f.state()].push_back(h);
                }
            }
        }
        BOOST_FOREACH(prechart_span_entry::value_type& pr, prechrt[spn]) {
            std::vector<sbmt::t2s::forest> v;
            BOOST_FOREACH(prechart_source_state_entry::value_type& ppr, pr.second) {
                std::vector<sbmt::t2s::hyp>& vv = ppr.second;
                std::sort(vv.begin(),vv.end(),gusc::greater());
                v.push_back(sbmt::t2s::forest(sbmt::t2s::varray_as_forest(vv)));
            }
            std::sort(v.begin(),v.end(),gusc::greater());
            gusc::shared_varray<sbmt::t2s::forest> sv(v);
            cse.insert(std::make_pair(pr.first,sv));
        }
        prechart_span_entry().swap(prechrt[spn]);
    }
    
    void set_cells( sbmt::cell_heuristic& c
                  , double weight
                  , sbmt::score_t unseen_cell_outside ) 
    {
        sfactory.set_cells(c,weight,unseen_cell_outside);
    }
   
    std::string hash_string(sbmt::grammar_in_mem const& g, edge_type const& e) const
    { 
        return sfactory.hash_string(g,e); }
    
    typedef gusc::any_generator<edge_type> generator_type;
    
    generator_type create_edge( grammar_type const& g 
                              , rule_type r
                              , edge_equiv_type const& eq1
                              , edge_equiv_type const& eq2 );

    
    generator_type create_edge( grammar_type const& g
                              , rule_type r
                              , edge_equiv_type const& eq );
    
    edge_type create_edge( sbmt::indexed_token const& lex
                         , sbmt::span_t s
                         , sbmt::score_t scr )
    { return sfactory.create_edge(lex,s,scr); }
    
    
    sbmt::score_t rule_heuristic(grammar_type const& g, rule_type r) const
    { return sfactory.rule_heuristic(g,r); }
    
    void component_info_scores( edge_type const& e
                              , grammar_type& g
                              , sbmt::feature_vector& scores
                              , sbmt::feature_vector& heuristics
                              , boost::function<bool(edge_type const&)> stop ) const
    { sfactory.component_info_scores(e,g,scores,heuristics,stop); }
    
    void component_info_scores( edge_type const& e
                              , grammar_type& g
                              , sbmt::feature_vector& scores
                              , boost::function<bool(edge_type const&)> stop ) const
    { sfactory.component_info_scores(e,g,scores,stop); }
};

struct make_t2s {
    typedef std::tr1::unordered_map< 
      size_t
    , gusc::shared_varray<sbmt::t2s::forest> 
    > children_map_t;
    typedef t2s_edge_factory::edge_type result_type;
    children_map_t children_forests(sbmt::xhyp hyp) const
    {
        children_map_t ret;
        BOOST_FOREACH(sbmt::indexed_syntax_rule::tree_node const& nd, hyp.rule().lhs()) {
            if (nd.indexed()) {
                result_type const&
                    ce = static_cast<sbmt::edge_equivalence_impl<result_type>*>(
                           const_cast<void*>(
                             hyp.child(nd.index()).id()
                           )
                         )->representative();
                sbmt::span_t span = ce.span();
                sbmt::indexed_token root = ce.root();
                ret.insert(std::make_pair(nd.index(),ef->chrt[span][root]));
            }
        }
        return ret;
    }
    explicit make_t2s(t2s_edge_factory* ef, sbmt::grammar_in_mem const* gram) 
    : ef(ef)
    , gram(gram) {}
    
    mutable t2s_edge_factory* ef;
    sbmt::grammar_in_mem const* gram;
    result_type operator()(result_type e) const
    {
        std::cerr << e.span() << " " << print(e.root(),gram->dict());
//<< " " << print(gram->get_syntax(gram->rule(e.rule_id())),gram->dict()) << " ";
        sbmt::span_t topspan = e.span();
        sbmt::indexed_token toproot = e.root();
        prechart_source_state_entry& smap = ef->prechrt[topspan][toproot];
        
        sbmt::grammar_in_mem* gm = const_cast<sbmt::grammar_in_mem*>(gram);
        t2s_edge_factory* efct = const_cast<t2s_edge_factory*>(ef);
        sbmt::edge_equivalence<result_type> eq(e);
        
        sbmt::xforest xf(sbmt::make_equiv_as_xforest(eq,*gm,*efct));
        
        int count = 0;
        sbmt::score_t scr = 0;
        int numbcks = 0;
        BOOST_FOREACH(sbmt::xhyp hyp, xf) {
            if (count == 10) break;
            
            children_map_t cmap = children_forests(hyp);
            gusc::shared_varray<sbmt::t2s::forest> 
                ff = sbmt::t2s::forests( hyp.rule()
                                       , *hyp.rule().lhs_root()
                                       , hyp.scores()
                                       , hyp.transition_score()
                                       , cmap
                                       , ef->rulemap
                                       , ef->suppmap );
            BOOST_FOREACH(sbmt::t2s::forest f, ff) {
                scr = std::max(scr,f.score());
                BOOST_FOREACH(sbmt::t2s::hyp h, f) {
                    smap[h.state()].push_back(h);
                    ++numbcks;
                }
                
            }
            
            ++count;
        }
        
        e.adjust_score(scr);
        std::cerr << "t2s=" << scr << " count="<< count << " num=" << numbcks << "\n"; 
        return e;
    }
};

t2s_edge_factory::generator_type 
t2s_edge_factory::create_edge( grammar_type const& g 
                             , rule_type r
                             , edge_equiv_type const& eq1
                             , edge_equiv_type const& eq2 )
{ 
    if (not g.is_complete_rule(r)) {
        return sfactory.create_edge(g,r,eq1,eq2); 
    } else {
        return gusc::generate_transform(
                 sfactory.create_edge(g,r,eq1,eq2)
               , make_t2s(this,&g)
               )
               ;
    }
}

t2s_edge_factory::generator_type 
t2s_edge_factory::create_edge( grammar_type const& g 
                             , rule_type r
                             , edge_equiv_type const& eq )
{ 
    if (not g.is_complete_rule(r)) {
        return sfactory.create_edge(g,r,eq); 
    } else {
        return gusc::generate_transform(
                 sfactory.create_edge(g,r,eq)
               , make_t2s(this,&g)
               )
               ;
    }
    
}

int main(int argc, char** argv)
{
    using namespace sbmt;
    using namespace sbmt::lazy;
    using namespace std;
    
    
    
    string lat = argv[1];
    string wts = argv[2];
    string global = argv[3];
    string psent = argv[4];
    string tgt = argv[5];
    
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
    
    //std::auto_ptr< edge_factory< edge_info_factory<null_info_factory> > > efactory(new edge_factory< edge_info_factory<null_info_factory> >());
    boost::shared_ptr<t2s_edge_factory> efactory(new t2s_edge_factory(tgt,gram,total_span));
    typedef t2s_edge_factory::edge_type edge_type;
    
    concrete_edge_factory<edge_type,grammar_in_mem> ecs(efactory);
    std::cerr << "t2s loaded\n";
    
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
            std::cerr << top_equiv.representative().root() << ' ' << top_equiv.representative().span() << "\n";
            BOOST_FOREACH(sbmt::t2s::forest frst,efactory->chrt[total_span][top_equiv.representative().root()]) {
                if (frst.state() == top_equiv.representative().root()) {
                    xforest xf(sbmt::t2s::forest_as_xforest(frst,gram.weights));
                    sbmt::t2s::xtree_generator gentree = sbmt::t2s::xtrees_from_xforest(xf);
                    while (gentree) {
                        sbmt::t2s::xtree_ptr tp = gentree();
                        cout << top_equiv.representative().inside_score() << "\t"
                             << tp->scr << "\t";
                        cout << sbmt::t2s::hyptree(tp,gram.dict()) << " -> \"fake\" ### id=1 "
                             << sbmt::t2s::nbest_features(sbmt::t2s::accum(tp),gram.feature_names()) << '\n';
                        break;
                    }
                }
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
